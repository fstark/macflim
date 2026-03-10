#include "../doctest.h"

#include "../common.hpp"
#include "../errors.hpp"
#include "../flim.hpp"

#include <cstring>
#include <filesystem>

namespace macflim
{

// Constants from flim class (private, so replicated here for testing)
constexpr size_t FLIM_COMMENT_SIZE = 1022;
constexpr size_t FLIM_CHECKSUM_SIZE = 2;

TEST_CASE("flim constructor - comment size handling")
{
    SUBCASE("empty comment gets resized and prefixed")
    {
        flim f("");
        CHECK(f.comment().size() == FLIM_COMMENT_SIZE);
        CHECK(f.comment().substr(0, 5) == "FLIM\n");
    }

    SUBCASE("short comment gets padded")
    {
        flim f("Hello World XYZ");
        CHECK(f.comment().size() == FLIM_COMMENT_SIZE);
        CHECK(f.comment().substr(0, 5) == "FLIM\n");
        // The first 5 chars of input are overwritten with "FLIM\n"
        // Input: "Hello World XYZ" -> "FLIM\n World XYZ" + padding
        CHECK(f.comment().substr(5, 10) == " World XYZ");
    }

    SUBCASE("exact size comment gets prefixed")
    {
        std::string long_comment(FLIM_COMMENT_SIZE, 'X');
        flim f(long_comment);
        CHECK(f.comment().size() == FLIM_COMMENT_SIZE);
        CHECK(f.comment().substr(0, 5) == "FLIM\n");
    }

    SUBCASE("oversized comment gets truncated")
    {
        std::string huge_comment(FLIM_COMMENT_SIZE + 100, 'Z');
        flim f(huge_comment);
        CHECK(f.comment().size() == FLIM_COMMENT_SIZE);
        CHECK(f.comment().substr(0, 5) == "FLIM\n");
    }
}

TEST_CASE("flim read - invalid signature")
{
    std::string temp_path = "/tmp/test_flim_bad_signature.flim";

    SUBCASE("wrong signature throws flim_error")
    {
        // Write file with bad signature
        {
            file_handle fh(temp_path, "wb");
            std::vector<char> bad_data(FLIM_COMMENT_SIZE, 0);
            memcpy(bad_data.data(), "BADH\n", 5); // Wrong signature
            fwrite(bad_data.data(), 1, FLIM_COMMENT_SIZE, fh.get());

            // Write valid checksum and header
            uint8_t checksum[2] = {0, 0};
            fwrite(checksum, 1, 2, fh.get());
            uint8_t header[4] = {0, 1, 0, 0}; // version 1, 0 components
            fwrite(header, 1, 4, fh.get());
        }

        // Try to read it
        {
            file_handle fh(temp_path, "rb");
            flim f;
            CHECK_THROWS_AS(f.read(fh), flim_error);
        }

        std::filesystem::remove(temp_path);
    }

    SUBCASE("missing newline in signature")
    {
        // Write file with signature missing newline
        {
            file_handle fh(temp_path, "wb");
            std::vector<char> bad_data(FLIM_COMMENT_SIZE, 0);
            memcpy(bad_data.data(), "FLIMX", 5); // Wrong 5th character
            fwrite(bad_data.data(), 1, FLIM_COMMENT_SIZE, fh.get());

            uint8_t checksum[2] = {0, 0};
            fwrite(checksum, 1, 2, fh.get());
            uint8_t header[4] = {0, 1, 0, 0};
            fwrite(header, 1, 4, fh.get());
        }

        {
            file_handle fh(temp_path, "rb");
            flim f;
            CHECK_THROWS_AS(f.read(fh), flim_error);
        }

        std::filesystem::remove(temp_path);
    }
}

TEST_CASE("flim read - truncated file errors")
{
    std::string temp_path = "/tmp/test_flim_truncated.flim";

    SUBCASE("truncated comment block")
    {
        // Write incomplete comment block
        {
            file_handle fh(temp_path, "wb");
            std::vector<char> partial_comment(FLIM_COMMENT_SIZE / 2, 0);
            memcpy(partial_comment.data(), "FLIM\n", 5);
            fwrite(partial_comment.data(), 1, partial_comment.size(), fh.get());
        }

        {
            file_handle fh(temp_path, "rb");
            flim f;
            CHECK_THROWS_AS(f.read(fh), flim_error);
        }

        std::filesystem::remove(temp_path);
    }

    SUBCASE("missing checksum")
    {
        // Write full comment but no checksum
        {
            file_handle fh(temp_path, "wb");
            std::vector<char> comment(FLIM_COMMENT_SIZE, 0);
            memcpy(comment.data(), "FLIM\n", 5);
            fwrite(comment.data(), 1, FLIM_COMMENT_SIZE, fh.get());
            // No checksum written
        }

        {
            file_handle fh(temp_path, "rb");
            flim f;
            CHECK_THROWS_AS(f.read(fh), flim_error);
        }

        std::filesystem::remove(temp_path);
    }

    SUBCASE("truncated checksum")
    {
        // Write comment + partial checksum
        {
            file_handle fh(temp_path, "wb");
            std::vector<char> comment(FLIM_COMMENT_SIZE, 0);
            memcpy(comment.data(), "FLIM\n", 5);
            fwrite(comment.data(), 1, FLIM_COMMENT_SIZE, fh.get());
            uint8_t partial_checksum[1] = {0};
            fwrite(partial_checksum, 1, 1, fh.get()); // Only 1 byte of 2
        }

        {
            file_handle fh(temp_path, "rb");
            flim f;
            CHECK_THROWS_AS(f.read(fh), flim_error);
        }

        std::filesystem::remove(temp_path);
    }

    SUBCASE("missing header")
    {
        // Write comment + checksum but no header
        {
            file_handle fh(temp_path, "wb");
            std::vector<char> comment(FLIM_COMMENT_SIZE, 0);
            memcpy(comment.data(), "FLIM\n", 5);
            fwrite(comment.data(), 1, FLIM_COMMENT_SIZE, fh.get());
            uint8_t checksum[2] = {0, 0};
            fwrite(checksum, 1, 2, fh.get());
            // No header written
        }

        {
            file_handle fh(temp_path, "rb");
            flim f;
            CHECK_THROWS_AS(f.read(fh), flim_error);
        }

        std::filesystem::remove(temp_path);
    }

    SUBCASE("truncated header")
    {
        // Write comment + checksum + partial header
        {
            file_handle fh(temp_path, "wb");
            std::vector<char> comment(FLIM_COMMENT_SIZE, 0);
            memcpy(comment.data(), "FLIM\n", 5);
            fwrite(comment.data(), 1, FLIM_COMMENT_SIZE, fh.get());
            uint8_t checksum[2] = {0, 0};
            fwrite(checksum, 1, 2, fh.get());
            uint8_t partial_header[2] = {0, 1}; // Only version, no component count
            fwrite(partial_header, 1, 2, fh.get());
        }

        {
            file_handle fh(temp_path, "rb");
            flim f;
            CHECK_THROWS_AS(f.read(fh), flim_error);
        }

        std::filesystem::remove(temp_path);
    }

    SUBCASE("truncated component directory")
    {
        // Write header claiming 1 component, but provide incomplete directory
        {
            file_handle fh(temp_path, "wb");
            std::vector<char> comment(FLIM_COMMENT_SIZE, 0);
            memcpy(comment.data(), "FLIM\n", 5);
            fwrite(comment.data(), 1, FLIM_COMMENT_SIZE, fh.get());
            uint8_t checksum[2] = {0, 0};
            fwrite(checksum, 1, 2, fh.get());
            uint8_t header[4] = {0, 1, 0, 1}; // version 1, 1 component
            fwrite(header, 1, 4, fh.get());
            // Component directory should be 10 bytes, write only 5
            uint8_t partial_dir[5] = {0, 0, 0, 0, 0};
            fwrite(partial_dir, 1, 5, fh.get());
        }

        {
            file_handle fh(temp_path, "rb");
            flim f;
            CHECK_THROWS_AS(f.read(fh), flim_error);
        }

        std::filesystem::remove(temp_path);
    }

    SUBCASE("missing component data")
    {
        // Write complete header claiming component with data, but no actual data
        {
            file_handle fh(temp_path, "wb");
            std::vector<char> comment(FLIM_COMMENT_SIZE, 0);
            memcpy(comment.data(), "FLIM\n", 5);
            fwrite(comment.data(), 1, FLIM_COMMENT_SIZE, fh.get());
            uint8_t checksum[2] = {0, 0};
            fwrite(checksum, 1, 2, fh.get());
            uint8_t header[4] = {0, 1, 0, 1}; // version 1, 1 component
            fwrite(header, 1, 4, fh.get());

            // Component directory: type=0, offset=0, size=100
            uint8_t dir[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 100};
            fwrite(dir, 1, 10, fh.get());
            // No component data written (should have 100 bytes)
        }

        {
            file_handle fh(temp_path, "rb");
            flim f;
            CHECK_THROWS_AS(f.read(fh), flim_error);
        }

        std::filesystem::remove(temp_path);
    }

    SUBCASE("truncated component data")
    {
        // Write component data but not enough
        {
            file_handle fh(temp_path, "wb");
            std::vector<char> comment(FLIM_COMMENT_SIZE, 0);
            memcpy(comment.data(), "FLIM\n", 5);
            fwrite(comment.data(), 1, FLIM_COMMENT_SIZE, fh.get());
            uint8_t checksum[2] = {0, 0};
            fwrite(checksum, 1, 2, fh.get());
            uint8_t header[4] = {0, 1, 0, 1}; // version 1, 1 component
            fwrite(header, 1, 4, fh.get());

            // Component directory: type=0, offset=0, size=50
            uint8_t dir[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 50};
            fwrite(dir, 1, 10, fh.get());

            // Write only 25 bytes of 50
            std::vector<uint8_t> partial_data(25, 0xAA);
            fwrite(partial_data.data(), 1, 25, fh.get());
        }

        {
            file_handle fh(temp_path, "rb");
            flim f;
            CHECK_THROWS_AS(f.read(fh), flim_error);
        }

        std::filesystem::remove(temp_path);
    }
}

TEST_CASE("flim read - empty file errors")
{
    std::string temp_path = "/tmp/test_flim_empty.flim";

    SUBCASE("completely empty file")
    {
        // Create empty file
        {
            file_handle fh(temp_path, "wb");
            // Write nothing
        }

        {
            file_handle fh(temp_path, "rb");
            flim f;
            CHECK_THROWS_AS(f.read(fh), flim_error);
        }

        std::filesystem::remove(temp_path);
    }
}

} // namespace macflim
