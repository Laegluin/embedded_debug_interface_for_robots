#include "app.h"
#include "buffer.h"
#include "control_table.h"
#include "main.h"
#include "parser.h"
#include <GUI.h>
#include <MULTIEDIT.h>
#include <memory>
#include <sstream>
#include <stm32f7xx.h>
#include <unordered_map>

#include "mx106_control_table.h"
#include "mx64_control_table.h"

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
    MULTIEDIT_HANDLE text;
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

    control_tables.emplace(DeviceId(0), std::make_unique<Mx64ControlTable>());
    control_tables.emplace(DeviceId(1), std::make_unique<Mx106ControlTable>());

    Widgets widgets = create_ui();

    uint32_t last_render = 0;
    uint32_t last_ui_update = 0;

    while (true) {
        for (auto& connection : connections) {
            auto now = HAL_GetTick();

            // only update UI every 500 ms
            if (now - last_ui_update > 500) {
                update_ui(widgets, control_tables);
                last_ui_update = now;
            }

            // render every 16ms (roughly 60 fps)
            if (now - last_render > 16) {
                GUI_Exec();
                last_render = now;
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

            auto result = update_control_table_map(
                connection.last_instruction_packet, connection.last_packet, control_tables);

            if (result != CommResult::Ok) {
                // TODO: err handling
            }
        } else {
            auto result = parse_instruction_packet(
                connection.last_packet, &connection.last_instruction_packet);

            connection.is_last_instruction_packet_known = result == InstructionParseResult::Ok;

            if (result != InstructionParseResult::Ok) {
                // TODO: err handling
                continue;
            }
        }
    }
}

static Widgets create_ui() {
    // WM_CF_MOTION_Y

    auto text = MULTIEDIT_CreateEx(
        0,
        0,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT,
        0,
        WM_CF_SHOW,
        MULTIEDIT_CF_READONLY | MULTIEDIT_CF_AUTOSCROLLBAR_V,
        GUI_ID_MULTIEDIT0,
        1000,
        "<loading>");

    // MULTIEDIT_ShowCursor(text, false);
    // MULTIEDIT_SetFocusable(text, false);
    auto background_color = MULTIEDIT_GetBkColor(text, MULTIEDIT_CI_EDIT);
    MULTIEDIT_SetBkColor(text, MULTIEDIT_CI_READONLY, background_color);
    auto text_color = MULTIEDIT_GetTextColor(text, MULTIEDIT_CI_EDIT);
    MULTIEDIT_SetTextColor(text, MULTIEDIT_CI_READONLY, text_color);

    return Widgets{
        .text = text,
    };
}

static void update_ui(const Widgets& widgets, const ControlTableMap& control_tables) {
    auto cursor_pos = MULTIEDIT_GetCursorCharPos(widgets.text);
    std::stringstream stream;

    for (auto& id_and_table : control_tables) {
        auto device_id = id_and_table.first;
        auto& control_table = id_and_table.second;

        stream << control_table->device_name() << " (" << device_id << ")\n\n";

        std::unordered_map<const char*, std::string> name_to_value;
        control_table->entries(name_to_value);

        for (auto& name_and_value : name_to_value) {
            auto name = name_and_value.first;
            auto& value = name_and_value.second;

            stream << name << ": " << value << "\n";
        }

        stream << "\n";
    }

    auto fmt_string = stream.str();
    MULTIEDIT_SetText(widgets.text, fmt_string.c_str());
    MULTIEDIT_SetCursorOffset(widgets.text, cursor_pos);
}
