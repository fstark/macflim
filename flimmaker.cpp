/**
 * The flimmaker tool takes a set of pgm images and generate a flim file suitable for playback by MacFlim on a vintage Mac
 */

/**
 * TODO:
 *      Pixel aging
 *      Cycle budget
 *      => Multiple codecs
 *      => Codec testing tools (flim generation)
 *      New codecs:
 *          Invert rect?
 *          Fill black/white, vertical/horizontal, 8, 16, 32
 *          Fill constant, vertical/horizontal, 8, 16, 32
 *      Works from arbitrary images size (incl : letterbox)
 *      Manages ffmpeg worker / mediainfo / sox
 *      Automatic grid.mp4 generation
 *      flimutil
 */

#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <math.h>
#include <assert.h>
#include <limits>
#include <iostream>
#include <stdio.h>
#include <vector>
#include <array>
#include <memory>
#define noLZG
#ifdef LZG
#include "lzg.h"
#endif
#include "flimencoder.hpp"

using namespace std::string_literals;

//  True if the global '-g' option was set
bool sDebug = false;

//  If defined, we add a "stamp" to each stream, to know where it is coming from
#define noSTAMP

#ifdef STAMP
static int sStream = 0;
#endif



#include "image.hpp"
#include "reader.hpp"
#include "writer.hpp"

inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

int num_from_string( const char **s )
{
    int n = 0;
    while (**s>='0' && **s<='9')
    {
        n = n*10 + (**s-'0');
        (*s)++;
    }

    return n;
}

//  Converts a timestamp into a second count
//  42 => 42
//  05:31 => 331
//  2:4 => 124
//  02:04.470 => 124.47
//  1230.2 => 1230.2
//  0001:1:1:3.1toto => 219663.1
double seconds_from_string( const char *s )
{
    double d = 0;

    for (;;)
    {
        if (*s>='0' && *s<='9')
            d = d*60 + num_from_string( &s );
        if (*s!=':')
            break;
        s++;
    }

    if (!*s)
        return d;
    
    if (*s=='.')
    {
        double f = 1;
        s++;
        while (*s>='0' && *s<='9')
        {
            f /= 10;
            d += f*(*s++-'0');
        }
    }

    return d;
}

void test_seconds_from_string()
{
    assert( seconds_from_string( "42" ) == 42 );
    assert( seconds_from_string( "05:31" ) == 331 );
    assert( seconds_from_string( "2:4" ) == 124 );
    assert( seconds_from_string( "02:04.470" ) == 124.47 );
    assert( seconds_from_string( "1230.2" ) == 1230.2 );
    assert( seconds_from_string( "0001:1:1:3.1toto" ) == 219663.1 );
}

//  Write a bunch of bytes in a file
void write_data( const char *file, u_int8_t *data, size_t len )
{
    // fprintf( stderr, "Writing [%s]\n", file );

    FILE *f = fopen( file, "wb" );

    while (len--)
        fputc( *data++, f );

    fclose( f );
}

const char *version = "2.0.0";

#include <iostream>
#include <chrono>
#include <ctime>    

void usage( const std::string name )
{
    std::cerr << "Usage\n";

    std::cerr << name << " INPUT [OPTIONS ...]\n";

    std::cerr << "  INPUT can be either a mp4 file name, a movie URL or a 'pgm' pattern.'\n";

    std::cerr << "\n  Input options:\n";

    std::cerr << "    --from TIME                 : time offset to start extracting from\n";
    std::cerr << "    --duration TIME             : time duration of the extraced clip\n";
    std::cerr << "    --fps FPS                   : for 'pgm' pattern, specifies the framerate to be used\n";
    std::cerr << "    --audio FILE                : for 'pgm', specifices a separate u8 22200 Hz wav file with audio\n";

    std::cerr << "\n  Output options:\n";

    std::cerr << "    --out FILE                  : name of the flim file to create (by default 'out.flim')\n";
    std::cerr << "    --mp4 FILE                  : outputs a 60fps mp4 file with the result\n";
    std::cerr << "    --gif FILE                  : outputs a 20fps gif file with the result, limited to 5 seconds\n";
    std::cerr << "    --pgm PATTERN               : output every generated image in a pgm file\n";

    std::cerr << "\n  Encoding options:\n";

    std::cerr << "    --profile PROFILE           : presents the specific encoding profile, which sets a suitable default for all encoding options\n";
    std::cerr << "      Defdaul is 'se30'. See below for description of profiles.\n";

    std::cerr << "    --byterate BYTERATE         : bytes per ticks available for video compression\n";
    std::cerr << "    --half-rate BOOLEAN         : if true, half of the images from the source will be dropped.\n";
    std::cerr << "    --group BOOLEAN             : if true, packs ticks together to present screen updates at the same rate as the input media. Only works on a se30.\n";

    std::cerr << "    --bars BOOLEAN              : if false, image is zoomed in so there are no black bars.\n";

    std::cerr << "    --dither DITHER             : specifies the type of dithering to be used.\n";
    std::cerr << "      'ordered' will use a 4x4 ordered dither matrix.\n";
    std::cerr << "      'error' will use an error diffusion algorithm.\n";
    std::cerr << "    --error-algorithm ALGORITHM : error diffusion algorithm to be used\n";
    std::cerr << "      Default 'floyd'. See below for the list of valid error dithering algorithms.\n";

    std::cerr << "    --error-stability FLOAT     : amount of error to be accumulated before changing a screen pixel\n";
    std::cerr << "    --error-bidi BOOLEAN        : if true, error diffusion is applied in different direction for even and odd scanlines.\n";
    std::cerr << "    --error-bleed PERCENT       : how much error is moved from a pixel to the neighbours.\n";

    std::cerr << "    --filters FILTERS           : specifies a set of filters to be applied on image afgter resizing, but before dithering\n";
    std::cerr << "    --codec CODEC               : adds a specific codec to the encoding. The first --codec parameter clears the profile codec list\n";

    std::cerr << "\n  Misc options:\n";

    std::cerr << "    --watermark STRING          : adds the string to the upper left corner of the generated flim for identification purposes.\n";
    std::cerr << "      use 'auto' to use the encoding parameters as watermark\n";

    std::cerr << "    --buffer-size SIZE          : buffer playback size\n";
    std::cerr << "    --debug BOOLEAN             : enables various debug options\n";
    
        // else if (!strcmp(*argv,"--cover-from"))
        // else if (!strcmp(*argv,"--cover-to"))
        // else if (!strcmp(*argv,"--cover"))
        // else if (!strcmp(*argv,"--diff-pattern"))
        // else if (!strcmp(*argv,"--change-pattern"))
        // else if (!strcmp(*argv,"--target-pattern"))
        // else if (!strcmp(*argv,"--comment"))

    std::cerr << "\nList of profiles names for the --profile option (default 'se30'):\n";
    for (auto n:{ "plus", "se", "se30", "perfect" })
    {
        encoding_profile p;
        encoding_profile::profile_named( n, p );
        std::cerr << "        " << n << " : " << p.description() << "\n";
    }

    std::cerr << "\nList of error diffusion algorithms for the --error_diffusion option (default 'floyd'):\n";

    error_diffusion_algorithms( []( const std::string name, const std::string description )
    {
        fprintf( stderr, "               %16s : %s\n", name.c_str(), description.c_str() );
    } );


    std::cerr << "use '" << name << " --help' for displaying this help page.\n";
}

//  The main function, does all the work
//  flimmaker [-g] --in <%d.pgm> --from <index> --to <index> --cover <index> --audio <audio.waw> --out <file>
int main( int argc, char **argv )
{
try
{
    // std::string in_arg = "movie-%06d.pgm";
    std::string input_file = "";
    std::string mp4_file = "";
    std::string gif_file = "";
    std::string out_arg = "out.flim";
    std::string audio_arg = "audio.raw";
    double from_index = 0;
    double to_index = std::numeric_limits<int>::max();
    double duration = 300;   //  5 minutes by default
    int cover_from = -1;
    int cover_to = -1;
    double fps = 24.0;
    std::string watermark = "";
    std::string pgm_pattern = ""; // "out-%06d.pgm";
    std::string diff_pattern = "";
    std::string change_pattern = "";
    std::string target_pattern = "";
    bool auto_watermark = false;

    const std::string cmd_name{ argv[0] };

    // test_ffmpeg( argv[1] );

    std::vector<flimcompressor::codec_spec> codecs;

    codecs.push_back( {} );
    codecs.back().signature = 0x00;
    codecs.back().penality = 1;
    codecs.back().coder = std::make_shared<null_compressor>();

    std::string comment = "FLIM\n";
    
    for (int i=0;i!=argc;i++)
    {
        if (i!=0)
            comment += " ";
        comment += argv[i];
    }

    comment += "\n";
    comment += "flimmaker-version: ";
    comment += version;
    comment += "\n";

    encoding_profile custom_profile;
    if (!encoding_profile::profile_named( "se30", custom_profile ))
    {
        std::cerr << "Cannot find default profile 'se30'\n";
        ::exit( EXIT_FAILURE );
    }

    // std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    // comment += "date: ";
    // comment += std::ctime(&time);

    // packz32_test();
    packz32opt_test();
    test_seconds_from_string();

    argc--;
    argv++;

    while (argc)
    {
        if (!strcmp(*argv,"--help"))
        {
            usage( cmd_name );
            ::exit( EXIT_SUCCESS );
        }

        // if (!strcmp(*argv,"--input"))
        //  Don't start with '--', this is the input file/url
        if (strncmp(*argv,"--",2))
        {
            if (input_file!="")
            {
                std::cerr << "Input file specified twice: '" << input_file << "' and '" << *argv << "'\n";
                ::exit( EXIT_FAILURE );
            }
            input_file = *argv;
        }
        else if (!strcmp(*argv,"--mp4"))
        {
            argc--;
            argv++;
            mp4_file = *argv;
        }
        else if (!strcmp(*argv,"--gif"))
        {
            argc--;
            argv++;
            gif_file = *argv;
        }
        else if (!strcmp(*argv,"--profile"))
        {
            argc--;
            argv++;
            if (!encoding_profile::profile_named( *argv, custom_profile ))
            {
                std::cerr << "Cannot find encoding profile '" << *argv << "'\n";
                ::exit( EXIT_FAILURE );
            }
        }
        else if (!strcmp(*argv,"--byterate"))
        {
            argc--;
            argv++;
            custom_profile.set_byterate( atoi( *argv ) );
        }
        else if (!strcmp(*argv,"--fps"))
        {
            argc--;
            argv++;
            fps = atof(*argv);
        }
        else if (!strcmp(*argv,"--buffer-size"))
        {
            argc--;
            argv++;
            custom_profile.set_buffer_size( atoi( *argv ) );
        }
        else if (!strcmp(*argv,"--half-rate"))
        {
            argc--;
            argv++;
            custom_profile.set_half_rate( bool_from(*argv) );
        }
        else if (!strcmp(*argv,"--group"))
        {
            argc--;
            argv++;
            custom_profile.set_group( bool_from(*argv) );
        }
        else if (!strcmp(*argv,"--debug"))
        {
            argc--;
            argv++;
            sDebug = bool_from( *argv );
        }
        else if (!strcmp(*argv,"--from"))
        {
            argc--;
            argv++;
            from_index = seconds_from_string(*argv);
        }
        else if (!strcmp(*argv,"--to"))
        {
            argc--;
            argv++;
            to_index = atof(*argv);
        }
        else if (!strcmp(*argv,"--duration"))
        {
            argc--;
            argv++;
            duration = seconds_from_string(*argv);
        }
        else if (!strcmp(*argv,"--cover-from"))
        {
            argc--;
            argv++;
            cover_from = atoi(*argv);
        }
        else if (!strcmp(*argv,"--cover-to"))
        {
            argc--;
            argv++;
            cover_to = atoi(*argv);
        }
        else if (!strcmp(*argv,"--cover"))
        {
            argc--;
            argv++;
            cover_from = atoi(*argv);
            cover_to = cover_from+23;
        }
        else if (!strcmp(*argv,"--audio"))
        {
            argc--;
            argv++;
            audio_arg = *argv;
        }
        else if (!strcmp(*argv,"--out"))
        {
            argc--;
            argv++;
            out_arg = *argv;
        }
        else if (!strcmp(*argv,"--out-pattern") || !strcmp(*argv,"--pgm-pattern") || !strcmp(*argv,"--pgm"))
        {
            argc--;
            argv++;
            pgm_pattern = *argv;
        }
        else if (!strcmp(*argv,"--diff-pattern"))
        {
            argc--;
            argv++;
            diff_pattern = *argv;
        }
        else if (!strcmp(*argv,"--change-pattern"))
        {
            argc--;
            argv++;
            change_pattern = *argv;
        }
        else if (!strcmp(*argv,"--target-pattern"))
        {
            argc--;
            argv++;
            target_pattern = *argv;
        }
        else if (!strcmp(*argv,"--comment"))
        {
            argc--;
            argv++;
            comment += "comment: ";
            comment += *argv;
            comment += "\n";
        }
        else if (!strcmp(*argv,"--watermark"))
        {
            argc--;
            argv++;
            if (!strcmp(*argv,"auto"))
                auto_watermark = true;
            else
                watermark = *argv;
        }
        else if (!strcmp(*argv,"--filters"))
        {
            argc--;
            argv++;
            custom_profile.set_filters( *argv );
        }
        else if (!strcmp(*argv,"--bars"))
        {
            argc--;
            argv++;
            custom_profile.set_bars( bool_from(*argv) );
        }
        else if (!strcmp(*argv,"--codec"))
        {
            argc--;
            argv++;
            //  --codec z32 --codec line-copy:count=20
            // codecs.push_back( flimcompressor::make_codec( "z32", 512, 342, "" ) );
            // codecs.push_back( flimcompressor::make_codec( "lines", 512, 342, "" ) );
            // codecs.push_back( flimcompressor::make_codec( "null", 512, 342, "" ) );
            // codecs.push_back( flimcompressor::make_codec( "invert", 512, 342, "" ) );
            codecs.push_back( flimcompressor::make_codec( *argv, 512, 342 ) );
        }
        else if (!strcmp(*argv,"--dither"))
        {
            argc--;
            argv++;
            custom_profile.set_dither( *argv );
        }
        else if (!strcmp(*argv,"--error-stability"))
        {
            argc--;
            argv++;
            custom_profile.set_stability( atof( *argv ) );
        }
        else if (!strcmp(*argv,"--error-algorithm"))
        {
            argc--;
            argv++;
            custom_profile.set_error_algorithm( *argv );
        }
        else if (!strcmp(*argv,"--error-bleed"))
        {
            argc--;
            argv++;
            custom_profile.set_error_bleed( atof(*argv) );
        }
        else if (!strcmp(*argv,"--error-bidi"))
        {
            argc--;
            argv++;
            custom_profile.set_error_bidi( bool_from(*argv) );
        }
        else
        {
            std::cerr << "Unknown argument " << *argv << "\n";
            return EXIT_FAILURE;
        }

        argc--;
        argv++;
    }

    if (input_file=="")
    {
        usage( cmd_name );
        exit( EXIT_FAILURE );
    }

    //  If input-file is an url, use youtube-dl to retreive content
    if (input_file.rfind( "https://", 0 )==0)
    {
        char buffer[1024];
        // sprintf( buffer, "youtube-dl '%s' --recode mkv --output '/tmp/out'", input_file.c_str() );
        sprintf( buffer, "youtube-dl '%s' -f mp4 --output '/tmp/out.mp4'", input_file.c_str() );
        // https://www.dailymotion.com/video/x1au0r --dither ordered --filters g1.5q5c

            //  Switch input file
        input_file = "/tmp/out.mp4"s;
        unlink( input_file.c_str() );

        int res = system( buffer );
        if (res!=0)
        {
            std::clog << "youtube-dl failed with error " << res << "\n";
            exit( EXIT_FAILURE );
        }
    }

    if (codecs.size()>1)
    {
        custom_profile.set_codecs( codecs );
    }

    if (auto_watermark)
    {
        if (watermark.size()>0)
            watermark += " ";
        watermark += custom_profile.description();
    }

    std::clog << "Encoding arguments :\n" << custom_profile.description() << "\n";

    std::unique_ptr<input_reader> r;
    if (ends_with( input_file, ".pgm" ))
    {
        std::clog << "Reading pgm from '" << input_file << "' pattern, at " << fps << " frames per second, using '" << audio_arg << "' audio file\n";
        std::clog << "( use --fps and --audio to change fps and audio )\n";
        r = std::make_unique<filesystem_reader>( input_file, fps, audio_arg, from_index, to_index );
    }
    else
    {
        r = make_ffmpeg_reader( input_file, from_index, duration );
        fps = r->frame_rate();
    }

    std::vector<std::unique_ptr<output_writer>> w;
    if (mp4_file!="")
        w.push_back( std::move(make_ffmpeg_writer( mp4_file, 512, 342 ) ) );
    if (gif_file!="")
        w.push_back( std::move(make_gif_writer( gif_file, 512, 342 ) ) );

    auto encoder = flimencoder{ custom_profile };
    encoder.set_fps( fps );
    encoder.set_comment( comment );
    encoder.set_cover( cover_from, cover_to+1 );
    encoder.set_watermark( watermark );
    encoder.set_out_pattern( pgm_pattern );
    encoder.set_diff_pattern( diff_pattern );
    encoder.set_change_pattern( change_pattern );
    encoder.set_target_pattern( target_pattern );

    // encoder.set_input_single_random();

        encoder.make_flim( out_arg, r.get(), w );
    }
    catch (const char *error)
    {
        std::cerr << "**** ERROR : [" << error << "]\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
