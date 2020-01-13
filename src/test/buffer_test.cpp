#include "buffer.h"
#include <catch2/catch.hpp>

TEST_CASE("read bytes from cursor", "[Cursor]") {
    uint8_t buf[]{0x00, 0xfd, 0x1a, 0x23, 0xb2, 0x88};
    Cursor cursor(buf, sizeof(buf));

    REQUIRE(cursor.remaining_bytes() == 6);

    uint8_t dst[4];
    REQUIRE(cursor.read(dst, 4) == 4);
    REQUIRE(cursor.remaining_bytes() == 2);
    REQUIRE(dst[0] == 0x00);
    REQUIRE(dst[1] == 0xfd);
    REQUIRE(dst[2] == 0x1a);
    REQUIRE(dst[3] == 0x23);

    REQUIRE(cursor.read(dst, 4) == 2);
    REQUIRE(cursor.remaining_bytes() == 0);
    REQUIRE(dst[0] == 0xb2);
    REQUIRE(dst[1] == 0x88);

    REQUIRE(cursor.read(dst, 4) == 0);
}

TEST_CASE("read from empty cursor", "[Cursor]") {
    Cursor cursor(nullptr, 0);

    REQUIRE(cursor.remaining_bytes() == 0);
    uint8_t dst[4];
    REQUIRE(cursor.read(dst, 4) == 0);
    REQUIRE(cursor.remaining_bytes() == 0);
}

TEST_CASE("byte vector initializer list ctor", "[FixedByteVector]") {
    SECTION("empty list") {
        FixedByteVector<0> vec{};
        REQUIRE(vec.size() == 0);
    }

    SECTION("fitting list") {
        FixedByteVector<2> vec{2, 3};
        REQUIRE(vec.size() == 2);
        REQUIRE(vec[0] == 2);
        REQUIRE(vec[1] == 3);
    }

    SECTION("smaller list") {
        FixedByteVector<4> vec{2, 3};
        REQUIRE(vec.size() == 2);
        REQUIRE(vec[0] == 2);
        REQUIRE(vec[1] == 3);
    }

    SECTION("larger list") {
        REQUIRE_THROWS_AS((FixedByteVector<0>{2, 3}), OutOfRange);
    }
}

TEST_CASE("push slice to byte vector", "[FixedByteVector]") {
    FixedByteVector<4> vec;
    REQUIRE(vec.size() == 0);

    SECTION("empty slice") {
        REQUIRE(vec.try_push_slice(nullptr, 0));
        REQUIRE(vec.size() == 0);
    }

    SECTION("fitting slice") {
        uint8_t slice[]{7, 8, 1, 2};
        REQUIRE(vec.try_push_slice(slice, sizeof(slice)));

        REQUIRE(vec.size() == 4);
        REQUIRE(vec[0] == 7);
        REQUIRE(vec[1] == 8);
        REQUIRE(vec[2] == 1);
        REQUIRE(vec[3] == 2);
    }

    SECTION("smaller slice") {
        uint8_t slice[]{7, 8};
        REQUIRE(vec.try_push_slice(slice, sizeof(slice)));

        REQUIRE(vec.size() == 2);
        REQUIRE(vec[0] == 7);
        REQUIRE(vec[1] == 8);
    }

    SECTION("larger slice") {
        uint8_t slice[]{7, 8, 1, 2, 33};
        REQUIRE_FALSE(vec.try_push_slice(slice, sizeof(slice)));
    }
}

TEST_CASE("extend byte vector", "[FixedByteVector]") {
    FixedByteVector<4> vec;

    SECTION("extend by zero") {
        REQUIRE(vec.try_extend_by(0));
        REQUIRE(vec.size() == 0);
    }

    SECTION("extend") {
        REQUIRE(vec.try_extend_by(2));
        REQUIRE(vec.size() == 2);
        REQUIRE(vec.try_extend_by(2));
        REQUIRE(vec.size() == 4);
    }

    SECTION("extend too far") {
        REQUIRE_FALSE(vec.try_extend_by(5));
    }
}

TEST_CASE("shrink byte vector", "[FixedByteVector]") {
    FixedByteVector<4> vec{1, 2, 3, 4};

    SECTION("shrink by zero") {
        vec.shrink_by(0);
        REQUIRE(vec.size() == 4);
    }

    SECTION("shrink") {
        vec.shrink_by(2);
        REQUIRE(vec.size() == 2);
        vec.shrink_by(2);
        REQUIRE(vec.size() == 0);
    }

    SECTION("shrink too far") {
        vec.shrink_by(5);
        REQUIRE(vec.size() == 0);
    }
}
