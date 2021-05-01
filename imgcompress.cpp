#include "imgcompress.hpp"

//  ------------------------------------------------------------------
//  Almost completely tested implementation of packbits
//  Compresses 'length' bytes from 'buffer' into 'out', and return the compressed size
//  (unused for now)
//  ------------------------------------------------------------------
int packbits( u_int8_t *out, const u_int8_t *buffer, int length )
{
    const u_int8_t *orig = out;
    const u_int8_t *end = buffer+length;

    while (buffer<end)
    {
        //  We look for the next pair of identical characters
        const u_int8_t *next_pair = buffer;
        for (next_pair = buffer;next_pair<end-1;next_pair++)
            if (next_pair[0]==next_pair[1])
                break;

        //  If we didn't find a pair up to the last two chars, we skip to the end
        if (next_pair==end-1)
            next_pair = end;

        //  All character until next_pair don't repeat
        if (next_pair!=buffer)
        {
                //  We have to write len litterals
            u_int32_t len = next_pair-buffer;
            while (len)
            {
                    //  We can write at most 128 literals in one go
                u_int8_t sub_length = len>128?128:len;
                len -= sub_length;
                *out++ = sub_length-1;
                while (sub_length--)
                    *out++ = *buffer++;
            }
        }

        assert( buffer==next_pair );

        //  Now, we are at the start of the next run, or at the end of the stream
        if (buffer==end)
            break;

        assert( buffer<end-1 ); //  As we have a run, we have at least two chars

        u_int8_t c = *buffer;

        //  Find the len of the run
        int len = 0;
        while (*buffer==c)
        {
            len++;
            buffer++;
            if (len==128)
                break;
            if (buffer==end)
                break;
        }

        *out++ = -len+1;
        *out++ = c;

        //  We don't care about the fact that the run may continue, it will be handled by the next loop iteration
    }

    return out-orig;
}

void pack_test()
{
    u_int8_t in0[] =
    {
        0xAA, 0xAA, 0xAA, 0x80, 0x00, 0x2A, 0xAA, 0xAA, 0xAA,
        0xAA, 0x80, 0x00, 0x2A, 0x22, 0xAA, 0xAA, 0xAA, 0xAA,
        0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA
    };
    u_int8_t out0[] =
    {
        0xFE, 0xAA, 0x02, 0x80, 0x00, 0x2A, 0xFD, 0xAA, 0x03,
        0x80, 0x00, 0x2A, 0x22, 0xF7, 0xAA
    };

    u_int8_t buffer[1024];
    int len;
    
    len = packbits( buffer, in0, sizeof(in0) );
    assert( len==sizeof(out0) );
    assert( memcmp( buffer, out0, len )==0 );

    u_int8_t in1[] = {};
    u_int8_t out1[] = {};

    len = packbits( buffer, in1, sizeof(in1) );
    assert( len==sizeof(out1) );
    assert( memcmp( buffer, out1, len )==0 );

    u_int8_t in2[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    };
    u_int8_t out2[] = { 0x81, 0x00, 0xF1, 0x00 };

    len = packbits( buffer, in2, sizeof(in2) );
    assert( len==sizeof(out2) );
    assert( memcmp( buffer, out2, len )==0 );

    u_int8_t in3[] = {
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
    };

    u_int8_t out3[] = {
        0x7f,
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x0f,
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
    };

    len = packbits( buffer, in3, sizeof(in3) );
    assert( len==sizeof(out3) );
    assert( memcmp( buffer, out3, len )==0 );
}

/*
Another packing idea for run-length encoding at bit level. Would be goot for long black or white sequences, but would be awful on 50% grays

0=>7 bits of data
10=> up to 32+7 black
11=> up to 32+7 white

Take next 7 bits
If all equal => encode equal
Else encode specific
*/
int packzeroes( u_int8_t *out, const u_int8_t *const buffer, int length )
{
    const u_int8_t *orig = out;
    const u_int8_t *start = buffer;
    const u_int8_t *end = start+length;

    while (start<end)
    {
        //  We look for the next zero
        const u_int8_t *next_zero;
        for (next_zero = start;next_zero<end;next_zero++)
        {
            if (next_zero==end-1 && !*next_zero)
                break;
            if (!*next_zero && *(next_zero+1)==0)
                break;
        }

        //  All character until next_zero don't repeat
        if (next_zero!=start)
        {
                //  We have to write len litterals
            u_int32_t len = next_zero-start;
            while (len)
            {
                    //  We can write at most 127 literals in one go
                u_int8_t sub_length = len>127?127:len;
                len -= sub_length;
                *out++ = sub_length;
                while (sub_length--)
                    *out++ = *start++;
            }
        }

        assert( next_zero==start );

        //  Now, we are at the start of the next run of zeroes, or at the end of the stream
        if (start==end)
            break;

        //  Find the len of the run
        int len = 0;
        while (!*start)
        {
            len++;
            start++;
            if (len==128)
                break;
            if (start==end)
                break;
        }
        *out++ = -len;

            // fprintf( stderr, "%d ", len );
        //  We don't care about the fact that the run may continue, it will be handled by the next loop iteration
    }

    *out++ = 0;

    // for (int i=0;i!=std::min(32768,length);i++)
    //     printf( "-> %02x ", buffer[i] );
    // printf( "\n" );

    // for (int i=0;i!=std::min(32768L,out-orig);i++)
    //     printf( "<- %02x ", orig[i] );
    // printf( "\n" );

    return out-orig;
}

//  ------------------------------------------------------------------
//  Unpack zero-packed data and xor it into destination
//  Should return pair of new (source,dest)
//  ------------------------------------------------------------------
void unpackzeroesx( char *d, const char *s, size_t maxlen )
{
    char *m = d+maxlen;
    while (1)
    {
        auto n = *s++;
        if (n==0)
            break;
        if (n>0)
        {
            while (n--)
                *d++ ^= *s++;
        }
        else
            d -= n;
        assert( d<=m );
    }
}




int packz32( u_int32_t *out, const u_int32_t *const buffer, int length )
{
    const u_int32_t *orig = out;
    const u_int32_t *start = buffer;
    const u_int32_t *end = start+length;

    while (start<end)
    {
        //  We look for the next non-zero
        const u_int32_t *next_non_zero;
        for (next_non_zero = start;next_non_zero<end;next_non_zero++)
        {
            if (*next_non_zero)
                break;
        }

        //  Here, we have:
        //  [ 0 0 0 0 0 ] x0 x1 x2

        u_int16_t zero_count = next_non_zero-start;
        start += zero_count;

        //  We look for the next zero
        const u_int32_t *next_zero;
        for (next_zero = start;next_zero<end;next_zero++)
        {
            if (!*next_zero)
                break;
        }

        u_int16_t non_zero_count = next_zero-start;

        u_int32_t header = (zero_count<<16)+non_zero_count;

        *out++ = header;

        while (non_zero_count--)
            *out++ = *start++;
    }

    *out++ = 0;

    return out-orig;
}

#include <iostream>

void packz32_test()
{
    u_int32_t in0[] =
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000002, 0x00000003, 0x00000000
    };
    u_int32_t out0[] =
    {
        0x00040003, 0x00000001, 0x00000002, 0x00000003, 0x00010000, 0x00000000
    };

    u_int32_t buffer[1024];
    int len;
    
    len = packz32( buffer, in0, sizeof(in0)/sizeof(*in0) );
    std::clog << len << "\n";

    assert( len==sizeof(out0)/sizeof(*in0) );
    assert( memcmp( buffer, out0, len )==0 );
}


void packz32opt_test()
{
    std::vector<u_int32_t> in0 =
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000002, 0x00000003, 0x00000000
    };
    std::vector<bool> in0b =
    {
        false, false, false, false, true, true, true, false
    };
    std::vector<uint32_t> out0 =
    {
        0x00040003, 0x00000001,
        0x00000002, 0x00000003,
        0x00000000
    };

    auto res0 = packz32opt( in0, in0b );

    assert( res0==out0 );
}

