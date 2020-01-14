#include "app.h"
#include "buffer.h"
#include "control_table.h"
#include "parser.h"
#include <GUI.h>
#include <memory>
#include <stm32f7xx.h>
#include <unordered_map>

using ControlTableMap = std::unordered_map<DeviceId, std::unique_ptr<ControlTable>>;

enum class CommResult {
    Ok,
    Error,
};

class Connection {
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

void handle_incoming_packets(Connection&, ControlTableMap&);
CommResult handle_status_packet(const InstructionPacket&, const Packet&, const ControlTableMap&);

void run(const std::vector<ReceiveBuf*>& bufs) {
    ControlTableMap control_tables;
    std::vector<Connection> connections;

    connections.reserve(bufs.size());

    for (auto buf : bufs) {
        connections.push_back(Connection(buf));
    }

    GUI_SetColor(GUI_GREEN);
    GUI_DispString("Hello world");

    uint32_t last_ui_update = 0;

    while (true) {
        for (auto& connection : connections) {
            auto now = HAL_GetTick();

            // render every 16ms (roughly 60 fps)
            if (now - last_ui_update > 16) {
                GUI_Exec();
                last_ui_update = now;
            }

            handle_incoming_packets(connection, control_tables);
        }
    }
}

void handle_incoming_packets(Connection& connection, ControlTableMap& control_tables) {
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

CommResult handle_status_packet(
    const InstructionPacket& instruction_packet,
    const Packet& status_packet,
    const ControlTableMap& control_tables) {
    // TODO: process packet
    return CommResult::Ok;
}
