#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

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

std::string usageText(std::string_view programName) {
    std::ostringstream out;
    out << "Usage: " << programName << " [--max-steps N] [path-to-script.cvm]\n"
        << "  --max-steps N   Stop after N VM instructions (0 disables the limit).\n";
    return out.str();
}

std::size_t parseMaxSteps(const std::string& text) {
    std::size_t parsed = 0;
    try {
        std::size_t position = 0;
        const unsigned long long raw = std::stoull(text, &position, 10);
        if (position != text.size()) {
            throw std::runtime_error("Invalid --max-steps value '" + text + "'.");
        }
        if (raw > std::numeric_limits<std::size_t>::max()) {
            throw std::runtime_error("The --max-steps value is too large.");
        }
        parsed = static_cast<std::size_t>(raw);
    } catch (const std::invalid_argument&) {
        throw std::runtime_error("Invalid --max-steps value '" + text + "'.");
    } catch (const std::out_of_range&) {
        throw std::runtime_error("The --max-steps value is too large.");
    }
    return parsed;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc > 1) {
        const std::string_view firstArg = argv[1];
        if (firstArg == "--help" || firstArg == "-h") {
            std::cout << usageText(argv[0]);
            return 0;
        }
    }

    try {
        std::cout << std::unitbuf;

        cvm::VMOptions vmOptions;
        std::string sourcePath;
        for (int index = 1; index < argc; ++index) {
            const std::string argument = argv[index];

            if (argument == "--max-steps") {
                if (index + 1 >= argc) {
                    throw std::runtime_error("Missing value after --max-steps.");
                }
                vmOptions.maxInstructions = parseMaxSteps(argv[++index]);
            } else if (!argument.empty() && argument[0] == '-') {
                throw std::runtime_error("Unknown option '" + argument + "'.\n" + usageText(argv[0]));
            } else if (!sourcePath.empty()) {
                throw std::runtime_error("Expected only one input file.\n" + usageText(argv[0]));
            } else {
                sourcePath = argument;
            }
        }

        if (sourcePath.empty() && std::filesystem::exists("examples/example.cvm")) {
            sourcePath = "examples/example.cvm";
        } else if (sourcePath.empty() && std::filesystem::exists("examples/first.cvm")) {
            sourcePath = "examples/first.cvm";
        } else if (sourcePath.empty()) {
            throw std::runtime_error(
                "No input file provided and no default script was found.\n" + usageText(argv[0]));
        }

        const std::string source = readFile(sourcePath);

        // === File ===
        // std::cout << "=== File ===\n" << sourcePath << "\n\n";

        // === Source ===
        // std::cout << "=== Source ===\n" << source << "\n\n";

        cvm::Lexer lexer(source);
        std::vector<cvm::Token> tokens = lexer.scanTokens();

        // === Tokens ===
        // std::cout << "=== Tokens ===\n";
        // for (const auto& token : tokens) {
        //     std::cout << cvm::formatToken(token) << '\n';
        // }
        // std::cout << '\n';

        cvm::Parser parser(tokens);
        std::vector<cvm::StmtPtr> program = parser.parse();

        // === AST ===
        // std::cout << "=== AST ===\n" << cvm::toString(program) << "\n\n";

        cvm::Compiler compiler;
        cvm::Chunk chunk = compiler.compile(program);

        // === Bytecode ===
        // std::cout << "=== Bytecode ===\n" << cvm::disassemble(chunk) << "\n\n";

        std::cout << "=== VM Output ===\n";
        cvm::VirtualMachine vm;
        vm.run(chunk, std::cout, vmOptions);
    } catch (const std::exception& error) {
        std::string msg = error.what();

        // Try to extract line number from error message
        std::regex lineRegex("line (\\d+)");
        std::smatch match;
        std::string lineInfo;
        if (std::regex_search(msg, match, lineRegex)) {
            lineInfo = "[Line " + match[1].str() + "] ";
        }

        // Determine error type from context
        std::string errorType = "RuntimeError";
        if (msg.find("Parse error") != std::string::npos) {
            errorType = "SyntaxError";
        } else if (msg.find("Unexpected character") != std::string::npos) {
            errorType = "LexerError";
        } else if (msg.find("Undefined variable") != std::string::npos) {
            errorType = "NameError";
        } else if (msg.find("Division by zero") != std::string::npos) {
            errorType = "MathError";
        } else if (msg.find("already declared") != std::string::npos || msg.find("already defined") != std::string::npos) {
            errorType = "DeclarationError";
        } else if (msg.find("Expected number") != std::string::npos || msg.find("Expected bool") != std::string::npos) {
            errorType = "TypeError";
        } else if (msg.find("stack") != std::string::npos) {
            errorType = "StackError";
        } else if (msg.find("Invalid input") != std::string::npos) {
            errorType = "InputError";
        }

        std::cerr << lineInfo << errorType << ": " << msg << '\n';
        return 1;
    }

    return 0;
}
