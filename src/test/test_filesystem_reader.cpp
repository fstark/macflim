#include "../filesystem_reader.hpp"

#include "../doctest.h"

#include <filesystem>

namespace macflim
{

// Helper to create unique temp file patterns
static std::string temp_pattern()
{
    static int counter = 0;
    return "test_fs_reader_" + std::to_string(counter++) + "_%04d.pgm";
}

// Helper to clean up test files
static void cleanup_files(const std::string &pattern, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        std::string filename = simplesprintf(pattern, i);
        std::filesystem::remove(filename);
    }
}

TEST_CASE("filesystem_reader - reads single frame")
{
    std::string pattern = temp_pattern();

    // Create one test image
    grayscale img(512, 342);
    fill(img, 0.3);
    std::string filename = simplesprintf(pattern, 0);
    write_grayscale(filename, img);

    // Create reader (function expects non-const references)
    std::string audio_path = "";
    auto reader = make_filesystem_reader(pattern, 24.0, audio_path, 0, 1);

    // Should return frame rate
    CHECK(reader->frame_rate() == 24.0);

    // Read first frame
    auto frame = reader->next();
    REQUIRE(frame != nullptr);
    CHECK(frame->W() == 512);
    CHECK(frame->H() == 342);

    // Next call should return nullptr (end of sequence)
    auto frame2 = reader->next();
    CHECK(frame2 == nullptr);

    // Subsequent calls should also return nullptr
    auto frame3 = reader->next();
    CHECK(frame3 == nullptr);

    cleanup_files(pattern, 1);
}

TEST_CASE("filesystem_reader - reads multiple frames")
{
    std::string pattern = temp_pattern();
    const size_t frame_count = 5;

    // Create test images with different brightness levels
    for (size_t i = 0; i < frame_count; i++)
    {
        grayscale img(512, 342);
        fill(img, i * 0.2); // 0.0, 0.2, 0.4, 0.6, 0.8
        std::string filename = simplesprintf(pattern, i);
        write_grayscale(filename, img);
    }

    // Create reader
    std::string audio_path = "";
    auto reader = make_filesystem_reader(pattern, 30.0, audio_path, 0, frame_count);

    CHECK(reader->frame_rate() == 30.0);

    // Read all frames
    for (size_t i = 0; i < frame_count; i++)
    {
        auto frame = reader->next();
        REQUIRE(frame != nullptr);
        CHECK(frame->W() == 512);
        CHECK(frame->H() == 342);
    }

    // Should be done now
    auto last = reader->next();
    CHECK(last == nullptr);

    cleanup_files(pattern, frame_count);
}

TEST_CASE("filesystem_reader - reads from offset")
{
    std::string pattern = temp_pattern();
    const size_t total_frames = 10;
    const size_t from_frame = 5;
    const size_t count = 3;

    // Create 10 test images
    for (size_t i = 0; i < total_frames; i++)
    {
        grayscale img(512, 342);
        fill(img, i * 0.1);
        std::string filename = simplesprintf(pattern, i);
        write_grayscale(filename, img);
    }

    // Read only frames 5, 6, 7
    std::string audio_path = "";
    auto reader = make_filesystem_reader(pattern, 15.0, audio_path, from_frame, count);

    // Should read exactly 3 frames
    for (size_t i = 0; i < count; i++)
    {
        auto frame = reader->next();
        REQUIRE(frame != nullptr);
    }

    // Should be done
    auto done = reader->next();
    CHECK(done == nullptr);

    cleanup_files(pattern, total_frames);
}

TEST_CASE("filesystem_reader - handles missing file")
{
    std::string pattern = temp_pattern();

    // Create only frame 0 and 2, skip frame 1
    grayscale img(512, 342);
    fill(img, 0.5);
    std::string filename0 = simplesprintf(pattern, 0);
    std::string filename2 = simplesprintf(pattern, 2);
    write_grayscale(filename0, img);
    write_grayscale(filename2, img);

    // Try to read 3 frames (0, 1, 2) - should fail at frame 1
    std::string audio_path = "";
    auto reader = make_filesystem_reader(pattern, 24.0, audio_path, 0, 3);

    // Frame 0 should work
    auto frame0 = reader->next();
    CHECK(frame0 != nullptr);

    // Frame 1 is missing, should return nullptr
    auto frame1 = reader->next();
    CHECK(frame1 == nullptr);

    // Should stay nullptr
    auto frame2 = reader->next();
    CHECK(frame2 == nullptr);

    std::filesystem::remove(filename0);
    std::filesystem::remove(filename2);
}

TEST_CASE("filesystem_reader - handles nonexistent first file")
{
    std::string pattern = "nonexistent_pattern_%04d.pgm";
    std::string audio_path = "";
    auto reader = make_filesystem_reader(pattern, 24.0, audio_path, 0, 5);

    // First call should fail and return nullptr
    auto frame = reader->next();
    CHECK(frame == nullptr);

    // Subsequent calls should also return nullptr
    auto frame2 = reader->next();
    CHECK(frame2 == nullptr);
}

TEST_CASE("filesystem_reader - different frame rates")
{
    std::string pattern = temp_pattern();

    grayscale img(512, 342);
    fill(img, 0.5);
    std::string filename = simplesprintf(pattern, 0);
    write_grayscale(filename, img);

    SUBCASE("Low frame rate")
    {
        std::string audio_path = "";
        auto reader = make_filesystem_reader(pattern, 10.0, audio_path, 0, 1);
        CHECK(reader->frame_rate() == 10.0);
    }

    SUBCASE("High frame rate")
    {
        std::string audio_path = "";
        auto reader = make_filesystem_reader(pattern, 60.0, audio_path, 0, 1);
        CHECK(reader->frame_rate() == 60.0);
    }

    SUBCASE("Standard 24 fps")
    {
        std::string audio_path = "";
        auto reader = make_filesystem_reader(pattern, 24.0, audio_path, 0, 1);
        CHECK(reader->frame_rate() == 24.0);
    }

    std::filesystem::remove(filename);
}

TEST_CASE("filesystem_reader - zero count")
{
    std::string pattern = temp_pattern();

    grayscale img(512, 342);
    fill(img, 0.5);
    std::string filename = simplesprintf(pattern, 0);
    write_grayscale(filename, img);

    // Create reader with count=0
    std::string audio_path = "";
    auto reader = make_filesystem_reader(pattern, 24.0, audio_path, 0, 0);

    // Should immediately return nullptr
    auto frame = reader->next();
    CHECK(frame == nullptr);

    std::filesystem::remove(filename);
}

TEST_CASE("filesystem_reader - next_sound returns nullptr")
{
    std::string pattern = temp_pattern();

    grayscale img(512, 342);
    fill(img, 0.5);
    std::string filename = simplesprintf(pattern, 0);
    write_grayscale(filename, img);

    std::string audio_path = "test_audio.raw"; // Audio not actually implemented
    auto reader = make_filesystem_reader(pattern, 24.0, audio_path, 0, 1);

    // next_sound() should return nullptr (not implemented)
    auto sound = reader->next_sound();
    CHECK(sound == nullptr);

    std::filesystem::remove(filename);
}

TEST_CASE("filesystem_reader - pattern with different digit counts")
{
    SUBCASE("2 digits")
    {
        std::string pattern = temp_pattern();
        // Replace %04d with %02d in the pattern
        size_t pos = pattern.find("%04d");
        if (pos != std::string::npos)
        {
            pattern.replace(pos, 4, "%02d");
        }

        grayscale img(512, 342);
        fill(img, 0.5);
        std::string filename = simplesprintf(pattern, 0);
        write_grayscale(filename, img);

        std::string audio_path = "";
        auto reader = make_filesystem_reader(pattern, 24.0, audio_path, 0, 1);

        auto frame = reader->next();
        CHECK(frame != nullptr);

        std::filesystem::remove(filename);
    }
}

TEST_CASE("filesystem_reader - large frame count")
{
    std::string pattern = temp_pattern();
    const size_t large_count = 100;

    // Create many test images
    for (size_t i = 0; i < large_count; i++)
    {
        grayscale img(512, 342);
        fill(img, (i % 10) * 0.1);
        std::string filename = simplesprintf(pattern, i);
        write_grayscale(filename, img);
    }

    // Read all of them
    std::string audio_path = "";
    auto reader = make_filesystem_reader(pattern, 24.0, audio_path, 0, large_count);

    size_t read_count = 0;
    while (auto frame = reader->next())
    {
        CHECK(frame->W() == 512);
        CHECK(frame->H() == 342);
        read_count++;
    }

    CHECK(read_count == large_count);

    cleanup_files(pattern, large_count);
}

} // namespace macflim
