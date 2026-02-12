//
// Created by Nithin Kondabathini on 13/1/2026.
//

#include "Parser.h"
#include "AST.h"
#include "../Tokenizer/Tokenizer.h"
#include "../SemanticAnalyser/GlobalRegistry.h"
#include <filesystem>
namespace fs = std::filesystem;

namespace nand2tetris::jack {
    Parser::Parser(Tokenizer &tokenizer, GlobalRegistry& registry):tokenizer(tokenizer),globalRegistry(registry) {
        // Initialize the parser by pointing to the first token available in the tokenizer.
        // The tokenizer is assumed to be already initialized and pointing to the first token.
        currentToken=&tokenizer.current();
    }

    std::unique_ptr<ClassNode> Parser::parse() {
        // The entry point for parsing a Jack file. Every Jack file must contain exactly one class.
        auto classNode = parseClass();

        // Constraint: Single Class Check
        // After parseClass() finishes, it consumes the final '}'.
        // The currentToken should now be EOF. If it's anything else, it's an error.
        if (currentToken->getType() != TokenType::END_OF_FILE) {
            tokenizer.errorHere("Syntax Error: A Jack file must contain exactly one class. Found extra tokens after class body.");
        }

        return classNode;
    }


    void Parser::advance() {
        // Move the tokenizer forward to the next token and update our local reference.
        tokenizer.advance();
        currentToken = &tokenizer.current();
    }

    bool Parser::check(const TokenType type) const {
        // Check if the current token matches the expected type without consuming it.
        return currentToken->getType()==type;
    }

    bool Parser::check(const std::string_view text) const {
        // Check if the current token's text value matches the expected string without consuming it.
        return currentToken->getValue()==text;
    }

    void Parser::consume(const TokenType type, const std::string_view errorMessage) {
        // If the current token matches the expected type, consume it (advance).
        // Otherwise, report a syntax error at the current token's location.
        if (check(type)) {
            advance();
        } else {
            tokenizer.errorAt(currentToken->getLine(),currentToken->getColumn(), errorMessage);
        }
    }

    void Parser::consume(const std::string_view text, const std::string_view errorMessage) {
        // If the current token matches the expected text, consume it (advance).
        // Otherwise, report a syntax error.
        if (check(text)) {
            advance();
        } else {
            tokenizer.errorAt(currentToken->getLine(),currentToken->getColumn(), errorMessage);
        }
    }

    std::unique_ptr<ClassNode> Parser::parseClass() {
        // Grammar: 'class' className '{' classVarDec* subroutineDec* '}'
        int line = currentToken->getLine();
        int col = currentToken->getColumn();

        // 1. Expect 'class' keyword
        consume("class", "Expected 'class' keyword");

        // 2. Expect class name (identifier)
        std::string_view className = currentToken->getValue();
        consume(TokenType::IDENTIFIER, "Expected class name");

        const fs::path filePath(tokenizer.getFilePath());
        const std::string expectedName = filePath.stem().string();

        if (className != expectedName) {
            tokenizer.errorHere("Class name mismatch. The class defined in '" +
                                filePath.filename().string() + "' must be named '" + expectedName +
                                "', but found '" + std::string(className) + "'.");
        }

        currentClassName=className;
        if (globalRegistry.classExists(className)) {
            tokenizer.errorHere("Duplicate class definition: Class '" + std::string(className) + "' is already "
                                                                                                 "defined.");
        }
        globalRegistry.registerClass(className);

        // 3. Expect opening brace '{'
        consume("{", "Expected '{'");

        std::vector<std::unique_ptr<ClassVarDecNode>> classVars;
        std::vector<std::unique_ptr<SubroutineDecNode>> subroutineDecs;

        // 4. Parse class body: variable declarations followed by subroutine declarations.
        // We loop until we hit the closing brace '}'.
        while (!check("}")) {
            const std::string_view val=currentToken->getValue();

            // Distinguish between class variables (static/field) and subroutines (constructor/method/function).
            if (val=="static"||val=="field") {
                classVars.push_back(parseClassVarDec());
            }else if (val=="constructor"||val=="method"||val=="function") {
                subroutineDecs.push_back(parseSubroutine());
            }else {
                // If we encounter anything else, it's a syntax error.
                tokenizer.errorAt(currentToken->getLine(),currentToken->getColumn(), "Expected class variable or subroutine declaration");
            }
        }

        // 5. Expect closing brace '}'
        consume("}", "Expected '}' to close class body");

        return std::make_unique<ClassNode>(className, std::move(classVars), std::move(subroutineDecs), line, col);
    }

    std::unique_ptr<ClassVarDecNode> Parser::parseClassVarDec() {
        // Grammar: ('static' | 'field') type varName (',' varName)* ';'
        int line = currentToken->getLine();
        int col = currentToken->getColumn();

        // 1. Determine if it's a static or field variable.
        // We assume the caller has already verified the token is "static" or "field".
        ClassVarKind kind;
        if (check("static")) {
            kind=ClassVarKind::STATIC;
        }else {
            kind=ClassVarKind::FIELD;
        }

        advance(); // Consume 'static' or 'field'

        // 2. Parse the type (int, char, boolean, or a class name).
        std::string_view type = currentToken->getValue();
        if (check("int")||check("boolean")||check("char")||check("float")||check(TokenType::IDENTIFIER)) {
            advance();
        }else {
            tokenizer.errorAt(currentToken->getLine(), currentToken->getColumn(), "Expected variable type (int, char, boolean, or class name)");
        }

        // 3. Parse the list of variable names.
        std::vector<std::string_view> names;

        // The first variable name is mandatory.
        names.push_back(currentToken->getValue());
        consume(TokenType::IDENTIFIER, "Expected variable name");

        // Handle multiple variables declared in the same line (e.g., static int x, y, z;)
        while (true) {
            if (check(",")) {
                advance(); // Consume the comma
            } else if (check(TokenType::IDENTIFIER)) {
                // Predictive error handling: if we see an identifier but no comma, it's a likely syntax error.
                tokenizer.errorAt(currentToken->getLine(), currentToken->getColumn(), "Missing ',' between variable identifiers");
            } else if (check(";")) {
                // Valid end of the list
                break;
            }else {
                tokenizer.errorAt(currentToken->getLine(), currentToken->getColumn(),"Expected ',' or ';' after variable name");
            }

            // Consume the next variable name
            names.push_back(currentToken->getValue());
            consume(TokenType::IDENTIFIER, "Expected variable name");
        }

        // 4. Expect the closing semicolon.
        consume(";", "Expected ';' at the end of variable declaration");

        return std::make_unique<ClassVarDecNode>(kind, type, std::move(names), line, col);
    }

    std::unique_ptr<SubroutineDecNode> Parser::parseSubroutine() {
        // Grammar: ('constructor' | 'function' | 'method') ('void' | type) subroutineName '(' parameterList ')'
        // subroutineBody: '{' varDec* statements '}'
        int line = currentToken->getLine();
        int col = currentToken->getColumn();

        // 1. Determine the subroutine type (constructor, function, or method).
        SubroutineType type;
        if (currentToken->getValue()=="constructor") {
            type=SubroutineType::CONSTRUCTOR;
        }else if (currentToken->getValue()=="function") {
            type=SubroutineType::FUNCTION;
        }else {
            type=SubroutineType::METHOD;
        }
        advance(); // Consume the subroutine type keyword

        // 2. Parse the return type.
        // Can be 'void', a primitive type (int, boolean, char), or a class name (identifier).
        std::string_view returnType;
        if (currentToken->getValue()=="void") {
            returnType="void";
            advance();
        }else if (check("int")||check("float")||check("boolean")||check("char")||check(TokenType::IDENTIFIER)) {
            returnType=currentToken->getValue();
            advance();
        }else {
            tokenizer.errorAt(currentToken->getLine(), currentToken->getColumn(), "Expected return type void, int, char, boolean, or class name");
        }

        // 3. Parse the subroutine name.
        std::string_view subroutineName=currentToken->getValue();
        consume(TokenType::IDENTIFIER, "Expected subroutine name");

        // 4. Parse the parameter list enclosed in parentheses.
        std::vector<Parameter> parameters;
        consume("(","Expected '(' to open parameter list");

        // Check if parameter list is empty by looking for the closing parenthesis.
        if (!check(")")) {
            // Loop to parse parameters separated by commas.
            while (true) {
                // Parse parameter type
                const std::string_view pType = currentToken->getValue();
                if (check("int")||check("boolean")||check("char")||check(TokenType::IDENTIFIER)) {
                    advance();
                }else {
                    tokenizer.errorAt(currentToken->getLine(), currentToken->getColumn(), "Expected parameter type (int, char, boolean, or class name)");
                }

                // Parse parameter name
                const std::string_view pName = currentToken->getValue();
                consume(TokenType::IDENTIFIER, "Expected parameter name");

                parameters.push_back({pType, pName});

                // Check for comma to continue or closing parenthesis to break
                if (check(",")) {
                    advance(); // Found a comma, expect another parameter
                } else if (check(")")) {
                    break; // Found closing parenthesis, end of list
                }else {
                    // PREDICTIVE ERROR: If we see a type-like keyword or an identifier,
                    // they definitely just forgot the comma.
                    if (check("int") || check("boolean") || check("char") || check(TokenType::IDENTIFIER)) {
                        tokenizer.errorAt(currentToken->getLine(), currentToken->getColumn(), "Missing ',' between parameters");
                    }
                    // If it's not a type, they probably forgot to close the list.
                    tokenizer.errorAt(currentToken->getLine(), currentToken->getColumn(), "Expected ')' to close parameter list");
                }
            }
        }


        consume(")", "Expected ')' to close parameter list");

        //Convert AST Parameters to String Vector for Registry
        std::vector<std::string_view> paramTypes;
        paramTypes.reserve(parameters.size());
        for (const auto& p : parameters) {
            paramTypes.push_back(p.type);
        }

        const bool isStatic = (type == SubroutineType::FUNCTION || type == SubroutineType::CONSTRUCTOR);
        globalRegistry.registerMethod(
        currentClassName,      // We saved this in parseClass
        subroutineName,        // Parsed earlier in this function
        returnType,            // Parsed earlier in this function
        paramTypes,            // Created just now
        isStatic,
        line,                  // From start of subroutine
        col
    );

        // 5. Parse the subroutine body.
        consume("{","Expected '{' to open subroutine body");

        std::vector<std::unique_ptr<VarDecNode>> localVars;

        // Parse local variable declarations (must come before statements).
        while (check("var")) {
            localVars.push_back(parseVarDec());
        }

        // Parse statements until the closing brace.
        std::vector<std::unique_ptr<StatementNode> > statements = parseStatements();

        consume("}","Expected '}' to close subroutine body");

        return std::make_unique<SubroutineDecNode>(type,returnType,subroutineName,parameters,std::move(localVars),std::move(statements), line, col);
    }

    std::unique_ptr<VarDecNode> Parser::parseVarDec() {
        // Grammar: 'var' type varName (',' varName)* ';'
        int line = currentToken->getLine();
        int col = currentToken->getColumn();

        // 1. Consume 'var' keyword.
        consume("var", "Expected 'var' keyword");

        // 2. Parse the type (int, char, boolean, or a class name).
        std::string_view type = currentToken->getValue();
        if (check("int")||check("boolean")||check("char")||check("float")||check(TokenType::IDENTIFIER)) {
            advance();
        }else {
            tokenizer.errorAt(currentToken->getLine(), currentToken->getColumn(), "Expected variable type (int, char, boolean, or class name)");
        }

        // 3. Parse the list of variable names.
        std::vector<std::string_view> names;

        // First variable name is mandatory.
        names.push_back(currentToken->getValue());
        consume(TokenType::IDENTIFIER, "Expected variable name");

        // Handle multiple variables declared in the same line (e.g., var int x, y, z;)
        while (true) {
            if (check(",")) {
                advance(); // Consume the comma
            } else if (check(TokenType::IDENTIFIER)) {
                // Predictive error handling: if we see an identifier but no comma, it's a likely syntax error.
                tokenizer.errorAt(currentToken->getLine(), currentToken->getColumn(), "Missing ',' between variable identifiers");
            } else {
                break; // No comma and no identifier? The list is finished.
            }

            // Consume the next variable name
            names.push_back(currentToken->getValue());
            consume(TokenType::IDENTIFIER, "Expected variable name");
        }

        // 4. Expect the closing semicolon.
        consume(";", "Expected ';' at the end of variable declaration");

        return std::make_unique<VarDecNode>(type,std::move(names), line, col);
    }

    std::vector<std::unique_ptr<StatementNode>> Parser::parseStatements() {
        std::vector<std::unique_ptr<StatementNode>> list;
        while (!check("}")) {
            list.push_back(parseStatement());
        }
        return list;
    }

    std::unique_ptr<StatementNode> Parser::parseStatement() {
        if (check("let")) {
            return parseLetStatement();
        }
        if (check("if")) {
            return parseIfStatement();
        }
        if (check("while")) {
            return parseWhileStatement();
        }
        if (check("do")) {
            return parseDoStatement();
        }
        if (check("return")) {
            return parseReturnStatement();
        }

        // The 'Junk' Handler:
        tokenizer.errorAt(currentToken->getLine(),currentToken->getColumn(), "Unknown statement or unexpected text");
    }

    std::unique_ptr<LetStatementNode> Parser::parseLetStatement() {
        // Grammar: 'let' varName ('[' expression ']')? '=' expression ';'
        int line = currentToken->getLine();
        int col = currentToken->getColumn();

        //move past let:
        consume("let","Expected a 'let' keyword");

        //get the variable name
        std::string_view varName=currentToken->getValue();
        consume(TokenType::IDENTIFIER,"Expected variable name");

        std::unique_ptr<ExpressionNode> indexExpr=nullptr; ///< The index expression for array assignment (optional).
        if (check("[")) {
            advance();
            indexExpr=parseExpression();
            consume("]","Expected ']' to close array index");
        }
        // Checkpoint 2: If we didn't see '[', we MUST see '='.
        // If we see an identifier here, it's a specific error.
        else if (check(TokenType::IDENTIFIER)) {
            tokenizer.errorAt(currentToken->getLine(), currentToken->getColumn(),
                              "Unexpected identifier; perhaps you forgot a '[' for an array?");
        }
        // If we see anything else that isn't '=', it's a general assignment error.
        else if (!check("=")) {
            tokenizer.errorAt(currentToken->getLine(), currentToken->getColumn(),
                              "Expected '=' after variable name");
        }

        consume("=","Expected an `=`");

        std::unique_ptr<ExpressionNode> exp=parseExpression();

        consume(";", "Expected ';' at end of let statement");

        return std::make_unique<LetStatementNode>(varName, std::move(indexExpr), std::move(exp), line, col);
    }

    std::unique_ptr<IfStatementNode> Parser::parseIfStatement() {
        // Grammar: 'if' '(' expression ')' '{' statements '}' ('else' '{' statements '}')?
        int line = currentToken->getLine();
        int col = currentToken->getColumn();

        consume("if", "Expected 'if' keyword");

        // 1. Condition Header '('
        consume("(", "Expected '(' after 'if'");
        std::unique_ptr<ExpressionNode> condition=parseExpression();
        // 2. Condition Closer ')'
        // If it's missing, we check if they accidentally started the block '{' early.
        if (check("{")) {
            tokenizer.errorAt(currentToken->getLine(), currentToken->getColumn(),
                              "Missing ')' before opening brace '{'");
        }

        consume(")", "Expected ')' after if-condition");


        // 3. If-Body '{ statements }'
        consume("{", "Expected '{' to start if-block");
        std::vector<std::unique_ptr<StatementNode>> ifStatements=parseStatements();
        consume("}", "Expected '}' to close if-block");

        std::vector<std::unique_ptr<StatementNode>> elseStatements;
        if (check("else")) {
            advance(); // consume 'else'
            consume("{", "Expected '{' to start else-block");
            elseStatements = parseStatements();
            consume("}", "Expected '}' to close else-block");
        }

        return std::make_unique<IfStatementNode>(std::move(condition), std::move(ifStatements), std::move(elseStatements), line, col);
    }


    std::unique_ptr<WhileStatementNode> Parser::parseWhileStatement() {
        // Grammar: 'while' '(' expression ')' '{' statements '}'
        int line = currentToken->getLine();
        int col = currentToken->getColumn();

        consume("while", "Expected 'while' keyword");

        // 1. Condition
        consume("(", "Expected '(' after 'while'");
        std::unique_ptr<ExpressionNode> condition = parseExpression();
        if (check("{")) {
            tokenizer.errorAt(currentToken->getLine(), currentToken->getColumn(),"Missing ')' before opening brace '{'");
        }
        consume(")", "Expected ')' after while-condition");

        // 2. Body
        consume("{", "Expected '{' to start while-loop body");
        std::vector<std::unique_ptr<StatementNode>> body = parseStatements();
        consume("}", "Expected '}' to close while-loop body");

        return std::make_unique<WhileStatementNode>(std::move(condition), std::move(body), line, col);
    }

    std::unique_ptr<ReturnStatementNode> Parser::parseReturnStatement() {
        // Grammar: 'return' expression? ';'
        int line = currentToken->getLine();
        int col = currentToken->getColumn();

        consume("return", "Expected 'return' keyword");

        std::unique_ptr<ExpressionNode> value = nullptr;

        // 1. Check if there is an expression to return.
        // In Jack, if the next token is not ';', it MUST be an expression.
        if (!check(";")) {
            // Handle "junk" or missing semicolon cases:
            // If we see a '}' or another statement keyword, they likely forgot the ';'
            if (check("}") || check("let") || check("if") || check("while") || check("do")) {
                tokenizer.errorAt(currentToken->getLine(), currentToken->getColumn(),
                                  "Missing ';' after return keyword");
            }
            // It's not a ';' and not a new block/statement, so it must be an expression.
            value = parseExpression();
        }

        // 2. Final check for the semicolon
        consume(";", "Expected ';' after return statement");

        return std::make_unique<ReturnStatementNode>(std::move(value), line, col);
    }


    std::unique_ptr<DoStatementNode> Parser::parseDoStatement() {
        //Grammar: `do' subroutineName '('expressionList')'|(className|varName)`.` subroutineName
        int line = currentToken->getLine();
        int col = currentToken->getColumn();

        consume("do","Expected 'do' keyword");
        std::unique_ptr<CallNode> call = parseSubroutineCall();
        consume(";", "Expected ';' after do subroutine call");
        return std::make_unique<DoStatementNode>(std::move(call), line, col);
    }

    std::unique_ptr<ExpressionNode> Parser::parseExpression() {
        // Grammar: term (op term)*
        // op: + - * / & | < > =
        int line = currentToken->getLine();
        int col = currentToken->getColumn();

        // 1. Compile the first term
        std::unique_ptr<ExpressionNode> left_term=parseTerm();

        // 2. Look for binary operators: +, -, *, /, &, |, <, >, =
        while (isBinaryOp()) {
            char op = currentToken->getValue()[0];
            advance(); // consume the operator
            std::unique_ptr<ExpressionNode> right_term = parseTerm();

            // Wrap the existing 'left' and the new 'right' into a new BinaryOpNode
            // This handles left-associativity (e.g., 1 + 2 + 3)
            left_term = std::make_unique<BinaryOpNode>(std::move(left_term), op, std::move(right_term), line, col);
        }

        return left_term;
    }

    bool Parser::isBinaryOp() const {
        // Helper to check if the current token is one of the Jack binary operators.
        if (currentToken->getType()!=TokenType::SYMBOL) {
            return false;
        }

        const std::string_view val = currentToken->getValue();

        return val == "+" || val == "-" || val == "*" || val == "/" ||
           val == "&" || val == "|" || val == "<" || val == ">" || val == "=";
    }


    std::unique_ptr<ExpressionNode> Parser::parseTerm() {
        // Grammar: integerConstant | stringConstant | keywordConstant | varName |
        //          varName '[' expression ']' | subroutineCall | '(' expression ')' | unaryOp term
        int line = currentToken->getLine();
        int col = currentToken->getColumn();

        // 1. Integer Constant
        if (check(TokenType::INT_CONST)) {
            const auto* intToken = static_cast<const IntToken*>(currentToken);// NOLINT(*-pro-type-static-cast-downcast)
            int32_t val = intToken->getInt();
            advance();
            return std::make_unique<IntegerLiteralNode>(val, line, col);
        }

        if (check(TokenType::FLOAT_CONST)) {
            const auto* floatToken = static_cast<const FloatToken*>(currentToken);// NOLINT(*-pro-type-static-cast-downcast)
            double val = floatToken->getFloat();
            advance();
            return std::make_unique<FloatLiteralNode>(val, line, col);
        }

        // 2. String Constant
        if (check(TokenType::STRING_CONST)) {
            std::string_view val = currentToken->getValue();
            advance();
            return std::make_unique<StringLiteralNode>(val, line, col);
        }

        // 3. Keyword Constant (true, false, null, this)
        if (currentToken->getType()==TokenType::KEYWORD) {
            const std::string_view val=currentToken->getValue();

            if (val == "true") {
                advance();
                return std::make_unique<KeywordLiteralNode>(Keyword::TRUE_, line, col);
            } else if (val == "false") {
                advance();
                return std::make_unique<KeywordLiteralNode>(Keyword::FALSE_, line, col);
            } else if (val == "null") {
                advance();
                return std::make_unique<KeywordLiteralNode>(Keyword::NULL_, line, col);
            } else if (val == "this") {
                advance();
                return std::make_unique<KeywordLiteralNode>(Keyword::THIS_, line, col);
            }else {
                tokenizer.errorAt(currentToken->getLine(),currentToken->getColumn(),"Inappropriate keyword used in expression.");
            }
        }


        // 4. Identifier (Variable, Array Access, or Subroutine Call)
        if (check(TokenType::IDENTIFIER)) {
            std::string_view name = currentToken->getValue();

            // Use PEEK to distinguish between x, x[i], and x.method()
            const Token& next = tokenizer.peek();
            if (next.getValue()=="[") {
                // Array Access: varName '[' expression ']'
                advance(); //consume name
                advance(); // consume '['
                std::unique_ptr<ExpressionNode> exp=parseExpression();
                consume("]", "Expected ']' after array index");
                return std::make_unique<IdentifierNode>(name, line, col, std::move(exp));
            }else if (next.getValue()=="("||next.getValue()==".") {
                // Subroutine Call
                return parseSubroutineCall();
            }else {
                // Simple Variable
                advance();
                return std::make_unique<IdentifierNode>(name, line, col);
            }
        }

        // 5. Parenthesized Expression: '(' expression ')'
        if (check("(")) {
            advance();
            std::unique_ptr<ExpressionNode> expr = parseExpression();
            consume(")", "Expected ')' to close expression");
            return expr;
        }

        // 6. Unary Operators: '-' or '~'
        if (check("-") || check("~")) {
            char op = currentToken->getValue()[0];
            advance();
            std::unique_ptr<ExpressionNode> term = parseTerm();
            return std::make_unique<UnaryOpNode>(op, std::move(term), line, col);
        }

        const std::string err = "Expected an expression term, but found '" + std::string(currentToken->getValue()) + "'";
        tokenizer.errorAt(currentToken->getLine(), currentToken->getColumn(), err);
    }


    std::vector<std::unique_ptr<ExpressionNode>> Parser::parseExpressionList() {
        std::vector<std::unique_ptr<ExpressionNode>> list;

        // 1. Handle the empty list case: do method()
        if (check(")")) {
            return list;
        }

        // 2. Parse the first mandatory expression (must exist if not followed immediately by ')'
        list.push_back(parseExpression());

        // 3. Loop to handle subsequent comma-separated expressions
        while (true) {
            if (check(",")) {
                advance(); // Consume the comma
                list.push_back(parseExpression());
            }
            else if (check(")")) {
                // Success: reached the end of the argument list
                break;
            }
            else {
                // FOCUSED ERROR: If we find a new term/junk without a comma or closing ')'.
                // Because errorAt throws an exception, this safely terminates the parse.
                tokenizer.errorAt(currentToken->getLine(), currentToken->getColumn(),
                                  "Expected ',' between arguments");
            }
        }

        return list;
    }

    std::unique_ptr<CallNode> Parser::parseSubroutineCall() {
        int line = currentToken->getLine();
        int col = currentToken->getColumn();

        // Save the first identifier to determine context later
        const std::string_view firstPart = currentToken->getValue();
        consume(TokenType::IDENTIFIER, "Expected subroutine, class, or variable name");

        std::string_view classNameOrVar;
        std::string_view subroutineName;

        // 1. Check for the dot '.' symbol (indicates a call on an object or a static class method)
        if (check(".")) {
            advance(); // Move past '.'
            classNameOrVar = firstPart; // The first part was the class/variable name
            subroutineName = currentToken->getValue();
            consume(TokenType::IDENTIFIER, "Expected subroutine name after '.'");
        } else {
            // 2. Direct call (e.g., draw()): The first part was the actual subroutine name
            subroutineName = firstPart;
        }

        // 3. Parse the argument list enclosed in parentheses
        consume("(", "Expected '(' for argument list");

        // Call our helper to parse zero or more expressions
        std::vector<std::unique_ptr<ExpressionNode>> agrs = parseExpressionList();

        // Ensure the argument list is properly closed
        consume(")", "Expected ')' to close argument list");

        // Return the AST node with all captured information
        return std::make_unique<CallNode>(classNameOrVar, subroutineName, std::move(agrs), line, col);
    }
    
}
