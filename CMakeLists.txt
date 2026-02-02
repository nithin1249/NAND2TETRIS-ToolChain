cmake_minimum_required(VERSION 4.1.2)
project(NAND2TETRIS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Ensure strictly standard C++)


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

if(WIN32)
    target_link_libraries(NAND2TETRIS psapi)
endif()

if(APPLE)
    # On Mac -> produces "jack_mac"
    set_target_properties(NAND2TETRIS PROPERTIES OUTPUT_NAME "NAND2TETRIS_mac")
elseif(WIN32)
    # On Windows -> produces "jack_win.exe" (CMake adds .exe automatically)
    set_target_properties(NAND2TETRIS PROPERTIES OUTPUT_NAME "JackCompiler_Release/bin/NAND2TETRIS_win.exe")
elseif(UNIX)
    # On Linux -> produces "jack_linux"
    set_target_properties(NAND2TETRIS PROPERTIES OUTPUT_NAME "JackCompiler_Release/bin/NAND2TETRIS_linux")
endif()





