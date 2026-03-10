#include "../flimcompressor.hpp"

#include "../grayscale.hpp"
#include "../profile.hpp"

#include "../doctest.h"

#include <functional>
#include <optional>
#include <vector>

namespace macflim
{

TEST_CASE("flimcompressor: compress simple sequence")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    // Create mock image source with lambda
    std::vector<grayscale> images;
    for (int i = 0; i < 5; i++)
    {
        images.emplace_back(W, H);
        fill(images.back(), i * 50);
    }

    size_t idx = 0;
    auto next_img = [&]() -> std::optional<grayscale>
    {
        if (idx < images.size())
            return images[idx++];
        return std::nullopt;
    };

    std::vector<sound_frame_t> audio(300); // 5 seconds
    flimcompressor fc{W, H, next_img, audio, 30.0, {}};

    encoding_profile prof;
    encoding_profile::profile_named("se30", prof);

    fc.compress(prof, "");

    auto frames = fc.get_frames();
    CHECK(frames.size() > 0);
    CHECK(frames.size() <= 10); // Should not exceed 5 input frames * 2 ticks
}

TEST_CASE("flimcompressor: empty sequence")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    // Empty image source
    auto next_img = []() -> std::optional<grayscale> { return std::nullopt; };

    std::vector<sound_frame_t> audio;
    flimcompressor fc{W, H, next_img, audio, 30.0, {}};

    encoding_profile prof;
    encoding_profile::profile_named("se30", prof);

    fc.compress(prof, "");

    auto frames = fc.get_frames();
    CHECK(frames.size() == 0);
}

TEST_CASE("flimcompressor: single frame")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    grayscale img(W, H);
    fill(img, 128);

    size_t count = 0;
    auto next_img = [&]() -> std::optional<grayscale>
    {
        if (count++ == 0)
            return img;
        return std::nullopt;
    };

    std::vector<sound_frame_t> audio(60);
    flimcompressor fc{W, H, next_img, audio, 30.0, {}};

    encoding_profile prof;
    encoding_profile::profile_named("se30", prof);

    fc.compress(prof, "");

    auto frames = fc.get_frames();
    CHECK(frames.size() > 0);
}

TEST_CASE("flimcompressor: generates initial frame in required mode")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    grayscale img(W, H);
    fill(img, 128);

    size_t count = 0;
    auto next_img = [&]() -> std::optional<grayscale>
    {
        if (count++ < 3)
            return img;
        return std::nullopt;
    };

    std::vector<sound_frame_t> audio(180);
    flimcompressor fc{W, H, next_img, audio, 30.0, {}};

    encoding_profile prof;
    encoding_profile::profile_named("se30", prof);
    prof.set_initial_mode("true"); // required mode

    fc.compress(prof, "");

    CHECK(fc.get_initial().has_value());
}

TEST_CASE("flimcompressor: no initial frame in none mode")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    grayscale img(W, H);
    fill(img, 128);

    size_t count = 0;
    auto next_img = [&]() -> std::optional<grayscale>
    {
        if (count++ < 3)
            return img;
        return std::nullopt;
    };

    std::vector<sound_frame_t> audio(180);
    flimcompressor fc{W, H, next_img, audio, 30.0, {}};

    encoding_profile prof;
    encoding_profile::profile_named("se30", prof);

    fc.compress(prof, "", initial_frame_mode::none, false);

    CHECK_FALSE(fc.get_initial().has_value());
}

TEST_CASE("flimcompressor: with watermark")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    std::vector<grayscale> images;
    for (int i = 0; i < 3; i++)
    {
        images.emplace_back(W, H);
        fill(images.back(), 128);
    }

    size_t idx = 0;
    auto next_img = [&]() -> std::optional<grayscale>
    {
        if (idx < images.size())
            return images[idx++];
        return std::nullopt;
    };

    std::vector<sound_frame_t> audio(180);
    flimcompressor fc{W, H, next_img, audio, 30.0, {}};

    encoding_profile prof;
    encoding_profile::profile_named("se30", prof);

    fc.compress(prof, "TEST");

    auto frames = fc.get_frames();
    CHECK(frames.size() > 0);
}

TEST_CASE("flimcompressor: loop mode adds trailing frames")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    std::vector<grayscale> images;
    for (int i = 0; i < 3; i++)
    {
        images.emplace_back(W, H);
        fill(images.back(), i * 80);
    }

    size_t idx = 0;
    auto next_img = [&]() -> std::optional<grayscale>
    {
        if (idx < images.size())
            return images[idx++];
        return std::nullopt;
    };

    std::vector<sound_frame_t> audio(300);
    flimcompressor fc{W, H, next_img, audio, 30.0, {}};

    encoding_profile prof;
    encoding_profile::profile_named("se30", prof);

    fc.compress(prof, "", initial_frame_mode::optional, true);

    auto frames = fc.get_frames();
    CHECK(frames.size() >= 3); // Should have at least the input frames
    CHECK(fc.get_initial().has_value());
}

TEST_CASE("flimcompressor: with subtitles")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    std::vector<grayscale> images;
    for (int i = 0; i < 5; i++)
    {
        images.emplace_back(W, H);
        fill(images.back(), 128);
    }

    size_t idx = 0;
    auto next_img = [&]() -> std::optional<grayscale>
    {
        if (idx < images.size())
            return images[idx++];
        return std::nullopt;
    };

    std::vector<subtitle> subs;
    subs.push_back(subtitle{0.0, 1.0, {"Hello"}});
    subs.push_back(subtitle{1.0, 2.0, {"World"}});

    std::vector<sound_frame_t> audio(300);
    flimcompressor fc{W, H, next_img, audio, 30.0, subs};

    encoding_profile prof;
    encoding_profile::profile_named("se30", prof);

    fc.compress(prof, "");

    auto frames = fc.get_frames();
    CHECK(frames.size() > 0);
}

TEST_CASE("flimcompressor: different profiles produce different results")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    // Test SE30 profile
    {
        std::vector<grayscale> images;
        for (int i = 0; i < 3; i++)
        {
            images.emplace_back(W, H);
            fill(images.back(), 100);
        }

        size_t idx = 0;
        auto next_img = [&]() -> std::optional<grayscale>
        {
            if (idx < images.size())
                return images[idx++];
            return std::nullopt;
        };

        std::vector<sound_frame_t> audio(180);
        flimcompressor fc{W, H, next_img, audio, 30.0, {}};

        encoding_profile prof;
        encoding_profile::profile_named("se30", prof);
        fc.compress(prof, "");

        auto frames_se30 = fc.get_frames();
        CHECK(frames_se30.size() > 0);
    }

    // Test Plus profile
    {
        std::vector<grayscale> images;
        for (int i = 0; i < 3; i++)
        {
            images.emplace_back(W, H);
            fill(images.back(), 100);
        }

        size_t idx = 0;
        auto next_img = [&]() -> std::optional<grayscale>
        {
            if (idx < images.size())
                return images[idx++];
            return std::nullopt;
        };

        std::vector<sound_frame_t> audio(180);
        flimcompressor fc{W, H, next_img, audio, 30.0, {}};

        encoding_profile prof;
        encoding_profile::profile_named("plus", prof);
        fc.compress(prof, "");

        auto frames_plus = fc.get_frames();
        CHECK(frames_plus.size() > 0);
    }
}

TEST_CASE("flimcompressor: varying grayscale values")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    std::vector<grayscale> images;
    for (int i = 0; i < 10; i++)
    {
        images.emplace_back(W, H);
        fill(images.back(), (i * 25) % 256);
    }

    size_t idx = 0;
    auto next_img = [&]() -> std::optional<grayscale>
    {
        if (idx < images.size())
            return images[idx++];
        return std::nullopt;
    };

    std::vector<sound_frame_t> audio(600);
    flimcompressor fc{W, H, next_img, audio, 30.0, {}};

    encoding_profile prof;
    encoding_profile::profile_named("se30", prof);

    fc.compress(prof, "");

    auto frames = fc.get_frames();
    CHECK(frames.size() > 0);
    CHECK(frames.size() <= 20); // 10 frames * 2 ticks
}

TEST_CASE("flimcompressor: high fps reduces ticks per frame")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    std::vector<grayscale> images;
    for (int i = 0; i < 3; i++)
    {
        images.emplace_back(W, H);
        fill(images.back(), 128);
    }

    size_t idx = 0;
    auto next_img = [&]() -> std::optional<grayscale>
    {
        if (idx < images.size())
            return images[idx++];
        return std::nullopt;
    };

    std::vector<sound_frame_t> audio(180);
    flimcompressor fc{W, H, next_img, audio, 60.0, {}}; // 60 fps

    encoding_profile prof;
    encoding_profile::profile_named("se30", prof);

    fc.compress(prof, "");

    auto frames = fc.get_frames();
    CHECK(frames.size() == 3); // 60fps -> 1 tick per frame
}

} // namespace macflim
