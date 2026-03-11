#include "../doctest.h"

#include "../streaming/adaptive_rate_controller.hpp"

#include <cstddef>

namespace macflim
{

namespace
{

void feed_outcomes(adaptive_rate_controller &ctrl, size_t count, bool displayed)
{
    for (size_t i = 0; i < count; ++i)
        ctrl.record_outcome(displayed);
}

} // namespace

TEST_CASE("adaptive_rate_controller: initial state is 50% of max")
{
    adaptive_rate_controller ctrl(6000);
    CHECK(ctrl.current_byterate() == 3000);
    CHECK(ctrl.max_byterate() == 6000);
    CHECK(ctrl.budget_for_next_frame() == 3000);
}

TEST_CASE("adaptive_rate_controller: initial byterate respects minimum")
{
    adaptive_rate_controller ctrl(10);
    CHECK(ctrl.current_byterate() >= 8); // MIN_BYTERATE
}

TEST_CASE("adaptive_rate_controller: no change during partial window")
{
    adaptive_rate_controller ctrl(6000);
    size_t initial = ctrl.current_byterate();

    //  Feed 30 misses — less than a full window of 60
    feed_outcomes(ctrl, 30, false);
    CHECK(ctrl.current_byterate() == initial);
}

TEST_CASE("adaptive_rate_controller: decrease on >10% loss in full window")
{
    adaptive_rate_controller ctrl(6000);
    size_t before = ctrl.current_byterate();

    //  Fill window with 53 acks + 7 misses = >10% loss
    feed_outcomes(ctrl, 53, true);
    feed_outcomes(ctrl, 7, false);

    CHECK(ctrl.current_byterate() < before);
    CHECK(ctrl.current_byterate() == before * 80 / 100);
}

TEST_CASE("adaptive_rate_controller: no decrease at exactly 10% loss")
{
    adaptive_rate_controller ctrl(6000);
    size_t before = ctrl.current_byterate();

    //  Fill window with 54 acks + 6 misses = exactly 10%
    feed_outcomes(ctrl, 54, true);
    feed_outcomes(ctrl, 6, false);

    CHECK(ctrl.current_byterate() == before);
}

TEST_CASE("adaptive_rate_controller: increase after 120 consecutive clean frames")
{
    adaptive_rate_controller ctrl(6000);
    size_t before = ctrl.current_byterate();

    feed_outcomes(ctrl, 120, true);

    CHECK(ctrl.current_byterate() > before);
    CHECK(ctrl.current_byterate() == before * 105 / 100);
}

TEST_CASE("adaptive_rate_controller: no increase before 120 clean frames")
{
    adaptive_rate_controller ctrl(6000);
    size_t before = ctrl.current_byterate();

    feed_outcomes(ctrl, 119, true);

    CHECK(ctrl.current_byterate() == before);
}

TEST_CASE("adaptive_rate_controller: increase capped at max_byterate")
{
    adaptive_rate_controller ctrl(100);
    //  Start at 50, increase several times
    for (int i = 0; i < 50; ++i)
        feed_outcomes(ctrl, 120, true);

    CHECK(ctrl.current_byterate() <= ctrl.max_byterate());
}

TEST_CASE("adaptive_rate_controller: decrease floored at MIN_BYTERATE")
{
    adaptive_rate_controller ctrl(100);

    //  Repeated heavy loss should bottom out at 8
    for (int i = 0; i < 50; ++i)
    {
        feed_outcomes(ctrl, 50, true);
        feed_outcomes(ctrl, 10, false);
    }

    CHECK(ctrl.current_byterate() >= 8);
}

TEST_CASE("adaptive_rate_controller: single miss resets consecutive clean counter")
{
    adaptive_rate_controller ctrl(6000);
    size_t before = ctrl.current_byterate();

    //  119 clean frames, then 1 miss, then 1 more clean — no increase
    feed_outcomes(ctrl, 119, true);
    ctrl.record_outcome(false);
    ctrl.record_outcome(true);

    CHECK(ctrl.current_byterate() == before);
}

TEST_CASE("adaptive_rate_controller: decrease clears window — no double-decrease")
{
    adaptive_rate_controller ctrl(6000);

    //  Trigger a decrease
    feed_outcomes(ctrl, 53, true);
    feed_outcomes(ctrl, 7, false);
    size_t after_first_decrease = ctrl.current_byterate();

    //  Feed a few more misses — window was cleared, so < 60 total, no second decrease
    feed_outcomes(ctrl, 5, false);
    CHECK(ctrl.current_byterate() == after_first_decrease);
}

TEST_CASE("adaptive_rate_controller: recovery cycle — decrease then increase back")
{
    adaptive_rate_controller ctrl(6000);
    size_t initial = ctrl.current_byterate();

    //  Trigger decrease
    feed_outcomes(ctrl, 53, true);
    feed_outcomes(ctrl, 7, false);
    size_t decreased = ctrl.current_byterate();
    CHECK(decreased < initial);

    //  Recover: 120 clean frames → increase
    feed_outcomes(ctrl, 120, true);
    CHECK(ctrl.current_byterate() > decreased);
}

TEST_CASE("adaptive_rate_controller: sustained loss keeps reducing")
{
    adaptive_rate_controller ctrl(6000);

    size_t prev = ctrl.current_byterate();
    for (int round = 0; round < 5; ++round)
    {
        //  Each round: fill a window with heavy loss
        feed_outcomes(ctrl, 50, true);
        feed_outcomes(ctrl, 10, false);
        CHECK(ctrl.current_byterate() <= prev);
        prev = ctrl.current_byterate();
    }

    CHECK(ctrl.current_byterate() < 6000 / 2);
}

TEST_CASE("adaptive_rate_controller: full recovery to max after sustained clean")
{
    adaptive_rate_controller ctrl(1000);

    //  Drive it down
    for (int i = 0; i < 10; ++i)
    {
        feed_outcomes(ctrl, 50, true);
        feed_outcomes(ctrl, 10, false);
    }
    CHECK(ctrl.current_byterate() < 500);

    //  Recover fully: many rounds of 120 clean frames
    for (int i = 0; i < 200; ++i)
        feed_outcomes(ctrl, 120, true);

    CHECK(ctrl.current_byterate() == 1000);
}

} // namespace macflim
