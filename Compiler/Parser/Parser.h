//
// Created by Nithin Kondabathini on 13/1/2026.
//

#ifndef NAND2TETRIS_PARSER_H
#define NAND2TETRIS_PARSER_H

#include "../Tokenizer/Tokenizer.h"
#include "../Tokenizer/TokenTypes.h"
#include "AST.h"

namespace nand2tetris::jack {

	class Parser {
		Tokenizer& tokenizer;
		const Token* currentToken=nullptr;

		//helper methods:
		void advance();
		bool check(TokenType type) const;
		bool check(std::string_view text) const;
		void consume(TokenType type, std::string_view errorMessage);
		void consume(std::string_view text, std::string_view errorMessage);

		// --- Grammar Rules (Recursive Descent) ---
		std::unique_ptr<ClassNode> compileClass();
		std::unique_ptr<ClassVarDecNode> compileClassVarDec();
		std::unique_ptr<SubroutineDecNode> compileSubroutine();
		std::unique_ptr<VarDecNode> compileVarDec();
		std::vector<std::unique_ptr<StatementNode>> compileStatements();
		std::unique_ptr<StatementNode> compileStatement();
		std::unique_ptr<ExpressionNode> compileExpression();
		std::unique_ptr<ExpressionNode> compileTerm();

		public:
			explicit Parser(Tokenizer& tokenizer);
			std::unique_ptr<ClassNode> parse();
	};
};

#endif //NAND2TETRIS_PARSER_H