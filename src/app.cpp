#include "app.h"
#include "buffer.h"
#include "control_table.h"
#include "option.h"
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
    Connection(UART_HandleTypeDef* uart) :
        uart(uart),
        buf(Cursor(nullptr, 0)),
        parser(Parser()),
        last_instruction_packet(Option<InstructionPacket>()),
        last_packet(Packet{
            DeviceId(0),
            Instruction::Ping,
            Error(),
            FixedByteVector<PACKET_BUF_LEN>(),
        }) {}

    UART_HandleTypeDef* uart;
    Cursor buf;
    Parser parser;
    Option<InstructionPacket> last_instruction_packet;
    Packet last_packet;
};

void handle_incoming_packets(Connection&, ControlTableMap&);
CommResult handle_status_packet(const InstructionPacket&, const Packet&, const ControlTableMap&);

void run(const std::vector<UART_HandleTypeDef*>& uarts) {
    ControlTableMap control_tables;
    std::vector<Connection> connections;

    connections.reserve(uarts.size());

    for (auto uart : uarts) {
        connections.push_back(Connection(uart));
    }

    GUI_DispString("Hello world");

    while (true) {
        GUI_Exec();

        for (auto& connection : connections) {
            handle_incoming_packets(connection, control_tables);
        }
    }
}

void handle_incoming_packets(Connection& connection, ControlTableMap& control_tables) {
    if (connection.buf.remaining_bytes() == 0) {
        // TODO: read from uart
    }

    auto result = connection.parser.parse(connection.buf, connection.last_packet);
    if (result != ParseResult::packet_available()) {
        // TODO: err handling
    }

    if (connection.last_packet.instruction == Instruction::Status) {
        if (connection.last_instruction_packet.is_some()) {
            auto result = handle_status_packet(
                connection.last_instruction_packet.unwrap(),
                connection.last_packet,
                control_tables);

            if (result != CommResult::Ok) {
                // TODO: err handling
            }
        }
    } else {
        if (connection.last_instruction_packet.is_none()) {
            connection.last_instruction_packet = Option<InstructionPacket>(InstructionPacket());
        }

        auto result = parse_instruction_packet(
            connection.last_packet, connection.last_instruction_packet.unwrap());

        if (result != InstrPacketParseResult::Ok) {
            // TODO: err handling
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
