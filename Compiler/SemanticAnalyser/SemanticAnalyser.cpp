//
// Created by Nithin Kondabathini on 24/1/2026.
//

#include "SemanticAnalyser.h"
#include <stdexcept>

namespace nand2tetris::jack {
    SemanticAnalyser::SemanticAnalyser(const GlobalRegistry &registry):registry(registry){};

    void SemanticAnalyser::error(const std::string_view message, const Node &node) const {
        // Format error message with file, line, and column information.
        throw std::runtime_error("Semantic Error [" + std::string(currentClassName) + ".jack:" +
            std::to_string(node.getLine()) + ":" + std::to_string(node.getCol()) + "]: " +
            std::string(message));
    }

    void SemanticAnalyser::checkTypeMatch(const std::string_view expected, const std::string_view actual, const Node &locationNode) const {
        if (expected == actual) return;
        if (actual == "null") return; // Null is assignable to any object type.
        if (expected == "char" && actual == "int") return; // Jack allows char/int interchangeability.

        error("Type Mismatch. Expected '" + std::string(expected) + "', Got '" + std::string(actual) + "'", locationNode);
    }

    void SemanticAnalyser::analyseClass(const ClassNode& class_node) {
        currentClassName=class_node.className;

        SymbolTable masterTable;

        // 1. Process Class Variables (Static/Field)
        for (const std::unique_ptr<ClassVarDecNode>& var:class_node.classVars) {
            const SymbolKind kind = (var->kind == ClassVarKind::STATIC) ? SymbolKind::STATIC : SymbolKind::FIELD;

            // Verify the type exists (if it's a class type)
            if (!registry.classExists(var->type)) {
                error("Unknown type '" + std::string(var->type) + "'", *var);
            }

            // Add variables to the class-level symbol table
            for (const std::string_view& name : var->varNames) {
                masterTable.define(name, var->type, kind,var->getLine(),var->getCol());
            }
        }

        // 2. Process Subroutines
        for (const std::unique_ptr<SubroutineDecNode>& sub : class_node.subroutineDecs) {
            analyseSubroutine(*sub, masterTable);
        }
    }

    void SemanticAnalyser::analyseSubroutine(const SubroutineDecNode &sub, const SymbolTable &masterTable) {
        currentSubroutineName=sub.name;

        // Determine kind string for logic checks
        if (sub.subType == SubroutineType::CONSTRUCTOR) currentSubroutineKind = "constructor";
        else if (sub.subType == SubroutineType::FUNCTION) currentSubroutineKind = "function";
        else currentSubroutineKind = "method";

        // 1. Create Local Scope (Copy Master)
        // We start with a copy of the class-level table to inherit static/field vars.
        SymbolTable localTable = masterTable;
        localTable.startSubroutine();

        // 2. Define 'this' for methods
        //  operate on the current instance, so 'this' is the first implicit argument.
        if (sub.subType == SubroutineType::METHOD) {
            localTable.define("this", currentClassName, SymbolKind::ARG, sub.getLine(), 0);
        }

        // 3. Define Arguments
        for (const auto&[type, name] : sub.parameters) {
            if (!registry.classExists(type)) {
                error("Unknown type '" + std::string(type) + "' for argument '" + std::string(name) + "'", sub);
            }
            localTable.define(name, type, SymbolKind::ARG, sub.getLine(), 0);
        }

        // 4. Define Local Variables
        for (const std::unique_ptr<VarDecNode>& varDecl : sub.localVars) {
            if (!registry.classExists(varDecl->type)) {
                error("Unknown type '" + std::string(varDecl->type) + "'", *varDecl);
            }
            for (const std::string_view& name : varDecl->varNames) {
                localTable.define(name, varDecl->type, SymbolKind::LCL, varDecl->getLine(), varDecl->getCol());
            }
        }

        // 5. Analyze Statements
        analyseStatements(sub.statements, localTable);
    }


    void SemanticAnalyser::analyseStatements(const std::vector<std::unique_ptr<StatementNode> > &stmts, SymbolTable &table) const {
        for (const std::unique_ptr<StatementNode>& stmt : stmts) {
            switch (stmt->getType()) {
                case ASTNodeType::LET_STATEMENT:
                    analyseLet(static_cast<const LetStatementNode&>(*stmt), table); // NOLINT(*-pro-type-static-cast-downcast)
                    break;
                case ASTNodeType::DO_STATEMENT:
                    analyseDo(static_cast<const DoStatementNode&>(*stmt), table); // NOLINT(*-pro-type-static-cast-downcast)
                    break;
                case ASTNodeType::IF_STATEMENT:
                    analyseIf(static_cast<const IfStatementNode&>(*stmt), table); // NOLINT(*-pro-type-static-cast-downcast)
                    break;
                case ASTNodeType::WHILE_STATEMENT:
                    analyseWhile(static_cast<const WhileStatementNode&>(*stmt), table); // NOLINT(*-pro-type-static-cast-downcast)
                    break;
                case ASTNodeType::RETURN_STATEMENT:
                    analyseReturn(static_cast<const ReturnStatementNode&>(*stmt), table); // NOLINT(*-pro-type-static-cast-downcast)
                    break;
                default:
                    error("Unknown statement type found in AST", *stmt);
            }
        }
    }

    void SemanticAnalyser::analyseLet(const LetStatementNode &node, SymbolTable &table)const{
        // 1. Check Variable Existence
        if (table.kindOf(node.varName) == SymbolKind::NONE) {
            error("Undefined variable '" + std::string(node.varName) + "'", node);
        }
        const std::string_view varType = table.typeOf(node.varName);

        // 2. Array Indexing Check
        if (node.indexExpr) {
            if (varType != "Array") {
                error("Cannot index non-array variable '" + std::string(node.varName) + "'", node);
            }
            const std::string_view idxType = analyseExpression(*node.indexExpr, table);
            if (idxType != "int") {
                error("Array index must be an integer.", *node.indexExpr);
            }
        }

        // 3. Value Check
        const std::string_view exprType = analyseExpression(*node.valueExpr, table);

        // If it's a standard assignment (not array index), types must match.
        // Note: Array element assignment (arr[i] = x) is not strictly type-checked in standard Jack
        // because arrays are untyped (void*), but we could enforce it if we knew the array's content type.
        // Here we only check direct variable assignment.
        if (!node.indexExpr) {
            checkTypeMatch(varType, exprType, *node.valueExpr);
        }
    }

    void SemanticAnalyser::analyseIf(const IfStatementNode &node, SymbolTable &table)const {
        const std::string_view condType = analyseExpression(*node.condition, table);
        if (condType != "boolean") {
            error("If condition must be boolean.", *node.condition);
        }
        analyseStatements(node.ifStatements, table);
        if (!node.elseStatements.empty()) {
            analyseStatements(node.elseStatements, table);
        }
    }

    void SemanticAnalyser::analyseDo(const DoStatementNode &node, SymbolTable &table)const {
        // 'do' just wraps a CallNode. Analyze the call.
        const CallNode& call = *node.callExpression;
        analyseSubroutineCall(call.classNameOrVar, call.functionName, call.arguments, table, call);
    }

    void SemanticAnalyser::analyseWhile(const WhileStatementNode &node, SymbolTable &table)const {
        const std::string_view condType = analyseExpression(*node.condition, table);
        if (condType != "boolean") {
            error("While condition must be boolean.", *node.condition);
        }
        analyseStatements(node.body, table);
    }

    void SemanticAnalyser::analyseReturn(const ReturnStatementNode &node, SymbolTable &table) const {
        const MethodSignature sig = registry.getSignature(currentClassName, currentSubroutineName);
        const std::string_view requiredType = sig.returnType;

        // 1. Constructor Rules
        if (currentSubroutineKind == "constructor") {
            if (!node.expression) error("Constructor must return 'this'.", node);

            // Check if returning 'this'
            bool returningThis = false;
            if (node.expression->getType() == ASTNodeType::KEYWORD_LITERAL) {
                auto& k = static_cast<const KeywordLiteralNode&>(*node.expression);// NOLINT(*-pro-type-static-cast-downcast)
                if (k.value == Keyword::THIS_) returningThis = true;
            }
            if (!returningThis) error("Constructor must return 'this'.", *node.expression);
        }

        // 2. Void Function Rules
        if (requiredType == "void") {
            if (node.expression) {
                error("Void function cannot return a value.", *node.expression);
            }
        }
        // 3. Value Function Rules
        else {
            if (!node.expression) {
                error("Function must return a value of type '" + std::string(requiredType) + "'.", node);
            }
            const std::string_view actualType = analyseExpression(*node.expression, table);
            checkTypeMatch(requiredType, actualType, *node.expression);
        }
    }


    std::string_view SemanticAnalyser::analyseExpression(const ExpressionNode &node, SymbolTable &table) const {
        switch (node.getType()) {
            case ASTNodeType::INTEGER_LITERAL:
                return "int";
            case ASTNodeType::STRING_LITERAL:
                return "String";
            case ASTNodeType::KEYWORD_LITERAL: {
                const auto& n = static_cast<const KeywordLiteralNode&>(node); // NOLINT(*-pro-type-static-cast-downcast)
                switch(n.value) {
                    case Keyword::TRUE_:
                    case Keyword::FALSE_: return "boolean";
                    case Keyword::THIS_:  return currentClassName;
                    case Keyword::NULL_:  return "null";
                    default: return "void";
                }
            }

            case ASTNodeType::IDENTIFIER: {
                auto& n = static_cast<const IdentifierNode&>(node); // NOLINT(*-pro-type-static-cast-downcast)
                const std::string_view type = table.typeOf(n.name);
                if (type.empty()) {
                    error("Undefined variable '" + std::string(n.name) + "'", node);
                }
                if (n.indexExpr) {
                    if (type != "Array") error("Cannot index non-array variable.", node);
                    if (analyseExpression(*n.indexExpr, table) != "int") {
                        error("Array index must be an integer.", *n.indexExpr);
                    }
                    return "int"; // Array access is always int
                }
                // Return the type directly from the table to avoid local variable reference issues.
                return table.typeOf(n.name);
            }
            case ASTNodeType::BINARY_OP: {
                auto& n = static_cast<const BinaryOpNode&>(node); // NOLINT(*-pro-type-static-cast-downcast)
                const std::string_view left = analyseExpression(*n.left, table);
                const std::string_view right = analyseExpression(*n.right, table);

                // Math (+ - * /) -> Returns INT
                if (std::string("+-*/").find(n.op) != std::string::npos) {
                    checkTypeMatch("int", left, *n.left);
                    checkTypeMatch("int", right, *n.right);
                    return "int";
                }

                // Inequality (< >) -> Returns BOOLEAN
                if (n.op == '<' || n.op == '>') {
                    checkTypeMatch("int", left, *n.left);
                    checkTypeMatch("int", right, *n.right);
                    return "boolean";
                }

                // Equality (=) -> Returns BOOLEAN
                if (n.op == '=') {
                    // Allow (Alien == Alien) or (Alien == null) or (int == int)
                    if (left != right && left != "null" && right != "null") {
                        error("Comparison type mismatch: " + std::string(left) + " vs " + std::string(right), node);
                    }
                    return "boolean";
                }

                // Logic (& |) -> Returns BOOLEAN
                if (n.op == '&' || n.op == '|') {
                    checkTypeMatch("boolean", left, *n.left);
                    checkTypeMatch("boolean", right, *n.right);
                    return "boolean";
                }
                return "void";
            }

            case ASTNodeType::UNARY_OP: {
                auto& n = static_cast<const UnaryOpNode&>(node); // NOLINT(*-pro-type-static-cast-downcast)
                const std::string_view inner = analyseExpression(*n.term, table);

                if (n.op == '-') {
                    checkTypeMatch("int", inner, *n.term);
                    return "int";
                }
                if (n.op == '~') {
                    checkTypeMatch("boolean", inner, *n.term);
                    return "boolean";
                }
                return "void";
            }
            case ASTNodeType::SUBROUTINE_CALL: {
                auto& n = static_cast<const CallNode&>(node);  // NOLINT(*-pro-type-static-cast-downcast)
                return analyseSubroutineCall(n.classNameOrVar, n.functionName, n.arguments, table, node);
			}
			default:
				return "void";
		}

	}


	std::string_view SemanticAnalyser::analyseSubroutineCall(const std::string_view classNameOrVar, const std::string_view functionName,
		const std::vector<std::unique_ptr<ExpressionNode> > &args, SymbolTable &table, const Node &locationNode) const {
		std::string_view targetClass;
        const std::string_view targetMethod = functionName;
        bool isMethodCall = false;

        // 1. Determine Target Class and Call Type
        if (classNameOrVar.empty()) { // Implicit 'this' call: foo()
            targetClass = currentClassName;
        	if (!registry.methodExists(targetClass,targetMethod)) {
        		error("Method '" + std::string(targetMethod) + "' not found in class '" + std::string(targetClass) +
        			"'", locationNode);
        	}
            const auto sig = registry.getSignature(targetClass, targetMethod);
            if (currentSubroutineKind == "function" && !sig.isStatic) {
                 error("Cannot call method '" + std::string(functionName) + "' from static function without object.", locationNode);
            }
            isMethodCall = !sig.isStatic;
        } else {
            const std::string_view type = table.typeOf(classNameOrVar);
            if (!type.empty()) { // It's a Variable: a.foo()
                targetClass = type;
                isMethodCall = true;
            } else { // It's a Class: Math.abs()
                if (!registry.classExists(classNameOrVar)) {
                    error("Undefined class '" + std::string(classNameOrVar) + "'", locationNode);
                }
                targetClass = classNameOrVar;
                isMethodCall = false;
            }
        }

        // 2. Verify Method Existence
        if (!registry.methodExists(targetClass, targetMethod)) {
            error("Method '" + std::string(targetMethod) + "' not found in class '" + std::string(targetClass) + "'", locationNode);
        }

        const auto sig = registry.getSignature(targetClass, targetMethod);

        // 3. Static/Method Mismatch Checks
        if (isMethodCall && sig.isStatic) {
            error("Cannot call static function '" + std::string(targetMethod) + "' on an object instance.", locationNode);
        }
        if (!isMethodCall && !sig.isStatic) {
            error("Cannot call method '" + std::string(targetMethod) + "' as a static function.", locationNode);
        }

        // 4. Argument Count Check
        if (args.size() != sig.parameters.size()) {
            error("Argument count mismatch. Expected " + std::to_string(sig.parameters.size()) +
                  ", Got " + std::to_string(args.size()), locationNode);
        }

        // 5. Argument Type Check
        for (size_t i = 0; i < args.size(); ++i) {
            const std::string_view argType = analyseExpression(*args[i], table);
            checkTypeMatch(sig.parameters[i], argType, *args[i]);
        }

        return sig.returnType;
	}
}
