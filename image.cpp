#include "image.hpp"

#include <iostream>
#include <math.h>

//  ------------------------------------------------------------------
//  Copy image (#### : is operator=?)
//  ------------------------------------------------------------------
void copy_image( image &dest, const image &source )
{
    dest = source;
}

//  ------------------------------------------------------------------
//  Fills image with constant color, 50% gray by default
//  ------------------------------------------------------------------
void fill( image &img, float value )
{
    for (int y=0;y!=img.H();y++)
        for (int x=0;x!=img.W();x++)
            img.at(x,y) = value;
}

//  ------------------------------------------------------------------
//  Sharpens the image
//  ------------------------------------------------------------------
image sharpen( const image &src )
{
    float kernel[3][3] = {
        {  0.0, -1.0,  0.0 },
        { -1.0,  5.0, -1.0 },
        {  0.0, -1.0,  0.0 },
    };

    auto res = src;

    for (int x=0;x!=src.W();x++)
        res.at(x,0) = res.at(x,src.H()-1) = 0;
    for (int y=0;y!=src.H();y++)
        res.at(0,y) = res.at(src.W()-1,y) = 0;

    fill( res, 0 );

    for (int x=1;x!=src.W()-1;x++)
        for (int y=1;y!=src.H()-1;y++)
        {
            float v = 0;
            for (int x0=0;x0!=3;x0++)
                for (int y0=0;y0!=3;y0++)
                {
                    v += src.at(x+x0-1,y+y0-1)*kernel[x0][y0];
                }
            res.at(x,y) = v;
        }

    return res;
}

//  ------------------------------------------------------------------
//  Blurs the image with a 3x3 kernel
//  ------------------------------------------------------------------
image blur3( const image &src )
{
    static float kernel[3][3] = {
        { 1.0/9, 1.0/9, 1.0/9 },
        { 1.0/9, 1.0/9, 1.0/9 },
        { 1.0/9, 1.0/9, 1.0/9 },
    };

    auto res = src;

    // for (int x=0;x!=W;x++)
    //     res[x][0] = res[x][H-1] = 0;
    // for (int y=0;y!=H;y++)
    //     res[0][y] = res[W-1][y] = 0;

    fill( res, 0 );

    for (int x=1;x!=src.W()-1;x++)
        for (int y=1;y!=src.H()-1;y++)
        {
            float v = 0;
            for (int x0=0;x0!=3;x0++)
                for (int y0=0;y0!=3;y0++)
                {
                    v += src.at(x+x0-1,y+y0-1)*kernel[x0][y0];
                }
            if (v<0) v = 0;
            if (v>1) v = 1;
            res.at(x,y) = v;
        }

    return res;
}

//  ------------------------------------------------------------------
//  Blurs the image more with a 5x5 kernel
//  ------------------------------------------------------------------
image blur5( const image &src )
{
    float kernel[5][5] = {
        { 1.0/256, 4.0/256, 6.0/256, 4.0/256, 1.0/256},
        { 4.0/256,16.0/256,24.0/256,16.0/256, 4.0/256},
        { 6.0/256,24.0/256,36.0/256,24.0/256, 6.0/256},
        { 4.0/256,16.0/256,24.0/256,16.0/256, 4.0/256},
        { 1.0/256, 4.0/256, 6.0/256, 4.0/256, 1.0/256},
    };

    auto res = src;

    for (int x=0;x!=src.W();x++)
        res.at(x,0) = res.at(x,src.H()-1) = res.at(x,1) = res.at(x,src.H()-2) = 0;
    for (int y=0;y!=src.H();y++)
        res.at(0,y) = res.at(src.W()-1,y) = res.at(1,y) = res.at(src.W()-2,y) = 0;

    fill( res, 0 );

    for (int x=2;x!=src.W()-2;x++)
        for (int y=2;y!=src.H()-2;y++)
        {
            float v = 0;
            for (int x0=0;x0!=5;x0++)
                for (int y0=0;y0!=5;y0++)
                {
                    v += src.at(x+x0-2,y+y0-2)*kernel[x0][y0];
                }
            res.at(x,y) = v;
        }

    return res;
}

//  ------------------------------------------------------------------
//  Gamma corrects the image
//  ------------------------------------------------------------------
image gamma( const image &src, double gamma )
{
    image res = src;

    for (int x=0;x!=src.W();x++)
        for (int y=0;y!=src.H();y++)
        {
            res.at(x,y) = pow( src.at(x,y), gamma );
        }

    return res;
}

//  ------------------------------------------------------------------
image zoom_out( const image &src )
{
    const double bx = 32;
    const double a = ((src.W()/2)-bx)/(src.W()/2);
    const double by = src.H()/2-a*(src.H()/2);

    image res = src;
    for (int y=0;y!=src.H();y++)
        for (int x=0;x!=src.W();x++)
        {
            int from_x = (x-bx)/a;
            int from_y = (y-by)/a;

            if (from_x>0 && from_x<src.W() && from_y>0 && from_y<src.H())
                res.at(x,y) = src.at(from_x,from_y);
            else
                res.at(x,y) = 0;
        }
    
    return res;
}

//  ------------------------------------------------------------------
//  Reduce an image to half the size
//  ------------------------------------------------------------------
void reduce_image_half( image &dest, const image &source )
{
    for (int y=0;y!=dest.H();y++)
        for (int x=0;x!=dest.W();x++)
            dest.at(x,y) = (source.at(2*x,2*y)+source.at(2*x+1,2*y)+source.at(2*x,2*y+1)+source.at(2*x+1,2*y+1))/4;
}

//  ------------------------------------------------------------------
//  Copies source into dest, with resize
//  ------------------------------------------------------------------

void copy_resize( image &dest, const image &source )
{
    double rw = source.W()*1.0/dest.W();
    double rh = source.H()*1.0/dest.H();
    for (int y=0;y!=dest.H();y++)
        for (int x=0;x!=dest.W();x++)
            dest.at(x,y) = source.at(x*rw,y*rh);
}

//  ------------------------------------------------------------------
//  Adds a pixel in the 4 corners of the image
//  ------------------------------------------------------------------
void plot4( image &img, int x, int y, int c )
{
    img.at(x,y) = c;
    img.at(img.W()-1-x,y) = c;
    img.at(x,img.H()-1-y) = c;
    img.at(img.W()-1-x,img.H()-1-y) = c;
}

//  ------------------------------------------------------------------
//  Draw a horizonal line in the 4 corners of the image
//  ------------------------------------------------------------------
void hlin4( image &img, int x0, int x1, int y, int c )
{
    while (x0<x1)
        plot4( img, x0++, y, c );
}

//  ------------------------------------------------------------------
//  As Steve Jobs asked for Mac desktops to have round corners, forces rounded corners on generated flims.
//  ------------------------------------------------------------------
void round_corners( image &img )
{
    hlin4( img, 0, 5, 0, 0 );
    hlin4( img, 0, 3, 1, 0 );
    hlin4( img, 0, 2, 2, 0 );
    hlin4( img, 0, 1, 3, 0 );
    hlin4( img, 0, 1, 4, 0 );
}

image round_corners( const image& img )
{
    image res = img;
    round_corners( res );
    return res;
}

image quantize( const image &img, int n )
{
    image res = img;

    for (int y=0;y!=res.H();y++)
        for (int x=0;x!=res.W();x++)
            res.at(x,y) = ((int)(img.at(x,y)*(n-1)+.5))/(double)(n-1);

    return res;
}

//  ------------------------------------------------------------------
//  Apply a filter on an image
//  ------------------------------------------------------------------
typedef enum
{
    kBlur3 = 'b',
    kBlur5 = 'B',
    kSharpen = 's',
    kGamma = 'g',
    kRoundCorners = 'c',
    kZoomOut = 'z',
    kQuantize16 = 'q'
}   eFilters;

image filter( const image &from, eFilters filter, double arg=0 )
{
    switch (filter)
    {
        case kBlur3:
            return blur3( from );
        case kBlur5:
            return blur5( from );
        case kSharpen:
            return sharpen( from );
        case kGamma:
            return gamma( from, arg?arg:1.6 );
        case kRoundCorners:
            return round_corners( from );
        case kZoomOut:
            return zoom_out( from );
        case kQuantize16:
        case 'Q':
            return quantize( from, arg?arg:16 );
    }
    std::cerr << "**** ERROR: filter ['" << (char)filter << "'] (" << (int)filter << ") unknown\n";
    throw "Unknown filter";
}

inline bool extract_filter( const char *&p, char &f, double &arg )
{
    arg = 0;
    if (!*p)
        return 0;

        //  Filter name
    f = *p++;

        //  Filter argument (int)
    while (*p>='0' && *p<='9')
        arg = arg*10+ (*p++)-'0';

        //  Decimal part of filter argument
    if (*p=='.')
    {
        p++;
        double scale = 0.1;
        while (*p>='0' && *p<='9')
        {
            arg += ((*p++)-'0')*scale;
            scale /= 10;
        }
    }

    // printf( "filter==%c arg==%f\n", f, arg );

    return true;
}

//  ------------------------------------------------------------------
//  Apply a sequence of filters
//  ------------------------------------------------------------------

image filter( const image &from, const char *filters )
{
    image res = from;
    char f;
    double arg;

    while (extract_filter( filters, f, arg ))
        res = filter( res, (eFilters)f, arg );

    return res;
}

//  ------------------------------------------------------------------
//  Reads an image from a grayscale PGM file of the right size
//  ------------------------------------------------------------------
inline int correct( int v )
{
    if (v<=0x01)
        v = 0;
    if (v>=0xfc)
        v = 0xff;
    return v;
}

// int correct( int v )
// {
//     v = ((v+8)/16)*16;
//     if (v>=255)
//         v = 255;
//     return v;
// }

bool read_image( image &result, const char *file )
{
    // fprintf( stderr, "Reading [%s]\n", file );

    image image( 512, 342 );
    fill( image, 0 );

    FILE *f = fopen( file, "rb" );

    if (!f)
        return false;

    for (int i=0;i!=15;i++)
        fgetc( f );

    for (int y=0;y!=image.H();y++)
        for (int x=0;x!=image.W();x++)
        {    // img[x][y] = ((int)(fgetc(f)/255.0*16))/16.0;
            image.at(x,y) = correct( fgetc(f) )/255.0;
        }

    fclose( f );

    result = image;

    //  result = sharpen( image );
    //  result = blur5(blur5(blur5(blur5( image ))));
    // result = sharpen(sharpen(blur5( image )));

    // result = blur5( image );

//    result = gamma( image, 1.6 );
//    result = blur5( image );
//    result = sharpen( result );
//    result = sharpen( result );
//    result = zoom_out( result );
    //  result = filter( image, "gs" );

    return true;
}

//  ------------------------------------------------------------------
//  Generates a PGM image
//  ------------------------------------------------------------------
void write_image( const char *file, image &img )
{
    // fprintf( stderr, "Writing [%s]\n", file );

    FILE *f = fopen( file, "wb" );

    fprintf( f, "P5\n%ld %ld\n255\n", img.W(), img.H() );
    for (int y=0;y!=img.H();y++)
        for (int x=0;x!=img.W();x++)
            fputc( img.at(x,y)*255, f );

    fclose( f );
}



//  ------------------------------------------------------------------
//  Basic "standard" floyd-steinberg, suitable for static images
//  Dest will only contain 0 or 1, corresponding to the dithering of the source
//  Here for reference, not used
//  ------------------------------------------------------------------
void quantize( image &dest, const image &source )
{
    dest = source;

    for (int y=0;y!=source.H();y++)
        for (int x=0;x!=source.W();x++)
        {
            float source_color = dest.at(x,y);
            float color;
            float error;
            color = source_color<=0.5?0:1;
            error = source_color - color;
            dest.at(x,y) = color;
            float e0 = error * 7 / 16;
            float e1 = error * 3 / 16;
            float e2 = error * 5 / 16;
            float e3 = error * 1 / 16;
            if (x<source.W()-1)
                dest.at(x+1,y) = dest.at(x+1,y) + e0;
            if (x>0 && y<source.H()-1)
                dest.at(x-1,y+1) = dest.at(x-1,y+1) + e1;
            if (y<source.H()-1)
                dest.at(x,y+1) = dest.at(x,y+1) + e2;
            if (x<source.W()-1 && y<source.H()-1)
                dest.at(x+1,y+1) = dest.at(x+1,y+1) + e3;
        }
}

static int dither[8][8] =
{
    { 0, 32, 8, 40, 2, 34, 10, 42},
    {48, 16, 56, 24, 50, 18, 58, 26},
    {12, 44, 4, 36, 14, 46, 6, 38},
    {60, 28, 52, 20, 62, 30, 54, 22},
    { 3, 35, 11, 43, 1, 33, 9, 41},
    {51, 19, 59, 27, 49, 17, 57, 25},
    {15, 47, 7, 39, 13, 45, 5, 37},
    {63, 31, 55, 23, 61, 29, 53, 21}
};

void quantize2( image &dest, const image &source, const image &previous, float stability )
{
    for (int y=0;y!=source.H();y++)
        for (int x=0;x!=source.W();x++)
        {
            //  The color we'd like this pixel to be
            float color = source.at(x,y);

            //  We look at our position in the dithering matrix
            int xd = x%8;
            int yd = y%8;

            //  We look if we are in the threshold
            bool threshold = dither[xd][yd]<color*64;

            if (threshold)
                dest.at(x,y) = 1;
            else
                dest.at(x,y) = 0;
        }
}

//  ------------------------------------------------------------------
//  Motion floyd-steinberg
//  This will create a black/white 'dest' image from a grayscale 'source'
//  while trying to respect the placement of pixels
//  from the black/white 'previous' image
//  ------------------------------------------------------------------
void quantize( image &dest, const image &source, const image &previous, float stability )
{
    dest = source;

    for (int y=0;y!=source.H();y++)
        for (int x=0;x!=source.W();x++)
        {
            //  The color we'd like this pixel to be
            float source_color = dest.at(x,y);

            //  Increasing the stability value will makes the image choose previous frame's pixel more often
            //  Images will be "stable", but there will be some "ghosting artifacts"

            double stability2 = stability;

/*
            //  We lookup at the real color we are targetting
            float real_color = source[x][y];

            //  We look at our position in the dithering matrix
            int xd = x%8;
            int yd = y%8;

            //  We look if we are in the threshold
            bool threshold = dither[xd][yd]<real_color*63;

            if (threshold)
                stability2 *= 1.5;
            else
                stability2 /= 1.5;
*/

            //  We chose either back or white for this pixel
            //  Starting with the current color, including error propagated form previous pixels,
            //  we decide that:
            //  If previous frame pixel was black, we stay back if color<0.5+stability/2
            //  If previous frame pixel was white, we stay white if color>0.5-stability/2
            float color = source_color<=0.5-(previous.at(x,y)-0.5)*stability2?0:1;
            dest.at(x,y) = color;

            //  By doing this, we made an error (too much white or too much black)
            //  that we need to keep track of
            float error = source_color - color;

            // if (fabs(error)<0.10)
            //     error = 0;

            //  We now distribute the error between the 4 next values
            //  (if they exist). The values may over or underflow
            //  but it is fine as pixels can be <0 or >1
            float e0 = error * 7 / 16;
            float e1 = error * 3 / 16;
            float e2 = error * 5 / 16;
            float e3 = error * 1 / 16;

            if (x<source.W()-1)
                dest.at(x+1,y) = dest.at(x+1,y) + e0;
            if (x>0 && y<source.H()-1)
                dest.at(x-1,y+1) = dest.at(x-1,y+1) + e1;
            if (y<source.H()-1)
                dest.at(x,y+1) = dest.at(x,y+1) + e2;
            if (x<source.W()-1 && y<source.H()-1)
                dest.at(x+1,y+1) = dest.at(x+1,y+1) + e3;
        }
}