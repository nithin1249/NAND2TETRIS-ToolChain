#include <iostream>
#include "Compiler/Tokenizer.h"

using namespace nand2tetris::jack;

int main(int const argc, char** argv) {
	if (argc<2) {
		std::cerr << "Usage: jack_tokens <file.jack>\n";
		return 1;
	}
	const std::string some=std::string(argv[1]);
	Tokenizer tokenizer(some);
	while (true) {
		tokenizer.advance();
		const Token& t = tokenizer.current();
		if (t.type == TokenType::END_OF_FILE) break;

		std::cout << "Token: ";

		switch (t.type) {
			case TokenType::KEYWORD:
				std::cout << "KEYWORD(" << t.text << ")";
				break;
			case TokenType::SYMBOL:
				std::cout << "SYMBOL(" << t.text << ")";
				break;
			case TokenType::IDENTIFIER:
				std::cout << "IDENTIFIER(" << t.text << ")";
				break;
			case TokenType::INT_CONST:
				std::cout << "INT(" << t.intVal << ")";
				break;
			case TokenType::STRING_CONST:
				std::cout << "STRING(\"" << t.text << "\")";
				break;
			default:
				break;
		}

		std::cout << "\n";
	}

}