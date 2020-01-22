#include "cursor.h"
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
