#include "app.h"
#include "buffer.h"
#include "control_table.h"
#include "main.h"
#include "parser.h"
#include <GUI.h>
#include <SWIPELIST.h>
#include <TEXT.h>
#include <memory>
#include <sstream>
#include <stm32f7xx.h>
#include <unordered_map>

#include "mx106_control_table.h"
#include "mx64_control_table.h"

using ControlTableMap = std::unordered_map<DeviceId, std::unique_ptr<ControlTable>>;

enum class CommResult {
    Ok,
    Error,
};

struct Connection {
  public:
    Connection(ReceiveBuf* buf) :
        buf(buf),
        parser(Parser()),
        is_last_instruction_packet_known(false),
        last_packet(Packet{
            DeviceId(0),
            Instruction::Ping,
            Error(),
            FixedByteVector<PACKET_BUF_LEN>(),
        }) {}

    ReceiveBuf* buf;
    Parser parser;
    InstructionPacket last_instruction_packet;
    bool is_last_instruction_packet_known;
    Packet last_packet;
};

struct Widgets {
    SWIPELIST_Handle list;
};

static void handle_incoming_packets(Connection&, ControlTableMap&);
static Widgets create_ui();
static void update_ui(const Widgets& widgets, const ControlTableMap& control_tables);
CommResult handle_status_packet(const InstructionPacket&, const Packet&, const ControlTableMap&);

void run(const std::vector<ReceiveBuf*>& bufs) {
    ControlTableMap control_tables;
    std::vector<Connection> connections;

    connections.reserve(bufs.size());

    for (auto buf : bufs) {
        connections.push_back(Connection(buf));
    }

    Widgets widgets = create_ui();
    uint32_t last_ui_update = 0;

    while (true) {
        for (auto& connection : connections) {
            auto now = HAL_GetTick();

            // render every 16ms (roughly 60 fps)
            if (now - last_ui_update > 16) {
                update_ui(widgets, control_tables);
                GUI_Exec();
                last_ui_update = now;
            }

            handle_incoming_packets(connection, control_tables);
        }
    }
}

static void handle_incoming_packets(Connection& connection, ControlTableMap& control_tables) {
    Cursor* cursor;

    if (connection.buf->is_front_ready) {
        connection.buf->back.reset();
        cursor = &connection.buf->front;
    } else if (connection.buf->is_back_ready) {
        connection.buf->front.reset();
        cursor = &connection.buf->back;
    } else {
        return;
    }

    while (cursor->remaining_bytes() > 0) {
        auto result = connection.parser.parse(*cursor, connection.last_packet);
        if (result != ParseResult::packet_available()) {
            // TODO: err handling
            continue;
        }

        if (connection.last_packet.instruction == Instruction::Status) {
            if (!connection.is_last_instruction_packet_known) {
                continue;
            }

            auto result = handle_status_packet(
                connection.last_instruction_packet, connection.last_packet, control_tables);

            if (result != CommResult::Ok) {
                // TODO: err handling
            }
        } else {
            auto result = parse_instruction_packet(
                connection.last_packet, &connection.last_instruction_packet);

            connection.is_last_instruction_packet_known = result == InstrPacketParseResult::Ok;

            if (result != InstrPacketParseResult::Ok) {
                // TODO: err handling
                continue;
            }
        }
    }
}

static Widgets create_ui() {
    SWIPELIST_Handle list = SWIPELIST_CreateEx(
        0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, WM_CF_SHOW, 0, GUI_ID_SWIPELIST0);

    auto unselected_background_color = SWIPELIST_GetBkColor(list, SWIPELIST_CI_BK_ITEM_UNSEL);
    SWIPELIST_SetBkColor(list, SWIPELIST_CI_BK_ITEM_SEL, unselected_background_color);

    return Widgets{
        .list = list,
    };
}

static void update_ui(const Widgets& widgets, const ControlTableMap& control_tables) {
    return;

    for (auto& id_and_table : control_tables) {
        auto device_id = id_and_table.first;
        auto& control_table = id_and_table.second;

        std::unordered_map<const char*, std::string> name_to_value;
        control_table->entries(name_to_value);

        std::stringstream stream;
        stream << control_table->device_name() << "(" << device_id << ")";
        auto header = stream.str();
        stream.str("");

        int item_idx = SWIPELIST_GetNumItems(widgets.list);
        SWIPELIST_AddItem(widgets.list, header.c_str(), 0);

        for (auto& name_and_value : name_to_value) {
            auto name = name_and_value.first;
            auto& value = name_and_value.second;

            stream << name << ": " << value;
            auto fmt_string = stream.str();
            stream.str("");
            SWIPELIST_AddItemText(widgets.list, item_idx, fmt_string.c_str());
        }
    }
}

CommResult handle_status_packet(
    const InstructionPacket& instruction_packet,
    const Packet& status_packet,
    const ControlTableMap& control_tables) {
    // TODO: process packet
    return CommResult::Ok;
}
