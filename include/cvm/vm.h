#pragma once

#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "cvm/bytecode.h"

namespace cvm {

class VirtualMachine {
  public:
    void run(const Chunk& chunk, std::ostream& out);

  private:
    std::uint8_t readByte(const Chunk& chunk, std::size_t& ip, const char* context) const;
    Value readConstant(const Chunk& chunk, std::size_t& ip) const;
    const std::string& readGlobalName(const Chunk& chunk, std::size_t& ip) const;
    void push(Value value);
    Value pop();

    std::vector<Value> stack_;
    std::unordered_map<std::string, Value> globals_;
};

}  // namespace cvm
