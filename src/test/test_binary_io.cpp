#include "../bitmap.hpp"
#include "../common.hpp"
#include "../doctest.h"
#include "../imgcompress.hpp"

namespace macflim
{

TEST_CASE("Binary I/O - write1")
{
    SUBCASE("write single byte")
    {
        std::vector<uint8_t> result;
        auto out = std::back_inserter(result);
        write1(out, 0x42);
        REQUIRE(result.size() == 1);
        CHECK(result[0] == 0x42);
    }

    SUBCASE("write multiple bytes")
    {
        std::vector<uint8_t> result;
        auto out = std::back_inserter(result);
        write1(out, 0x00);
        write1(out, 0xFF);
        write1(out, 0xAB);
        REQUIRE(result.size() == 3);
        CHECK(result[0] == 0x00);
        CHECK(result[1] == 0xFF);
        CHECK(result[2] == 0xAB);
    }
}

TEST_CASE("Binary I/O - write2")
{
    SUBCASE("write zero")
    {
        std::vector<uint8_t> result;
        auto out = std::back_inserter(result);
        write2(out, 0x0000);
        REQUIRE(result.size() == 2);
        CHECK(result[0] == 0x00);
        CHECK(result[1] == 0x00);
    }

    SUBCASE("write big-endian uint16")
    {
        std::vector<uint8_t> result;
        auto out = std::back_inserter(result);
        write2(out, 0x1234);
        REQUIRE(result.size() == 2);
        CHECK(result[0] == 0x12);
        CHECK(result[1] == 0x34);
    }

    SUBCASE("write max value")
    {
        std::vector<uint8_t> result;
        auto out = std::back_inserter(result);
        write2(out, 0xFFFF);
        REQUIRE(result.size() == 2);
        CHECK(result[0] == 0xFF);
        CHECK(result[1] == 0xFF);
    }
}

TEST_CASE("Binary I/O - write4")
{
    SUBCASE("write zero")
    {
        std::vector<uint8_t> result;
        auto out = std::back_inserter(result);
        write4(out, 0x00000000);
        REQUIRE(result.size() == 4);
        CHECK(result[0] == 0x00);
        CHECK(result[1] == 0x00);
        CHECK(result[2] == 0x00);
        CHECK(result[3] == 0x00);
    }

    SUBCASE("write big-endian uint32")
    {
        std::vector<uint8_t> result;
        auto out = std::back_inserter(result);
        write4(out, 0x12345678);
        REQUIRE(result.size() == 4);
        CHECK(result[0] == 0x12);
        CHECK(result[1] == 0x34);
        CHECK(result[2] == 0x56);
        CHECK(result[3] == 0x78);
    }

    SUBCASE("write max value")
    {
        std::vector<uint8_t> result;
        auto out = std::back_inserter(result);
        write4(out, 0xFFFFFFFF);
        REQUIRE(result.size() == 4);
        CHECK(result[0] == 0xFF);
        CHECK(result[1] == 0xFF);
        CHECK(result[2] == 0xFF);
        CHECK(result[3] == 0xFF);
    }
}

TEST_CASE("Binary I/O - read2")
{
    SUBCASE("read zero")
    {
        uint8_t data[] = {0x00, 0x00};
        const uint8_t *p = data;
        uint16_t result = read2(p);
        CHECK(result == 0x0000);
        CHECK(p == data + 2);
    }

    SUBCASE("read big-endian uint16")
    {
        uint8_t data[] = {0x12, 0x34};
        const uint8_t *p = data;
        uint16_t result = read2(p);
        CHECK(result == 0x1234);
        CHECK(p == data + 2);
    }

    SUBCASE("read max value")
    {
        uint8_t data[] = {0xFF, 0xFF};
        const uint8_t *p = data;
        uint16_t result = read2(p);
        CHECK(result == 0xFFFF);
        CHECK(p == data + 2);
    }
}

TEST_CASE("Binary I/O - read4")
{
    SUBCASE("read zero")
    {
        uint8_t data[] = {0x00, 0x00, 0x00, 0x00};
        const uint8_t *p = data;
        uint32_t result = read4(p);
        CHECK(result == 0x00000000);
        CHECK(p == data + 4);
    }

    SUBCASE("read big-endian uint32")
    {
        uint8_t data[] = {0x12, 0x34, 0x56, 0x78};
        const uint8_t *p = data;
        uint32_t result = read4(p);
        CHECK(result == 0x12345678);
        CHECK(p == data + 4);
    }

    SUBCASE("read max value")
    {
        uint8_t data[] = {0xFF, 0xFF, 0xFF, 0xFF};
        const uint8_t *p = data;
        uint32_t result = read4(p);
        CHECK(result == 0xFFFFFFFF);
        CHECK(p == data + 4);
    }
}

TEST_CASE("Binary I/O - read/write round-trip")
{
    SUBCASE("uint16 round-trip")
    {
        for (uint32_t val : {0x0000, 0x1234, 0x00FF, 0xFF00, 0xFFFF})
        {
            std::vector<uint8_t> buffer;
            auto out = std::back_inserter(buffer);
            write2(out, val);

            const uint8_t *p = buffer.data();
            uint16_t result = read2(p);
            CHECK(result == val);
        }
    }

    SUBCASE("uint32 round-trip")
    {
        std::vector<uint32_t> test_values = {0x00000000, 0x12345678, 0x000000FF, 0xFF000000, 0xFFFFFFFF, 0xDEADBEEF};
        for (uint32_t val : test_values)
        {
            std::vector<uint8_t> buffer;
            auto out = std::back_inserter(buffer);
            write4(out, val);

            const uint8_t *p = buffer.data();
            uint32_t result = read4(p);
            CHECK(result == val);
        }
    }
}

TEST_CASE("Binary I/O - bytes_from_value_be")
{
    SUBCASE("uint8_t")
    {
        auto bytes = bytes_from_value_be<uint8_t>(0x42);
        REQUIRE(bytes.size() == 1);
        CHECK(bytes[0] == 0x42);
    }

    SUBCASE("uint16_t")
    {
        auto bytes = bytes_from_value_be<uint16_t>(0x1234);
        REQUIRE(bytes.size() == 2);
        CHECK(bytes[0] == 0x12);
        CHECK(bytes[1] == 0x34);
    }

    SUBCASE("uint32_t")
    {
        auto bytes = bytes_from_value_be<uint32_t>(0x12345678);
        REQUIRE(bytes.size() == 4);
        CHECK(bytes[0] == 0x12);
        CHECK(bytes[1] == 0x34);
        CHECK(bytes[2] == 0x56);
        CHECK(bytes[3] == 0x78);
    }

    SUBCASE("zero values")
    {
        auto bytes8 = bytes_from_value_be<uint8_t>(0);
        CHECK(bytes8[0] == 0x00);

        auto bytes16 = bytes_from_value_be<uint16_t>(0);
        CHECK(bytes16[0] == 0x00);
        CHECK(bytes16[1] == 0x00);

        auto bytes32 = bytes_from_value_be<uint32_t>(0);
        CHECK(bytes32[0] == 0x00);
        CHECK(bytes32[1] == 0x00);
        CHECK(bytes32[2] == 0x00);
        CHECK(bytes32[3] == 0x00);
    }
}

TEST_CASE("Binary I/O - bytes_from_values_be")
{
    SUBCASE("empty vector")
    {
        std::vector<uint16_t> values;
        auto bytes = bytes_from_values_be(values);
        CHECK(bytes.empty());
    }

    SUBCASE("single uint16")
    {
        std::vector<uint16_t> values = {0x1234};
        auto bytes = bytes_from_values_be(values);
        REQUIRE(bytes.size() == 2);
        CHECK(bytes[0] == 0x12);
        CHECK(bytes[1] == 0x34);
    }

    SUBCASE("multiple uint16")
    {
        std::vector<uint16_t> values = {0x1234, 0x5678, 0xABCD};
        auto bytes = bytes_from_values_be(values);
        REQUIRE(bytes.size() == 6);
        CHECK(bytes[0] == 0x12);
        CHECK(bytes[1] == 0x34);
        CHECK(bytes[2] == 0x56);
        CHECK(bytes[3] == 0x78);
        CHECK(bytes[4] == 0xAB);
        CHECK(bytes[5] == 0xCD);
    }

    SUBCASE("uint32 values")
    {
        std::vector<uint32_t> values = {0x12345678, 0x9ABCDEF0};
        auto bytes = bytes_from_values_be(values);
        REQUIRE(bytes.size() == 8);
        CHECK(bytes[0] == 0x12);
        CHECK(bytes[1] == 0x34);
        CHECK(bytes[2] == 0x56);
        CHECK(bytes[3] == 0x78);
        CHECK(bytes[4] == 0x9A);
        CHECK(bytes[5] == 0xBC);
        CHECK(bytes[6] == 0xDE);
        CHECK(bytes[7] == 0xF0);
    }
}

TEST_CASE("Binary I/O - mypopcount")
{
    CHECK(mypopcount(0) == 0);
    CHECK(mypopcount(1) == 1);
    CHECK(mypopcount(2) == 1);
    CHECK(mypopcount(3) == 2);
    CHECK(mypopcount(7) == 3);
    CHECK(mypopcount(8) == 1);
    CHECK(mypopcount(15) == 4);
    CHECK(mypopcount(0xFF) == 8);
    CHECK(mypopcount(0xFFFF) == 16);
    CHECK(mypopcount(0xFFFFFFFF) == 32);
    CHECK(mypopcount(0xAAAAAAAA) == 16); // alternating bits
    CHECK(mypopcount(0x55555555) == 16); // alternating bits
}

TEST_CASE("Binary I/O - offset_t")
{
    SUBCASE("4x4 bitmap traversal")
    {
        offset_t offset(4, 4);

        // First column: offsets 0, 4, 8, 12
        CHECK(offset.linear() == 0);
        CHECK_FALSE(offset.increment());
        CHECK(offset.linear() == 4);
        CHECK_FALSE(offset.increment());
        CHECK(offset.linear() == 8);
        CHECK_FALSE(offset.increment());
        CHECK(offset.linear() == 12);
        CHECK(offset.increment()); // Wraps to column 2

        // Second column: offsets 1, 5, 9, 13
        CHECK(offset.linear() == 1);
        CHECK_FALSE(offset.increment());
        CHECK(offset.linear() == 5);
    }

    SUBCASE("2x3 bitmap complete traversal")
    {
        offset_t offset(2, 3);
        std::vector<size_t> positions;

        for (int i = 0; i < 6; i++)
        {
            positions.push_back(offset.linear());
            offset.increment();
        }

        // Should visit: column 0 (0,2,4), then column 1 (1,3,5)
        CHECK(positions[0] == 0);
        CHECK(positions[1] == 2);
        CHECK(positions[2] == 4);
        CHECK(positions[3] == 1);
        CHECK(positions[4] == 3);
        CHECK(positions[5] == 5);
    }

    SUBCASE("1x1 bitmap")
    {
        offset_t offset(1, 1);
        CHECK(offset.linear() == 0);
        CHECK(offset.increment());   // Wraps immediately after first position
        CHECK(offset.linear() == 1); // Wraps back, offset becomes 1 (not 0)
    }
}

} // namespace macflim
