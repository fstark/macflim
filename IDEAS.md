# Ideas for Future Features

## Alternative Compression: Zero-Run Encoding at Bit Level

An idea for run-length encoding at bit level. Would be good for long black or white sequences, but would be awful on 50% grays.

```
0=>7 bits of data
10=> up to 32+7 black
11=> up to 32+7 white

Take next 7 bits
If all equal => encode equal
Else encode specific
```

### Implementation: `packzeroes`

```cpp
int packzeroes( uint8_t *out, const uint8_t *const buffer, int length )
{
    const uint8_t *orig = out;
    const uint8_t *start = buffer;
    const uint8_t *end = start+length;

    while (start<end)
    {
        //  We look for the next zero
        const uint8_t *next_zero;
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
            uint32_t len = next_zero-start;
            while (len)
            {
                    //  We can write at most 127 literals in one go
                uint8_t sub_length = len>127?127:len;
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
```

```
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
```

```
int packz32( uint32_t *out, const uint32_t *const buffer, int length )
{
    const uint32_t *orig = out;
    const uint32_t *start = buffer;
    const uint32_t *end = start+length;

    while (start<end)
    {
        //  We look for the next non-zero
        const uint32_t *next_non_zero;
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
        const uint32_t *next_zero;
        for (next_zero = start;next_zero<end;next_zero++)
        {
            if (!*next_zero)
                break;
        }

        u_int16_t non_zero_count = next_zero-start;

        uint32_t header = (zero_count<<16)+non_zero_count;

        *out++ = header;

        while (non_zero_count--)
            *out++ = *start++;
    }

    *out++ = 0;

    return out-orig;
}
```

```
void packz32_test()
{
    uint32_t in0[] =
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000002, 0x00000003, 0x00000000
    };
    uint32_t out0[] =
    {
        0x00040003, 0x00000001, 0x00000002, 0x00000003, 0x00010000, 0x00000000
    };

    uint32_t buffer[1024];
    int len;
    
    len = packz32( buffer, in0, sizeof(in0)/sizeof(*in0) );
    std::clog << len << "\n";

    assert( len==sizeof(out0)/sizeof(*in0) );
    assert( memcmp( buffer, out0, len )==0 );
}
```
