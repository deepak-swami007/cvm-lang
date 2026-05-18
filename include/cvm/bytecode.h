#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "cvm/type.h"
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
    std::vector<std::size_t> lines;
    std::vector<Value> constants;
    std::vector<std::string> names;
    std::vector<DeclType> nameTypes;
    std::size_t currentLine = 1;

    void writeOp(OpCode op) {
        code.push_back(static_cast<std::uint8_t>(op));
        lines.push_back(currentLine);
    }

    void writeByte(std::uint8_t byte) {
        code.push_back(byte);
        lines.push_back(currentLine);
    }

    std::uint8_t addConstant(Value value) {
        if (constants.size() >= 255) {
            throw std::runtime_error("Too many constants in one chunk.");
        }
        constants.push_back(value);
        return static_cast<std::uint8_t>(constants.size() - 1);
    }

    std::uint8_t addName(const std::string& name, DeclType declType = DeclType::Auto) {
        for (std::size_t index = 0; index < names.size(); ++index) {
            if (names[index] == name) {
                if (declType != DeclType::Auto) {
                    nameTypes[index] = declType;
                }
                return static_cast<std::uint8_t>(index);
            }
        }

        if (names.size() >= 255) {
            throw std::runtime_error("Too many global names in one chunk.");
        }

        names.push_back(name);
        nameTypes.push_back(declType);
        return static_cast<std::uint8_t>(names.size() - 1);
    }
};

namespace detail {

inline constexpr std::string_view kBytecodeMagic = "CVMBC";
inline constexpr std::uint8_t kBytecodeVersion = 1;

inline void writeBytes(std::ostream& out, const void* data, std::size_t size, const char* context) {
    out.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    if (!out) {
        throw std::runtime_error("Failed to write " + std::string(context) + ".");
    }
}

inline void readBytes(std::istream& in, void* data, std::size_t size, const char* context) {
    in.read(static_cast<char*>(data), static_cast<std::streamsize>(size));
    if (!in) {
        throw std::runtime_error("Failed to read " + std::string(context) + ".");
    }
}

template <typename Unsigned>
inline void writeUnsigned(std::ostream& out, Unsigned value, const char* context) {
    static_assert(std::is_unsigned_v<Unsigned>);
    std::array<std::uint8_t, sizeof(Unsigned)> bytes{};
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = static_cast<std::uint8_t>((value >> (index * 8)) & 0xffu);
    }
    writeBytes(out, bytes.data(), bytes.size(), context);
}

template <typename Unsigned>
inline Unsigned readUnsigned(std::istream& in, const char* context) {
    static_assert(std::is_unsigned_v<Unsigned>);
    std::array<std::uint8_t, sizeof(Unsigned)> bytes{};
    readBytes(in, bytes.data(), bytes.size(), context);

    Unsigned value = 0;
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        value |= static_cast<Unsigned>(bytes[index]) << (index * 8);
    }
    return value;
}

inline std::uint8_t declTypeByte(DeclType type) {
    const int raw = static_cast<int>(type);
    if (raw < 0 || raw > static_cast<int>(DeclType::Char)) {
        throw std::runtime_error("Invalid declaration type while serializing bytecode.");
    }
    return static_cast<std::uint8_t>(raw);
}

inline DeclType readDeclType(std::istream& in, const char* context) {
    const std::uint8_t raw = readUnsigned<std::uint8_t>(in, context);
    if (raw > static_cast<std::uint8_t>(DeclType::Char)) {
        throw std::runtime_error("Invalid declaration type in bytecode file.");
    }
    return static_cast<DeclType>(raw);
}

inline std::uint64_t sizeToU64(std::size_t value, const char* context) {
    if (value > static_cast<std::size_t>(std::numeric_limits<std::uint64_t>::max())) {
        throw std::runtime_error(std::string(context) + " is too large to serialize.");
    }
    return static_cast<std::uint64_t>(value);
}

inline std::size_t u64ToSize(std::uint64_t value, const char* context) {
    if (value > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error(std::string(context) + " is too large for this platform.");
    }
    return static_cast<std::size_t>(value);
}

inline void writeString(std::ostream& out, const std::string& value, const char* context) {
    writeUnsigned(out, sizeToU64(value.size(), context), context);
    if (!value.empty()) {
        writeBytes(out, value.data(), value.size(), context);
    }
}

inline std::string readString(std::istream& in, const char* context) {
    const std::size_t size = u64ToSize(readUnsigned<std::uint64_t>(in, context), context);
    std::string value(size, '\0');
    if (!value.empty()) {
        readBytes(in, value.data(), value.size(), context);
    }
    return value;
}

enum class SerializedValueTag : std::uint8_t {
    Nil = 0,
    Int32 = 1,
    Int64 = 2,
    Float = 3,
    Double = 4,
    Bool = 5,
    Char = 6,
};

inline void writeValue(std::ostream& out, const Value& value) {
    if (std::holds_alternative<std::monostate>(value)) {
        writeUnsigned(out, static_cast<std::uint8_t>(SerializedValueTag::Nil), "value tag");
        return;
    }
    if (const auto* integer = std::get_if<int32_t>(&value)) {
        writeUnsigned(out, static_cast<std::uint8_t>(SerializedValueTag::Int32), "value tag");
        writeUnsigned(out, static_cast<std::uint32_t>(*integer), "int32 value");
        return;
    }
    if (const auto* integer = std::get_if<int64_t>(&value)) {
        writeUnsigned(out, static_cast<std::uint8_t>(SerializedValueTag::Int64), "value tag");
        writeUnsigned(out, static_cast<std::uint64_t>(*integer), "int64 value");
        return;
    }
    if (const auto* number = std::get_if<float>(&value)) {
        writeUnsigned(out, static_cast<std::uint8_t>(SerializedValueTag::Float), "value tag");
        writeUnsigned(out, std::bit_cast<std::uint32_t>(*number), "float value");
        return;
    }
    if (const auto* number = std::get_if<double>(&value)) {
        writeUnsigned(out, static_cast<std::uint8_t>(SerializedValueTag::Double), "value tag");
        writeUnsigned(out, std::bit_cast<std::uint64_t>(*number), "double value");
        return;
    }
    if (const auto* boolean = std::get_if<bool>(&value)) {
        writeUnsigned(out, static_cast<std::uint8_t>(SerializedValueTag::Bool), "value tag");
        writeUnsigned(out, static_cast<std::uint8_t>(*boolean ? 1 : 0), "bool value");
        return;
    }

    writeUnsigned(out, static_cast<std::uint8_t>(SerializedValueTag::Char), "value tag");
    writeUnsigned(out, static_cast<std::uint8_t>(std::get<char>(value)), "char value");
}

inline Value readValue(std::istream& in) {
    const auto tag = static_cast<SerializedValueTag>(readUnsigned<std::uint8_t>(in, "value tag"));
    switch (tag) {
        case SerializedValueTag::Nil:
            return Value{std::monostate{}};
        case SerializedValueTag::Int32:
            return Value{static_cast<int32_t>(readUnsigned<std::uint32_t>(in, "int32 value"))};
        case SerializedValueTag::Int64:
            return Value{static_cast<int64_t>(readUnsigned<std::uint64_t>(in, "int64 value"))};
        case SerializedValueTag::Float:
            return Value{std::bit_cast<float>(readUnsigned<std::uint32_t>(in, "float value"))};
        case SerializedValueTag::Double:
            return Value{std::bit_cast<double>(readUnsigned<std::uint64_t>(in, "double value"))};
        case SerializedValueTag::Bool:
            return Value{readUnsigned<std::uint8_t>(in, "bool value") != 0};
        case SerializedValueTag::Char:
            return Value{static_cast<char>(readUnsigned<std::uint8_t>(in, "char value"))};
    }

    throw std::runtime_error("Invalid value tag in bytecode file.");
}

}  // namespace detail

inline void writeChunkFile(const Chunk& chunk, const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Could not open bytecode file for writing: " + path);
    }

    detail::writeBytes(out, detail::kBytecodeMagic.data(), detail::kBytecodeMagic.size(), "bytecode header");
    detail::writeUnsigned(out, detail::kBytecodeVersion, "bytecode version");
    detail::writeUnsigned(out, detail::sizeToU64(chunk.code.size(), "Bytecode size"), "code size");
    detail::writeUnsigned(out, detail::sizeToU64(chunk.lines.size(), "Line table size"), "line table size");
    detail::writeUnsigned(out, detail::sizeToU64(chunk.constants.size(), "Constant table size"), "constant table size");
    detail::writeUnsigned(out, detail::sizeToU64(chunk.names.size(), "Global name table size"), "global name table size");

    if (!chunk.code.empty()) {
        detail::writeBytes(out, chunk.code.data(), chunk.code.size(), "bytecode instructions");
    }

    for (std::size_t line : chunk.lines) {
        detail::writeUnsigned(out, detail::sizeToU64(line, "Source line number"), "source line number");
    }

    for (const Value& constant : chunk.constants) {
        detail::writeValue(out, constant);
    }

    for (std::size_t index = 0; index < chunk.names.size(); ++index) {
        detail::writeUnsigned(out, detail::declTypeByte(chunk.nameTypes[index]), "global declaration type");
        detail::writeString(out, chunk.names[index], "global name");
    }
}

inline Chunk readChunkFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Could not open bytecode file: " + path);
    }

    std::string magic(detail::kBytecodeMagic.size(), '\0');
    detail::readBytes(in, magic.data(), magic.size(), "bytecode header");
    if (magic != detail::kBytecodeMagic) {
        throw std::runtime_error("Invalid bytecode file header.");
    }

    const std::uint8_t version = detail::readUnsigned<std::uint8_t>(in, "bytecode version");
    if (version != detail::kBytecodeVersion) {
        throw std::runtime_error("Unsupported bytecode version " + std::to_string(version) + ".");
    }

    Chunk chunk;
    const std::size_t codeSize = detail::u64ToSize(detail::readUnsigned<std::uint64_t>(in, "code size"), "Code size");
    const std::size_t lineCount =
        detail::u64ToSize(detail::readUnsigned<std::uint64_t>(in, "line table size"), "Line table size");
    const std::size_t constantCount =
        detail::u64ToSize(detail::readUnsigned<std::uint64_t>(in, "constant table size"), "Constant table size");
    const std::size_t nameCount =
        detail::u64ToSize(detail::readUnsigned<std::uint64_t>(in, "global name table size"), "Global name table size");

    if (constantCount > 255) {
        throw std::runtime_error("Bytecode file contains too many constants for this VM.");
    }
    if (nameCount > 255) {
        throw std::runtime_error("Bytecode file contains too many globals for this VM.");
    }

    chunk.code.resize(codeSize);
    if (!chunk.code.empty()) {
        detail::readBytes(in, chunk.code.data(), chunk.code.size(), "bytecode instructions");
    }

    chunk.lines.reserve(lineCount);
    for (std::size_t index = 0; index < lineCount; ++index) {
        chunk.lines.push_back(
            detail::u64ToSize(detail::readUnsigned<std::uint64_t>(in, "source line number"), "Source line number"));
    }

    if (chunk.lines.size() != chunk.code.size()) {
        throw std::runtime_error("Bytecode file has mismatched code and line table sizes.");
    }

    chunk.constants.reserve(constantCount);
    for (std::size_t index = 0; index < constantCount; ++index) {
        chunk.constants.push_back(detail::readValue(in));
    }

    chunk.names.reserve(nameCount);
    chunk.nameTypes.reserve(nameCount);
    for (std::size_t index = 0; index < nameCount; ++index) {
        chunk.nameTypes.push_back(detail::readDeclType(in, "global declaration type"));
        chunk.names.push_back(detail::readString(in, "global name"));
    }

    chunk.currentLine = chunk.lines.empty() ? 1 : chunk.lines.back();
    return chunk;
}

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
