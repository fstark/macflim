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
    size_t byterate = 6000;
    double fps = 24.0;
    size_t buffer_size = 300000;
    double stability = 0.3;
    bool half_rate = false;
    bool group = false;
    std::string filters = "gsc";
    std::string watermark = "";
    bool auto_watermark = false;

    std::string comment = "FLIM\n";
    
    comment += "command-line:";

    for (int i=0;i!=argc;i++)
    {
        comment += " ";
        comment += argv[i];
    }

    comment += "\n";
    comment += "flimmaker-version: ";
    comment += version;
    comment += "\n";

    std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    comment += "date: ";
    comment += std::ctime(&time);

    // packz32_test();
    packz32opt_test();

    argc--;
    argv++;

    while (argc)
    {
        // if (!strcmp(*argv,"--compression-target"))
        // {
        //     argc--;
        //     argv++;
        //     compression_target = atof(*argv)/100.0;
        //     argc--;
        //     argv++;
        // }
        // else
        if (!strcmp(*argv,"--byterate"))
        {
            argc--;
            argv++;
            byterate = atoi(*argv);
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
            buffer_size = atoi(*argv);
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--half-rate"))
        {
            half_rate = true;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--group"))
        {
            group = true;
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
            stability = atof( *argv );
            argc--;
            argv++;
        }
        // else if (!strcmp(*argv,"--max-stability"))
        // {
        //     argc--;
        //     argv++;
        //     g_max_stability = atof( *argv );
        //     argc--;
        //     argv++;
        // }
        else if (!strcmp(*argv,"--out"))
        {
            argc--;
            argv++;
            out_arg = *argv;
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
            filters = *argv;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--auto-watermark"))
        {
            argc--;
            argv++;
            auto_watermark = true;
        }
        else
        {
            std::cerr << "Unknown argument " << *argv << "\n";
            return EXIT_FAILURE;
        }
    }

    if (auto_watermark)
    {
        char buffer[1024];
        sprintf( buffer, "br:%ld st:%.2f%s%s fi:%s", byterate, stability, half_rate?" half-rate":"", group?" group":"", filters.c_str() );
        if (watermark.size()>0)
            watermark += " ";
        watermark += buffer;
    }

    auto encoder = flimencoder{ 512, 342, in_arg, audio_arg };
    encoder.set_byterate( byterate );
    encoder.set_fps( fps );
    encoder.set_buffer_size( buffer_size );
    encoder.set_stability( stability );
    encoder.set_half_rate( half_rate );
    encoder.set_group( group );
    std::clog << "[" << comment << "]\n";
    encoder.set_comment( comment );
    encoder.set_filters( filters );
    encoder.set_cover( cover_from, cover_to+1 );
    encoder.set_watermark( watermark );

    encoder.make_flim( out_arg, from_index, to_index );

    return EXIT_SUCCESS;
}
