#include "app.h"
#include "control_table.h"
#include "main.h"
#include "parser.h"
#include <GUI.h>
#include <LISTVIEW.h>
#include <memory>
#include <sstream>
#include <stm32f7xx.h>
#include <unordered_map>

struct Connection {
  public:
    Connection(ReceiveBuf* buf) :
        buf(buf),
        parser(Parser()),
        last_packet(Packet{
            DeviceId(0),
            Instruction::Ping,
            Error(),
            std::vector<uint8_t>(),
        }) {}

    ReceiveBuf* buf;
    Parser parser;
    Packet last_packet;
};

struct Widgets {
    LISTVIEW_Handle list;
};

static void handle_incoming_packets(Connection&, ControlTableMap&);
static Widgets create_ui();
static void update_ui(const Widgets& widgets, const ControlTableMap& control_tables);
ProtocolResult
    handle_status_packet(const InstructionPacket&, const Packet&, const ControlTableMap&);

void run(const std::vector<ReceiveBuf*>& bufs) {
    ControlTableMap control_table_map;
    std::vector<Connection> connections;

    connections.reserve(bufs.size());

    for (auto buf : bufs) {
        connections.push_back(Connection(buf));
    }

    Widgets widgets = create_ui();

    uint32_t last_render = 0;
    uint32_t last_ui_update = 0;

    while (true) {
        for (auto& connection : connections) {
            auto now = HAL_GetTick();

            // only update UI every 500 ms
            if (now - last_ui_update > 500) {
                update_ui(widgets, control_table_map);
                last_ui_update = now;
            }

            // render every 16ms (roughly 60 fps)
            if (now - last_render > 16) {
                GUI_Exec();
                last_render = now;
            }

            handle_incoming_packets(connection, control_table_map);
        }
    }
}

static void handle_incoming_packets(Connection& connection, ControlTableMap& control_table_map) {
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
        auto parse_result = connection.parser.parse(*cursor, &connection.last_packet);

        if (parse_result != ParseResult::PacketAvailable) {
            if (parse_result == ParseResult::NeedMoreData) {
                return;
            }

            // TODO: err handling
            continue;
        }

        auto result = control_table_map.receive(connection.last_packet);
        if (result != ProtocolResult::Ok) {
            // TODO: err handling
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

static void update_ui(const Widgets& widgets, const ControlTableMap& control_table_map) {
    size_t row_idx = 0;
    auto num_rows = LISTVIEW_GetNumRows(widgets.list);

    for (auto& id_and_table : control_table_map) {
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

        auto formatted_fields = control_table->fmt_fields();

        for (auto& name_and_value : formatted_fields) {
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
