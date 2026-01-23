//
// Created by Nithin Kondabathini on 24/1/2026.
//

#ifndef NAND2TETRIS_SEMANTIC_ANALYSER_H
#define NAND2TETRIS_SEMANTIC_ANALYSER_H


#include "GlobalRegistry.h"
#include "SymbolTable.h"
#include "../Parser/AST.h"

namespace nand2tetris::jack{

	class SemanticAnalyser {
		public:
			explicit SemanticAnalyser(const GlobalRegistry& registry);
			void analyseClass(const ClassNode& class_node);
		private:
			const GlobalRegistry& registry;

			// State
			std::string_view currentClassName;
			std::string_view currentSubroutineName;
			std::string_view currentSubroutineKind; // "function", "method", "constructor"

			void analyseSubroutine(const SubroutineDecNode& sub, const SymbolTable& masterTable);

			void analyzeStatements(const std::vector<std::unique_ptr<StatementNode>>& stmts, SymbolTable& table);
			void analyzeLet(const LetStatementNode& node, SymbolTable& table);
			void analyzeIf(const IfStatementNode& node, SymbolTable& table);
			void analyzeWhile(const WhileStatementNode& node, SymbolTable& table);
			void analyzeDo(const DoStatementNode& node, SymbolTable& table);
			void analyzeReturn(const ReturnStatementNode& node, SymbolTable& table);

			std::string_view analyzeExpression(const ExpressionNode& node, SymbolTable& table);
			std::string_view analyzeTerm(const ExpressionNode& node, SymbolTable& table);

			std::string_view analyzeSubroutineCall(std::string_view name1,
										   std::string_view name2,
										   const std::vector<std::unique_ptr<ExpressionNode>>& args,
										   SymbolTable& table, int line);
			void checkTypeMatch(std::string_view expected, std::string_view actual, int line);
	};
}


#endif //NAND2TETRIS_SEMANTIC_ANALYSER_H