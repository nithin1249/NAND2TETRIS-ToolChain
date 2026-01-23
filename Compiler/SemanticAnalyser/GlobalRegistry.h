//
// Created by Nithin Kondabathini on 23/1/2026.
//

#ifndef NAND2TETRIS_GLOBAL_REGISTRY_H
#define NAND2TETRIS_GLOBAL_REGISTRY_H
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

namespace nand2tetris::jack {

    /**
     * @brief Represents the signature of a Jack subroutine (method, function, or constructor).
     */
    struct MethodSignature {
        std::string_view returnType;        ///< The return type of the subroutine (e.g., "int", "void").
        std::vector<std::string_view> parameters; ///< List of parameter types.
        bool isConstructor;                 ///< True if this is a constructor.
        bool isStatic;                      ///< True if this is a static function.
        int line;                           ///< Line number of the declaration.
        int column;                         ///< Column number of the declaration.
    };

    /**
     * @brief A thread-safe registry for tracking all classes and their methods across the entire program.
     *
     * This class is used to perform semantic analysis, such as checking if a called method exists
     * and if the arguments match the expected parameters. It acts as a global symbol table for
     * class and subroutine definitions.
     */
    class GlobalRegistry {
        public:
            GlobalRegistry()=default;
            ~GlobalRegistry()=default;

            /**
             * @brief Registers a new class in the registry.
             *
             * @param className The name of the class to register.
             */
            void registerClass(std::string_view className);

            /**
             * @brief Registers a new method (or function/constructor) for a specific class.
             *
             * @param className The name of the class the method belongs to.
             * @param methodName The name of the method.
             * @param returnType The return type of the method.
             * @param params A vector of parameter types.
             * @param isStatic True if the method is static (function).
             * @param isConstructor True if the method is a constructor.
             * @param line The line number of the declaration.
             * @param column The column number of the declaration.
             * @throws std::runtime_error If the method is already defined in the class.
             */
            void registerMethod(std::string_view className,std::string_view methodName,std::string_view returnType,
                const std::vector<std::string_view>& params, bool isStatic,bool isConstructor, int line,int column);

            /**
             * @brief Checks if a class exists in the registry.
             *
             * Also returns true for built-in types (int, boolean, char, void).
             *
             * @param className The name of the class to check.
             * @return True if the class exists or is a built-in type, false otherwise.
             */
            bool classExists(std::string_view className) const;

            /**
             * @brief Checks if a method exists within a specific class.
             *
             * @param className The name of the class.
             * @param methodName The name of the method.
             * @return True if the method exists, false otherwise.
             */
            bool methodExists(std::string_view className,std::string_view methodName)const;

            /**
             * @brief Retrieves the signature of a specific method.
             *
             * @param className The name of the class.
             * @param methodName The name of the method.
             * @return The MethodSignature struct containing details about the method.
             * @throws std::runtime_error If the method is not found.
             */
            MethodSignature getSignature(std::string_view className,std::string_view methodName);

        private:
            // Map: ClassName -> (MethodName -> Signature)
            std::unordered_map<std::string_view,std::unordered_map<std::string_view,MethodSignature>> methods;
            // Set: ClassNames
            std::unordered_set<std::string_view> classes;
            mutable std::mutex mtx; // Thread safety
    };
}


#endif //NAND2TETRIS_GLOBAL_REGISTRY_H