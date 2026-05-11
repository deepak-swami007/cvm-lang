#pragma once

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "cvm/value.h"

namespace cvm {

enum class OpCode : std::uint8_t {
    Nil,
    True,
    False,
    Constant,
    DefineGlobal,
    GetGlobal,
    SetGlobal,
    Equal,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    Not,
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Power,
    BitwiseAnd,
    BitwiseOr,
    BitwiseNot,
    Negate,
    Jump,
    JumpIfFalse,
    Loop,
    Print,
    Input,
    CastToInt,
    CastToDouble,
    Pop,
    Halt,
};

class Chunk {
  public:
    std::vector<std::uint8_t> code;
    std::vector<Value> constants;
    std::vector<std::string> names;

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

    std::uint8_t addName(const std::string& name) {
        for (std::size_t index = 0; index < names.size(); ++index) {
            if (names[index] == name) {
                return static_cast<std::uint8_t>(index);
            }
        }

        if (names.size() >= 255) {
            throw std::runtime_error("Too many global names in one chunk.");
        }

        names.push_back(name);
        return static_cast<std::uint8_t>(names.size() - 1);
    }
};

inline std::string_view opcodeName(OpCode op) {
    switch (op) {
        case OpCode::Nil: return "OP_NIL";
        case OpCode::True: return "OP_TRUE";
        case OpCode::False: return "OP_FALSE";
        case OpCode::Constant: return "OP_CONSTANT";
        case OpCode::DefineGlobal: return "OP_DEFINE_GLOBAL";
        case OpCode::GetGlobal: return "OP_GET_GLOBAL";
        case OpCode::SetGlobal: return "OP_SET_GLOBAL";
        case OpCode::Equal: return "OP_EQUAL";
        case OpCode::Greater: return "OP_GREATER";
        case OpCode::GreaterEqual: return "OP_GREATER_EQUAL";
        case OpCode::Less: return "OP_LESS";
        case OpCode::LessEqual: return "OP_LESS_EQUAL";
        case OpCode::Not: return "OP_NOT";
        case OpCode::Add: return "OP_ADD";
        case OpCode::Subtract: return "OP_SUBTRACT";
        case OpCode::Multiply: return "OP_MULTIPLY";
        case OpCode::Divide: return "OP_DIVIDE";
        case OpCode::Modulo: return "OP_MODULO";
        case OpCode::Power: return "OP_POWER";
        case OpCode::BitwiseAnd: return "OP_BITWISE_AND";
        case OpCode::BitwiseOr: return "OP_BITWISE_OR";
        case OpCode::BitwiseNot: return "OP_BITWISE_NOT";
        case OpCode::Negate: return "OP_NEGATE";
        case OpCode::Jump: return "OP_JUMP";
        case OpCode::JumpIfFalse: return "OP_JUMP_IF_FALSE";
        case OpCode::Loop: return "OP_LOOP";
        case OpCode::Print: return "OP_PRINT";
        case OpCode::Input: return "OP_INPUT";
        case OpCode::CastToInt: return "OP_CAST_TO_INT";
        case OpCode::CastToDouble: return "OP_CAST_TO_DOUBLE";
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

        switch (op) {
            case OpCode::Constant: {
                if (offset + 1 >= chunk.code.size()) {
                    out << " <missing operand>";
                    offset = chunk.code.size();
                    break;
                }

                const std::uint8_t constantIndex = chunk.code[offset + 1];
                out << ' ' << static_cast<int>(constantIndex);
                if (constantIndex < chunk.constants.size()) {
                    out << " (" << formatValue(chunk.constants[constantIndex]) << ")";
                } else {
                    out << " (<invalid constant index>)";
                }
                offset += 2;
                break;
            }
            case OpCode::DefineGlobal:
            case OpCode::GetGlobal:
            case OpCode::SetGlobal:
            case OpCode::Input: {
                if (offset + 1 >= chunk.code.size()) {
                    out << " <missing operand>";
                    offset = chunk.code.size();
                    break;
                }

                const std::uint8_t nameIndex = chunk.code[offset + 1];
                out << ' ' << static_cast<int>(nameIndex);
                if (nameIndex < chunk.names.size()) {
                    out << " (" << chunk.names[nameIndex] << ")";
                } else {
                    out << " (<invalid name index>)";
                }
                offset += 2;
                break;
            }
            case OpCode::Jump:
            case OpCode::JumpIfFalse:
            case OpCode::Loop: {
                if (offset + 2 >= chunk.code.size()) {
                    out << " <missing operand>";
                    offset = chunk.code.size();
                    break;
                }

                const std::uint16_t jumpOffset =
                    static_cast<std::uint16_t>((chunk.code[offset + 1] << 8) | chunk.code[offset + 2]);
                const std::size_t target =
                    op == OpCode::Loop ? offset + 3 - jumpOffset : offset + 3 + jumpOffset;
                out << ' ' << jumpOffset << " -> " << target;
                offset += 3;
                break;
            }
            default:
                offset += 1;
                break;
        }

        if (offset < chunk.code.size()) {
            out << '\n';
        }
    }

    return out.str();
}

}  // namespace cvm
