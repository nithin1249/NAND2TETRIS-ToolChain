//
// Created by Nithin Kondabathini on 26/1/2026.
//

#include "CodeGenerator.h"

namespace nand2tetris::jack {
    CodeGenerator::CodeGenerator(const GlobalRegistry &registry, std::ostream &out):registry(registry),writer(out){}

    std::string CodeGenerator::getUniqueLabel() {
        return "L" + std::to_string(labelCounter++);
    }

    void CodeGenerator::compileClass(const ClassNode &node) {
        currentClassName=node.getClassName();
        symbolTable = SymbolTable(); // Reset symbol table for the new class

        // 1. Define Class Variables (Static/Field) in the symbol table
        for (const auto& var : node.classVars) {
            const SymbolKind kind = (var->kind == ClassVarKind::STATIC) ? SymbolKind::STATIC : SymbolKind::FIELD;
            for (const auto& name : var->varNames) {
                symbolTable.define(name, var->type, kind, var->getLine(), var->getCol());
            }
        }

        // 2. Compile Subroutines
        for (const auto& sub : node.subroutineDecs) {
            compileSubroutine(*sub);
        }
    }

    void CodeGenerator::compileSubroutine(const SubroutineDecNode& node) {
        symbolTable.startSubroutine(); // Clear local scope

        // Define 'this' for methods (argument 0)
        if (node.subType == SubroutineType::METHOD) {
            symbolTable.define("this", currentClassName, SymbolKind::ARG, node.getLine(), node.getCol());
        }

        // Define Arguments
        for (const auto& param : node.parameters) {
            symbolTable.define(param.name, param.type, SymbolKind::ARG, node.getLine(), node.getCol());
        }

        // Define Local Variables
        for (const auto& var : node.localVars) {
            for (const auto& name : var->varNames) {
                symbolTable.define(name, var->type, SymbolKind::LCL, var->getLine(), var->getCol());
            }
        }

        // Write Function Declaration
        const std::string funcName=std::string(currentClassName) + "." + std::string(node.name);
        const int nLocals = symbolTable.varCount(SymbolKind::LCL);
        writer.writeFunction(funcName, nLocals);

        // Handle Constructor/Method specific setup
        if (node.subType == SubroutineType::CONSTRUCTOR) {
            // Constructor: allocate memory for instance
            // Size = number of fields in the class
            const int nFields = symbolTable.varCount(SymbolKind::FIELD);
            writer.writePush(Segment::CONST, nFields);
            writer.writeCall("Memory.alloc", 1);
            writer.writePop(Segment::POINTER, 0); // Set 'this' to the new address
        }
        else if (node.subType == SubroutineType::METHOD) {
            // Method: map 'argument 0' (the object reference) to 'pointer 0' (this)
            writer.writePush(Segment::ARG, 0);
            writer.writePop(Segment::POINTER, 0);
        }

        compileStatements(node.statements);
    }


    void CodeGenerator::compileStatements(const std::vector<std::unique_ptr<StatementNode>> &stmts) {
        for (const auto& stmt : stmts) {
            switch (stmt->getType()) {
                case ASTNodeType::LET_STATEMENT: compileLet(static_cast<const LetStatementNode&>(*stmt));break; // NOLINT(*-pro-type-static-cast-downcast)
                case ASTNodeType::IF_STATEMENT:     compileIf(static_cast<const IfStatementNode&>(*stmt)); break;// NOLINT(*-pro-type-static-cast-downcast)
                case ASTNodeType::WHILE_STATEMENT:  compileWhile(static_cast<const WhileStatementNode&>(*stmt)); break;// NOLINT(*-pro-type-static-cast-downcast)
                case ASTNodeType::DO_STATEMENT:     compileDo(static_cast<const DoStatementNode&>(*stmt)); break;// NOLINT(*-pro-type-static-cast-downcast)
                case ASTNodeType::RETURN_STATEMENT: compileReturn(static_cast<const ReturnStatementNode&>(*stmt)); break;// NOLINT(*-pro-type-static-cast-downcast)
                default: break;
            }
        }
    }

    void CodeGenerator::compileDo(const DoStatementNode& node) {
        // 'do' statements execute a subroutine for side effects.
        // The return value is pushed onto the stack, so we must pop it to keep the stack clean.
        compileSubroutineCall(*node.callExpression);
        writer.writePop(Segment::TEMP, 0);
    }

    void CodeGenerator::compileReturn(const ReturnStatementNode& node) {
        if (node.expression) {
            compileExpression(*node.expression);
        }else {
            // Void methods/functions must return 0 (constant 0)
            writer.writePush(Segment::CONST, 0);
        }
        writer.writeReturn();
    }


    void CodeGenerator::compileLet(const LetStatementNode& node) {
        if (node.indexExpr) {
            // Array Assignment: arr[i] = expr

            // 1. Push array base address
            const SymbolKind kind = symbolTable.kindOf(node.varName);
            const int index = symbolTable.indexOf(node.varName);
            Segment seg;
            switch(kind) {
                case SymbolKind::STATIC: seg = Segment::STATIC; break;
                case SymbolKind::FIELD:  seg = Segment::THIS;   break;
                case SymbolKind::ARG:    seg = Segment::ARG;    break;
                case SymbolKind::LCL:    seg = Segment::LOCAL;  break;
                default: seg=Segment::TEMP; break;
            }
            writer.writePush(seg, index);

            // 2. Push index and add to base
            compileExpression(*node.indexExpr);
            writer.writeArithmetic(Command::ADD);

            // 3. Push value to assign
            compileExpression(*node.valueExpr);

            // 4. Perform assignment using THAT pointer
            writer.writePop(Segment::TEMP, 0);    // Save value to temp
            writer.writePop(Segment::POINTER, 1); // Set THAT = address (base + index)
            writer.writePush(Segment::TEMP, 0);   // Restore value
            writer.writePop(Segment::THAT, 0);    // *THAT = value
        }else {
            // Simple Assignment: var = expr
            compileExpression(*node.valueExpr);

            const SymbolKind kind = symbolTable.kindOf(node.varName);
            const int index = symbolTable.indexOf(node.varName);

            Segment seg;
            switch(kind) {
                case SymbolKind::STATIC: seg = Segment::STATIC; break;
                case SymbolKind::FIELD:  seg = Segment::THIS;   break;
                case SymbolKind::ARG:    seg = Segment::ARG;    break;
                case SymbolKind::LCL:    seg = Segment::LOCAL;  break;
                default: seg = Segment::TEMP; break; // Should be unreachable given semantic checks
            }
            writer.writePop(seg, index);
        }
    }

    void CodeGenerator::compileWhile(const WhileStatementNode& node) {
        const std::string labelExp = getUniqueLabel();
        const std::string labelEnd = getUniqueLabel();

        writer.writeLabel(labelExp);

        // Evaluate condition
        compileExpression(*node.condition);
        writer.writeArithmetic(Command::NOT); // Negate for "if-goto end" logic
        writer.writeIf(labelEnd);

        compileStatements(node.body);
        writer.writeGoto(labelExp);

        writer.writeLabel(labelEnd);
    }

    void CodeGenerator::compileIf(const IfStatementNode& node) {
        const std::string labelElse = getUniqueLabel();
        const std::string labelEnd = getUniqueLabel();

        // Evaluate condition
        compileExpression(*node.condition);
        writer.writeArithmetic(Command::NOT);
        writer.writeIf(labelElse); // Jump to else if condition is false

        compileStatements(node.ifStatements);
        writer.writeGoto(labelEnd); // Skip else block

        writer.writeLabel(labelElse);
        if (!node.elseStatements.empty()) {
            compileStatements(node.elseStatements);
        }
        writer.writeLabel(labelEnd);
    }

    void CodeGenerator::compileExpression(const ExpressionNode &node) {
        if (node.getType()==ASTNodeType::BINARY_OP) {
            const auto& bin = static_cast<const BinaryOpNode&>(node); // NOLINT(*-pro-type-static-cast-downcast)
            compileExpression(*bin.left);
            compileExpression(*bin.right);
            switch(bin.op) {
                case '+': writer.writeArithmetic(Command::ADD); break;
                case '-': writer.writeArithmetic(Command::SUB); break;
                case '*': writer.writeCall("Math.multiply", 2); break;
                case '/': writer.writeCall("Math.divide", 2); break;
                case '&': writer.writeArithmetic(Command::AND); break;
                case '|': writer.writeArithmetic(Command::OR); break;
                case '<': writer.writeArithmetic(Command::LT); break;
                case '>': writer.writeArithmetic(Command::GT); break;
                case '=': writer.writeArithmetic(Command::EQ); break;
                default: break;
            }
        }else if (node.getType()==ASTNodeType::UNARY_OP) {
            const auto& un = static_cast<const UnaryOpNode&>(node);// NOLINT(*-pro-type-static-cast-downcast)
            compileTerm(*un.term); // Evaluate term first

            if (un.op == '-') writer.writeArithmetic(Command::NEG);
            else if (un.op == '~') writer.writeArithmetic(Command::NOT);
        }else {
            compileTerm(node);
        }
    }

    void CodeGenerator::compileTerm(const ExpressionNode& node) {
        switch (node.getType()) {
            case ASTNodeType::INTEGER_LITERAL:{
                auto& n = static_cast<const IntegerLiteralNode&>(node); // NOLINT(*-pro-type-static-cast-downcast)
                writer.writePush(Segment::CONST, n.value);
                break;
            }
            case ASTNodeType::STRING_LITERAL:{
                auto& n = static_cast<const StringLiteralNode&>(node);// NOLINT(*-pro-type-static-cast-downcast)
                writer.writeStringConstant(n.value);
                break;
            }
            case ASTNodeType::KEYWORD_LITERAL: {
                auto& n = static_cast<const KeywordLiteralNode&>(node);// NOLINT(*-pro-type-static-cast-downcast)
                if (n.value == Keyword::TRUE_) {
                    writer.writePush(Segment::CONST, 1);
                    writer.writeArithmetic(Command::NEG); // True is -1 (111...111 in 2's complement)
                } else if (n.value == Keyword::FALSE_ || n.value == Keyword::NULL_) {
                    writer.writePush(Segment::CONST, 0);
                } else if (n.value == Keyword::THIS_) {
                    writer.writePush(Segment::POINTER, 0);
                }
                break;
            }
            case ASTNodeType::IDENTIFIER: {
                auto& n = static_cast<const IdentifierNode&>(node);// NOLINT(*-pro-type-static-cast-downcast)
                const SymbolKind kind = symbolTable.kindOf(n.name);
                const int index = symbolTable.indexOf(n.name);
                Segment seg;

                switch(kind) {
                    case SymbolKind::STATIC: seg = Segment::STATIC; break;
                    case SymbolKind::FIELD:  seg = Segment::THIS;   break;
                    case SymbolKind::ARG:    seg = Segment::ARG;    break;
                    case SymbolKind::LCL:    seg = Segment::LOCAL;  break;
                    default: seg = Segment::TEMP; break;
                }
                if (n.indexExpr) {
                    // Array Access: x[i]
                    writer.writePush(seg, index); // Push Array Base
                    compileExpression(*n.indexExpr); // Push Index
                    writer.writeArithmetic(Command::ADD); // Base + Index
                    writer.writePop(Segment::POINTER, 1); // Set THAT ptr to (Base + Index)
                    writer.writePush(Segment::THAT, 0);   // Get *THAT
                } else {
                    // Simple Variable
                    writer.writePush(seg, index);
                }
                break;
            }
            case ASTNodeType::SUBROUTINE_CALL: {
                compileSubroutineCall(static_cast<const CallNode&>(node));// NOLINT(*-pro-type-static-cast-downcast)
                break;
            }
            default:break;
        }
    }


    void CodeGenerator::compileSubroutineCall(const CallNode &node) {
        int nArgs=0;
        std::string funcName = std::string(node.functionName);

        if (node.classNameOrVar.empty()) {
            // Implicit 'this' call: foo() -> Class.foo(this)
            writer.writePush(Segment::POINTER, 0); // Push 'this'
            funcName = std::string(currentClassName) + "." + funcName;
            nArgs = 1;
        }else {
            // Check if classNameOrVar is a variable (instance call) or a class (static call)
            if (symbolTable.kindOf(node.classNameOrVar) != SymbolKind::NONE) {
                // It is a variable: a.foo() -> ClassOfA.foo(a)
                const SymbolKind kind = symbolTable.kindOf(node.classNameOrVar);
                const int index = symbolTable.indexOf(node.classNameOrVar);
                const std::string type = std::string(symbolTable.typeOf(node.classNameOrVar));
                Segment seg;
                switch(kind) {
                    case SymbolKind::STATIC: seg = Segment::STATIC; break;
                    case SymbolKind::FIELD:  seg = Segment::THIS;   break;
                    case SymbolKind::ARG:    seg = Segment::ARG;    break;
                    default: seg = Segment::LOCAL; break;
                }
                writer.writePush(seg, index); // Push the object instance
                funcName = type + "." + funcName;
                nArgs = 1;
            }else {
                // It is a class: Math.abs() -> Math.abs()
                funcName = std::string(node.classNameOrVar) + "." + funcName;
                nArgs = 0;
            }

        }

        // Push arguments
        for (const auto& arg : node.arguments) {
            compileExpression(*arg);
            nArgs++;
        }

        writer.writeCall(funcName, nArgs);
    }
}
