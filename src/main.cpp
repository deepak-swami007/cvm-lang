#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
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

std::optional<std::size_t> extractLineNumber(const std::string& message) {
    static const std::regex kLineRegex("line (\\d+)");
    std::smatch match;
    if (!std::regex_search(message, match, kLineRegex)) {
        return std::nullopt;
    }

    return static_cast<std::size_t>(std::stoull(match[1].str()));
}

std::optional<std::string> extractHighlightText(const std::string& message) {
    static const std::regex kNearRegex("near '([^']*)'");
    static const std::regex kQuotedRegex("'([^']+)'");

    std::smatch match;
    if (std::regex_search(message, match, kNearRegex)) {
        return match[1].str();
    }
    if (std::regex_search(message, match, kQuotedRegex)) {
        return match[1].str();
    }
    return std::nullopt;
}

std::optional<std::string_view> readSourceLine(std::string_view source, std::size_t lineNumber) {
    if (lineNumber == 0) {
        return std::nullopt;
    }

    std::size_t currentLine = 1;
    std::size_t lineStart = 0;
    for (std::size_t index = 0; index <= source.size(); ++index) {
        if (index == source.size() || source[index] == '\n') {
            if (currentLine == lineNumber) {
                return source.substr(lineStart, index - lineStart);
            }
            lineStart = index + 1;
            ++currentLine;
        }
    }

    return std::nullopt;
}

void printSourceContext(std::ostream& err,
                        std::string_view sourcePath,
                        std::string_view source,
                        std::size_t lineNumber,
                        const std::string& message) {
    const auto line = readSourceLine(source, lineNumber);
    if (!line.has_value()) {
        return;
    }

    err << " --> " << sourcePath << ':' << lineNumber << '\n';
    err << lineNumber << " | " << *line << '\n';

    std::size_t caretColumn = 0;
    if (const auto highlight = extractHighlightText(message)) {
        const std::size_t found = line->find(*highlight);
        if (found != std::string_view::npos) {
            caretColumn = found;
        }
    } else {
        caretColumn = line->find_first_not_of(" \t");
        if (caretColumn == std::string_view::npos) {
            caretColumn = 0;
        }
    }

    err << "  | " << std::string(caretColumn, ' ') << "^\n";
}

}  // namespace

int main(int argc, char** argv) {
    std::string sourcePath;
    std::string source;

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

        source = readFile(sourcePath);

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

        cvm::VirtualMachine vm;
        vm.run(chunk, std::cout, vmOptions);
    } catch (const std::exception& error) {
        std::string msg = error.what();

        // Try to extract line number from error message
        std::string lineInfo;
        if (const auto lineNumber = extractLineNumber(msg)) {
            lineInfo = "[Line " + std::to_string(*lineNumber) + "] ";
        }

        // Determine error type from context
        std::string errorType = "RuntimeError";
        if (msg.find("Parse error") != std::string::npos) {
            errorType = "SyntaxError";
        } else if (msg.find("Unexpected character") != std::string::npos) {
            errorType = "LexerError";
        } else if (msg.find("char literal") != std::string::npos || msg.find("Char literal") != std::string::npos ||
                   msg.find("escape sequence") != std::string::npos) {
            errorType = "LexerError";
        } else if (msg.find("Invalid numeric literal") != std::string::npos) {
            errorType = "LexerError";
        } else if (msg.find("Numeric literal") != std::string::npos && msg.find("out of range") != std::string::npos) {
            errorType = "OverflowError";
        } else if (msg.find("Undefined variable") != std::string::npos) {
            errorType = "NameError";
        } else if (msg.find("Division by zero") != std::string::npos || msg.find("Modulo by zero") != std::string::npos) {
            errorType = "MathError";
        } else if (msg.find("already declared") != std::string::npos || msg.find("already defined") != std::string::npos) {
            errorType = "DeclarationError";
        } else if (msg.find("Expected number") != std::string::npos || msg.find("Expected bool") != std::string::npos ||
                   msg.find("Expected int") != std::string::npos || msg.find("Cannot add") != std::string::npos ||
                   msg.find("Cannot assign") != std::string::npos) {
            errorType = "TypeError";
        } else if (msg.find("stack") != std::string::npos) {
            errorType = "StackError";
        } else if (msg.find("Invalid input") != std::string::npos ||
                   msg.find("input for variable") != std::string::npos ||
                   msg.find("input for auto input") != std::string::npos) {
            errorType = "InputError";
        } else if (msg.find("outside of a loop") != std::string::npos) {
            errorType = "SyntaxError";
        } else if (msg.find("overflow") != std::string::npos || msg.find("Overflow") != std::string::npos) {
            errorType = "OverflowError";
        }
        std::cerr << lineInfo << errorType << ": " << msg << '\n';
        if (const auto lineNumber = extractLineNumber(msg); lineNumber.has_value() && !source.empty()) {
            printSourceContext(std::cerr, sourcePath, source, *lineNumber, msg);
        }
        return 1;
    }

    return 0;
}
