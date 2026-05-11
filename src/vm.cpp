#include "cvm/vm.h"

#include <stdexcept>

namespace cvm {

void VirtualMachine::run(const Chunk& chunk, std::ostream& out) {
    stack_.clear();
    std::size_t ip = 0;

    while (ip < chunk.code.size()) {
        const auto instruction = static_cast<OpCode>(chunk.code[ip++]);

        switch (instruction) {
            case OpCode::Constant: {
                const std::uint8_t constantIndex = chunk.code.at(ip++);
                push(chunk.constants.at(constantIndex));
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
        }
    }

    throw std::runtime_error("VM reached the end of bytecode without OP_HALT.");
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

