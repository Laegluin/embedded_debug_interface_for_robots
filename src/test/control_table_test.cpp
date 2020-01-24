#include "control_table.h"
#include "device/mx106.h"
#include "device/mx64.h"
#include <catch2/catch.hpp>

TEST_CASE("flow control for incoming packets", "[ControlTableMap]") {
    ControlTableMap control_table_map;

    Packet ping_dev_4{
        DeviceId(4),
        Instruction::Ping,
        Error(),
        std::vector<uint8_t>(),
    };

    Packet dev_4_ping_resp{
        DeviceId(4),
        Instruction::Status,
        Error(),
        std::vector<uint8_t>{0x37, 0x01, 0x06},
    };

    Packet ping_dev_5{
        DeviceId(5),
        Instruction::Ping,
        Error(),
        std::vector<uint8_t>(),
    };

    Packet dev_5_ping_resp{
        DeviceId(5),
        Instruction::Status,
        Error(),
        std::vector<uint8_t>{0x41, 0x01, 0x08},
    };

    auto result = control_table_map.receive(ping_dev_4);
    REQUIRE(result == ProtocolResult::Ok);
    result = control_table_map.receive(dev_4_ping_resp);
    REQUIRE(result == ProtocolResult::Ok);
    result = control_table_map.receive(ping_dev_5);
    REQUIRE(result == ProtocolResult::Ok);
    result = control_table_map.receive(dev_5_ping_resp);
    REQUIRE(result == ProtocolResult::Ok);

    auto& dev_4 = control_table_map.get(DeviceId(4));
    REQUIRE(dev_4.model_number() == Mx64ControlTable::MODEL_NUMBER);
    auto& dev_4_mx64 = static_cast<const Mx64ControlTable&>(dev_4);

    auto& dev_5 = control_table_map.get(DeviceId(5));
    REQUIRE(dev_5.model_number() == Mx106ControlTable::MODEL_NUMBER);
    auto& dev_5_mx106 = static_cast<const Mx106ControlTable&>(dev_5);
}
