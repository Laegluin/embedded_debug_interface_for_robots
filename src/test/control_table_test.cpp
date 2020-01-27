#include "control_table.h"
#include "device/mx106.h"
#include "device/mx64.h"
#include <catch2/catch.hpp>

TEST_CASE("read and write to control table memory", "[ControlTableMemory]") {
    ControlTableMemory mem({
        Segment::new_data(0x0100, 32),
        Segment::new_data(0x0120, 32),
        Segment::new_indirect_address(0x0300, 0x0200, 32),
    });

    uint32_t value;
    REQUIRE(mem.read_uint32(0x011e, &value));
    REQUIRE(value == 0);
    REQUIRE(mem.write_uint32(0x011e, 420000042));
    REQUIRE(mem.read_uint32(0x011e, &value));
    REQUIRE(value == 420000042);

    REQUIRE(mem.write_uint32(0x0200, 123));
    REQUIRE(mem.read_uint32(0x0200, &value));
    REQUIRE(value == 123);

    REQUIRE_FALSE(mem.read_uint32(0x0000, &value));
    REQUIRE_FALSE(mem.write_uint32(0x0000, 321));

    REQUIRE_FALSE(mem.read_uint32(0x0160, &value));
    REQUIRE_FALSE(mem.write_uint32(0x0160, 321));

    REQUIRE_FALSE(mem.read_uint32(0x0300, &value));
    REQUIRE_FALSE(mem.write_uint32(0x0300, 321));
}

TEST_CASE("resolve addresses", "[ControlTableMemory]") {
    ControlTableMemory mem({
        Segment::new_indirect_address(0x0000, 0x0200, 32),
    });

    REQUIRE(mem.resolve_addr(0x0000) == 0x0000);
    REQUIRE(mem.resolve_addr(0x000f) == 0x0000);
    REQUIRE(mem.resolve_addr(0x0010) == 0x0010);
    REQUIRE(mem.resolve_addr(0x0200) == 0x0200);
    REQUIRE(mem.resolve_addr(0xffff) == 0xffff);

    REQUIRE(mem.write_uint16(0x0200, 0x1000));
    REQUIRE(mem.write_uint16(0x021e, 0x2000));

    REQUIRE(mem.resolve_addr(0x0000) == 0x1000);
    REQUIRE(mem.resolve_addr(0x000f) == 0x2000);
    REQUIRE(mem.resolve_addr(0x0010) == 0x0010);
    REQUIRE(mem.resolve_addr(0x0200) == 0x0200);
    REQUIRE(mem.resolve_addr(0xffff) == 0xffff);
}

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
    auto& dev_5 = control_table_map.get(DeviceId(5));
    REQUIRE(dev_4.model_number() == Mx64ControlTable::MODEL_NUMBER);
    REQUIRE(dev_5.model_number() == Mx106ControlTable::MODEL_NUMBER);

    uint8_t firmware_version;
    REQUIRE(dev_4.memory().read_uint8(6, &firmware_version));
    REQUIRE(firmware_version == 0x06);
    REQUIRE(dev_5.memory().read_uint8(6, &firmware_version));
    REQUIRE(firmware_version == 0x08);

    SECTION("read packets") {
        Packet dev_4_read{
            DeviceId(4),
            Instruction::Read,
            Error(),
            std::vector<uint8_t>{0x40, 0x00, 0x04, 0x00},
        };

        Packet dev_4_read_resp{
            DeviceId(4),
            Instruction::Status,
            Error(),
            std::vector<uint8_t>{0x01, 0x02, 0x02, 0x01},
        };

        auto result = control_table_map.receive(dev_4_read);
        REQUIRE(result == ProtocolResult::Ok);
        result = control_table_map.receive(dev_4_read_resp);
        REQUIRE(result == ProtocolResult::Ok);

        uint8_t buf[4];
        REQUIRE(dev_4.memory().read(0x0040, buf, 4));
        REQUIRE(buf[0] == 0x01);
        REQUIRE(buf[1] == 0x02);
        REQUIRE(buf[2] == 0x02);
        REQUIRE(buf[3] == 0x01);
    }

    SECTION("write packets") {
        Packet dev_4_write{
            DeviceId(4),
            Instruction::Write,
            Error(),
            std::vector<uint8_t>{0x0a, 0x00, 0x01, 0x02, 0x03},
        };

        Packet dev_4_empty_write{
            DeviceId(4),
            Instruction::Write,
            Error(),
            std::vector<uint8_t>{0x0a, 0x00},
        };

        Packet dev_5_write{
            DeviceId(5),
            Instruction::Write,
            Error(),
            std::vector<uint8_t>{0x20, 0x00, 0x03, 0x02, 0x42},
        };

        auto result = control_table_map.receive(dev_4_write);
        REQUIRE(result == ProtocolResult::Ok);
        result = control_table_map.receive(dev_5_write);
        REQUIRE(result == ProtocolResult::Ok);

        uint8_t buf[3];
        REQUIRE(dev_4.memory().read(0x000a, buf, 3));
        REQUIRE(buf[0] == 0x01);
        REQUIRE(buf[1] == 0x02);
        REQUIRE(buf[2] == 0x03);

        REQUIRE(dev_5.memory().read(0x0020, buf, 3));
        REQUIRE(buf[0] == 0x03);
        REQUIRE(buf[1] == 0x02);
        REQUIRE(buf[2] == 0x42);

        result = control_table_map.receive(dev_4_empty_write);
        REQUIRE(result == ProtocolResult::Ok);

        REQUIRE(dev_4.memory().read(0x000a, buf, 3));
        REQUIRE(buf[0] == 0x01);
        REQUIRE(buf[1] == 0x02);
        REQUIRE(buf[2] == 0x03);
    }

    SECTION("sync read packets") {
        Packet sync_read_packet{
            DeviceId::broadcast(),
            Instruction::SyncRead,
            Error(),
            std::vector<uint8_t>{0x0f, 0x00, 0x04, 0x00, 0x04, 0x05},
        };

        Packet dev_4_sync_read_resp{
            DeviceId(4),
            Instruction::Status,
            Error(),
            std::vector<uint8_t>{0x0d, 0x0e, 0x0a, 0x0d},
        };

        Packet dev_5_sync_read_resp{
            DeviceId(5),
            Instruction::Status,
            Error(),
            std::vector<uint8_t>{0x0b, 0x0e, 0x0e, 0x0f},
        };

        auto result = control_table_map.receive(sync_read_packet);
        REQUIRE(result == ProtocolResult::Ok);
        result = control_table_map.receive(dev_4_sync_read_resp);
        REQUIRE(result == ProtocolResult::Ok);
        result = control_table_map.receive(dev_5_sync_read_resp);
        REQUIRE(result == ProtocolResult::Ok);

        uint8_t buf[4];
        REQUIRE(dev_4.memory().read(0x000f, buf, 4));
        REQUIRE(buf[0] == 0x0d);
        REQUIRE(buf[1] == 0x0e);
        REQUIRE(buf[2] == 0x0a);
        REQUIRE(buf[3] == 0x0d);

        REQUIRE(dev_5.memory().read(0x000f, buf, 4));
        REQUIRE(buf[0] == 0x0b);
        REQUIRE(buf[1] == 0x0e);
        REQUIRE(buf[2] == 0x0e);
        REQUIRE(buf[3] == 0x0f);
    }

    SECTION("sync write packets") {
        Packet sync_write_packet{
            DeviceId::broadcast(),
            Instruction::SyncWrite,
            Error(),
            std::vector<uint8_t>{0x10, 0x00, 0x02, 0x00, 0x04, 0x01, 0x02, 0x05, 0x03, 0x04},
        };

        auto result = control_table_map.receive(sync_write_packet);
        REQUIRE(result == ProtocolResult::Ok);

        uint8_t buf[2];
        REQUIRE(dev_4.memory().read(0x0010, buf, 2));
        REQUIRE(buf[0] == 0x01);
        REQUIRE(buf[1] == 0x02);

        REQUIRE(dev_5.memory().read(0x0010, buf, 2));
        REQUIRE(buf[0] == 0x03);
        REQUIRE(buf[1] == 0x04);
    }

    SECTION("bulk read packets") {
        Packet bulk_read_packet{
            DeviceId::broadcast(),
            Instruction::BulkRead,
            Error(),
            std::vector<uint8_t>{0x04, 0x11, 0x00, 0x04, 0x00, 0x05, 0x22, 0x00, 0x03, 0x00},
        };

        Packet dev_4_bulk_read_resp{
            DeviceId(4),
            Instruction::Status,
            Error(),
            std::vector<uint8_t>{0x11, 0x22, 0x33, 0x44},
        };

        Packet dev_5_bulk_read_resp{
            DeviceId(5),
            Instruction::Status,
            Error(),
            std::vector<uint8_t>{0xaa, 0xbb, 0xcc},
        };

        auto result = control_table_map.receive(bulk_read_packet);
        REQUIRE(result == ProtocolResult::Ok);
        result = control_table_map.receive(dev_4_bulk_read_resp);
        REQUIRE(result == ProtocolResult::Ok);
        result = control_table_map.receive(dev_5_bulk_read_resp);
        REQUIRE(result == ProtocolResult::Ok);

        uint8_t buf[4];
        REQUIRE(dev_4.memory().read(0x0011, buf, 4));
        REQUIRE(buf[0] == 0x11);
        REQUIRE(buf[1] == 0x22);
        REQUIRE(buf[2] == 0x33);
        REQUIRE(buf[3] == 0x44);

        REQUIRE(dev_5.memory().read(0x0022, buf, 3));
        REQUIRE(buf[0] == 0xaa);
        REQUIRE(buf[1] == 0xbb);
        REQUIRE(buf[2] == 0xcc);
    }

    SECTION("bulk write packets") {
        Packet bulk_write_packet{
            DeviceId::broadcast(),
            Instruction::BulkWrite,
            Error(),
            std::vector<uint8_t>{
                0x04,
                0x10,
                0x00,
                0x02,
                0x00,
                0xab,
                0xcd,
                0x05,
                0x20,
                0x00,
                0x03,
                0x00,
                0xf0,
                0xe0,
                0xd0,
            },
        };

        auto result = control_table_map.receive(bulk_write_packet);
        REQUIRE(result == ProtocolResult::Ok);

        uint8_t buf[3];
        REQUIRE(dev_4.memory().read(0x0010, buf, 2));
        REQUIRE(buf[0] == 0xab);
        REQUIRE(buf[1] == 0xcd);

        REQUIRE(dev_5.memory().read(0x0020, buf, 3));
        REQUIRE(buf[0] == 0xf0);
        REQUIRE(buf[1] == 0xe0);
        REQUIRE(buf[2] == 0xd0);
    }

    SECTION("device disconnect") {
        Packet read_packet{
            DeviceId(4),
            Instruction::Read,
            Error(),
            std::vector<uint8_t>{0x00, 0x00, 0x04, 0x00},
        };

        Packet empty_resp_packet{
            DeviceId(4),
            Instruction::Status,
            Error(200),
            std::vector<uint8_t>(),
        };

        REQUIRE_FALSE(control_table_map.is_disconnected(DeviceId(4)));
        REQUIRE_FALSE(control_table_map.is_disconnected(DeviceId(5)));

        for (size_t i = 0; i < ControlTableMap::MAX_ALLOWED_MISSED_PACKETS + 1; i++) {
            auto result = control_table_map.receive(read_packet);
            REQUIRE(result == ProtocolResult::Ok);
            REQUIRE_FALSE(control_table_map.is_disconnected(DeviceId(4)));
            REQUIRE_FALSE(control_table_map.is_disconnected(DeviceId(5)));
        }

        // send another packet to end processing of the last one
        auto result = control_table_map.receive(read_packet);
        REQUIRE(result == ProtocolResult::Ok);

        REQUIRE(control_table_map.is_disconnected(DeviceId(4)));
        REQUIRE_FALSE(control_table_map.is_disconnected(DeviceId(5)));

        result = control_table_map.receive(empty_resp_packet);
        REQUIRE(result == ProtocolResult::StatusHasError);

        REQUIRE_FALSE(control_table_map.is_disconnected(DeviceId(4)));
        REQUIRE_FALSE(control_table_map.is_disconnected(DeviceId(5)));
    }
}
