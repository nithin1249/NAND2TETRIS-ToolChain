//
// Created by Nithin Kondabathini on 23/1/2026.
//

#ifndef NAND2TETRIS_TOKEN_H
#define NAND2TETRIS_TOKEN_H

#include <string>
#include <unordered_map>

namespace nand2tetris::jack {

    /**
     * @brief Represents the different types of tokens in the Jack language.
     */
    enum class TokenType {
        KEYWORD,        ///< A reserved keyword (e.g., class, method, int).
        SYMBOL,         ///< A symbol or operator (e.g., {, }, +, =).
        IDENTIFIER,     ///< A user-defined identifier (variable name, class name, etc.).
        INT_CONST,      ///< An integer constant (0-32767).
        STRING_CONST,   ///< A string constant enclosed in double quotes.
        END_OF_FILE     ///< Represents the end of the input stream.
    };

    /**
     * @brief Represents the specific keywords in the Jack language.
     */
    enum class Keyword {
        CLASS, METHOD, FUNCTION, CONSTRUCTOR,
        INT, BOOLEAN, CHAR, VOID,
        VAR, STATIC, FIELD, LET, DO, IF,
        ELSE, WHILE, RETURN, TRUE_, FALSE_, NULL_, THIS_
    };

    /**
     * @brief Converts a Keyword enum value to its string representation.
     *
     * @param kw The Keyword to convert.
     * @return The string representation of the keyword (e.g., "class", "while").
     */
    inline const char* keywordToString(const Keyword kw) {
        using K = Keyword;
        switch (kw) {
            case K::CLASS:       return "class";
            case K::METHOD:      return "method";
            case K::FUNCTION:    return "function";
            case K::CONSTRUCTOR: return "constructor";
            case K::INT:         return "int";
            case K::CHAR:        return "char";
            case K::BOOLEAN:     return "boolean";
            case K::VOID:        return "void";
            case K::VAR:         return "var";
            case K::STATIC:      return "static";
            case K::FIELD:       return "field";
            case K::LET:         return "let";
            case K::DO:          return "do";
            case K::IF:          return "if";
            case K::ELSE:        return "else";
            case K::WHILE:       return "while";
            case K::RETURN:      return "return";
            case K::TRUE_:       return "true";
            case K::FALSE_:      return "false";
            case K::NULL_:       return "null";
            case K::THIS_:       return "this";
        }
        return "<unknown>";
    }

    /**
     * @brief Checks if a string corresponds to a Jack keyword.
     *
     * @param s The string to check.
     * @param outKw Output parameter where the corresponding Keyword enum will be stored if found.
     * @return true if the string is a keyword, false otherwise.
     */

    inline bool isKeywordString(std::string_view s, Keyword &outKw) {
        static const std::unordered_map<std::string_view, Keyword> keywordMap = {
            {"class",       Keyword::CLASS},
            {"method",      Keyword::METHOD},
            {"function",    Keyword::FUNCTION},
            {"constructor", Keyword::CONSTRUCTOR},
            {"int",         Keyword::INT},
            {"boolean",     Keyword::BOOLEAN},
            {"char",        Keyword::CHAR},
            {"void",        Keyword::VOID},
            {"var",         Keyword::VAR},
            {"static",      Keyword::STATIC},
            {"field",       Keyword::FIELD},
            {"let",         Keyword::LET},
            {"do",          Keyword::DO},
            {"if",          Keyword::IF},
            {"else",        Keyword::ELSE},
            {"while",       Keyword::WHILE},
            {"return",      Keyword::RETURN},
            {"true",        Keyword::TRUE_},
            {"false",       Keyword::FALSE_},
            {"null",        Keyword::NULL_},
            {"this",        Keyword::THIS_}
        };

        const auto it = keywordMap.find(s);
        if (it != keywordMap.end()) {
            outKw = it->second;
            return true;
        }

        return false;
    }

    /**
     * @brief Converts a TokenType enum value to its string representation.
     *
     * @param t The TokenType to convert.
     * @return The string representation of the token type.
     */
    inline const char* typeToString(const TokenType t) {
        switch(t) {
            case TokenType::KEYWORD:      return "KEYWORD";
            case TokenType::SYMBOL:       return "SYMBOL";
            case TokenType::IDENTIFIER:   return "IDENTIFIER";
            case TokenType::INT_CONST:    return "INT_CONST";
            case TokenType::STRING_CONST: return "STRING_CONST";
            case TokenType::END_OF_FILE:  return "EOF";
        }
        return "UNKNOWN";
    }

    /**
     * @brief Base class representing a token in the Jack language.
     *
     * This abstract struct holds common information for all tokens, such as type and location.
     */
    struct Token {
        protected:
            TokenType type = TokenType::END_OF_FILE; ///< The type of the token.
            int line;                        ///< The line number where the token appears.
            int column;                      ///< The column number where the token appears.

        public:
            /**
             * @brief Constructs a new Token.
             *
             * @param t The type of the token.
             * @param line The line number.
             * @param column The column number.
             */
            explicit Token(const TokenType t, const int line, const int column)
                : type(t), line(line), column(column) {}

            /**
             * @brief Gets the type of the token.
             * @return The TokenType.
             */
            TokenType getType() const { return type; }

            /**
             * @brief Gets the line number of the token.
             * @return The line number.
             */
            int getLine() const { return line; }

            /**
             * @brief Gets the column number of the token.
             * @return The column number.
             */
            int getColumn() const { return column; }

            /**
             * @brief Returns a string representation of the token for debugging.
             * @return A string describing the token.
             */
            virtual std::string toString() const = 0;

            /**
             * @brief Gets the string value of the token if applicable.
             * @return A string_view of the token's value.
             */
            virtual std::string_view getValue() const { return ""; }

            virtual ~Token() = default;
    };

    /**
     * @brief Represents a token that holds text content (Identifier, String Constant, Symbol).
     */
    struct TextToken final : public Token {
        private:
            std::string_view text; ///< The text content of the token.

        public:
            /**
             * @brief Constructs a new TextToken.
             *
             * @param t The type of the token.
             * @param text The text content.
             * @param line The line number.
             * @param column The column number.
             */
            TextToken(const TokenType t, const std::string_view text, const int line, const int column)
                : Token(t, line, column), text(text) {};

            /**
             * @brief Gets the text content of the token.
             * @return The text content.
             */
            std::string_view getText() const { return text; }

            std::string toString() const override {
                return "[" + std::to_string(line) + ":" + std::to_string(column) + "] " +
                       typeToString(type) + " '" + std::string(text) + "'";
            }

            ~TextToken() override = default;

            std::string_view getValue() const override { return text; }
    };

    /**
     * @brief Represents an integer constant token.
     */
    struct IntToken final : public Token {
        private:
            int intVal = 0; ///< The integer value.

        public:
            /**
             * @brief Constructs a new IntToken.
             *
             * @param val The integer value.
             * @param line The line number.
             * @param column The column number.
             */
            explicit IntToken(const int val, const int line, const int column)
                : Token(TokenType::INT_CONST, line, column), intVal(val) {}

            /**
             * @brief Gets the integer value.
             * @return The integer value.
             */
            int getInt() const { return intVal; }

            std::string toString() const override {
                return "[" + std::to_string(line) + ":" + std::to_string(column) + "] " +
                       typeToString(type) + " '" + std::to_string(intVal) + "'";
            }

            ~IntToken() override = default;
    };

    /**
     * @brief Represents a keyword token.
     */
    struct KeywordToken final : public Token {
        private:
            Keyword keyword{}; ///< The specific keyword.

        public:
            /**
             * @brief Constructs a new KeywordToken.
             *
             * @param keyword The keyword value.
             * @param line The line number.
             * @param column The column number.
             */
            explicit KeywordToken(const Keyword keyword, const int line, const int column)
                : Token(TokenType::KEYWORD, line, column), keyword(keyword) {}

            /**
             * @brief Gets the keyword value.
             * @return The Keyword.
             */
            Keyword getKeyword() const { return keyword; }

            std::string toString() const override {
                return "[" + std::to_string(line) + ":" + std::to_string(column) + "] " +
                       typeToString(type) + " '" + keywordToString(keyword) + "'";
            }

            ~KeywordToken() override = default;

            std::string_view getValue() const override { return keywordToString(this->keyword); }
    };

    /**
     * @brief Represents the End-Of-File token.
     */
    struct EofToken final : public Token {
        /**
         * @brief Constructs a new EofToken.
         *
         * @param line The line number.
         * @param column The column number.
         */
        EofToken(const int line, const int column)
            : Token(TokenType::END_OF_FILE, line, column) {}

        std::string toString() const override {
            return "[" + std::to_string(line) + ":" + std::to_string(column) + "] " +
                   typeToString(type) + " '<EOF>'";
        }

        ~EofToken() override = default;
    };
}

#endif //NAND2TETRIS_TOKEN_H