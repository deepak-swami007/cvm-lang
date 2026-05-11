#pragma once

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace cvm {

using Value = double;

enum class OpCode : std::uint8_t {
    Constant,
    Add,
    Subtract,
    Multiply,
    Divide,
    Negate,
    Print,
    Pop,
    Halt,
};

class Chunk {
  public:
    std::vector<std::uint8_t> code;
    std::vector<Value> constants;

    void writeOp(OpCode op) {
        code.push_back(static_cast<std::uint8_t>(op));
    }

    void writeByte(std::uint8_t byte) {
        code.push_back(byte);
    }

    std::uint8_t addConstant(Value value) {
        if (constants.size() >= 255) {
            throw std::runtime_error("Too many constants in one chunk.");
        }
        constants.push_back(value);
        return static_cast<std::uint8_t>(constants.size() - 1);
    }
};

inline std::string_view opcodeName(OpCode op) {
    switch (op) {
        case OpCode::Constant: return "OP_CONSTANT";
        case OpCode::Add: return "OP_ADD";
        case OpCode::Subtract: return "OP_SUBTRACT";
        case OpCode::Multiply: return "OP_MULTIPLY";
        case OpCode::Divide: return "OP_DIVIDE";
        case OpCode::Negate: return "OP_NEGATE";
        case OpCode::Print: return "OP_PRINT";
        case OpCode::Pop: return "OP_POP";
        case OpCode::Halt: return "OP_HALT";
    }

    return "OP_UNKNOWN";
}

inline std::string disassemble(const Chunk& chunk) {
    std::ostringstream out;
    std::size_t offset = 0;

    while (offset < chunk.code.size()) {
        const auto op = static_cast<OpCode>(chunk.code[offset]);
        out << offset << ": " << opcodeName(op);

        if (op == OpCode::Constant) {
            const std::uint8_t constantIndex = chunk.code.at(offset + 1);
            out << ' ' << static_cast<int>(constantIndex) << " (" << chunk.constants.at(constantIndex) << ")";
            offset += 2;
        } else {
            offset += 1;
        }

        if (offset < chunk.code.size()) {
            out << '\n';
        }
    }

    return out.str();
}

}  // namespace cvm
