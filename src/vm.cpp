#include "cvm/vm.h"

#include <stdexcept>

namespace cvm {

void VirtualMachine::run(const Chunk& chunk, std::ostream& out) {
    stack_.clear();
    globals_.clear();
    std::size_t ip = 0;

    while (ip < chunk.code.size()) {
        const auto instruction = static_cast<OpCode>(readByte(chunk, ip, "opcode"));

        switch (instruction) {
            case OpCode::Constant:
                push(readConstant(chunk, ip));
                break;
            case OpCode::DefineGlobal: {
                const std::string& name = readGlobalName(chunk, ip);
                if (globals_.contains(name)) {
                    throw std::runtime_error("Variable '" + name + "' is already defined.");
                }

                globals_.emplace(name, pop());
                break;
            }
            case OpCode::GetGlobal: {
                const std::string& name = readGlobalName(chunk, ip);
                const auto it = globals_.find(name);
                if (it == globals_.end()) {
                    throw std::runtime_error("Undefined variable '" + name + "'.");
                }

                push(it->second);
                break;
            }
            case OpCode::SetGlobal: {
                const std::string& name = readGlobalName(chunk, ip);
                const auto it = globals_.find(name);
                if (it == globals_.end()) {
                    throw std::runtime_error("Undefined variable '" + name + "'.");
                }
                if (stack_.empty()) {
                    throw std::runtime_error("Cannot assign to '" + name + "' because the VM stack is empty.");
                }

                it->second = stack_.back();
                break;
            }
            case OpCode::Add: {
                const Value right = pop();
                const Value left = pop();
                push(left + right);
                break;
            }
            case OpCode::Subtract: {
                const Value right = pop();
                const Value left = pop();
                push(left - right);
                break;
            }
            case OpCode::Multiply: {
                const Value right = pop();
                const Value left = pop();
                push(left * right);
                break;
            }
            case OpCode::Divide: {
                const Value right = pop();
                const Value left = pop();
                if (right == 0.0) {
                    throw std::runtime_error("Division by zero.");
                }
                push(left / right);
                break;
            }
            case OpCode::Negate:
                push(-pop());
                break;
            case OpCode::Print:
                out << pop() << '\n';
                break;
            case OpCode::Pop:
                pop();
                break;
            case OpCode::Halt:
                return;
            default:
                throw std::runtime_error("Unknown opcode byte " + std::to_string(static_cast<int>(instruction)) + ".");
        }
    }

    throw std::runtime_error("VM reached the end of bytecode without OP_HALT.");
}

std::uint8_t VirtualMachine::readByte(const Chunk& chunk, std::size_t& ip, const char* context) const {
    if (ip >= chunk.code.size()) {
        throw std::runtime_error("Unexpected end of bytecode while reading " + std::string(context) + ".");
    }

    return chunk.code[ip++];
}

Value VirtualMachine::readConstant(const Chunk& chunk, std::size_t& ip) const {
    const std::uint8_t constantIndex = readByte(chunk, ip, "constant operand");
    if (constantIndex >= chunk.constants.size()) {
        throw std::runtime_error("Invalid constant index " + std::to_string(constantIndex) + ".");
    }

    return chunk.constants[constantIndex];
}

const std::string& VirtualMachine::readGlobalName(const Chunk& chunk, std::size_t& ip) const {
    const std::uint8_t nameIndex = readByte(chunk, ip, "global operand");
    if (nameIndex >= chunk.names.size()) {
        throw std::runtime_error("Invalid global name index " + std::to_string(nameIndex) + ".");
    }

    return chunk.names[nameIndex];
}

void VirtualMachine::push(Value value) {
    stack_.push_back(value);
}

Value VirtualMachine::pop() {
    if (stack_.empty()) {
        throw std::runtime_error("VM stack underflow.");
    }

    const Value value = stack_.back();
    stack_.pop_back();
    return value;
}

}  // namespace cvm
