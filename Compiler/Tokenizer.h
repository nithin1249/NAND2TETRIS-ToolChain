//
// Created by Nithin Kondabathini on 23/11/2025.
//

#ifndef NAND2TETRIS_TOKENIZER_H
#define NAND2TETRIS_TOKENIZER_H

#include <string>
#include <iostream>
#include <memory>
#include "TokenTypes.h"

namespace nand2tetris::jack {
	class Tokenizer{
		public:
			explicit Tokenizer(const std::string& filePath);
			bool hasMoreTokens() const;
			void advance();
			const Token& current() const;
			static bool isKeywordString(const std::string& s, Keyword& outKw);

		private:
			std::string src;
			std::size_t pos=0;
			std::size_t line  = 1;
			std::size_t column = 1;

			std::string fileName;

			std::unique_ptr<Token> currentToken;

			void loadFile(const std::string& filePath);
			void skipWhitespaceAndComments();
			std::unique_ptr<Token> nextToken();

			std::unique_ptr<Token> readIdentifierOrKeyword(std::size_t tokenline, std::size_t tokencolumn);
			std::unique_ptr<Token> readNumber(std::size_t tokenline, std::size_t tokencolumn);
			std::unique_ptr<Token> readString(std::size_t tokenline, std::size_t tokencolumn);
		    void advanceChar();

			 [[noreturn]] void errorHere(const std::string& message) const;
			 [[noreturn]] void errorAt(const std::size_t errLine,const std::size_t errColumn,const std::string& message) const;
	};
}

#endif