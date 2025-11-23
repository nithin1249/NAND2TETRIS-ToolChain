//
// Created by Nithin Kondabathini on 23/11/2025.
//

#ifndef NAND2TETRIS_TOKEN_H
#define NAND2TETRIS_TOKEN_H

#include<string>

namespace nand2tetris::jack {
	enum class TokenType {
		KEYWORD,
		SYMBOL,
		IDENTIFIER,
		INT_CONST,
		STRING_CONST,
		END_OF_FILE
	};

	enum class Keyword {
		CLASS,METHOD,FUNCTION,CONSTRUCTOR,
		INT, BOOLEAN, CHAR, VOID,
		VAR, STATIC, FIELD, LET, DO, IF,
		ELSE, WHILE, RETURN, TRUE_, FALSE_, NULL_, THIS_

	};

	struct Token {
		TokenType type=TokenType::END_OF_FILE;
		std::string text; //raw spelling;
		Keyword keyword{}; //only valid if type==KEYWORD
		int intVal=0; // only valid if type== INT_CONST
	};

}

#endif