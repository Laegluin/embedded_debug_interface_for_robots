#include "control_table.h"
#include <catch2/catch.hpp>

TEST_CASE("write and resolve addresses with `AddressMap`", "[AddressMap]") {
    AddressMap<0x1000, 0x0100, 4> map;

    SECTION("unrelated addresses") {
        REQUIRE(map.resolve_addr(0x0000) == 0x0000);
        REQUIRE(map.resolve_addr(0x00ff) == 0x00ff);
        REQUIRE(map.resolve_addr(0x1000) == 0x1000);
        REQUIRE(map.resolve_addr(0xffff) == 0xffff);
    }

    SECTION("aligned writes") {
        uint8_t addrs[]{0x00, 0x00, 0x01, 0x00};
        bool is_ok = map.write(0x1000, addrs, sizeof(addrs));
        REQUIRE(is_ok);
        is_ok = map.write_uint16(0x1004, 0x0010);
        REQUIRE(is_ok);
        is_ok = map.write_uint16(0x1006, 0x0011);
        REQUIRE(is_ok);

        REQUIRE(map.resolve_addr(0x0100) == 0x0000);
        REQUIRE(map.resolve_addr(0x0101) == 0x0001);
        REQUIRE(map.resolve_addr(0x0102) == 0x0010);
        REQUIRE(map.resolve_addr(0x0103) == 0x0011);
    }

    SECTION("unaligned writes") {
        uint8_t addrs[]{0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x11, 0x00};
        bool is_ok = map.write(0x1000, addrs, 1);
        REQUIRE(is_ok);
        is_ok = map.write(0x1001, addrs + 1, 4);
        REQUIRE(is_ok);
        is_ok = map.write(0x1005, addrs + 5, 3);
        REQUIRE(is_ok);

        REQUIRE(map.resolve_addr(0x0100) == 0x0000);
        REQUIRE(map.resolve_addr(0x0101) == 0x0001);
        REQUIRE(map.resolve_addr(0x0102) == 0x0010);
        REQUIRE(map.resolve_addr(0x0103) == 0x0011);
    }
}

TEST_CASE("`AddressMap` write bounds checks", "[AddressMap]") {
    AddressMap<0x1000, 0x0100, 4> map;

    uint8_t addrs[]{0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x11, 0x00};

    bool is_ok = map.write(0x1000, addrs, sizeof(addrs));
    REQUIRE(is_ok);
    is_ok = map.write(0x1001, addrs, sizeof(addrs));
    REQUIRE_FALSE(is_ok);
    is_ok = map.write(0x1008, addrs, sizeof(addrs));
    REQUIRE_FALSE(is_ok);

    is_ok = map.write_uint16(0x1000, 0);
    REQUIRE(is_ok);
    is_ok = map.write_uint16(0x1006, 0);
    REQUIRE(is_ok);
    is_ok = map.write_uint16(0x1007, 0);
    REQUIRE_FALSE(is_ok);
}

TEST_CASE("read and write data to `DataSegment`", "[DataSegment]") {
    DataSegment<0x0000, 4> segment_at_zero;
    DataSegment<0x00ff, 4> segment_at_ff;

    SECTION("byte array") {
        uint8_t buf[]{1, 2, 3, 4};

        bool is_ok = segment_at_zero.write(0x0000, buf, sizeof(buf));
        REQUIRE(is_ok);
        REQUIRE(segment_at_zero.uint8_at(0x0000) == 1);
        REQUIRE(segment_at_zero.uint8_at(0x0001) == 2);
        REQUIRE(segment_at_zero.uint8_at(0x0002) == 3);
        REQUIRE(segment_at_zero.uint8_at(0x0003) == 4);

        is_ok = segment_at_ff.write(0x00ff, buf, sizeof(buf));
        REQUIRE(is_ok);
        REQUIRE(segment_at_ff.uint8_at(0x00ff) == 1);
        REQUIRE(segment_at_ff.uint8_at(0x0100) == 2);
        REQUIRE(segment_at_ff.uint8_at(0x0101) == 3);
        REQUIRE(segment_at_ff.uint8_at(0x0102) == 4);
    }

    SECTION("byte") {
        bool is_ok = segment_at_zero.write_uint8(0x0000, 123);
        REQUIRE(is_ok);
        REQUIRE(segment_at_zero.uint8_at(0x0000) == 123);

        is_ok = segment_at_ff.write_uint8(0x0100, 123);
        REQUIRE(is_ok);
        REQUIRE(segment_at_ff.uint8_at(0x0100) == 123);
    }

    SECTION("uint16") {
        bool is_ok = segment_at_zero.write_uint16(0x0000, 10000);
        REQUIRE(is_ok);
        REQUIRE(segment_at_zero.uint16_at(0x0000) == 10000);

        is_ok = segment_at_ff.write_uint16(0x0100, 10000);
        REQUIRE(is_ok);
        REQUIRE(segment_at_ff.uint16_at(0x0100) == 10000);
    }

    SECTION("uint32") {
        bool is_ok = segment_at_zero.write_uint32(0x0000, 222111);
        REQUIRE(is_ok);
        REQUIRE(segment_at_zero.uint32_at(0x0000) == 222111);

        is_ok = segment_at_ff.write_uint32(0x00ff, 222111);
        REQUIRE(is_ok);
        REQUIRE(segment_at_ff.uint32_at(0x00ff) == 222111);
    }
}

TEST_CASE("`DataSegment` write bounds checks", "[DataSegment]") {
    DataSegment<0x0000, 32> segment_at_zero;
    DataSegment<0x00ff, 32> segment_at_ff;

    SECTION("byte array") {
        uint8_t buf[]{1, 2, 3, 4, 5, 6};

        bool is_ok = segment_at_zero.write(0x001d, buf, sizeof(buf));
        REQUIRE_FALSE(is_ok);
        REQUIRE(segment_at_zero.uint8_at(0x001d) == 0);
        REQUIRE(segment_at_zero.uint8_at(0x001f) == 0);
        is_ok = segment_at_zero.write(0x00ff, buf, sizeof(buf));
        REQUIRE_FALSE(is_ok);

        is_ok = segment_at_ff.write(0x011a, buf, sizeof(buf));
        REQUIRE_FALSE(is_ok);
        REQUIRE(segment_at_ff.uint8_at(0x011a) == 0);
        REQUIRE(segment_at_ff.uint8_at(0x011b) == 0);
        REQUIRE(segment_at_ff.uint8_at(0x011c) == 0);
        REQUIRE(segment_at_ff.uint8_at(0x011d) == 0);
        is_ok = segment_at_ff.write(0x0fff, buf, sizeof(buf));
        REQUIRE_FALSE(is_ok);
    }

    SECTION("byte") {
        bool is_ok = segment_at_zero.write_uint8(0x0020, 141);
        REQUIRE_FALSE(is_ok);
        is_ok = segment_at_zero.write_uint8(0x00ff, 141);
        REQUIRE_FALSE(is_ok);

        is_ok = segment_at_ff.write_uint8(0x0000, 141);
        REQUIRE_FALSE(is_ok);
        is_ok = segment_at_ff.write_uint8(0x011f, 141);
        REQUIRE_FALSE(is_ok);
        is_ok = segment_at_ff.write_uint8(0x0fff, 141);
        REQUIRE_FALSE(is_ok);
    }

    SECTION("uint16") {
        bool is_ok = segment_at_zero.write_uint16(0x001f, 11222);
        REQUIRE_FALSE(is_ok);
        REQUIRE(segment_at_zero.uint8_at(0x001f) == 0);
        is_ok = segment_at_zero.write_uint16(0x00ff, 11222);
        REQUIRE_FALSE(is_ok);

        is_ok = segment_at_ff.write_uint16(0x0000, 11222);
        REQUIRE_FALSE(is_ok);
        is_ok = segment_at_ff.write_uint16(0x011e, 11222);
        REQUIRE_FALSE(is_ok);
        REQUIRE(segment_at_ff.uint8_at(0x011e) == 0);
        is_ok = segment_at_ff.write_uint16(0x0fff, 11222);
        REQUIRE_FALSE(is_ok);
    }

    SECTION("uint32") {
        bool is_ok = segment_at_zero.write_uint32(0x001d, 123456);
        REQUIRE_FALSE(is_ok);
        REQUIRE(segment_at_zero.uint8_at(0x001d) == 0);
        REQUIRE(segment_at_zero.uint8_at(0x001e) == 0);
        REQUIRE(segment_at_zero.uint8_at(0x001f) == 0);
        is_ok = segment_at_zero.write_uint32(0x00ff, 123456);
        REQUIRE_FALSE(is_ok);

        is_ok = segment_at_ff.write_uint32(0x0000, 123456);
        REQUIRE_FALSE(is_ok);
        is_ok = segment_at_ff.write_uint32(0x011c, 123456);
        REQUIRE_FALSE(is_ok);
        REQUIRE(segment_at_ff.uint8_at(0x011c) == 0);
        REQUIRE(segment_at_ff.uint8_at(0x011d) == 0);
        REQUIRE(segment_at_ff.uint8_at(0x011e) == 0);
        is_ok = segment_at_ff.write_uint32(0x0fff, 123456);
        REQUIRE_FALSE(is_ok);
    }
}
