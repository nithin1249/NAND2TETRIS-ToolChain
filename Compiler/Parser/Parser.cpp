//
// Created by Nithin Kondabathini on 13/1/2026.
//

#include "Parser.h"

namespace nand2tetris::jack {
	Parser::Parser(Tokenizer &tokenizer):tokenizer(tokenizer) {
		// Points to the first token already provided by the Tokenizer
		currentToken=&tokenizer.current();
	}

	std::unique_ptr<ClassNode> Parser::parse() {
		return compileClass();
	}


	void Parser::advance() {
		tokenizer.advance();
		currentToken = &tokenizer.current();
	}

	bool Parser::check(const TokenType type) const {
		return currentToken->getType()==type;
	}

	bool Parser::check(const std::string_view text) const {
		return currentToken->getValue()==text;
	}

	void Parser::consume(const TokenType type, const std::string_view errorMessage) {
		if (check(type)) {
			advance();
		} else {
			tokenizer.errorAt(currentToken->getLine(),currentToken->getColumn(), errorMessage);
		}
	}

	void Parser::consume(const std::string_view text, const std::string_view errorMessage) {
		if (check(text)) {
			advance();
		} else {
			tokenizer.errorAt(currentToken->getLine(),currentToken->getColumn(), errorMessage);
		}
	}


}
