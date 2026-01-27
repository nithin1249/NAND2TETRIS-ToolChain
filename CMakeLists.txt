cmake_minimum_required(VERSION 4.0)
project(NAND2TETRIS)

set(CMAKE_CXX_STANDARD 26)

add_executable(NAND2TETRIS main.cpp
        Compiler/Tokenizer/Tokenizer.cpp
        Compiler/Tokenizer/Tokenizer.h
        Compiler/Tokenizer/TokenTypes.h
        Compiler/Parser/AST.h
        Compiler/Parser/Parser.cpp
        Compiler/Parser/Parser.h
        Compiler/SemanticAnalyser/SymbolTable.cpp
        Compiler/SemanticAnalyser/SymbolTable.h
        Compiler/SemanticAnalyser/GlobalRegistry.cpp
        Compiler/SemanticAnalyser/GlobalRegistry.h
        Compiler/SemanticAnalyser/SemanticAnalyser.cpp
        Compiler/SemanticAnalyser/SemanticAnalyser.h
        Compiler/VMWriter/VMWriter.cpp
        Compiler/VMWriter/VMWriter.h
        Compiler/CodeGenerator/CodeGenerator.cpp
        Compiler/CodeGenerator/CodeGenerator.h
)

add_executable(jack_tokens
        tests/JackTokensTest/jack_tokens.cpp
        Compiler/Tokenizer/Tokenizer.cpp
        Compiler/Tokenizer/Tokenizer.h
        Compiler/Tokenizer/TokenTypes.h
)
add_executable(jack_parser
        Compiler/Tokenizer/Tokenizer.cpp
        Compiler/Tokenizer/Tokenizer.h
        Compiler/Tokenizer/TokenTypes.h
        Compiler/SemanticAnalyser/GlobalRegistry.h
        Compiler/SemanticAnalyser/GlobalRegistry.cpp
        Compiler/Parser/AST.h
        Compiler/Parser/Parser.cpp
        Compiler/Parser/Parser.h
        tests/JackParserTest/jack_parser.cpp
)

add_executable(jack_semantic
        Compiler/Tokenizer/Tokenizer.cpp
        Compiler/Tokenizer/Tokenizer.h
        Compiler/Tokenizer/TokenTypes.h
        Compiler/Parser/AST.h
        Compiler/Parser/Parser.cpp
        Compiler/Parser/Parser.h
        Compiler/SemanticAnalyser/SymbolTable.cpp
        Compiler/SemanticAnalyser/SymbolTable.h
        Compiler/SemanticAnalyser/GlobalRegistry.cpp
        Compiler/SemanticAnalyser/GlobalRegistry.h
        Compiler/SemanticAnalyser/SemanticAnalyser.cpp
        Compiler/SemanticAnalyser/SemanticAnalyser.h
        tests/JackSemanticAnalyser/jack_semantic.cpp
)

add_executable(jack_codegen
        Compiler/Tokenizer/Tokenizer.cpp
        Compiler/Tokenizer/Tokenizer.h
        Compiler/Tokenizer/TokenTypes.h
        Compiler/Parser/AST.h
        Compiler/Parser/Parser.cpp
        Compiler/Parser/Parser.h
        Compiler/SemanticAnalyser/SymbolTable.cpp
        Compiler/SemanticAnalyser/SymbolTable.h
        Compiler/SemanticAnalyser/GlobalRegistry.cpp
        Compiler/SemanticAnalyser/GlobalRegistry.h
        Compiler/SemanticAnalyser/SemanticAnalyser.cpp
        Compiler/SemanticAnalyser/SemanticAnalyser.h
        Compiler/VMWriter/VMWriter.cpp
        Compiler/VMWriter/VMWriter.h
        Compiler/CodeGenerator/CodeGenerator.cpp
        Compiler/CodeGenerator/CodeGenerator.h
        tests/JackCodeGenTest/jack_codegen.cpp
)





