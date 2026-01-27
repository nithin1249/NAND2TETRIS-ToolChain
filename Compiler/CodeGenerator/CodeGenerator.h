//
// Created by Nithin Kondabathini on 26/1/2026.
//

#ifndef NAND2TETRIS_CODE_GENERATOR_H
#define NAND2TETRIS_CODE_GENERATOR_H
#include "../Parser/AST.h"
#include "../SemanticAnalyser/GlobalRegistry.h"
#include "../SemanticAnalyser/SymbolTable.h"
#include "../VMWriter/VMWriter.h"

namespace nand2tetris::jack {

    /**
     * @brief Generates VM code from the Abstract Syntax Tree (AST).
     *
     * This class traverses the AST and emits corresponding VM commands using the VMWriter.
     * It manages symbol tables for variable resolution and handles control flow logic.
     */
    class CodeGenerator {
        public:
            /**
             * @brief Constructs a CodeGenerator.
             *
             * @param registry The global registry containing class and method signatures.
             * @param out The output stream to write VM code to.
             */
            CodeGenerator(const GlobalRegistry& registry, std::ostream& out);

            /**
             * @brief Compiles a class node into VM code.
             *
             * @param node The root node of the class AST.
             */
            void compileClass(const ClassNode& node);
        private:
            const GlobalRegistry& registry; ///< Reference to the global registry.
            VMWriter writer;                ///< Helper to write VM commands.
            SymbolTable symbolTable;        ///< Symbol table for variable resolution.
            std::string_view currentClassName; ///< Name of the class currently being compiled.
            int labelCounter = 0;           ///< Counter for generating unique labels.

            /**
             * @brief Generates a unique label string.
             * @return A unique label (e.g., "L1", "L2").
             */
            std::string getUniqueLabel();

            /**
             * @brief Compiles a subroutine declaration.
             *
             * Sets up the symbol table, writes the function declaration, handles constructor/method setup,
             * and compiles the body statements.
             *
             * @param sub The subroutine declaration node.
             */
            void compileSubroutine(const SubroutineDecNode& sub);

            /**
             * @brief Compiles a list of statements.
             *
             * @param stmts The vector of statement nodes.
             */
            void compileStatements(const std::vector<std::unique_ptr<StatementNode>>& stmts);

            /**
             * @brief Compiles a 'do' statement.
             *
             * Compiles the subroutine call and pops the return value (temp 0).
             *
             * @param node The do statement node.
             */
            void compileDo(const DoStatementNode& node);

            /**
             * @brief Compiles a 'let' statement.
             *
             * Handles variable assignment and array indexing logic.
             *
             * @param node The let statement node.
             */
            void compileLet(const LetStatementNode& node);

            /**
             * @brief Compiles a 'while' statement.
             *
             * Generates loop labels, condition evaluation, and jump commands.
             *
             * @param node The while statement node.
             */
            void compileWhile(const WhileStatementNode& node);

            /**
             * @brief Compiles a 'return' statement.
             *
             * Pushes the return value (or 0 for void) and writes the return command.
             *
             * @param node The return statement node.
             */
            void compileReturn(const ReturnStatementNode& node);

            /**
             * @brief Compiles an 'if' statement.
             *
             * Generates labels for else/end blocks, condition evaluation, and jump commands.
             *
             * @param node The if statement node.
             */
            void compileIf(const IfStatementNode& node);

            /**
             * @brief Compiles an expression.
             *
             * Handles binary operations and delegates to compileTerm.
             *
             * @param node The expression node.
             */
            void compileExpression(const ExpressionNode& node);

            /**
             * @brief Compiles a term (integer, string, keyword, identifier, unary op).
             *
             * @param node The expression node representing a term.
             */
            void compileTerm(const ExpressionNode& node);

            /**
             * @brief Compiles a subroutine call.
             *
             * Handles implicit 'this', instance calls, and static calls. Pushes arguments and writes the call command.
             *
             * @param node The call node.
             */
            void compileSubroutineCall(const CallNode& node);
    };


}




#endif //NAND2TETRIS_CODE_GENERATOR_H