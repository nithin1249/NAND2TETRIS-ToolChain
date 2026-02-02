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

    GlobalRegistry::GlobalRegistry() {
        loadStandardLibrary();
    }

    void GlobalRegistry::registerMethod(const std::string_view className, const std::string_view methodName,
                                        const std::string_view returnType, const std::vector<std::string_view> &params, const bool isStatic,const int
                                        line, const int column) {
        std::scoped_lock lock(mtx);

        // Check for duplicate method definition within the same class.
        if (methods[className].count(methodName)) {
            const auto& existing = methods[className][methodName];
            const std::string msg =
                "Semantic Error [" + std::to_string(line) + ":" + std::to_string(column) + "]: " +
                "Subroutine '" + std::string(methodName) + "' is already defined in class '" +
                std::string(className) + "' (Previous declaration at line " +
                std::to_string(existing.line)+" "+std::to_string(existing.column) + ").";

            throw std::runtime_error(msg);
        }

        // Store the method signature.
        methods[className][methodName] = {returnType, params, isStatic, line,column};
    }

    bool GlobalRegistry::classExists(const std::string_view className) const {
        // Built-in primitive types are always considered "existing classes" for type checking purposes.
        if (className=="int"||className=="boolean"||className=="char") {
            return true;
        }

        // Standard Library classes (simulated)
        // Note: These are commented out in the original code, implying they might be registered manually
        // or handled differently. If they are part of the compilation unit, they will be in 'classes'.
        /*if (className == "Array" || className == "String" || className == "Math" || className == "Output" ||
            className == "Screen" || className == "Keyboard" || className == "Memory" || className == "Sys") return
            true;*/

        // Check against the set of user-defined classes registered so far.
        return classes.count(className);
    }

    bool GlobalRegistry::methodExists(const std::string_view className, const std::string_view methodName)const {
        // First, check if the class exists in our method map.
        const auto it = methods.find(className);
        if (it == methods.end()) {
            return false;
        }
        // Then, check if the method exists within that class.
        return it->second.count(methodName);
    }

    MethodSignature GlobalRegistry::getSignature(const std::string_view className,
                                                 const std::string_view methodName) const {
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

    int GlobalRegistry::getClassCount() const {
        return static_cast<int>(classes.size());
    }

    void GlobalRegistry::loadStandardLibrary() {
        // --- MATH CLASS ---
        registerClass("Math");
        registerMethod("Math", "init",      "void", {},             true,  0, 0);
        registerMethod("Math", "abs",       "int",  {"int"},        true,  0, 0);
        registerMethod("Math", "multiply",  "int",  {"int", "int"}, true,  0, 0);
        registerMethod("Math", "divide",    "int",  {"int", "int"}, true,  0, 0);
        registerMethod("Math", "min",       "int",  {"int", "int"}, true,  0, 0);
        registerMethod("Math", "max",       "int",  {"int", "int"}, true,  0, 0);
        registerMethod("Math", "sqrt",      "int",  {"int"},        true,  0, 0);
        registerMethod("Math","bit","boolean",{"int","int"},true,0,0);

        // --- STRING CLASS ---
        // Note: Constructors ('new') are usually treated as 'static' in the OS API logic
        // because you call them on the class (String.new), not an object.
        registerClass("String");
        registerMethod("String", "new",           "String", {"int"},           true,  0, 0);
        registerMethod("String", "dispose",       "void",   {},                false, 0, 0);
        registerMethod("String", "length",        "int",    {},                false, 0, 0);
        registerMethod("String", "charAt",        "char",   {"int"},           false, 0, 0);
        registerMethod("String", "setCharAt",     "void",   {"int", "char"},   false, 0, 0);
        registerMethod("String", "appendChar",    "String", {"char"},          false, 0, 0);
        registerMethod("String", "eraseLastChar", "void",   {},                false, 0, 0);
        registerMethod("String", "intValue",      "int",    {},                false, 0, 0);
        registerMethod("String", "setInt",        "void",   {"int"},           false, 0, 0);
        registerMethod("String", "backSpace",     "char",   {},                false, 0, 0);
        registerMethod("String", "doubleQuote",   "char",   {},                false, 0, 0);
        registerMethod("String", "newLine",       "char",   {},                false, 0, 0);
        registerMethod("String","int2String","void",{},false,0,0);

        // --- ARRAY CLASS ---
        registerClass("Array");
        registerMethod("Array", "new",     "Array", {"int"}, true,  0, 0);
        registerMethod("Array", "dispose", "void",  {},      false, 0, 0);

        // --- OUTPUT CLASS ---
        registerClass("Output");
        registerMethod("Output", "init", "void", {}, true, 0, 0);
        registerMethod("Output", "moveCursor", "void", {"int", "int"}, true, 0, 0);
        registerMethod("Output", "printChar", "void", {"char"}, true, 0, 0);
        registerMethod("Output", "printString", "void", {"String"}, true, 0, 0);
        registerMethod("Output", "printInt", "void", {"int"}, true, 0, 0);
        registerMethod("Output", "println", "void", {}, true, 0, 0);
        registerMethod("Output", "backSpace", "void", {}, true, 0, 0);
        registerMethod("Output", "initMap", "void", {}, true, 0, 0);
        registerMethod("Output", "create", "void", {"int","int","int","int","int","int","int","int","int","int","int","int"}, true, 0, 0);
        registerMethod("Output", "getMap", "Array", {"char"}, true, 0, 0);
        registerMethod("Output", "incrementCursor", "void", {}, true, 0, 0);
        registerMethod("Output", "decrementCursor", "void", {}, true, 0, 0);

        // --- SCREEN CLASS ---
        registerClass("Screen");
        registerMethod("Screen", "init",          "void", {},                              true, 0, 0);
        registerMethod("Screen", "clearScreen",   "void", {},                              true, 0, 0);
        registerMethod("Screen", "setColor",      "void", {"boolean"},                     true, 0, 0);
        registerMethod("Screen", "drawPixel",     "void", {"int", "int"},                  true, 0, 0);
        registerMethod("Screen", "drawLine",      "void", {"int", "int", "int", "int"},    true, 0, 0);
        registerMethod("Screen", "drawRectangle", "void", {"int", "int", "int", "int"},    true, 0, 0);
        registerMethod("Screen", "drawCircle",    "void", {"int", "int", "int"},           true, 0, 0);

        // --- KEYBOARD CLASS ---
        registerClass("Keyboard");
        registerMethod("Keyboard", "init",       "void",   {},         true, 0, 0);
        registerMethod("Keyboard", "keyPressed", "char",   {},         true, 0, 0);
        registerMethod("Keyboard", "readChar",   "char",   {},         true, 0, 0);
        registerMethod("Keyboard", "readLine",   "String", {"String"}, true, 0, 0);
        registerMethod("Keyboard", "readInt",    "int",    {"String"}, true, 0, 0);

        // --- MEMORY CLASS ---
        registerClass("Memory");
        registerMethod("Memory", "init",    "void", {},             true, 0, 0);
        registerMethod("Memory", "peek",    "int",  {"int"},        true, 0, 0);
        registerMethod("Memory", "poke",    "void", {"int", "int"}, true, 0, 0);
        registerMethod("Memory", "alloc",   "int",  {"int"},        true, 0, 0);
        registerMethod("Memory", "deAlloc", "void", {"Array"},        true, 0, 0);

        // --- SYS CLASS ---
        registerClass("Sys");
        registerMethod("Sys", "init",  "void", {},      true, 0, 0);
        registerMethod("Sys", "halt",  "void", {},      true, 0, 0);
        registerMethod("Sys", "error", "void", {"int"}, true, 0, 0);
        registerMethod("Sys", "wait",  "void", {"int"}, true, 0, 0);
    }

    void GlobalRegistry::dumpToJSON(const std::string &filename) const {
        std::ofstream out(filename);
        out << "{\n";
        out << "  \"registry\": [\n";

        bool firstMethod = true;

        // Iterate over all classes
        for (const auto& [className, methodMap] : methods) {

            // Iterate over all methods in this class
            for (const auto& [methodName, sig] : methodMap) {
                if (!firstMethod) out << ",\n";
                firstMethod = false;

                out << "    {\n";
                out << "      \"class\": \"" << className << "\",\n";
                out << "      \"method\": \"" << methodName << "\",\n";
                out << "      \"type\": \"" << (sig.isStatic ? "function" : "method") << "\",\n";
                out << "      \"return\": \"" << sig.returnType << "\",\n";

                // Format parameters: "int, char"
                out << "      \"params\": \"";
                for (size_t i = 0; i < sig.parameters.size(); ++i) {
                    out << sig.parameters[i];
                    if (i < sig.parameters.size() - 1) out << ", ";
                }
                out << "\"\n";
                out << "    }";
            }
        }

        out << "\n  ]\n";
        out << "}\n";
        out.close();
    }
}
