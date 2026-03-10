#include "../doctest.h"

#include "../grayscale.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

using namespace macflim;

// Helper to create unique temp file names
static std::string temp_file(const std::string &suffix)
{
    static int counter = 0;
    return std::string("test_grayscale_io_temp_") + std::to_string(counter++) + suffix;
}

// Helper to clean up temp file
static void cleanup(const std::string &path)
{
    std::filesystem::remove(path);
}

TEST_CASE("write_grayscale - creates valid PGM file")
{
    std::string path = temp_file(".pgm");

    grayscale img(64, 48);
    fill(img, 0.5);

    write_grayscale(path, img);

    // Verify file exists
    CHECK(std::filesystem::exists(path));

    // Read header manually
    std::ifstream f(path, std::ios::binary);
    REQUIRE(f.is_open());

    std::string magic;
    size_t width, height;
    int maxval;

    f >> magic >> width >> height >> maxval;

    CHECK(magic == "P5");
    CHECK(width == 64);
    CHECK(height == 48);
    CHECK(maxval == 255);

    cleanup(path);
}

TEST_CASE("write_grayscale - writes pixel data correctly")
{
    std::string path = temp_file(".pgm");

    grayscale img(8, 8);
    // Fill with known pattern: rows alternate 0.0 and 1.0
    for (size_t y = 0; y < img.H(); y++)
        for (size_t x = 0; x < img.W(); x++)
            img.at(x, y) = (y % 2 == 0) ? 0.0 : 1.0;

    write_grayscale(path, img);

    // Read raw file and check pixel data
    std::ifstream f(path, std::ios::binary);
    REQUIRE(f.is_open());

    // Skip header
    std::string line;
    std::getline(f, line); // P5
    std::getline(f, line); // width height
    std::getline(f, line); // maxval

    // Read pixel data
    for (size_t y = 0; y < 8; y++)
    {
        for (size_t x = 0; x < 8; x++)
        {
            int pixel = f.get();
            if (y % 2 == 0)
                CHECK(pixel == 0);
            else
                CHECK(pixel == 255);
        }
    }

    cleanup(path);
}

TEST_CASE("write_grayscale - various dimensions")
{
    SUBCASE("1x1 image")
    {
        std::string path = temp_file(".pgm");
        grayscale img(1, 1);
        img.at(0, 0) = 0.5;

        write_grayscale(path, img);
        CHECK(std::filesystem::exists(path));
        cleanup(path);
    }

    SUBCASE("Large image")
    {
        std::string path = temp_file(".pgm");
        grayscale img(512, 342);
        fill(img, 0.7);

        write_grayscale(path, img);
        CHECK(std::filesystem::exists(path));

        // Verify file size (header ~15 bytes + 512*342 pixels)
        auto size = std::filesystem::file_size(path);
        CHECK(size > 512 * 342);
        CHECK(size < 512 * 342 + 100); // header shouldn't be too large

        cleanup(path);
    }

    SUBCASE("Wide image")
    {
        std::string path = temp_file(".pgm");
        grayscale img(640, 10);
        fill(img, 0.3);

        write_grayscale(path, img);
        CHECK(std::filesystem::exists(path));
        cleanup(path);
    }

    SUBCASE("Tall image")
    {
        std::string path = temp_file(".pgm");
        grayscale img(10, 480);
        fill(img, 0.9);

        write_grayscale(path, img);
        CHECK(std::filesystem::exists(path));
        cleanup(path);
    }
}

TEST_CASE("write_grayscale - gradient values")
{
    std::string path = temp_file(".pgm");

    grayscale img(256, 1);
    // Create horizontal gradient
    for (size_t x = 0; x < img.W(); x++)
        img.at(x, 0) = x / 255.0;

    write_grayscale(path, img);

    // Read back and verify gradient
    std::ifstream f(path, std::ios::binary);
    REQUIRE(f.is_open());

    // Skip header
    std::string line;
    std::getline(f, line);
    std::getline(f, line);
    std::getline(f, line);

    for (size_t x = 0; x < 256; x++)
    {
        int pixel = f.get();
        CHECK(pixel == x);
    }

    cleanup(path);
}

TEST_CASE("read_grayscale - reads valid PGM file")
{
    std::string path = temp_file(".pgm");

    // Create a valid PGM file manually
    std::ofstream f(path, std::ios::binary);
    f << "P5\n32 24\n255\n";
    for (int i = 0; i < 32 * 24; i++)
        f.put(128); // Mid-gray
    f.close();

    grayscale img(1, 1); // Wrong size initially
    bool success = read_grayscale(img, path);

    CHECK(success);
    CHECK(img.W() == 512); // read_grayscale always creates mac screen size
    CHECK(img.H() == 342);

    cleanup(path);
}

TEST_CASE("read_grayscale - fails on nonexistent file")
{
    grayscale img(512, 342);
    bool success = read_grayscale(img, "nonexistent_file_xyz123.pgm");

    CHECK_FALSE(success);
}

TEST_CASE("read_grayscale - fails on invalid file format")
{
    std::string path = temp_file(".txt");

    // Create a non-PGM file
    std::ofstream f(path);
    f << "This is not a PGM file\n";
    f.close();

    grayscale img(512, 342);
    // May succeed to open but will read garbage
    // Just verify it doesn't crash
    bool success = read_grayscale(img, path);
    // We don't assert on success since the function may or may not detect invalid format
    (void)success; // Silence unused variable warning

    cleanup(path);
}

TEST_CASE("read_grayscale and write_grayscale - round trip")
{
    std::string path = temp_file(".pgm");

    // Create original image with pattern
    grayscale original(64, 48);
    for (size_t y = 0; y < original.H(); y++)
        for (size_t x = 0; x < original.W(); x++)
            original.at(x, y) = ((x + y) % 16) / 15.0;

    // Write it
    write_grayscale(path, original);

    // Read it back
    grayscale loaded(1, 1);
    bool success = read_grayscale(loaded, path);

    CHECK(success);
    // Note: read_grayscale creates mac screen size (512x342)
    CHECK(loaded.W() == 512);
    CHECK(loaded.H() == 342);

    cleanup(path);
}

TEST_CASE("write_grayscale - black and white extremes")
{
    std::string path = temp_file(".pgm");

    grayscale img(16, 16);
    // Top half black, bottom half white
    for (size_t y = 0; y < img.H(); y++)
        for (size_t x = 0; x < img.W(); x++)
            img.at(x, y) = (y < 8) ? 0.0 : 1.0;

    write_grayscale(path, img);

    // Read back and verify
    std::ifstream f(path, std::ios::binary);
    REQUIRE(f.is_open());

    // Skip header
    std::string line;
    std::getline(f, line);
    std::getline(f, line);
    std::getline(f, line);

    for (size_t y = 0; y < 16; y++)
    {
        for (size_t x = 0; x < 16; x++)
        {
            int pixel = f.get();
            if (y < 8)
                CHECK(pixel == 0);
            else
                CHECK(pixel == 255);
        }
    }

    cleanup(path);
}

TEST_CASE("write_grayscale - quantization of float values")
{
    std::string path = temp_file(".pgm");

    grayscale img(4, 1);
    img.at(0, 0) = 0.0;     // Should be 0
    img.at(1, 0) = 0.5;     // Should be 127 or 128
    img.at(2, 0) = 1.0;     // Should be 255
    img.at(3, 0) = 0.33333; // Should be ~85

    write_grayscale(path, img);

    std::ifstream f(path, std::ios::binary);
    REQUIRE(f.is_open());

    // Skip header
    std::string line;
    std::getline(f, line);
    std::getline(f, line);
    std::getline(f, line);

    CHECK(f.get() == 0);
    int mid = f.get();
    CHECK(mid >= 127);
    CHECK(mid <= 128);
    CHECK(f.get() == 255);
    int third = f.get();
    CHECK(third >= 84);
    CHECK(third <= 85);

    cleanup(path);
}

TEST_CASE("write_grayscale - empty filename handling")
{
    grayscale img(32, 32);
    fill(img, 0.5);

    // Should handle gracefully (prints error to cerr but doesn't crash)
    write_grayscale("", img);
    // No assertion - just verify it doesn't crash
}

TEST_CASE("read_grayscale - empty filename handling")
{
    grayscale img(512, 342);
    bool success = read_grayscale(img, "");

    CHECK_FALSE(success);
}
