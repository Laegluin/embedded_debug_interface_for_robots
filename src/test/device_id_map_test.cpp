#include "device_id_map.h"
#include <catch2/catch.hpp>

TEST_CASE("DeviceIdMap get and clear", "[DeviceIdMap]") {
    DeviceIdMap<uint32_t> map;

    for (size_t i = 0; i < DeviceId::num_values(); i++) {
        REQUIRE_FALSE(map.get(DeviceId(i)).is_present());
    }

    map.clear();

    for (size_t i = 0; i < DeviceId::num_values(); i++) {
        REQUIRE_FALSE(map.get(DeviceId(i)).is_present());
    }

    map.get(DeviceId(0)).set_value(1);
    map.get(DeviceId(10)).set_value(2);

    for (size_t i = 0; i < DeviceId::num_values(); i++) {
        if (i == 0) {
            REQUIRE(map.get(DeviceId(i)).is_present());
            REQUIRE(map.get(DeviceId(i)).value() == 1);
        } else if (i == 10) {
            REQUIRE(map.get(DeviceId(i)).is_present());
            REQUIRE(map.get(DeviceId(i)).value() == 2);
        } else {
            REQUIRE_FALSE(map.get(DeviceId(i)).is_present());
        }
    }

    map.clear();

    for (size_t i = 0; i < DeviceId::num_values(); i++) {
        REQUIRE_FALSE(map.get(DeviceId(i)).is_present());
    }
}

TEST_CASE("DeviceIdMap entry API", "[DeviceIdMap]") {
    DeviceIdMap<uint32_t> map;
    map.get(DeviceId(0)).set_value(42);
    map.get(DeviceId(22)).set_value(1);
    map.get(DeviceId(30)).set_value(0);
    map.get(DeviceId(31)).set_value(5);

    SECTION("test and get value") {
        REQUIRE(map.get(DeviceId(0)).is_present());
        REQUIRE(map.get(DeviceId(0)).value() == 42);
        REQUIRE(map.get(DeviceId(22)).is_present());
        REQUIRE(map.get(DeviceId(22)).value() == 1);
        REQUIRE(map.get(DeviceId(30)).is_present());
        REQUIRE(map.get(DeviceId(30)).value() == 0);
        REQUIRE(map.get(DeviceId(31)).is_present());
        REQUIRE(map.get(DeviceId(31)).value() == 5);

        REQUIRE_FALSE(map.get(DeviceId(1)).is_present());
        REQUIRE_FALSE(map.get(DeviceId(21)).is_present());
        REQUIRE_FALSE(map.get(DeviceId(23)).is_present());
        REQUIRE_FALSE(map.get(DeviceId(29)).is_present());
        REQUIRE_FALSE(map.get(DeviceId(32)).is_present());
    }

    SECTION("insertion when missing") {
        REQUIRE(map.get(DeviceId(0)).is_present());
        REQUIRE(map.get(DeviceId(0)).value() == 42);
        REQUIRE(map.get(DeviceId(0)).or_insert(21) == 42);
        REQUIRE(map.get(DeviceId(0)).is_present());
        REQUIRE(map.get(DeviceId(0)).value() == 42);

        REQUIRE_FALSE(map.get(DeviceId(1)).is_present());
        REQUIRE(map.get(DeviceId(1)).or_insert(21) == 21);
        REQUIRE(map.get(DeviceId(1)).is_present());
        REQUIRE(map.get(DeviceId(1)).value() == 21);

        REQUIRE(map.get(DeviceId(22)).is_present());
        REQUIRE(map.get(DeviceId(22)).value() == 1);
        REQUIRE(map.get(DeviceId(22)).or_insert_with([]() { return 11; }) == 1);
        REQUIRE(map.get(DeviceId(22)).is_present());
        REQUIRE(map.get(DeviceId(22)).value() == 1);

        REQUIRE_FALSE(map.get(DeviceId(23)).is_present());
        REQUIRE(map.get(DeviceId(23)).or_insert_with([]() { return 11; }) == 11);
        REQUIRE(map.get(DeviceId(23)).is_present());
        REQUIRE(map.get(DeviceId(23)).value() == 11);
    }

    SECTION("clear value") {
        REQUIRE(map.get(DeviceId(0)).is_present());
        REQUIRE(map.get(DeviceId(0)).value() == 42);
        map.get(DeviceId(0)).clear_value();
        REQUIRE_FALSE(map.get(DeviceId(0)).is_present());

        REQUIRE_FALSE(map.get(DeviceId(1)).is_present());
        map.get(DeviceId(1)).clear_value();
        REQUIRE_FALSE(map.get(DeviceId(1)).is_present());
    }
}

TEST_CASE("DeviceIdMap iterator", "[DeviceIdMap]") {
    DeviceIdMap<uint32_t> map;

    REQUIRE(map.begin() == map.end());

    map.get(DeviceId(0)).set_value(11);
    map.get(DeviceId(10)).set_value(22);
    map.get(DeviceId(21)).set_value(33);
    map.get(DeviceId(32)).set_value(22);

    auto iter = map.begin();

    REQUIRE(iter != map.end());
    REQUIRE((*iter).first == DeviceId(0));
    REQUIRE((*iter).second == 11);
    ++iter;

    REQUIRE(iter != map.end());
    REQUIRE((*iter).first == DeviceId(10));
    REQUIRE((*iter).second == 22);
    ++iter;

    REQUIRE(iter != map.end());
    REQUIRE((*iter).first == DeviceId(21));
    REQUIRE((*iter).second == 33);
    ++iter;

    REQUIRE(iter != map.end());
    REQUIRE((*iter).first == DeviceId(32));
    REQUIRE((*iter).second == 22);
    ++iter;

    REQUIRE(iter == map.end());
}
