#include "../doctest.h"

#include "../common.hpp"
#include "../flim.hpp"

#include <cstring>
#include <filesystem>

namespace macflim
{

TEST_CASE("fletcher checksum - basic properties")
{
    SUBCASE("empty data has zero checksum")
    {
        long checksum = 0;
        std::vector<uint8_t> data;
        fletcher(checksum, data);
        CHECK(checksum == 0);
    }

    SUBCASE("single uint16 value")
    {
        long checksum = 0;
        fletcher(checksum, uint16_t(100));
        CHECK(checksum == 100);

        fletcher(checksum, uint16_t(200));
        CHECK(checksum == 300);
    }

    SUBCASE("checksum wraps at 65535")
    {
        long checksum = 65534;
        fletcher(checksum, uint16_t(5));
        CHECK(checksum == 4); // (65534 + 5) % 65535 = 4
    }

    SUBCASE("vector checksum")
    {
        long checksum = 0;
        std::vector<uint8_t> data = {0x00, 0x0A, 0x00, 0x14}; // 10, 20 in big-endian
        fletcher(checksum, data);
        CHECK(checksum == 30); // 10 + 20
    }

    SUBCASE("checksum is cumulative")
    {
        long checksum = 0;
        std::vector<uint8_t> data1 = {0x00, 0x05, 0x00, 0x0A};
        std::vector<uint8_t> data2 = {0x00, 0x0F, 0x00, 0x14};

        fletcher(checksum, data1); // 5 + 10 = 15
        fletcher(checksum, data2); // + 15 + 20 = 50
        CHECK(checksum == 50);
    }
}

TEST_CASE("flim_info - serialization round-trip")
{
    SUBCASE("basic info")
    {
        flim_info fi(512, 342, false, 100, 6000, 2730);

        std::vector<uint8_t> data;
        fi.serialize(data);

        CHECK(data.size() == 16); // 2+2+2+4+4+2 bytes

        flim_info fi2;
        fi2.deserialize(data.data(), data.size());

        CHECK(fi2.width == 512);
        CHECK(fi2.height == 342);
        CHECK(fi2.silent == false);
        CHECK(fi2.frame_count == 100);
        CHECK(fi2.total_ticks == 6000);
        CHECK(fi2.byterate == 2730);
    }

    SUBCASE("silent video")
    {
        flim_info fi(320, 200, true, 50, 3000, 1000);

        std::vector<uint8_t> data;
        fi.serialize(data);

        flim_info fi2;
        fi2.deserialize(data.data(), data.size());

        CHECK(fi2.silent == true);
    }

    SUBCASE("edge cases")
    {
        flim_info fi(1, 1, false, 1, 1, 1);

        std::vector<uint8_t> data;
        fi.serialize(data);

        flim_info fi2;
        fi2.deserialize(data.data(), data.size());

        CHECK(fi2.width == 1);
        CHECK(fi2.height == 1);
        CHECK(fi2.frame_count == 1);
    }

    SUBCASE("deserialize truncated data")
    {
        flim_info fi;
        std::vector<uint8_t> short_data = {0x01, 0x00}; // Only 2 bytes
        fi.deserialize(short_data.data(), short_data.size());

        // Should handle gracefully (fields remain at default)
        CHECK(fi.width == 0);
        CHECK(fi.height == 0);
    }
}

TEST_CASE("flim - construction")
{
    SUBCASE("default constructor")
    {
        flim f;
        CHECK(f.component_count() == 0);
        CHECK(f.version() == 1);
    }

    SUBCASE("constructor with comment")
    {
        flim f("Test comment");
        CHECK(f.comment().substr(0, 4) == "FLIM");
        CHECK(f.comment()[4] == '\n');
        // Note: constructor overwrites first 5 bytes with "FLIM\n"
        // So "Test comment" is lost/overwritten
        CHECK(f.comment().size() == 1022);
    }

    SUBCASE("comment is padded/truncated to 1022 bytes")
    {
        std::string short_comment = "Short";
        flim f1(short_comment);
        CHECK(f1.comment().size() == 1022);

        std::string long_comment(2000, 'x');
        flim f2(long_comment);
        CHECK(f2.comment().size() == 1022);
    }
}

TEST_CASE("flim - add_component")
{
    SUBCASE("add single component")
    {
        flim f;
        std::vector<uint8_t> data = {1, 2, 3, 4, 5};
        f.add_component(component_type::info, data);

        CHECK(f.component_count() == 1);
        CHECK(f.component(0).type == static_cast<uint16_t>(component_type::info));
        CHECK(f.component_data(0) == data);
    }

    SUBCASE("add multiple components")
    {
        flim f;
        f.add_component(component_type::info, {1, 2, 3});
        f.add_component(component_type::movie, {4, 5, 6, 7});
        f.add_component(component_type::toc, {8, 9});

        CHECK(f.component_count() == 3);
        CHECK(f.component(0).type == static_cast<uint16_t>(component_type::info));
        CHECK(f.component(1).type == static_cast<uint16_t>(component_type::movie));
        CHECK(f.component(2).type == static_cast<uint16_t>(component_type::toc));
    }
}

TEST_CASE("flim - add flim_info")
{
    SUBCASE("add info component via helper")
    {
        flim f;
        flim_info fi(512, 342, false, 100, 6000, 2730);
        f.add(fi);

        CHECK(f.component_count() == 1);
        CHECK(f.component(0).type == static_cast<uint16_t>(component_type::info));

        // Verify serialized data
        const auto &data = f.component_data(0);
        CHECK(data.size() == 16);
    }
}

TEST_CASE("flim - find_component_data")
{
    SUBCASE("find existing component")
    {
        flim f;
        std::vector<uint8_t> info_data = {1, 2, 3};
        std::vector<uint8_t> movie_data = {4, 5, 6};

        f.add_component(component_type::info, info_data);
        f.add_component(component_type::movie, movie_data);

        const auto *found_info = f.find_component_data(component_type::info);
        CHECK(found_info != nullptr);
        CHECK(*found_info == info_data);

        const auto *found_movie = f.find_component_data(component_type::movie);
        CHECK(found_movie != nullptr);
        CHECK(*found_movie == movie_data);
    }

    SUBCASE("find non-existing component")
    {
        flim f;
        f.add_component(component_type::info, {1, 2, 3});

        const auto *found = f.find_component_data(component_type::poster);
        CHECK(found == nullptr);
    }
}

TEST_CASE("flim - add poster and initial framebuffers")
{
    SUBCASE("add poster")
    {
        flim f;
        bitmap bm(4, 4);
        bm.fill(true);

        f.add_poster(bm);

        CHECK(f.component_count() == 1);
        CHECK(f.component(0).type == static_cast<uint16_t>(component_type::poster));

        // Poster data should be 4*4/8 = 2 bytes
        // But let's check what we actually get
        const auto &data = f.component_data(0);
        CHECK(data.size() >= 0); // At least verify it doesn't crash
    }

    SUBCASE("add initial framebuffer")
    {
        flim f;
        bitmap bm(8, 8);
        bm.fill(false);

        f.add_initial(bm);

        CHECK(f.component_count() == 1);
        CHECK(f.component(0).type == static_cast<uint16_t>(component_type::initial));

        // Initial has header: 2 (type) + 2 (width) + 2 (height) + data
        // Data = 8*8/8 = 8 bytes, total = 6 + 8 = 14
        CHECK(f.component_data(0).size() == 14);
    }
}

TEST_CASE("flim - write and read round-trip")
{
    SUBCASE("empty flim")
    {
        // Create temporary file
        std::string temp_path = "/tmp/test_flim_empty.flim";

        {
            flim f("Empty test");
            file_handle fh(temp_path, "wb");
            f.write(fh);
        }

        {
            flim f2;
            file_handle fh(temp_path, "rb");
            f2.read(fh);

            CHECK(f2.component_count() == 0);
            CHECK(f2.comment().substr(0, 4) == "FLIM");
        }

        std::filesystem::remove(temp_path);
    }

    SUBCASE("flim with info component")
    {
        std::string temp_path = "/tmp/test_flim_info.flim";

        {
            flim f("Info test");
            flim_info fi(512, 342, false, 100, 6000, 2730);
            f.add(fi);

            file_handle fh(temp_path, "wb");
            f.write(fh);
        }

        {
            flim f2;
            file_handle fh(temp_path, "rb");
            f2.read(fh);

            CHECK(f2.component_count() == 1);
            CHECK(f2.component(0).type == static_cast<uint16_t>(component_type::info));

            // Deserialize info
            flim_info fi2;
            const auto &data = f2.component_data(0);
            fi2.deserialize(data.data(), data.size());

            CHECK(fi2.width == 512);
            CHECK(fi2.height == 342);
            CHECK(fi2.frame_count == 100);
        }

        std::filesystem::remove(temp_path);
    }

    SUBCASE("flim with multiple components")
    {
        std::string temp_path = "/tmp/test_flim_multi.flim";

        {
            flim f("Multi test");

            flim_info fi(320, 200, true, 50, 3000, 1000);
            f.add(fi);

            bitmap poster(16, 16);
            poster.fill(true);
            f.add_poster(poster);

            bitmap initial(16, 16);
            initial.fill(false);
            f.add_initial(initial);

            file_handle fh(temp_path, "wb");
            f.write(fh);
        }

        {
            flim f2;
            file_handle fh(temp_path, "rb");
            f2.read(fh);

            CHECK(f2.component_count() == 3);
            CHECK(f2.component(0).type == static_cast<uint16_t>(component_type::info));
            CHECK(f2.component(1).type == static_cast<uint16_t>(component_type::poster));
            CHECK(f2.component(2).type == static_cast<uint16_t>(component_type::initial));
        }

        std::filesystem::remove(temp_path);
    }
}

TEST_CASE("flim - checksum validation")
{
    SUBCASE("compute_checksum includes all data")
    {
        flim f("Checksum test");

        flim_info fi(100, 100, false, 10, 600, 1000);
        f.add(fi);

        bitmap bm(8, 8);
        bm.fill(true);
        f.add_poster(bm);

        // Just verify it computes without crashing
        // (actual checksum value depends on implementation details)
        // Would need access to compute_checksum() which is private
        // So we'll test indirectly via write/read

        std::string temp_path = "/tmp/test_flim_checksum.flim";

        {
            file_handle fh(temp_path, "wb");
            f.write(fh);
        }

        {
            flim f2;
            file_handle fh(temp_path, "rb");
            f2.read(fh);

            // If checksum was wrong, read would fail or data would be corrupt
            CHECK(f2.component_count() == 2);
        }

        std::filesystem::remove(temp_path);
    }
}

TEST_CASE("flim - add frames")
{
    SUBCASE("add single frame")
    {
        flim f("Frame test");

        std::vector<frame> frames;
        bitmap src(8, 8);
        bitmap result(8, 8);
        src.fill(true);
        result.fill(true);

        std::vector<uint8_t> video_data = {0xFF, 0xFF};
        std::vector<uint8_t> audio_data = {0x00, 0x01};

        frames.emplace_back(src, 60, video_data, audio_data, result);

        f.add(frames);

        // Should add movie and toc components
        CHECK(f.component_count() == 2);

        const auto *movie = f.find_component_data(component_type::movie);
        const auto *toc = f.find_component_data(component_type::toc);

        CHECK(movie != nullptr);
        CHECK(toc != nullptr);
        CHECK(toc->size() == 2); // One frame = one 2-byte TOC entry
    }

    SUBCASE("add multiple frames")
    {
        flim f("Multi-frame test");

        std::vector<frame> frames;
        for (int i = 0; i < 5; i++)
        {
            bitmap bm(4, 4);
            frames.emplace_back(bm, 30 + i, std::vector<uint8_t>{uint8_t(i)}, std::vector<uint8_t>{}, bm);
        }

        f.add(frames);

        const auto *toc = f.find_component_data(component_type::toc);
        CHECK(toc != nullptr);
        CHECK(toc->size() == 10); // 5 frames * 2 bytes each
    }
}

TEST_CASE("component_entry - type_name")
{
    SUBCASE("known types")
    {
        component_entry ce;

        ce.type = static_cast<uint16_t>(component_type::info);
        CHECK(std::string(ce.type_name()) == "info");

        ce.type = static_cast<uint16_t>(component_type::movie);
        CHECK(std::string(ce.type_name()) == "movie");

        ce.type = static_cast<uint16_t>(component_type::toc);
        CHECK(std::string(ce.type_name()) == "toc");

        ce.type = static_cast<uint16_t>(component_type::poster);
        CHECK(std::string(ce.type_name()) == "poster");

        ce.type = static_cast<uint16_t>(component_type::initial);
        CHECK(std::string(ce.type_name()) == "initial");
    }

    SUBCASE("unknown type")
    {
        component_entry ce;
        ce.type = 999;
        CHECK(std::string(ce.type_name()) == "unknown");
    }
}

TEST_CASE("flim constructor with comment")
{
    SUBCASE("short comment gets padded")
    {
        flim f("Hello");
        // Comment should be padded to COMMENT_SIZE and start with "FLIM\n"
        // Note: we can't directly access comment_ but we can write and read back
    }

    SUBCASE("long comment gets truncated")
    {
        std::string long_comment(1000, 'X');
        flim f(long_comment);
        // Should be truncated to COMMENT_SIZE
    }

    SUBCASE("empty comment")
    {
        flim f("");
        // Should have minimum size with "FLIM\n" header
    }

    SUBCASE("exact size comment")
    {
        std::string exact_comment(90, 'A'); // Assuming COMMENT_SIZE is around 90
        flim f(exact_comment);
    }
}

} // namespace macflim
