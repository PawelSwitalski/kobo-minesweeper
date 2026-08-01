#include "doctest/doctest.h"

#include "core/counter.h"

using namespace minesweeper::core;

TEST_CASE("counter starts at zero and increments") {
    Counter c;
    CHECK(c.value() == 0);
    c.increment();
    c.increment();
    CHECK(c.value() == 2);
}

TEST_CASE("counter JSON round-trip is lossless") {
    Counter a;
    a.increment();
    a.increment();
    a.increment();
    Counter b = Counter::fromJson(a.toJson());
    CHECK(b.value() == a.value());
}

TEST_CASE("malformed counter files are rejected") {
    CHECK_THROWS_AS(Counter::fromJson(""), std::exception);
    CHECK_THROWS_AS(Counter::fromJson("{"), std::exception);
    CHECK_THROWS_AS(Counter::fromJson("[]"), std::exception);
    CHECK_THROWS_AS(Counter::fromJson(R"({"schemaVersion":2,"count":1})"), std::exception);
    CHECK_THROWS_AS(Counter::fromJson(R"({"schemaVersion":1,"count":-1})"), std::exception);
}
