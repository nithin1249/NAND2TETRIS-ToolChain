//
// Created by Nithin Kondabathini on 23/11/2025.
//

#include "Tokenizer.h"
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <string_view>

namespace nand2tetris::jack {

    Tokenizer::Tokenizer(const std::string &filePath) : fileName(filePath) {
        // Load the raw file content into the buffer first.
        loadFile(filePath);
        // Prime the tokenizer by fetching the first token immediately.
        currentToken = fetchNext();
    }

    std::unique_ptr<Token> Tokenizer::fetchNext() {
        // Before attempting to read a token, we must bypass any whitespace or comments
        // that might precede it.
        skipWhitespaceAndComments();
        return nextToken();
    }

    const Token& Tokenizer::peek() {
        // Lazy load the lookahead token only when requested.
        // This allows us to see what's coming next without consuming it.
        if (!peekToken) {
            peekToken = fetchNext();
        }

        return *peekToken;
    }

    void Tokenizer::loadFile(const std::string &filePath) {
        // Enforce the .jack file extension requirement.
        if (filePath.length() < 5 || filePath.substr(filePath.length() - 5) != ".jack") {
            throw std::runtime_error("Invalid file extension. Expected a .jack file: " + filePath);
        }
        std::ifstream in(filePath);
        if (!in) {
            throw std::runtime_error("Cannot open Jack file: " + filePath);
        }
        // Read the entire file into the 'src' string buffer using iterators.
        src.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

        // Reset parsing state.
        pos = 0;
        line = 1;
        column = 1;
    }

    void Tokenizer::advanceChar() {
        if (pos >= src.size()) {
            return;
        }
        const char c = src[pos++];

        // Handle line endings to keep track of line/column numbers for error reporting.
        if (c == '\n') {
            ++line;
            column = 1;
        } else if (c == '\r') {
            // Ignore carriage returns; we count lines on \n.
        } else {
            ++column;
        }
    }

    bool Tokenizer::hasMoreTokens() const {
        // If currentToken is null, we assume we haven't started or are in a valid state
        // to fetch more, unless we explicitly hit EOF.
        if (!currentToken) {
            return true;
        }
        return currentToken->getType() != TokenType::END_OF_FILE;
    }

    void Tokenizer::advance() {
        // If we have previously peeked, the next token is already waiting in peekToken.
        // We simply move it to currentToken.
        if (peekToken) {
            currentToken = std::move(peekToken);
            return;
        }

        if (!hasMoreTokens()) {
            return;
        }
        // Otherwise, we parse the next token from the source.
        currentToken = fetchNext();
    }

    void Tokenizer::skipWhitespaceAndComments() {
        while (pos < src.size()) {
            const char c = src[pos];

            // Skip standard whitespace characters (space, tab, newline, etc.)
            if (std::isspace(static_cast<unsigned char>(c))) {
                advanceChar();
                continue;
            }

            // Check for line comments starting with "//"
            if (c == '/' && pos + 1 < src.size() && src[pos + 1] == '/') {
                advanceChar(); // consume first /
                advanceChar(); // consume second /
                // Consume everything until the end of the line
                while (pos < src.size() && src[pos] != '\n') {
                    advanceChar();
                }
                continue;
            }

            // Check for block comments starting with "/*"
            if (c == '/' && pos + 1 < src.size() && src[pos + 1] == '*') {
                advanceChar(); // consume /
                advanceChar(); // consume *
                // Consume everything until we find the closing "*/"
                while (pos + 1 < src.size() && !(src[pos] == '*' && src[pos + 1] == '/')) {
                    advanceChar();
                }

                if (pos + 1 >= src.size()) {
                    errorHere("Unterminated block comment");
                }
                advanceChar(); // consume *
                advanceChar(); // consume /
                continue;
            }

            // If we hit a character that isn't whitespace or a comment start, we are done.
            break;
        }
    }

    std::unique_ptr<Token> Tokenizer::nextToken() {
        // If we've reached the end of the source, return an EOF token.
        if (pos >= src.size()) {
            return std::make_unique<EofToken>(line, column);
        }

        // Capture the start position of the token for error reporting.
        std::size_t tokenLine = line;
        std::size_t tokenColumn = column;
        const char c = src[pos];

        // Check for single-character symbols used in Jack.
        constexpr std::string_view symbols = "{}()[].,;+-*/&|<>=~";
        if (symbols.find(c) != std::string_view::npos) {
            std::string_view symView = std::string_view(src).substr(pos, 1);
            advanceChar();
            return std::make_unique<TextToken>(TokenType::SYMBOL, symView, tokenLine, tokenColumn);
        }

        // Check for string constants starting with double quotes.
        if (c == '"') {
            return readString(tokenLine, tokenColumn);
        }

        // Check for integer constants (digits).
        if (std::isdigit(static_cast<unsigned char>(c))) {
            return readNumber(tokenLine, tokenColumn);
        }

        // Check for identifiers or keywords (letters or underscore).
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            return readIdentifierOrKeyword(tokenLine, tokenColumn);
        }

        errorHere("Unexpected character: '" + std::string(1, c) + "'");
    }

    std::unique_ptr<Token> Tokenizer::readString(const std::size_t tokenline, const std::size_t tokencolumn) {
        advanceChar(); // consume the opening quote "

        const std::size_t start = pos;
        // Read until we hit the closing quote.
        while (pos < src.size() && src[pos] != '"') {
            // Jack strings cannot contain newlines.
            if (src[pos] == '\n' || src[pos] == '\r') errorAt(tokenline, tokencolumn, "Newline in string");
            advanceChar();
        }

        std::string_view val = std::string_view(src).substr(start, pos - start);

        if (pos >= src.size()) {
            errorAt(tokenline, tokencolumn, "Unterminated string constant");
        }
        advanceChar(); // consume the closing quote "
        return std::make_unique<TextToken>(TokenType::STRING_CONST, val, tokenline, tokencolumn);
    }

    const Token& Tokenizer::current() const {
        if (!currentToken) {
            throw std::logic_error("Tokenizer::current() called with no current token");
        }
        return *currentToken;
    }

    std::unique_ptr<Token> Tokenizer::readNumber(const std::size_t tokenline, const std::size_t tokencolumn) {
        int value = 0;

        // Consume consecutive digits.
        while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) {
            const int digit = src[pos] - '0';

            // Check for overflow BEFORE updating the value.
            // The maximum allowed integer in Jack is 32767.
            // If value > 3276, then value * 10 >= 32760. Adding any digit > 7 would exceed 32767.
            if (value > 3276 || (value == 3276 && digit > 7)) {
                errorAt(tokenline, tokencolumn, "Integer constant too large (max 32767)");
            }

            value = value * 10 + digit;
            advanceChar();
        }

        return std::make_unique<IntToken>(value, tokenline, tokencolumn);
    }

    std::unique_ptr<Token> Tokenizer::readIdentifierOrKeyword(const std::size_t tokenline, const std::size_t tokencolumn) {
        const std::size_t start = pos;
        // Consume alphanumeric characters and underscores.
        while (pos < src.size() && (std::isalnum(static_cast<unsigned char>(src[pos])) || src[pos] == '_')) {
            advanceChar();
        }

        // Extract the text we just scanned.
        std::string_view s = std::string_view(src).substr(start, pos - start);

        // Check if this text matches a reserved keyword.
        Keyword kw;
        if (isKeywordString(s, kw)) {
            return std::make_unique<KeywordToken>(kw, tokenline, tokencolumn);
        }

        // Otherwise, it's a user-defined identifier.
        return std::make_unique<TextToken>(TokenType::IDENTIFIER, s, tokenline, tokencolumn);
    }

    [[noreturn]] void Tokenizer::errorAt(const std::size_t errLine, const std::size_t errColumn, const std::string_view message) const {
        // Format a standard error message: file:line:col: message
        const std::string full =
            fileName + ":" +
            std::to_string(errLine) + ":" +
            std::to_string(errColumn) + ": " +
            std::string(message);
        throw std::runtime_error(full);
    }

    [[noreturn]] void Tokenizer::errorHere(const std::string_view message) const {
        errorAt(line, column, message);
    }

}
