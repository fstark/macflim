
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tuple>

const char *SIGNATURE = "flim\r\n01\r\n";

void fail( const char *msg )
{
    fprintf( stderr, "Failed: %s", msg );
    exit( EXIT_FAILURE );
}

int read8( FILE *f )
{
    return fgetc( f );
}

void write8( FILE *f, int v )
{
    fputc( v, f );
}

int read16( FILE *f )
{
    return read8(f)*256+read8(f);
}

void write16( FILE *f, int v )
{
    write8( f, v>>8 );
    write8( f, v );
}

long read32( FILE *f )
{
    return read16(f)*65536L+read16(f);
}

void write32( FILE *f, long v )
{
    write16( f, v>>16 );
    write16( f, v );
}

#define noHACK

int main( int, char **argv )
{
    FILE *f = fopen( argv[1], "rb" );
    char buffer[1024];

    std::ignore = fread( buffer, 1, strlen(SIGNATURE), f );

    if (strncmp( buffer, SIGNATURE, strlen(SIGNATURE) ))
        fail( "Not a flim" );

    short stream_count = read16( f );

    printf( "STREAM COUNT: %d\n", stream_count );

    for (int i=0;i!=stream_count;i++)
    {
        int type = read16(f);
        int sub_type = read16(f);
        int width = read16(f);
        int height = read16(f);
        int fps = read16(f);
        int fps2 = read16(f);
        long frame_count = read32(f);
        long offset = read32(f);
        long length = read32(f);
        
        printf( "  #%d (%d/%d) %dx%d %.03f fps\n", i, type, sub_type, width, height, fps+fps2/65536.0 );
        printf( "           %ld frames, starting at %ld, length %ld\n", frame_count, offset, length );

        if (width*height/8*frame_count!=length)
        {
            printf( "ERROR: INCORRECT LENGTH %d x %ld = %ld is NOT equal to %ld\n", width*height/8, frame_count, width*height/8*frame_count, length );
        }


#ifdef HACK
        if (width==512 && height==342 && fps==12)
        {
            FILE *g = fopen( "out.flim", "wb" );
            fprintf( g,"%s", SIGNATURE );
            write16( g, 1 );    //  1 stream
            write16( g, type );
            write16( g, sub_type );
            write16( g, width );
            write16( g, height );
            write16( g, fps );
            write16( g, fps2 );
            write32( g, frame_count );
            write32( g, ftell(g)+8 );
            write32( g, length );

            char *p = malloc( length );
            long pos = ftell( f );
            fseek( f, offset, SEEK_SET );
            if (fread( p, length, 1, f )!=1)
                perror( "FAILED TO READ" );
            fseek( f, pos, SEEK_SET );
            if (fwrite( p, length, 1, g )!=1)
                perror( "FAILED TO WRITE" );
            free( p );

            fclose( g );
        }
#endif

    }

    fclose( f );

    return EXIT_SUCCESS;
}
