/**
 * The flimmaker tool takes a set of pgm images and generate a flim file suitable for playback by MacFlim on a vintage Mac
 */

#include <stdlib.h>
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
bool debug = false;

//  If defined, we add a "stamp" to each stream, to know where it is coming from
#define noSTAMP

#ifdef STAMP
static int sStream = 0;
#endif



#include "image.hpp"


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

//  The main function, does all the work
//  flimmaker [-g] --in <%d.pgm> --from <index> --to <index> --cover <index> --audio <audio.waw> --out <file>
int main( int argc, char **argv )
{
    std::string in_arg = "movie-%06d.pgm";
    std::string out_arg = "out.flim";
    std::string audio_arg = "audio.raw";
    int from_index = 1;
    int to_index = std::numeric_limits<int>::max();
    int cover_from = -1;
    int cover_to = -1;
    double fps = 24.0;
    std::string watermark = "";
    std::string out_pattern = "out-%06d.pgm";
    std::string diff_pattern = "";
    std::string change_pattern = "";
    std::string target_pattern = "";
    bool auto_watermark = false;

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

    encoding_profile custom_profile = encoding_profile::profile_named( "se30" );

    // std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    // comment += "date: ";
    // comment += std::ctime(&time);

    // packz32_test();
    packz32opt_test();

    argc--;
    argv++;

    while (argc)
    {
        if (!strcmp(*argv,"--profile"))
        {
            argc--;
            argv++;
            custom_profile = encoding_profile::profile_named( *argv );
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--byterate"))
        {
            argc--;
            argv++;
            custom_profile.set_byterate( atoi( *argv ) );
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--fps"))
        {
            argc--;
            argv++;
            fps = atof(*argv);
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--buffer-size"))
        {
            argc--;
            argv++;
            custom_profile.set_buffer_size( atoi( *argv ) );
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--half-rate"))
        {
            argc--;
            argv++;
            custom_profile.set_half_rate( bool_from(*argv) );
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--group"))
        {
            argc--;
            argv++;
            custom_profile.set_group( bool_from(*argv) );
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"-g"))
        {
            debug = true;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--in"))
        {
            argc--;
            argv++;
            in_arg = *argv;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--from"))
        {
            argc--;
            argv++;
            from_index = atoi(*argv);
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--to"))
        {
            argc--;
            argv++;
            to_index = atoi(*argv);
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--cover-from"))
        {
            argc--;
            argv++;
            cover_from = atoi(*argv);
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--cover-to"))
        {
            argc--;
            argv++;
            cover_to = atoi(*argv);
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--cover"))
        {
            argc--;
            argv++;
            cover_from = atoi(*argv);
            cover_to = cover_from+23;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--audio"))
        {
            argc--;
            argv++;
            audio_arg = *argv;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--stability"))
        {
            argc--;
            argv++;
            custom_profile.set_stability( atof( *argv ) );
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--out"))
        {
            argc--;
            argv++;
            out_arg = *argv;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--out-pattern"))
        {
            argc--;
            argv++;
            out_pattern = *argv;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--diff-pattern"))
        {
            argc--;
            argv++;
            diff_pattern = *argv;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--change-pattern"))
        {
            argc--;
            argv++;
            change_pattern = *argv;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--target-pattern"))
        {
            argc--;
            argv++;
            target_pattern = *argv;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--comment"))
        {
            argc--;
            argv++;
            comment += "comment: ";
            comment += *argv;
            comment += "\n";
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--watermark"))
        {
            argc--;
            argv++;
            watermark = *argv;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--filters"))
        {
            argc--;
            argv++;
            custom_profile.set_filters( *argv );
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--auto-watermark"))
        {
            argc--;
            argv++;
            auto_watermark = bool_from( *argv );
            argc--;
            argv++;
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
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--dither"))
        {
            argc--;
            argv++;
            custom_profile.set_dither( *argv );
            argc--;
            argv++;
        }
        else
        {
            std::cerr << "Unknown argument " << *argv << "\n";
            return EXIT_FAILURE;
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

    std::clog << "------------- ENCODING PROFILE -------------\n" << custom_profile.description() << "--------------------------------------------\n";

    auto encoder = flimencoder{ custom_profile, in_arg, audio_arg };
    encoder.set_fps( fps );
    encoder.set_comment( comment );
    encoder.set_cover( cover_from, cover_to+1 );
    encoder.set_watermark( watermark );
    encoder.set_out_pattern( out_pattern );
    encoder.set_diff_pattern( diff_pattern );
    encoder.set_change_pattern( change_pattern );
    encoder.set_target_pattern( target_pattern );

    // encoder.set_input_single_random();

    encoder.make_flim( out_arg, from_index, to_index );

    return EXIT_SUCCESS;
}
