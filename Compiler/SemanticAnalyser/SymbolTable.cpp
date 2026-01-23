//
// Created by Nithin Kondabathini on 21/1/2026.
//

#include "SymbolTable.h"

namespace nand2tetris::jack {

    // Helper function to convert SymbolKind enum to string for error messages.
    std::string kindToString(const SymbolKind kind) {
        switch(kind) {
            case SymbolKind::STATIC: return "static";
            case SymbolKind::FIELD:  return "field";
            case SymbolKind::ARG:    return "argument";
            case SymbolKind::LCL:    return "local";
            default: return "unknown";
        }
    }

    SymbolTable::SymbolTable() {
        // Initialize all running indices to 0.
        indices[SymbolKind::STATIC] = 0;
        indices[SymbolKind::FIELD] = 0;
        indices[SymbolKind::ARG] = 0;
        indices[SymbolKind::LCL] = 0;
    }

    void SymbolTable::startSubroutine() {
        // When starting a new subroutine, we clear the subroutine-level scope.
        // Class-level scope (STATIC, FIELD) remains untouched.
        subRoutineScope.clear();

        // Reset indices for subroutine-level variables.
        indices[SymbolKind::ARG] = 0;
        indices[SymbolKind::LCL] = 0;
    }

    const Symbol *SymbolTable::lookup(const std::string_view name) const {
        // 1. Check the subroutine scope (local variables and arguments) first.
        // This allows local variables to shadow class variables.
        const auto subIt = subRoutineScope.find(name);
        if (subIt != subRoutineScope.end()) {
            return &(subIt->second);
        }

        // 2. If not found, check the class scope (static and field variables).
        const auto classIt = classScope.find(name);
        if (classIt != classScope.end()) {
            return &(classIt->second);
        }

        // 3. Not found in either scope.
        return nullptr;
    }

    SymbolKind SymbolTable::kindOf(const std::string_view name) const {
        const Symbol* s=lookup(name);
        // Return the kind of variable if found, otherwise return NONE.
        return (s) ? s->kind:SymbolKind::NONE;
    }

    std::string_view SymbolTable::typeOf(const std::string_view name) const {
        const Symbol* s=lookup(name);
        // Return the type if found, otherwise return empty string.
        return (s) ? s->type:"";
    }

    int SymbolTable::indexOf(const std::string_view name) const {
        const Symbol* s=lookup(name);
        // Return the index if found, otherwise return -1.
        return (s) ? s->index:-1;
    }

    int SymbolTable::varCount(const SymbolKind kind) const {
        // Return the current count (next index) for the given kind.
        const auto it=indices.find(kind);
        if (it != indices.end()) {
            return it->second;
        }

        return 0;
    }


    void SymbolTable::define(const std::string_view name, const std::string_view type, const SymbolKind kind, const int line, const int col) {
        // Check if the variable is already defined in the *current* scope to prevent redefinition.
        // Note: lookup() checks both scopes, but for redefinition checks, we strictly care about
        // the scope we are about to insert into. However, checking lookup() is a safe conservative check
        // that also catches shadowing if we want to forbid it, or we can refine it to check specific maps.
        // Here, we use lookup() which implies we might error on shadowing class vars with locals if not careful,
        // BUT usually shadowing is allowed in many languages.
        // Let's refine the check: we should only check the specific scope map we are writing to.

        // Actually, the provided implementation used lookup(), which checks both.
        // If we want to allow shadowing (e.g. var int x; inside a method of class with field x),
        // we should only check subRoutineScope if kind is ARG/LCL, and classScope if kind is STATIC/FIELD.

        // However, sticking to the user's logic style, I will keep the lookup but clarify the intent.
        // If the user wants to allow shadowing, this check might be too aggressive if it finds it in class scope
        // while defining in subroutine scope.
        // Let's assume for now we want to prevent ANY collision for safety or simplicity,
        // or that the user intends to fix shadowing logic if needed.

        const Symbol* existing = lookup(name);

        // Refined check: Only throw if the existing symbol is in the SAME scope we are defining in.
        // If we are defining a local (LCL/ARG) and the existing is global (STATIC/FIELD), that's shadowing (usually valid).
        // If we are defining a global and existing is global, that's a redefinition error.
        // If we are defining a local and existing is local, that's a redefinition error.

        bool collision = false;
        if (existing) {
            const bool definingGlobal = (kind == SymbolKind::STATIC || kind == SymbolKind::FIELD);
            const bool existingGlobal = (existing->kind == SymbolKind::STATIC || existing->kind == SymbolKind::FIELD);

            if (definingGlobal == existingGlobal) {
                // Collision in the same scope level
                collision = true;
            }
            // If definingGlobal is false (local) and existingGlobal is true, it's shadowing.
            // If definingGlobal is true and existingGlobal is false, that shouldn't happen during parsing order usually.
        }

        if (collision) {
            const std::string msg =
                "Semantic Error [" + std::to_string(line) + ":" + std::to_string(col) + "]: " +
                "Variable '" + std::string(name) + "' is already defined as a " +
                kindToString(existing->kind) + " at [" +
                std::to_string(existing->declLine) + ":" + std::to_string(existing->declCol) + "].";
            throw std::runtime_error(msg);
        }

        // Create the new symbol, assigning it the current index for its kind.
        const Symbol symbol = {type, kind,indices[kind]++,line,col};

        // Insert into the appropriate scope map.
        if (kind == SymbolKind::STATIC || kind == SymbolKind::FIELD) {
            classScope[name] = symbol;
        } else {
            subRoutineScope[name] = symbol;
        }
    }
}
