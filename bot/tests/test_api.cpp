#include <fake/fake.h>
#include <tests/test_scenarios.h>

#include <catch2/catch_test_macros.hpp>

// TEST_CASE("Single getMe") {
//     telegram::FakeServer fake{"Single getMe"};
//     fake.Start();
//     TestSingleGetMe(fake.GetUrl());
//     fake.StopAndCheckExpectations();
// }

// TEST_CASE("getMe error handling") {
//     telegram::FakeServer fake{"getMe error handling"};
//     fake.Start();
//     TestGetMeErrorHandling(fake.GetUrl());
//     fake.StopAndCheckExpectations();
// }

// TEST_CASE("Single getUpdates and send messages") {
//     telegram::FakeServer fake{"Single getUpdates and send messages"};
//     fake.Start();
//     TestSingleGetUpdatesAndSendMessages(fake.GetUrl());
//     fake.StopAndCheckExpectations();
// }

TEST_CASE("Handle getUpdates offset") {
    telegram::FakeServer fake{"Handle getUpdates offset"};
    fake.Start();
    TestHandleGetUpdatesOffset(fake.GetUrl());
    fake.StopAndCheckExpectations();
}
