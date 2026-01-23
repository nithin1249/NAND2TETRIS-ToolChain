//
// Created by Nithin Kondabathini on 23/1/2026.
//

#include "GlobalRegistry.h"

namespace nand2tetris::jack {
    void GlobalRegistry::registerClass(const std::string_view className) {
        std::scoped_lock lock(mtx);
        // Insert the class name into the set of known classes.
        classes.insert(className);
    }

    void GlobalRegistry::registerMethod(const std::string_view className, const std::string_view methodName,
        const std::string_view returnType, const std::vector<std::string_view> &params, const bool isStatic, const bool isConstructor, const int
        line, const int column) {
        std::scoped_lock lock(mtx);

        // Check for duplicate method definition within the same class.
        if (methods[className].contains(methodName)) {
            const auto& existing = methods[className][methodName];
            const std::string msg =
                "Semantic Error [" + std::to_string(line) + ":" + std::to_string(column) + "]: " +
                "Subroutine '" + std::string(methodName) + "' is already defined in class '" +
                std::string(className) + "' (Previous declaration at line " +
                std::to_string(existing.line) + ").";

            throw std::runtime_error(msg);
        }

        // Store the method signature.
        methods[className][methodName] = {returnType, params, isConstructor,isStatic, line,column};
    }

    bool GlobalRegistry::classExists(const std::string_view className) const {
        // Built-in primitive types are always considered "existing classes" for type checking purposes.
        if (className=="int"||className=="boolean"||className=="char"||className=="void") {
            return true;
        }

        // Standard Library classes (simulated)
        // Note: These are commented out in the original code, implying they might be registered manually
        // or handled differently. If they are part of the compilation unit, they will be in 'classes'.
        /*if (className == "Array" || className == "String" || className == "Math" || className == "Output" ||
            className == "Screen" || className == "Keyboard" || className == "Memory" || className == "Sys") return
            true;*/

        // Check against the set of user-defined classes registered so far.
        return classes.contains(className);
    }

    bool GlobalRegistry::methodExists(const std::string_view className, const std::string_view methodName)const {
        // First, check if the class exists in our method map.
        const auto it = methods.find(className);
        if (it == methods.end()) {
            return false;
        }
        // Then, check if the method exists within that class.
        return it->second.contains(methodName);
    }

    MethodSignature GlobalRegistry::getSignature(const std::string_view className, const std::string_view methodName) {
        // Look up the class.
        const auto it=methods.find(className);
        if (it!=methods.end()) {
            // Look up the method within the class.
            const auto sit=it->second.find(methodName);
            if (sit!=it->second.end()) {
                return sit->second;
            }
        }
        // If not found, this indicates a logic error in the compiler (caller should have checked existence).
        throw std::runtime_error("Internal Compiler Error: Signature lookup failed for " + std::string(className) + "." + std::string(methodName));
    }
}
