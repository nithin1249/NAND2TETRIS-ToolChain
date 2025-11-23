//
// Created by Nithin Kondabathini on 23/11/2025.
//

#ifndef NAND2TETRIS_TOKENIZER_H
#define NAND2TETRIS_TOKENIZER_H

#include <string>
#include <iostream>
#include "Token.h"

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
			Token currentToken;

			void loadFile(const std::string& filePath);
			void skipWhitespaceAndComments();
			Token nextToken();

			Token readIdentifierOrKeyword();
			Token readNumber();
			Token readString();

	};
}

#endif