#include "imgcompress.hpp"

namespace macflim
{

    //  ------------------------------------------------------------------
    //  Almost completely tested implementation of packbits
    //  Compresses 'length' bytes from 'buffer' into 'out', and return the compressed size
    //  (unused for now)
    //  ------------------------------------------------------------------
    int packbits(uint8_t *out, const uint8_t *buffer, int length)
    {
        const uint8_t *orig = out;
        const uint8_t *end = buffer + length;

        while (buffer < end)
        {
            //  We look for the next pair of identical characters
            const uint8_t *next_pair = buffer;
            for (next_pair = buffer; next_pair < end - 1; next_pair++)
                if (next_pair[0] == next_pair[1])
                    break;

            //  If we didn't find a pair up to the last two chars, we skip to the end
            if (next_pair == end - 1)
                next_pair = end;

            //  All character until next_pair don't repeat
            if (next_pair != buffer)
            {
                //  We have to write len litterals
                uint32_t len = next_pair - buffer;
                while (len)
                {
                    //  We can write at most 128 literals in one go
                    uint8_t sub_length = len > 128 ? 128 : len;
                    len -= sub_length;
                    *out++ = sub_length - 1;
                    while (sub_length--)
                        *out++ = *buffer++;
                }
            }

            assert(buffer == next_pair);

            //  Now, we are at the start of the next run, or at the end of the stream
            if (buffer == end)
                break;

            assert(buffer < end - 1); //  As we have a run, we have at least two chars

            uint8_t c = *buffer;

            //  Find the len of the run
            int len = 0;
            while (*buffer == c)
            {
                len++;
                buffer++;
                if (len == 128)
                    break;
                if (buffer == end)
                    break;
            }

            *out++ = -len + 1;
            *out++ = c;

            //  We don't care about the fact that the run may continue, it will be handled by the next loop iteration
        }

        return out - orig;
    }

    void pack_test()
    {
        uint8_t in0[] =
            {
                0xAA, 0xAA, 0xAA, 0x80, 0x00, 0x2A, 0xAA, 0xAA, 0xAA,
                0xAA, 0x80, 0x00, 0x2A, 0x22, 0xAA, 0xAA, 0xAA, 0xAA,
                0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
        uint8_t out0[] =
            {
                0xFE, 0xAA, 0x02, 0x80, 0x00, 0x2A, 0xFD, 0xAA, 0x03,
                0x80, 0x00, 0x2A, 0x22, 0xF7, 0xAA};

        uint8_t buffer[1024];
        int len;

        len = packbits(buffer, in0, sizeof(in0));
        assert(len == sizeof(out0));
        assert(memcmp(buffer, out0, len) == 0);

        uint8_t in1[] = {};
        uint8_t out1[] = {};

        len = packbits(buffer, in1, sizeof(in1));
        assert(len == sizeof(out1));
        assert(memcmp(buffer, out1, len) == 0);

        uint8_t in2[] = {
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
        };
        uint8_t out2[] = {0x81, 0x00, 0xF1, 0x00};

        len = packbits(buffer, in2, sizeof(in2));
        assert(len == sizeof(out2));
        assert(memcmp(buffer, out2, len) == 0);

        uint8_t in3[] = {
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
        };

        uint8_t out3[] = {
            0x7f,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x0f,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
            0x00,
            0x01,
        };

        len = packbits(buffer, in3, sizeof(in3));
        assert(len == sizeof(out3));
        assert(memcmp(buffer, out3, len) == 0);
    }

    void packz32opt_test()
    {
        std::vector<uint32_t> in0 =
            {
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000002, 0x00000003, 0x00000000,
                0x00000000, 0x00000005, 0x00000006, 0x00000007};
        std::vector<bool> in0b =
            {
                false, false, false, false, true, true, true, false,
                false, true, true, true};
        std::vector<run<uint32_t>> out0 =
            {
                run<uint32_t>{4, {1, 2, 3}},
                run<uint32_t>{9, {5, 6, 7}}};

        auto res0 = pack<uint32_t>(std::begin(in0), std::begin(in0b), std::end(in0b), 1024, 1, 100);

        assert(res0 == out0);
    }

} // namespace macflim
