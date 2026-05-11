#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "cvm/ast.h"
#include "cvm/compiler.h"
#include "cvm/lexer.h"
#include "cvm/parser.h"
#include "cvm/vm.h"

namespace {

std::string readFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Could not open file: " + path);
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

}  // namespace

int main(int argc, char** argv) {
    try {
        std::cout << std::unitbuf;

        std::string sourcePath;
        if (argc > 1) {
            sourcePath = argv[1];
        } else if (std::filesystem::exists("examples/first.cvm")) {
            sourcePath = "examples/first.cvm";
        } else {
            throw std::runtime_error("No input file provided and examples/first.cvm was not found.");
        }

        const std::string source = readFile(sourcePath);

        std::cout << "=== File ===\n" << sourcePath << "\n\n";
        std::cout << "=== Source ===\n" << source << "\n\n";

        cvm::Lexer lexer(source);
        std::vector<cvm::Token> tokens = lexer.scanTokens();

        std::cout << "=== Tokens ===\n";
        for (const auto& token : tokens) {
            std::cout << cvm::formatToken(token) << '\n';
        }
        std::cout << '\n';

        cvm::Parser parser(tokens);
        std::vector<cvm::StmtPtr> program = parser.parse();

        std::cout << "=== AST ===\n" << cvm::toString(program) << "\n\n";

        cvm::Compiler compiler;
        cvm::Chunk chunk = compiler.compile(program);

        std::cout << "=== Bytecode ===\n" << cvm::disassemble(chunk) << "\n\n";

        std::cout << "=== VM Output ===\n";
        cvm::VirtualMachine vm;
        vm.run(chunk, std::cout);
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
