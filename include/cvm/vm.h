#pragma once

#include <ostream>
#include <string>
#include <string>
#include <unordered_map>
#include <vector>

#include "cvm/bytecode.h"

namespace cvm {

class VMOptions {
  public:
    std::size_t maxInstructions = 1'000'000;
};

class VirtualMachine {
  public:
    void run(const Chunk& chunk, std::ostream& out, const VMOptions& options = {});

  private:
    std::uint8_t readByte(const Chunk& chunk, std::size_t& ip, const char* context) const;
    std::uint16_t readShort(const Chunk& chunk, std::size_t& ip, const char* context) const;
    Value readConstant(const Chunk& chunk, std::size_t& ip) const;
    const std::string& readGlobalName(const Chunk& chunk, std::size_t& ip) const;
    const double& expectNumber(const Value& value, const char* context) const;
    bool expectBoolean(const Value& value, const char* context) const;
    void push(Value value);
    Value pop();

    std::vector<Value> stack_;
    std::unordered_map<std::string, Value> globals_;
};

}  // namespace cvm
