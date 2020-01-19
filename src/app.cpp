#include "app.h"
#include "buffer.h"
#include "control_table.h"
#include "main.h"
#include "parser.h"
#include <GUI.h>
#include <LISTVIEW.h>
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
    LISTVIEW_Handle list;
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
    auto list =
        LISTVIEW_CreateEx(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, WM_CF_SHOW, 0, GUI_ID_LISTVIEW0);

    LISTVIEW_SetAutoScrollV(list, true);
    LISTVIEW_AddColumn(list, 150, "Field", GUI_TA_LEFT | GUI_TA_VCENTER);
    LISTVIEW_AddColumn(list, DISPLAY_WIDTH - 150, "Value", GUI_TA_LEFT | GUI_TA_VCENTER);

    return Widgets{list};
}

static void update_ui(const Widgets& widgets, const ControlTableMap& control_tables) {
    size_t row_idx = 0;
    auto num_rows = LISTVIEW_GetNumRows(widgets.list);

    for (auto& id_and_table : control_tables) {
        auto device_id = id_and_table.first;
        auto& control_table = id_and_table.second;

        std::stringstream stream;
        stream << control_table->device_name() << " (" << device_id << ")";
        auto device_name_str = stream.str();
        if (row_idx >= num_rows) {
            const char* cells[] = {device_name_str.c_str(), "<-------------"};
            LISTVIEW_AddRow(widgets.list, cells);
            num_rows++;
        } else {
            LISTVIEW_SetItemText(widgets.list, 0, row_idx, device_name_str.c_str());
            LISTVIEW_SetItemText(widgets.list, 1, row_idx, "<-------------");
        }
        row_idx++;

        std::unordered_map<const char*, std::string> name_to_value;
        control_table->entries(name_to_value);

        for (auto& name_and_value : name_to_value) {
            auto name = name_and_value.first;
            auto value = name_and_value.second.c_str();

            if (row_idx >= num_rows) {
                const char* cells[] = {name, value};
                LISTVIEW_AddRow(widgets.list, cells);
                num_rows++;
            } else {
                LISTVIEW_SetItemText(widgets.list, 0, row_idx, name);
                LISTVIEW_SetItemText(widgets.list, 1, row_idx, value);
            }

            row_idx++;
        }
    }

    // delete no longer used rows
    for (size_t i = row_idx; i < num_rows; i++) {
        LISTVIEW_DeleteRow(widgets.list, i);
    }
}
