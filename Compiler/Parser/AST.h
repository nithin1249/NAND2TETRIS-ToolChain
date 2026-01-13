//
// Created by Nithin Kondabathini on 13/1/2026.
//

#ifndef NAND2TETRIS_AST_H
#define NAND2TETRIS_AST_H
#include <string>
#include <vector>
#include <memory>
#include "../Tokenizer/TokenTypes.h"

namespace nand2tetris::jack {
 	class Node {
 		public:
 			virtual ~Node()=default;
 	};

	enum class ClassVarKind{STATIC,FIELD};

	class ClassVarDecNode final : public Node {
		public:
			ClassVarKind kind;
			std::string_view type;
			std::vector<std::string_view> varNames;

			ClassVarDecNode(const ClassVarKind k, const std::string_view t, std::vector<std::string_view> names)
			: kind(k), type(t), varNames(std::move(names)) {};
			~ClassVarDecNode() override =default;
	};

	class VarDecNode final : public Node {
		public:
			std::string_view type;
			std::vector<std::string_view> varNames;
			VarDecNode(const std::string_view t, std::vector<std::string_view> names): type(t), varNames(std::move(names)){};
			~VarDecNode() override =default;
	};

	enum class SubroutineType { CONSTRUCTOR, FUNCTION, METHOD };

	// Parameters: list of (type, name) pairs
	struct Parameter {
		std::string_view type;
		std::string_view name;
	};

	class StatementNode : public Node {
		public:
			~StatementNode() override =default;
	};

	class ExpressionNode: public Node {
		public:
			~ExpressionNode() override =default;
	};

	//Basic constants
	class IntegerLiteralNode final: public ExpressionNode {
		public:
			int value;
			explicit IntegerLiteralNode(const int val) : value(val) {}
			~IntegerLiteralNode() override =default;
	};

	class StringLiteralNode final : public ExpressionNode {
		public:
			std::string_view value;
			explicit StringLiteralNode(const std::string_view val) : value(val) {}
			~StringLiteralNode() override =default;
	};

	class KeywordLiteralNode final : public ExpressionNode {
		public:
			Keyword value; // From your Tokenizer
			explicit KeywordLiteralNode(const Keyword val) : value(val) {}
	};

	//Binary operators and Unary Operators
	class BinaryOpNode final : public ExpressionNode {
		public:
			std::unique_ptr<ExpressionNode> left;
			char op; // '+', '-', '*', '/', '&', '|', '<', '>', '='
			std::unique_ptr<ExpressionNode> right;

			BinaryOpNode(std::unique_ptr<ExpressionNode> l, const char o, std::unique_ptr<ExpressionNode> r)
				: left(std::move(l)), op(o), right(std::move(r)) {}
			~BinaryOpNode() override =default;
	};

	class UnaryOpNode final : public ExpressionNode {
		public:
			char op; // '-', '~'
			std::unique_ptr<ExpressionNode> term;

			UnaryOpNode(const char o, std::unique_ptr<ExpressionNode> t)
				: op(o), term(std::move(t)) {}
			~UnaryOpNode() override =default;
	};

	class CallNode final : public ExpressionNode {
		public:
			std::string_view classNameOrVar; // Optional: "Math" in Math.sqrt(x)
			std::string_view functionName;   // "sqrt"
			std::vector<std::unique_ptr<ExpressionNode>> arguments;

			CallNode(const std::string_view cv, const std::string_view fn, std::vector<std::unique_ptr<ExpressionNode>> args)
				: classNameOrVar(cv), functionName(fn), arguments(std::move(args)) {}
			~CallNode() override =default;
	};

	class IdentifierNode final : public ExpressionNode {
		public:
			std::string_view name;
			std::unique_ptr<ExpressionNode> indexExpr; // nullptr unless it's an array like a[i]

			explicit IdentifierNode(const std::string_view n, std::unique_ptr<ExpressionNode> idx = nullptr)
				: name(n), indexExpr(std::move(idx)) {}
			~IdentifierNode() override =default;
	};


	class LetStatementNode final : public StatementNode {
		public:
			std::string_view varName;
			std::unique_ptr<ExpressionNode> indexExpr; // For array access like x[i]
			std::unique_ptr<ExpressionNode> valueExpr;

			LetStatementNode(const std::string_view name,std::unique_ptr<ExpressionNode> idx,
				std::unique_ptr<ExpressionNode> val) : varName(name), indexExpr(std::move(idx)), valueExpr(std::move(val)) {}
			~LetStatementNode() override =default;
	};

	class IfStatementNode final : public StatementNode {
		public:
			std::unique_ptr<ExpressionNode> condition;
			std::vector<std::unique_ptr<StatementNode>> ifStatements;
			std::vector<std::unique_ptr<StatementNode>> elseStatements;

			IfStatementNode(std::unique_ptr<ExpressionNode> cond, std::vector<std::unique_ptr<StatementNode>>
				ifStmts, std::vector<std::unique_ptr<StatementNode>> elseStmts) : condition(std::move(cond)),
				ifStatements(std::move(ifStmts)),elseStatements(std::move(elseStmts)) {};
			~IfStatementNode() override =default;
	};

	class WhileStatementNode final : public StatementNode {
		public:
			std::unique_ptr<ExpressionNode> condition;
			std::vector<std::unique_ptr<StatementNode>> body;

			WhileStatementNode(std::unique_ptr<ExpressionNode> cond, std::vector<std::unique_ptr<StatementNode>> b) :
				condition(std::move(cond)), body(std::move(b)) {}
			~WhileStatementNode() override =default;
	};

	class DoStatementNode final : public StatementNode {
		public:
			// do calls are effectively expressions used as statements
			std::unique_ptr<ExpressionNode> callExpression;

			explicit DoStatementNode(std::unique_ptr<ExpressionNode> call): callExpression(std::move(call)) {}
			~DoStatementNode() override =default;
	};

	class ReturnStatementNode final : public StatementNode {
		public:
			std::unique_ptr<ExpressionNode> expression; // Can be nullptr for 'return;'

			explicit ReturnStatementNode(std::unique_ptr<ExpressionNode> expr): expression(std::move(expr)) {}
			~ReturnStatementNode() override =default;
	};


	class SubroutineDecNode final :public Node {
		public:
			SubroutineType subType;
			std::string_view returnType;
			std::string_view name;
			std::vector<Parameter> parameters;

			std::vector<std::unique_ptr<VarDecNode>> localVars;
			std::vector<std::unique_ptr<StatementNode>> statements{};

		SubroutineDecNode(const SubroutineType st, const std::string_view ret, const std::string_view n,
			std::vector<std::unique_ptr<VarDecNode>> vars,std::vector<std::unique_ptr<StatementNode>> stmts): subType
			(st), returnType(ret), name(n),localVars(std::move(vars)), statements(std::move(stmts)) {}

		~SubroutineDecNode() override =default;
	};


 	class ClassNode final :public Node {
 		public:
 			std::string_view className;
 			std::vector<std::unique_ptr<ClassVarDecNode>> classVars;
 			std::vector<std::unique_ptr<SubroutineDecNode>> subroutineDecs;
 			explicit ClassNode(const std::string_view className):className(className){};
 			~ClassNode() override =default;
 	};
 }


#endif //NAND2TETRIS_AST_H