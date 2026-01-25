//
// Created by Nithin Kondabathini on 24/1/2026.
//

#ifndef NAND2TETRIS_SEMANTIC_ANALYSER_H
#define NAND2TETRIS_SEMANTIC_ANALYSER_H


#include "GlobalRegistry.h"
#include "SymbolTable.h"
#include "../Parser/AST.h"

namespace nand2tetris::jack{

    /**
     * @brief Performs semantic analysis on the Abstract Syntax Tree (AST).
     *
     * This class is responsible for type checking, scope resolution, and verifying
     * semantic rules of the Jack language (e.g., variable existence, method signatures,
     * return types).
     */
    class SemanticAnalyser {
        public:
            /**
             * @brief Constructs a SemanticAnalyser.
             *
             * @param registry The global registry containing class and method signatures.
             */
            explicit SemanticAnalyser(const GlobalRegistry& registry);

            /**
             * @brief Analyzes a class node and its contents.
             *
             * @param class_node The root node of the class AST.
             * @throws std::runtime_error if any semantic error is found.
             */
            void analyseClass(const ClassNode& class_node);
        private:
            const GlobalRegistry& registry; ///< Reference to the global registry.

            // State
            std::string_view currentClassName;      ///< Name of the class currently being analyzed.
            std::string_view currentSubroutineName; ///< Name of the subroutine currently being analyzed.
            std::string_view currentSubroutineKind; ///< Kind of the current subroutine ("function", "method", "constructor").

            /**
             * @brief Reports a semantic error and throws an exception.
             *
             * @param message The error message.
             * @param node The AST node where the error occurred (for location info).
             */
            [[noreturn]] void error(std::string_view message, const Node& node) const;

            /**
             * @brief Checks if two types match according to Jack's type rules.
             *
             * Handles implicit conversions (e.g., null to object, char to int).
             *
             * @param expected The expected type.
             * @param actual The actual type found.
             * @param locationNode The AST node for error reporting.
             */
            void checkTypeMatch(std::string_view expected, std::string_view actual,const Node& locationNode) const;


            /**
             * @brief Analyzes a subroutine declaration.
             *
             * Sets up the local symbol table and analyzes the subroutine body.
             *
             * @param sub The subroutine declaration node.
             * @param masterTable The class-level symbol table.
             */
            void analyseSubroutine(const SubroutineDecNode& sub, const SymbolTable& masterTable);

            /**
             * @brief Analyzes a list of statements.
             *
             * @param stmts The vector of statement nodes.
             * @param table The current symbol table.
             */
            void analyseStatements(const std::vector<std::unique_ptr<StatementNode>>& stmts, SymbolTable& table) const;

            /**
             * @brief Analyzes a 'let' statement.
             *
             * Checks variable existence, array indexing, and type compatibility.
             *
             * @param node The let statement node.
             * @param table The current symbol table.
             */
            void analyseLet(const LetStatementNode& node, SymbolTable& table)const;

            /**
             * @brief Analyzes an 'if' statement.
             *
             * Verifies the condition is boolean.
             *
             * @param node The if statement node.
             * @param table The current symbol table.
             */
            void analyseIf(const IfStatementNode& node, SymbolTable& table)const;

            /**
             * @brief Analyzes a 'while' statement.
             *
             * Verifies the condition is boolean.
             *
             * @param node The while statement node.
             * @param table The current symbol table.
             */
            void analyseWhile(const WhileStatementNode& node, SymbolTable& table)const;

            /**
             * @brief Analyzes a 'do' statement.
             *
             * Analyzes the underlying subroutine call.
             *
             * @param node The do statement node.
             * @param table The current symbol table.
             */
            void analyseDo(const DoStatementNode& node, SymbolTable& table)const;

            /**
             * @brief Analyzes a 'return' statement.
             *
             * Verifies the return value matches the subroutine's return type.
             * Checks constructor rules (must return 'this').
             *
             * @param node The return statement node.
             * @param table The current symbol table.
             */
            void analyseReturn(const ReturnStatementNode& node, SymbolTable& table) const;

            /**
             * @brief Analyzes an expression and returns its type.
             *
             * @param node The expression node.
             * @param table The current symbol table.
             * @return The type of the expression (e.g., "int", "boolean", "MyClass").
             */
            std::string_view analyseExpression(const ExpressionNode& node, SymbolTable& table)const;

            /**
             * @brief Analyzes a subroutine call.
             *
             * Resolves the target class/object, checks method existence, verifies argument count and types.
             *
             * @param classNameOrVar The class name or variable name (or empty for implicit 'this').
             * @param functionName The name of the function/method.
             * @param args The list of argument expressions.
             * @param table The current symbol table.
             * @param locationNode The AST node for error reporting.
             * @return The return type of the called subroutine.
             */
            std::string_view analyseSubroutineCall(std::string_view classNameOrVar,
                                               std::string_view functionName,
                                               const std::vector<std::unique_ptr<ExpressionNode>>& args,
                                               SymbolTable& table,
                                               const Node& locationNode)const;
    };
}


#endif //NAND2TETRIS_SEMANTIC_ANALYSER_H