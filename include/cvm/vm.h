#pragma once

#include <ostream>
#include <vector>

#include "cvm/bytecode.h"

namespace cvm {

class VirtualMachine {
  public:
    void run(const Chunk& chunk, std::ostream& out);

  private:
    void push(Value value);
    Value pop();

    std::vector<Value> stack_;
};

}  // namespace cvm

