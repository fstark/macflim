#include "../doctest.h"

#include "../subtitles.hpp"

#include <sstream>

using namespace macflim;

TEST_CASE("read_timestamps - valid timestamp format")
{
    auto result = read_timestamps("00:00:01,500 --> 00:00:05,250");
    REQUIRE(result.has_value());
    auto [start, stop] = result.value();
    CHECK(start == doctest::Approx(1.5));
    CHECK(stop == doctest::Approx(5.25));
}

TEST_CASE("read_timestamps - zero times")
{
    auto result = read_timestamps("00:00:00,000 --> 00:00:00,000");
    REQUIRE(result.has_value());
    auto [start, stop] = result.value();
    CHECK(start == doctest::Approx(0.0));
    CHECK(stop == doctest::Approx(0.0));
}

TEST_CASE("read_timestamps - hours and minutes")
{
    auto result = read_timestamps("01:23:45,678 --> 02:34:56,789");
    REQUIRE(result.has_value());
    auto [start, stop] = result.value();
    // 1h 23m 45s 678ms = 3600 + 1380 + 45 + 0.678
    CHECK(start == doctest::Approx(5025.678));
    // 2h 34m 56s 789ms = 7200 + 2040 + 56 + 0.789
    CHECK(stop == doctest::Approx(9296.789));
}

TEST_CASE("read_timestamps - malformed input with missing arrow")
{
    auto result = read_timestamps("00:00:01,500 00:00:05,250");
    CHECK_FALSE(result.has_value());
}

TEST_CASE("read_timestamps - malformed input with wrong separator")
{
    auto result = read_timestamps("00:00:01.500 --> 00:00:05.250");
    CHECK_FALSE(result.has_value());
}

TEST_CASE("read_timestamps - incomplete timestamp")
{
    auto result = read_timestamps("00:00:01,500 -->");
    CHECK_FALSE(result.has_value());
}

TEST_CASE("read_timestamps - empty string")
{
    auto result = read_timestamps("");
    CHECK_FALSE(result.has_value());
}

TEST_CASE("read_timestamps - garbage input")
{
    auto result = read_timestamps("not a timestamp");
    CHECK_FALSE(result.has_value());
}

TEST_CASE("next_subtitle - single line subtitle")
{
    std::stringstream ss;
    ss << "1\n";
    ss << "00:00:01,000 --> 00:00:03,000\n";
    ss << "Hello, World!\n";
    ss << "\n";

    auto result = next_subtitle(ss);
    REQUIRE(result.has_value());

    auto sub = result.value();
    CHECK(sub.start == doctest::Approx(1.0));
    CHECK(sub.stop == doctest::Approx(3.0));
    REQUIRE(sub.text.size() == 1);
    CHECK(sub.text[0] == "Hello, World!");
    CHECK_FALSE(sub.reverse);
}

TEST_CASE("next_subtitle - multi-line subtitle")
{
    std::stringstream ss;
    ss << "2\n";
    ss << "00:00:05,500 --> 00:00:08,200\n";
    ss << "First line\n";
    ss << "Second line\n";
    ss << "Third line\n";
    ss << "\n";

    auto result = next_subtitle(ss);
    REQUIRE(result.has_value());

    auto sub = result.value();
    CHECK(sub.start == doctest::Approx(5.5));
    CHECK(sub.stop == doctest::Approx(8.2));
    REQUIRE(sub.text.size() == 3);
    CHECK(sub.text[0] == "First line");
    CHECK(sub.text[1] == "Second line");
    CHECK(sub.text[2] == "Third line");
}

TEST_CASE("next_subtitle - reverse subtitle with brackets")
{
    std::stringstream ss;
    ss << "3\n";
    ss << "00:00:10,000 --> 00:00:12,000\n";
    ss << "[Reversed text]\n";
    ss << "\n";

    auto result = next_subtitle(ss);
    REQUIRE(result.has_value());

    auto sub = result.value();
    CHECK(sub.start == doctest::Approx(10.0));
    CHECK(sub.stop == doctest::Approx(12.0));
    REQUIRE(sub.text.size() == 1);
    CHECK(sub.text[0] == "Reversed text");
    CHECK(sub.reverse);
}

TEST_CASE("next_subtitle - handles carriage returns")
{
    std::stringstream ss;
    ss << "1\n";
    ss << "00:00:01,000 --> 00:00:03,000\n";
    ss << "Text with CR\r\n";
    ss << "\n";

    auto result = next_subtitle(ss);
    REQUIRE(result.has_value());

    auto sub = result.value();
    REQUIRE(sub.text.size() == 1);
    CHECK(sub.text[0] == "Text with CR");
}

TEST_CASE("next_subtitle - empty stream")
{
    std::stringstream ss;
    auto result = next_subtitle(ss);
    CHECK_FALSE(result.has_value());
}

TEST_CASE("next_subtitle - missing timestamp after index")
{
    std::stringstream ss;
    ss << "1\n";
    // No timestamp line

    auto result = next_subtitle(ss);
    CHECK_FALSE(result.has_value());
}

TEST_CASE("next_subtitle - skips malformed entries until valid timestamp")
{
    std::stringstream ss;
    ss << "1\n";
    ss << "This is not a timestamp\n";
    ss << "00:00:01,000 --> 00:00:03,000\n";
    ss << "Valid subtitle\n";
    ss << "\n";

    auto result = next_subtitle(ss);
    REQUIRE(result.has_value());

    auto sub = result.value();
    CHECK(sub.start == doctest::Approx(1.0));
    CHECK(sub.stop == doctest::Approx(3.0));
    REQUIRE(sub.text.size() == 1);
    CHECK(sub.text[0] == "Valid subtitle");
}

TEST_CASE("read_subtitles - multiple subtitles")
{
    std::stringstream ss;
    ss << "1\n";
    ss << "00:00:01,000 --> 00:00:03,000\n";
    ss << "First subtitle\n";
    ss << "\n";
    ss << "2\n";
    ss << "00:00:04,000 --> 00:00:06,000\n";
    ss << "Second subtitle\n";
    ss << "\n";
    ss << "3\n";
    ss << "00:00:07,000 --> 00:00:09,000\n";
    ss << "Third subtitle\n";
    ss << "\n";

    auto subs = read_subtitles(ss);
    REQUIRE(subs.size() == 3);

    CHECK(subs[0].start == doctest::Approx(1.0));
    CHECK(subs[0].text[0] == "First subtitle");

    CHECK(subs[1].start == doctest::Approx(4.0));
    CHECK(subs[1].text[0] == "Second subtitle");

    CHECK(subs[2].start == doctest::Approx(7.0));
    CHECK(subs[2].text[0] == "Third subtitle");
}

TEST_CASE("read_subtitles - empty stream")
{
    std::stringstream ss;
    auto subs = read_subtitles(ss);
    CHECK(subs.empty());
}

TEST_CASE("read_subtitles - single subtitle")
{
    std::stringstream ss;
    ss << "1\n";
    ss << "00:00:01,000 --> 00:00:03,000\n";
    ss << "Only one\n";
    ss << "\n";

    auto subs = read_subtitles(ss);
    REQUIRE(subs.size() == 1);
    CHECK(subs[0].text[0] == "Only one");
}

TEST_CASE("subtitles_extract - no offset, full duration")
{
    std::vector<subtitle> subs = {{1.0, 3.0, {"First"}}, {5.0, 7.0, {"Second"}}, {9.0, 11.0, {"Third"}}};

    auto result = subtitles_extract(subs, 0.0, 15.0);
    REQUIRE(result.size() == 3);

    CHECK(result[0].start == doctest::Approx(1.0));
    CHECK(result[0].stop == doctest::Approx(3.0));

    CHECK(result[1].start == doctest::Approx(5.0));
    CHECK(result[1].stop == doctest::Approx(7.0));

    CHECK(result[2].start == doctest::Approx(9.0));
    CHECK(result[2].stop == doctest::Approx(11.0));
}

TEST_CASE("subtitles_extract - with offset")
{
    std::vector<subtitle> subs = {{5.0, 7.0, {"First"}}, {10.0, 12.0, {"Second"}}, {15.0, 17.0, {"Third"}}};

    auto result = subtitles_extract(subs, 5.0, 15.0);
    REQUIRE(result.size() == 3);

    CHECK(result[0].start == doctest::Approx(0.0));
    CHECK(result[0].stop == doctest::Approx(2.0));

    CHECK(result[1].start == doctest::Approx(5.0));
    CHECK(result[1].stop == doctest::Approx(7.0));

    CHECK(result[2].start == doctest::Approx(10.0));
    CHECK(result[2].stop == doctest::Approx(12.0));
}

TEST_CASE("subtitles_extract - truncates at duration")
{
    std::vector<subtitle> subs = {{0.0, 5.0, {"First"}}, {6.0, 12.0, {"Second"}}, {13.0, 20.0, {"Third"}}};

    auto result = subtitles_extract(subs, 0.0, 10.0);
    REQUIRE(result.size() == 2);

    CHECK(result[0].start == doctest::Approx(0.0));
    CHECK(result[0].stop == doctest::Approx(5.0));

    CHECK(result[1].start == doctest::Approx(6.0));
    CHECK(result[1].stop == doctest::Approx(10.0)); // Truncated
}

TEST_CASE("subtitles_extract - filters out subtitles before range")
{
    std::vector<subtitle> subs = {{0.0, 2.0, {"Before"}}, {5.0, 7.0, {"During"}}, {10.0, 12.0, {"After"}}};

    auto result = subtitles_extract(subs, 5.0, 5.0);
    REQUIRE(result.size() == 1);

    CHECK(result[0].start == doctest::Approx(0.0));
    CHECK(result[0].stop == doctest::Approx(2.0));
    CHECK(result[0].text[0] == "During");
}

TEST_CASE("subtitles_extract - clamps start at zero")
{
    std::vector<subtitle> subs = {{3.0, 8.0, {"Overlapping"}}};

    auto result = subtitles_extract(subs, 5.0, 10.0);
    REQUIRE(result.size() == 1);

    CHECK(result[0].start == doctest::Approx(0.0)); // Clamped
    CHECK(result[0].stop == doctest::Approx(3.0));
}

TEST_CASE("subtitles_extract - empty input")
{
    std::vector<subtitle> subs;
    auto result = subtitles_extract(subs, 0.0, 10.0);
    CHECK(result.empty());
}

TEST_CASE("subtitles_extract - subtitles completely outside range")
{
    std::vector<subtitle> subs = {{0.0, 2.0, {"Before"}}, {20.0, 22.0, {"After"}}};

    auto result = subtitles_extract(subs, 5.0, 10.0);
    CHECK(result.empty());
}

TEST_CASE("subtitles_extract - preserves text content")
{
    std::vector<subtitle> subs = {{1.0, 3.0, {"Line 1", "Line 2", "Line 3"}}};

    auto result = subtitles_extract(subs, 0.0, 10.0);
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].text.size() == 3);
    CHECK(result[0].text[0] == "Line 1");
    CHECK(result[0].text[1] == "Line 2");
    CHECK(result[0].text[2] == "Line 3");
}

TEST_CASE("subtitles_extract - preserves reverse flag")
{
    std::vector<subtitle> subs = {{1.0, 3.0, {"Normal"}, false}, {5.0, 7.0, {"Reversed"}, true}};

    auto result = subtitles_extract(subs, 0.0, 10.0);
    REQUIRE(result.size() == 2);
    CHECK_FALSE(result[0].reverse);
    CHECK(result[1].reverse);
}
