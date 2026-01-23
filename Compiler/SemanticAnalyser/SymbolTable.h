//
// Created by Nithin Kondabathini on 21/1/2026.
//

#ifndef NAND2TETRIS_SYMBOL_TABLE_H
#define NAND2TETRIS_SYMBOL_TABLE_H
#include "../Parser/Parser.h"
#include <unordered_map>

namespace nand2tetris::jack{

    /**
     * @brief Enumeration representing the kind of symbol.
     */
    enum class SymbolKind {
        STATIC, ///< Static variable (class-level, shared).
        FIELD,  ///< Field variable (class-level, instance-specific).
        ARG,    ///< Argument variable (subroutine-level).
        LCL,    ///< Local variable (subroutine-level).
        NONE    ///< Represents a symbol not found in the table.
    };

    /**
     * @brief Structure representing a symbol in the symbol table.
     */
    struct Symbol{
        std::string_view type; ///< The data type of the symbol (e.g., "int", "boolean", "MyClass").
        SymbolKind kind;       ///< The kind of the symbol (STATIC, FIELD, ARG, LCL).
        int index;             ///< The running index of the symbol within its kind.
        int declLine;          ///< The line number where the symbol was declared.
        int declCol;           ///< The column number where the symbol was declared.
    };

    /**
     * @brief A symbol table for the Jack compiler.
     *
     * Manages the scope and properties of variables (identifiers) during compilation.
     * It handles two scopes: class-level (static, field) and subroutine-level (argument, local).
     */
    class SymbolTable {
        public:
            /**
             * @brief Constructs a new SymbolTable.
             *
             * Initializes indices for all symbol kinds to 0.
             */
            SymbolTable();

            SymbolTable(const SymbolTable& other) = default;
            ~SymbolTable()=default;

            /**
             * @brief Starts a new subroutine scope.
             *
             * Clears the subroutine-level symbol table and resets the indices for ARG and LCL.
             * Should be called when starting to compile a new subroutine.
             */
            void startSubroutine();

            /**
             * @brief Returns the number of variables of the given kind defined in the current scope.
             *
             * @param kind The symbol kind (STATIC, FIELD, ARG, LCL).
             * @return The count of variables of that kind.
             */
            int varCount(SymbolKind kind) const;

            /**
             * @brief Returns the kind of the named identifier.
             *
             * Searches the subroutine scope first, then the class scope.
             *
             * @param name The name of the identifier.
             * @return The SymbolKind of the identifier, or SymbolKind::NONE if not found.
             */
            SymbolKind kindOf(std::string_view name) const;

            /**
             * @brief Returns the type of the named identifier.
             *
             * @param name The name of the identifier.
             * @return The type of the identifier (e.g., "int"), or an empty string if not found.
             */
            std::string_view typeOf(std::string_view name) const;

            /**
             * @brief Returns the index of the named identifier.
             *
             * @param name The name of the identifier.
             * @return The index of the identifier, or -1 if not found.
             */
            int indexOf(std::string_view name) const;

            /**
             * @brief Defines a new variable in the symbol table.
             *
             * Adds the variable to the appropriate scope (class or subroutine) based on its kind,
             * assigns it the next available index, and increments the index counter for that kind.
             *
             * @param name The name of the variable.
             * @param type The type of the variable.
             * @param kind The kind of the variable.
             * @param line The line number of the declaration (for error reporting).
             * @param col The column number of the declaration (for error reporting).
             * @throws std::runtime_error if the variable is already defined in the current scope.
             */
            void define(std::string_view name, std::string_view type, SymbolKind kind,int line, int col);

        private:
            /**
             * @brief Helper to look up a symbol by name.
             *
             * Checks subroutine scope first, then class scope.
             *
             * @param name The name to look up.
             * @return A pointer to the Symbol if found, nullptr otherwise.
             */
            const Symbol* lookup(std::string_view name) const;

            std::unordered_map<std::string_view,Symbol> classScope;      ///< Stores class-level symbols (STATIC, FIELD).
            std::unordered_map<std::string_view,Symbol> subRoutineScope; ///< Stores subroutine-level symbols (ARG, LCL).
            std::unordered_map<SymbolKind, int> indices;                 ///< Tracks the next available index for each SymbolKind.

    };
};

#endif //NAND2TETRIS_SYMBOL_TABLE_H