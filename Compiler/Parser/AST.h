//
// Created by Nithin Kondabathini on 13/1/2026.
//

#ifndef NAND2TETRIS_AST_H
#define NAND2TETRIS_AST_H

#include <string>
#include <utility>
#include <vector>
#include <memory>
#include<iostream>
#include "../Tokenizer/TokenTypes.h"

namespace nand2tetris::jack {

    /**
     * @brief Enumeration of all possible AST node types.
     *
     * Used for runtime type identification of AST nodes.
     */
    enum class ASTNodeType {
        // --- High-Level Structure ---
        CLASS,              ///< ClassNode
        CLASS_VAR_DEC,      ///< ClassVarDecNode (static/field)
        SUBROUTINE_DEC,     ///< SubroutineDecNode (function/method/constructor)
        VAR_DEC,            ///< VarDecNode (local vars)

        // --- Statements ---
        LET_STATEMENT,      ///< LetStatementNode
        IF_STATEMENT,       ///< IfStatementNode
        WHILE_STATEMENT,    ///< WhileStatementNode
        DO_STATEMENT,       ///< DoStatementNode
        RETURN_STATEMENT,   ///< ReturnStatementNode

        // --- Expressions (Terms) ---
        INTEGER_LITERAL,    ///< IntegerLiteralNode
        FLOAT_LITERAL,      ///< FloatLiteralNode
        STRING_LITERAL,     ///< StringLiteralNode
        KEYWORD_LITERAL,    ///< KeywordLiteralNode
        BINARY_OP,          ///< BinaryOpNode
        UNARY_OP,           ///< UnaryOpNode
        SUBROUTINE_CALL,    ///< CallNode
        IDENTIFIER          ///< IdentifierNode
    };

    /**
     * @brief Base class for all nodes in the Abstract Syntax Tree (AST).
     *
     * All specific AST nodes inherit from this class. It provides a virtual destructor
     * to ensure proper cleanup of derived classes. It also stores the location (line, column)
     * of the node in the source code for error reporting.
     */
    class Node {
        public:
            /**
             * @brief Constructs a Node.
             *
             * @param nodeType The specific type of this AST node.
             * @param l The line number in the source code.
             * @param c The column number in the source code.
             */
            explicit Node(const ASTNodeType nodeType, const int l, const int c):nodeType(nodeType),line(l),column(c){};
            virtual ~Node() = default;

            /**
             * @brief Prints the XML representation of the AST node.
             *
             * @param out The output stream.
             * @param indent The indentation level.
             */
            virtual void printXml(std::ostream& out, int indent) const=0;

            /**
             * @brief Gets the type of the AST node.
             * @return The ASTNodeType.
             */
            ASTNodeType getType() const {return nodeType;};

            /**
             * @brief Gets the line number of the node.
             * @return The line number.
             */
            int getLine() const { return line; }

            /**
             * @brief Gets the column number of the node.
             * @return The column number.
             */
            int getCol() const { return column; }
        protected:
            const int line;   ///< Line number in source.
            const int column; ///< Column number in source.
            ASTNodeType nodeType; ///< The type of the node.
            friend class SemanticAnalyser;
            friend class CodeGenerator;
    };

    /**
     * @brief Enumeration for the kind of class variable.
     */
    enum class ClassVarKind {
        STATIC, ///< A static variable, shared by all instances of the class.
        FIELD   ///< A field variable, unique to each instance of the class.
    };

    /**
     * @brief Represents a class variable declaration (static or field).
     *
     * Example: `static int x, y;` or `field boolean isActive;`
     */
    class ClassVarDecNode final : public Node {
        protected:
            ClassVarKind kind; ///< The kind of variable (static or field).
            std::string_view type; ///< The data type of the variable(s) (e.g., "int", "boolean", "MyClass").
            std::vector<std::string_view> varNames; ///< A list of variable names declared in this statement.
            friend class SemanticAnalyser;
            friend class CodeGenerator;
        public:
            /**
             * @brief Constructs a ClassVarDecNode.
             *
             * @param k The kind of variable.
             * @param t The type of the variable.
             * @param names A vector of variable names.
             * @param l the line on source code.
             * @param c the column on source code.
             */
            ClassVarDecNode(const ClassVarKind k, const std::string_view t, std::vector<std::string_view> names, const int
                l, const int c)
                :Node(ASTNodeType::CLASS_VAR_DEC,l,c),kind(k),type(t), varNames(std::move(names)) {};
            ~ClassVarDecNode() override = default;

            void printXml(std::ostream& out, const int indent) const override {
                const std::string sp(indent, ' ');
                out << sp << "<classVarDec>\n";

                out << sp << "  <keyword> " << (kind == ClassVarKind::STATIC ? "static" : "field") << " </keyword>\n";

                if (type == "int" || type == "char" || type == "boolean"||type=="float") {
                    out << sp << "  <keyword> " << type << " </keyword>\n";
                } else {
                    out << sp << "  <identifier> " << type << " </identifier>\n";
                }


                for (size_t i = 0; i < varNames.size(); ++i) {
                    out << sp << "  <identifier> " << varNames[i] << " </identifier>\n";
                    if (i < varNames.size() - 1) out << sp << "  <symbol> , </symbol>\n";
                }
                out << sp << "  <symbol> ; </symbol>\n";
                out << sp << "</classVarDec>\n";
            }
    };

    /**
     * @brief Represents a local variable declaration within a subroutine.
     *
     * Example: `var int i, sum;`
     */
    class VarDecNode final : public Node {
        protected:
            std::string_view type; ///< The data type of the variable(s).
            std::vector<std::string_view> varNames; ///< A list of variable names declared.
            friend class SemanticAnalyser;
            friend class CodeGenerator;
        public:
            /**
             * @brief Constructs a VarDecNode.
             *
             * @param t The type of the variable.
             * @param names A vector of variable names.
             * @param l The line number.
             * @param c The column number.
             */
            VarDecNode(const std::string_view t, std::vector<std::string_view> names, const int l, const int c)
                : Node(ASTNodeType::VAR_DEC,l,c),type(t), varNames(std::move(names)) {};
            ~VarDecNode() override = default;
            void printXml(std::ostream& out, const int indent) const override {
                const std::string sp(indent, ' ');

                // In the Jack analyzer standard, local variable declarations
                // are wrapped in <varDec> tags.
                out << sp << "<varDec>\n";

                // The 'var' keyword
                out << sp << "  <keyword> var </keyword>\n";

                // The type (int, char, boolean, or className)
                // We treat the type as a keyword if it's a primitive, or an identifier if it's a class.
                if (type == "int" || type == "char" || type == "boolean"||type=="float") {
                    out << sp << "  <keyword> " << type << " </keyword>\n";
                } else {
                    out << sp << "  <identifier> " << type << " </identifier>\n";
                }

                // List of variable names separated by commas
                for (size_t i = 0; i < varNames.size(); ++i) {
                    out << sp << "  <identifier> " << varNames[i] << " </identifier>\n";

                    // Output a comma symbol if there are more names in the list
                    if (i < varNames.size() - 1) {
                        out << sp << "  <symbol> , </symbol>\n";
                    }
                }

                // The mandatory closing semicolon
                out << sp << "  <symbol> ; </symbol>\n";

                out << sp << "</varDec>\n";
            }
    };

    /**
     * @brief Enumeration for the type of subroutine.
     */
    enum class SubroutineType {
        CONSTRUCTOR, ///< A class constructor (creates a new instance).
        FUNCTION,    ///< A static function (belongs to the class).
        METHOD       ///< A method (belongs to an instance).
    };

    /**
     * @brief Represents a single parameter in a subroutine declaration.
     *
     * Example: `int x` in `function void foo(int x)`
     */
    struct Parameter {
        std::string_view type; ///< The data type of the parameter.
        std::string_view name; ///< The name of the parameter.
    };

    /**
     * @brief Base class for all statement nodes.
     *
     * Statements perform actions (e.g., let, if, while, do, return).
     */
    class StatementNode : public Node {
        protected:
            friend class SemanticAnalyser;
            friend class CodeGenerator;
        public:
            explicit StatementNode(const ASTNodeType nodeType,const int l, const int c):Node(nodeType,l,c){};
            ~StatementNode() override = default;
    };

    /**
     * @brief Base class for all expression nodes.
     *
     * Expressions evaluate to a value.
     */
    class ExpressionNode : public Node {
        protected:
            friend class SemanticAnalyser;
            friend class CodeGenerator;
        public:
            explicit ExpressionNode(const ASTNodeType nodeType,const int l, const int c):Node(nodeType,l,c){};
            ~ExpressionNode() override = default;
    };

    /**
     * @brief Represents an integer literal expression.
     *
     * Example: `42`
     */
    class IntegerLiteralNode final : public ExpressionNode {
        protected:
            int32_t value; ///< The integer value.
            friend class SemanticAnalyser;
            friend class CodeGenerator;
        public:
            /**
             * @brief Constructs an IntegerLiteralNode.
             * @param val The integer value.
             * @param l The line number.
             * @param c The column number.
             */
            explicit IntegerLiteralNode(const int32_t val,const int l, const int c)
                : ExpressionNode (ASTNodeType::INTEGER_LITERAL,l,c),value(val) {}
            ~IntegerLiteralNode() override = default;
            void printXml(std::ostream& out, const int indent) const override {
                const std::string sp(indent, ' ');
                out << sp << "<term>\n";  // Add Wrapper
                out << sp << "  <integerConstant> " << value << " </integerConstant>\n";
                out << sp << "</term>\n"; // Add Wrapper
            }
    };

    /**
     * @brief Represents an float literal expression.
     *
     * Example: `42.3`
     */
    class FloatLiteralNode final : public ExpressionNode {
        protected:
            double value;
            friend class SemanticAnalyser;
            friend class CodeGenerator;
        public:
            /**
             * @brief Constructs an FloatLiteralNode.
             * @param val The integer value.
             * @param l The line number.
             * @param c The column number.
             */
            explicit FloatLiteralNode(const double val, const int l, const int c)
                :ExpressionNode (ASTNodeType::FLOAT_LITERAL,l,c),value(val) {}

            ~FloatLiteralNode() override = default;

            void printXml(std::ostream& out, const int indent) const override {
                const std::string sp(indent, ' ');
                out << sp << "<term>\n";  // Add Wrapper
                out << sp << "  <FloatConstant> " << value << " </FloatConstant>\n";
                out << sp << "</term>\n"; // Add Wrapper
            }
            double getFloat() const{return value;}
    };



    /**
     * @brief Represents a string literal expression.
     *
     * Example: `"Hello World"`
     */
    class StringLiteralNode final : public ExpressionNode {
        protected:
            std::string_view value; ///< The string value (without quotes).
            friend class SemanticAnalyser;
            friend class CodeGenerator;
        public:
            /**
             * @brief Constructs a StringLiteralNode.
             * @param val The string content.
             * @param l The line number.
             * @param c The column number.
             */
            explicit StringLiteralNode(const std::string_view val,const int l, const int c) : ExpressionNode(ASTNodeType::STRING_LITERAL,l,c),value(val) {}
            ~StringLiteralNode() override = default;
            void printXml(std::ostream& out, const int indent) const override {
                const std::string sp(indent, ' ');
                out << sp << "<term>\n";  // Add Wrapper
                out << sp << "<stringConstant> " << value << " </stringConstant>\n";
                out << sp << "</term>\n"; // Add Wrapper
            }
    };

    /**
     * @brief Represents a keyword literal expression.
     *
     * Example: `true`, `false`, `null`, `this`
     */
    class KeywordLiteralNode final : public ExpressionNode {
        protected:
            Keyword value; ///< The keyword value.
            friend class SemanticAnalyser;
            friend class CodeGenerator;
        public:
            /**
             * @brief Constructs a KeywordLiteralNode.
             * @param val The keyword.
             * @param l The line number.
             * @param c The column number.
             */
            explicit KeywordLiteralNode(const Keyword val, const int l, const int c) :ExpressionNode(ASTNodeType::KEYWORD_LITERAL,l,c),value(val) {}
            void printXml(std::ostream& out, const int indent) const override {
                std::string val;
                switch(value) {
                    case Keyword::TRUE_:  val = "true";  break;
                    case Keyword::FALSE_: val = "false"; break;
                    case Keyword::NULL_:  val = "null";  break;
                    case Keyword::THIS_:  val = "this";  break;
                    default: val="no"; break;
                }

                const std::string sp(indent, ' ');
                out << sp << "<term>\n";  // Add Wrapper
                out << sp << "<keyword> " << val << " </keyword>\n";
                out << sp << "</term>\n"; // Add Wrapper
            }
    };

    /**
     * @brief Helper function to escape XML special characters.
     * @param op The character to escape.
     * @return The escaped string.
     */
    inline std::string escapeXml(const char op) {
        switch (op) {
            case '<': return "&lt;";
            case '>': return "&gt;";
            case '&': return "&amp;";
            case '"': return "&quot;";
            default:  return std::string(1,op);
        }
    }

    /**
     * @brief Represents a binary operation expression.
     *
     * Example: `x + y`, `a < b`
     */
    class BinaryOpNode final : public ExpressionNode {
        protected:
            std::unique_ptr<ExpressionNode> left; ///< The left operand.
            char op; ///< The operator symbol ('+', '-', '*', '/', '&', '|', '<', '>', '=').
            std::unique_ptr<ExpressionNode> right; ///< The right operand.
            friend class SemanticAnalyser;
            friend class CodeGenerator;
        public:
            /**
             * @brief Constructs a BinaryOpNode.
             * @param l The left operand.
             * @param o The operator character.
             * @param r The right operand.
             * @param line The line number.
             * @param column The column number.
             */
            BinaryOpNode(std::unique_ptr<ExpressionNode> l, const char o, std::unique_ptr<ExpressionNode> r,const int
                line, const int column)
                : ExpressionNode(ASTNodeType::BINARY_OP,line, column),left(std::move(l)), op(o), right(std::move(r)) {}
            ~BinaryOpNode() override = default;
            void printXml(std::ostream& out, const int indent) const override {
                const std::string sp(indent, ' ');
                // Left Term
                left->printXml(out, indent + 4);

                // Operator - ESCAPED HERE
                out << sp << "  <symbol> " << escapeXml(op) << " </symbol>\n";

                // Right Term
                right->printXml(out, indent + 4);
            }
    };



    /**
     * @brief Represents a unary operation expression.
     *
     * Example: `-x`, `~found`
     */
    class UnaryOpNode final : public ExpressionNode {
        protected:
            char op; ///< The operator symbol ('-', '~').
            std::unique_ptr<ExpressionNode> term; ///< The operand.
            friend class SemanticAnalyser;
            friend class CodeGenerator;
        public:
            /**
             * @brief Constructs a UnaryOpNode.
             * @param o The operator character.
             * @param t The operand.
             * @param line The line number.
             * @param column The column number.
             */
            UnaryOpNode(const char o, std::unique_ptr<ExpressionNode> t,const int line, const int column)
                : ExpressionNode(ASTNodeType::UNARY_OP,line, column),op(o), term(std::move(t)) {}
            ~UnaryOpNode() override = default;
            void printXml(std::ostream& out, const int indent) const override {
                const std::string sp(indent, ' ');
                out << sp << "<term>\n";
                out << sp << "  <symbol> " << op << " </symbol>\n";
                term->printXml(out, indent + 2);
                out << sp << "</term>\n";
            }
    };



    /**
     * @brief Represents a subroutine call expression.
     *
     * Example: `foo()`, `Math.sqrt(x)`, `obj.method()`
     */
    class CallNode final : public ExpressionNode {
        protected:
            std::string_view classNameOrVar; ///< The class name or variable name (optional). Empty if implicit `this`.
            std::string_view functionName;   ///< The name of the subroutine being called.
            std::vector<std::unique_ptr<ExpressionNode>> arguments; ///< The list of arguments passed to the call.
            friend class SemanticAnalyser;
            friend class CodeGenerator;
        public:
            /**
             * @brief Constructs a CallNode.
             * @param cv The class or variable name (can be empty).
             * @param fn The function name.
             * @param args The arguments.
             * @param l The line number.
             * @param c The column number.
             */
            CallNode(const std::string_view cv, const std::string_view fn,
                std::vector<std::unique_ptr<ExpressionNode>> args,const int l,const int c)
                : ExpressionNode(ASTNodeType::SUBROUTINE_CALL,l,c),classNameOrVar(cv), functionName(fn), arguments(std::move(args)) {}
            ~CallNode() override = default;
            void printXml(std::ostream& out, const int indent) const override {
                const std::string sp(indent, ' ');
                out << sp << "<term>\n";
                printRaw(out, indent + 2); // Call the helper
                out << sp << "</term>\n";
            }

            void printRaw(std::ostream& out, const int indent) const {
                const std::string sp(indent, ' ');
                if (!classNameOrVar.empty()) {
                    out << sp << "<identifier> " << classNameOrVar << " </identifier>\n";
                    out << sp << "<symbol> . </symbol>\n";
                }
                out << sp << "<identifier> " << functionName << " </identifier>\n";
                out << sp << "<symbol> ( </symbol>\n";
                out << sp << "<expressionList>\n";
                for (size_t i = 0; i < arguments.size(); ++i) {
                    out << sp << "  <expression>\n";
                    arguments[i]->printXml(out, indent + 4);
                    out << sp << "  </expression>\n";
                    if (i < arguments.size() - 1) {
                        out << sp << "  <symbol> , </symbol>\n";
                    }
                }
                out << sp << "</expressionList>\n";
                out << sp << "<symbol> ) </symbol>\n";
            }
    };

    /**
     * @brief Represents an identifier expression, possibly with an array index.
     *
     * Example: `x`, `arr[i]`
     */
    class IdentifierNode final : public ExpressionNode {
        protected:
            std::string_view name; ///< The name of the identifier.
            std::unique_ptr<ExpressionNode> indexExpr; ///< The index expression if it's an array access, otherwise nullptr.
            friend class SemanticAnalyser;
            friend class CodeGenerator;
        public:
            /**
             * @brief Constructs an IdentifierNode.
             * @param n The identifier name.
             * @param l The line number.
             * @param c The column number.
             * @param idx The index expression (optional).
             */
        explicit IdentifierNode(const std::string_view n,const int l, const int c,std::unique_ptr<ExpressionNode> idx = nullptr)
                : ExpressionNode(ASTNodeType::IDENTIFIER,l,c) ,name(n), indexExpr(std::move(idx)) {}
            ~IdentifierNode() override = default;
            void printXml(std::ostream& out, const int indent) const override {

                const std::string sp(indent, ' ');
                out << sp << "<term>\n";  // Add Wrapper
                out << sp << "  <identifier> " << name << " </identifier>\n";
                if (indexExpr) {
                    out << sp << "  <symbol> [ </symbol>\n";
                    out << sp << "  <expression>\n";
                    indexExpr->printXml(out, indent + 4);
                    out << sp << "  </expression>\n";
                    out << sp << "  <symbol> ] </symbol>\n";
                }
                out << sp << "</term>\n"; // Add Wrapper
            }
    };

    /**
     * @brief Represents a 'let' statement (assignment).
     *
     * Example: `let x = 5;`, `let arr[i] = y;`
     */
    class LetStatementNode final : public StatementNode {
        protected:
            std::string_view varName; ///< The name of the variable being assigned to.
            std::unique_ptr<ExpressionNode> indexExpr; ///< The index expression for array assignment (optional).
            std::unique_ptr<ExpressionNode> valueExpr; ///< The expression evaluating to the new value.
            friend class SemanticAnalyser;
            friend class CodeGenerator;
        public:
            /**
             * @brief Constructs a LetStatementNode.
             * @param name The variable name.
             * @param idx The index expression (can be nullptr).
             * @param val The value expression.
             * @param l The line number.
             * @param c The column number.
             */
            LetStatementNode(const std::string_view name, std::unique_ptr<ExpressionNode> idx,
                             std::unique_ptr<ExpressionNode> val,const int l, const int c)
                : StatementNode(ASTNodeType::LET_STATEMENT,l,c) ,varName(name), indexExpr(std::move(idx)), valueExpr(std::move(val)) {}
            ~LetStatementNode() override = default;
            void printXml(std::ostream& out, const int indent) const override {
                const std::string sp(indent, ' ');
                out << sp << "<letStatement>\n";
                out << sp << "  <keyword> let </keyword>\n";
                out << sp << "  <identifier> " << varName << " </identifier>\n";

                if (indexExpr) {
                    out << sp << "  <symbol> [ </symbol>\n";
                    out << sp << "  <expression>\n";
                    indexExpr->printXml(out, indent + 4);
                    out << sp << "  </expression>\n";
                    out << sp << "  <symbol> ] </symbol>\n";
                }

                out << sp << "  <symbol> = </symbol>\n";
                out << sp << "  <expression>\n";
                valueExpr->printXml(out, indent + 4);
                out << sp << "  </expression>\n";
                out << sp << "  <symbol> ; </symbol>\n";
                out << sp << "</letStatement>\n";
            }
    };

    /**
     * @brief Represents an 'if' statement.
     *
     * Example: `if (x > 0) { ... } else { ... }`
     */
    class IfStatementNode final : public StatementNode {
        protected:
            std::unique_ptr<ExpressionNode> condition; ///< The condition expression.
            std::vector<std::unique_ptr<StatementNode>> ifStatements; ///< The statements to execute if true.
            std::vector<std::unique_ptr<StatementNode>> elseStatements; ///< The statements to execute if false (optional).
            friend class SemanticAnalyser;
            friend class CodeGenerator;
        public:
            /**
             * @brief Constructs an IfStatementNode.
             * @param cond The condition.
             * @param ifStmts The 'if' block statements.
             * @param elseStmts The 'else' block statements.
             * @param l The line number.
             * @param c The column number.
             */
            IfStatementNode(std::unique_ptr<ExpressionNode> cond, std::vector<std::unique_ptr<StatementNode>> ifStmts,
                            std::vector<std::unique_ptr<StatementNode>> elseStmts,const int l, const int c)
                : StatementNode(ASTNodeType::IF_STATEMENT,l,c) ,condition(std::move(cond)), ifStatements(std::move(ifStmts)),
                elseStatements(std::move(elseStmts)){};
            ~IfStatementNode() override = default;
            void printXml(std::ostream& out, const int indent) const override {
                const std::string sp(indent, ' ');
                out << sp << "<ifStatement>\n";
                out << sp << "  <keyword> if </keyword>\n";
                out << sp << "  <symbol> ( </symbol>\n";
                out << sp << "  <expression>\n";
                condition->printXml(out, indent + 2);
                out << sp << "  </expression>\n";
                out << sp << "  <symbol> ) </symbol>\n";

                out << sp << "  <symbol> { </symbol>\n";
                out << sp << "  <statements>\n";
                for (const auto& stmt : ifStatements) stmt->printXml(out, indent + 4);
                out << sp << "  </statements>\n";
                out << sp << "  <symbol> } </symbol>\n";

                if (!elseStatements.empty()) {
                    out << sp << "  <keyword> else </keyword>\n";
                    out << sp << "  <symbol> { </symbol>\n";
                    out << sp << "  <statements>\n";
                    for (const auto& stmt : elseStatements) stmt->printXml(out, indent + 4);
                    out << sp << "  </statements>\n";
                    out << sp << "  <symbol> } </symbol>\n";
                }
                out << sp << "</ifStatement>\n";
            }
    };

    /**
     * @brief Represents a 'while' statement.
     *
     * Example: `while (x > 0) { ... }`
     */
    class WhileStatementNode final : public StatementNode {
        protected:
            std::unique_ptr<ExpressionNode> condition; ///< The loop condition.
            std::vector<std::unique_ptr<StatementNode>> body; ///< The loop body statements.
            friend class SemanticAnalyser;
            friend class CodeGenerator;
        public:
            /**
             * @brief Constructs a WhileStatementNode.
             * @param cond The condition.
             * @param b The body statements.
             * @param l The line number.
             * @param c The column number.
             */
            WhileStatementNode(std::unique_ptr<ExpressionNode> cond, std::vector<std::unique_ptr<StatementNode>> b,
                const int l,const int c)
                : StatementNode(ASTNodeType::WHILE_STATEMENT,l,c),condition(std::move(cond)), body(std::move(b)) {}
            ~WhileStatementNode() override = default;
            void printXml(std::ostream& out, const int indent) const override {
                const std::string sp(indent, ' ');
                out << sp << "<whileStatement>\n";
                out << sp << "  <keyword> while </keyword>\n";
                out << sp << "  <symbol> ( </symbol>\n";
                out << sp << "  <expression>\n";
                condition->printXml(out, indent + 2);
                out << sp << "  </expression>\n";
                out << sp << "  <symbol> ) </symbol>\n";

                out << sp << "  <symbol> { </symbol>\n";
                out << sp << "  <statements>\n";
                for (const auto& stmt : body) stmt->printXml(out, indent + 4);
                out << sp << "  </statements>\n";
                out << sp << "  <symbol> } </symbol>\n";
                out << sp << "</whileStatement>\n";
            }
    };

    /**
     * @brief Represents a 'do' statement.
     *
     * Example: `do Output.printInt(x);`
     * 'do' statements are used to call subroutines for their side effects, ignoring the return value.
     */
    class DoStatementNode final : public StatementNode {
        protected:
            std::unique_ptr<CallNode> callExpression; ///< The subroutine call expression.
            friend class SemanticAnalyser;
            friend class CodeGenerator;
        public:
            /**
             * @brief Constructs a DoStatementNode.
             * @param call The call expression.
             * @param l The line number.
             * @param c The column number.
             */
            explicit DoStatementNode(std::unique_ptr<CallNode> call,const int l, const int c) : StatementNode(ASTNodeType::DO_STATEMENT,l,c),
                callExpression(std::move(call)){};
            ~DoStatementNode() override = default;
            void printXml(std::ostream& out, const int indent) const override {
                const std::string sp(indent, ' ');
                out << sp << "<doStatement>\n";
                out << sp << "  <keyword> do </keyword>\n";

                // Calls the print method of the CallNode
                callExpression->printRaw(out, indent + 2);

                out << sp << "  <symbol> ; </symbol>\n";
                out << sp << "</doStatement>\n";
            }
    };

    /**
     * @brief Represents a 'return' statement.
     *
     * Example: `return;`, `return x + 1;`
     */
    class ReturnStatementNode final : public StatementNode {
        protected:
            std::unique_ptr<ExpressionNode> expression; ///< The return value expression (optional).
            friend class SemanticAnalyser;
            friend class CodeGenerator;

        public:
            /**
             * @brief Constructs a ReturnStatementNode.
             * @param expr The return expression (can be nullptr).
             * @param l The line number.
             * @param c The column number.
             */
            explicit ReturnStatementNode(std::unique_ptr<ExpressionNode> expr,const int l,const int c) : StatementNode
                (ASTNodeType::RETURN_STATEMENT,l,c), expression(std::move(expr)) {}
            ~ReturnStatementNode() override = default;
            void printXml(std::ostream& out, const int indent) const override {
                const std::string sp(indent, ' ');
                out << sp << "<returnStatement>\n";
                out << sp << "  <keyword> return </keyword>\n";

                if (expression) {
                    out << sp << "  <expression>\n";
                    expression->printXml(out, indent + 2);
                    out << sp << "  </expression>\n";
                }

                out << sp << "  <symbol> ; </symbol>\n";
                out << sp << "</returnStatement>\n";
            }
    };

    /**
     * @brief Represents a subroutine declaration (constructor, function, or method).
     */
    class SubroutineDecNode final : public Node {
        protected:
            SubroutineType subType; ///< The type of subroutine (constructor, function, method).
            std::string_view returnType; ///< The return type (e.g., "void", "int", "MyClass").
            std::string_view name; ///< The name of the subroutine.
            std::vector<Parameter> parameters; ///< The list of parameters.

            std::vector<std::unique_ptr<VarDecNode>> localVars; ///< The local variable declarations.
            std::vector<std::unique_ptr<StatementNode>> statements; ///< The body statements.
            friend class SemanticAnalyser;
            friend class CodeGenerator;

        public:
            /**
             * @brief Constructs a SubroutineDecNode.
             * @param st The subroutine type.
             * @param ret The return type.
             * @param n The name.
             * @param parameters The list of parameters.
             * @param vars The local variables.
             * @param stmts The body statements.
             * @param l The line number.
             * @param c The column number.
             */
            SubroutineDecNode(const SubroutineType st, const std::string_view ret, const std::string_view n,
                std::vector<Parameter> parameters, std::vector<std::unique_ptr<VarDecNode>> vars,
                std::vector<std::unique_ptr<StatementNode>> stmts,const int l, const int c)
                : Node(ASTNodeType::SUBROUTINE_DEC,l,c),subType(st), returnType(ret), name(n),parameters(std::move
                    (parameters)),localVars(std::move
                    (vars)),statements(std::move(stmts)) {};

            ~SubroutineDecNode() override = default;

            void printXml(std::ostream& out, const int indent) const override {
                const std::string sp(indent, ' ');
                out << sp << "<subroutineDec>\n";
                const std::string typeStr = (subType == SubroutineType::CONSTRUCTOR ? "constructor" :
                                      (subType == SubroutineType::FUNCTION ? "function" : "method"));
                out << sp << "  <keyword> " << typeStr << " </keyword>\n";
                if (returnType == "int" || returnType == "void" || returnType == "boolean" || returnType ==
                    "char"||returnType=="float") {
                    out << sp << "  <keyword> " << returnType << " </keyword>\n";
                } else {
                    out << sp << "  <identifier> " << returnType << " </identifier>\n";
                }

                out << sp << "  <identifier> " << name << " </identifier>\n";
                out << sp << "  <symbol> ( </symbol>\n";
                out << sp << "  <parameterList>\n";
                for (size_t i = 0; i < parameters.size(); ++i) {
                    if (parameters[i].type=="int"||parameters[i].type=="boolean"||parameters[i].type=="char") {
                        out<< sp << "  <keyword>"<<parameters[i].type<<" </keyword>\n";
                    }else {
                        out << sp << "  <identifier> " << parameters[i].type << " </identifier>\n";
                    }
                    out << sp << "    <identifier> " << parameters[i].name << " </identifier>\n";
                    if (i < parameters.size() - 1) out << sp << "    <symbol> , </symbol>\n";
                }
                out << sp << "  </parameterList>\n";
                out << sp << "  <symbol> ) </symbol>\n";
                out << sp << "  <subroutineBody>\n";
                out << sp << "    <symbol> { </symbol>\n";
                for (const auto& var : localVars) var->printXml(out, indent + 4);
                out << sp << "    <statements>\n";
                for (const auto& stmt : statements) stmt->printXml(out, indent + 6);
                out << sp << "    </statements>\n";
                out << sp << "    <symbol> } </symbol>\n";
                out << sp << "  </subroutineBody>\n";
                out << sp << "</subroutineDec>\n";
            }
    };

    /**
     * @brief Represents a complete Jack class.
     *
     * This is the root node of the AST for a single file.
     */
    class ClassNode final : public Node {
        protected:
            std::string_view className; ///< The name of the class.
            std::vector<std::unique_ptr<ClassVarDecNode>> classVars; ///< The class-level variable declarations.
            std::vector<std::unique_ptr<SubroutineDecNode>> subroutineDecs; ///< The subroutine declarations.
            friend class SemanticAnalyser;
            friend class CodeGenerator;
        public:
            /**
             * @brief Constructs a ClassNode.
             * @param className The name of the class.
             * @param classVars The list of class variables (Static and Field)
             * @param subroutineDecs The list of subroutine declarations (constructors, method, function)
             * @param l The line number.
             * @param c The column number.
             */
            explicit ClassNode(const std::string_view className,std::vector<std::unique_ptr<ClassVarDecNode>>
                classVars,std::vector<std::unique_ptr<SubroutineDecNode>> subroutineDecs,const int l, const int c) :
                Node(ASTNodeType::CLASS,l,c),className(className),
                classVars(std::move(classVars)), subroutineDecs(std::move(subroutineDecs)) {};
            ~ClassNode() override = default;
            void printXml(std::ostream& out, int indent)const override {
                out << "<class>\n";
                out << "  <keyword> class </keyword>\n";
                out << "  <identifier> " << className << " </identifier>\n";
                out << "  <symbol> { </symbol>\n";
                for (const auto& var : classVars) var->printXml(out, 2);
                for (const auto& sub : subroutineDecs) sub->printXml(out, 2);
                out << "  <symbol> } </symbol>\n";
                out << "</class>\n";
            }
            std::string_view getClassName()const { return className; }
            std::size_t get_Number_of_Subroutines()const {return subroutineDecs.size();}
            std::size_t get_Number_of_classVars() const {return classVars.size();}


    };
}

#endif //NAND2TETRIS_AST_H