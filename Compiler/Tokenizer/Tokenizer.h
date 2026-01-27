//
// Created by Nithin Kondabathini on 23/11/2025.
//

#ifndef NAND2TETRIS_TOKENIZER_H
#define NAND2TETRIS_TOKENIZER_H

#include <string>
#include <string_view>
#include <memory>
#include "TokenTypes.h"

namespace nand2tetris::jack {

    /**
     * @brief A tokenizer for the Jack language.
     *
     * This class reads a Jack source file and breaks it down into a stream of tokens.
     * It handles whitespace, comments, and various token types (keywords, symbols, identifiers, etc.).
     */
    class Tokenizer {
        public:
            /**
             * @brief Constructs a Tokenizer for the given file.
             *
             * @param filePath The path to the .jack file to tokenize.
             * @throws std::runtime_error if the file cannot be opened or has an invalid extension.
             */
            explicit Tokenizer(const std::string& filePath);

            /**
             * @brief Checks if there are more tokens in the input.
             *
             * @return true if there are more tokens, false otherwise.
             */
            bool hasMoreTokens() const;

            /**
             * @brief Advances to the next token.
             *
             * This updates the current token to the next one in the stream.
             */
            void advance();

            /**
             * @brief Returns the current token.
             *
             * @return A reference to the current Token object.
             * @throws std::logic_error if there is no current token.
             */
            const Token& current() const;


            /**
             * @brief Peeks at the next token without advancing the current token.
             *
             * @return A reference to the next Token object.
             */
            const Token& peek();

            /**
             * @brief Reports an error at the current tokenizer position and throws an exception.
             *
             * @param message The error message.
             */
            [[noreturn]] void errorHere(std::string_view message) const;

            /**
             * @brief Reports an error at a specific location and throws an exception.
             *
             * @param errLine The line number of the error.
             * @param errColumn The column number of the error.
             * @param message The error message.
             */
            [[noreturn]] void errorAt(size_t errLine, size_t errColumn, std::string_view message) const;


        private:
            std::string src;        ///< The source code content.
            std::size_t pos = 0;    ///< Current character position in the source.
            std::size_t line = 1;   ///< Current line number.
            std::size_t column = 1; ///< Current column number.

            std::string fileName;   ///< The name of the file being tokenized.

            std::unique_ptr<Token> currentToken; ///< The current token.
            std::unique_ptr<Token> peekToken = nullptr; ///< The next token (used for lookahead).

            /**
             * @brief Loads the content of the file into the source buffer.
             *
             * @param filePath The path to the file.
             */
            void loadFile(const std::string& filePath);

            /**
             * @brief Skips whitespace and comments in the source code.
             */
            void skipWhitespaceAndComments();

            /**
             * @brief Scans and returns the next token from the source.
             *
             * @return A unique_ptr to the next Token.
             */
            std::unique_ptr<Token> nextToken();

            /**
             * @brief Helper to fetch the next token, handling whitespace skipping.
             *
             * @return A unique_ptr to the next Token.
             */
            std::unique_ptr<Token> fetchNext();


            /**
             * @brief Reads an identifier or a keyword from the source.
             *
             * @param tokenline The starting line of the token.
             * @param tokencolumn The starting column of the token.
             * @return A unique_ptr to the resulting Token.
             */
            std::unique_ptr<Token> readIdentifierOrKeyword(std::size_t tokenline, std::size_t tokencolumn);

            /**
             * @brief Reads an integer constant from the source.
             *
             * @param tokenline The starting line of the token.
             * @param tokencolumn The starting column of the token.
             * @return A unique_ptr to the resulting Token.
             */
            std::unique_ptr<Token> readNumber(std::size_t tokenline, std::size_t tokencolumn);

            /**
             * @brief Reads a string constant from the source.
             *
             * @param tokenline The starting line of the token.
             * @param tokencolumn The starting column of the token.
             * @return A unique_ptr to the resulting Token.
             */
            std::unique_ptr<Token> readString(std::size_t tokenline, std::size_t tokencolumn);

            /**
             * @brief Advances the current character position.
             *
             * Updates line and column counters accordingly.
             */
            void advanceChar();

    };
}

#endif //NAND2TETRIS_TOKENIZER_H