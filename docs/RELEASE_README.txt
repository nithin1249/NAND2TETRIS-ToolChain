Jack Compiler Toolchain (Nand2Tetris)
=====================================

A high-performance, cross-platform compiler for the Jack programming language.

This tool functions as a modern "Frontend Compiler," translating high-level Jack source code (.jack) into optimized Virtual Machine intermediate code (.vm).

ARCHITECTURAL HIGHLIGHTS
------------------------
This compiler implements a sophisticated, multi-stage compilation pipeline rarely seen in basic implementations:

1. Syntax Analysis (Parse Tree):
   First, the code is parsed into a complete Abstract Syntax Tree (AST), ensuring rigorous syntax validation before any code is generated.

2. Semantic Analysis (Global Registry):
   Unlike simple one-pass compilers, this tool performs a Semantic Analysis pass across ALL files in your project. It resolves symbol types, checks class scope, and validates variable usage globally.

3. Intermediate Code Generation:
   Finally, it synthesizes the analyzed data to produce clean, efficient VM code (.vm) ready for the Hack Virtual Machine or future binary translation.

*Future Roadmap:* An extension is currently in development to translate this VM code directly into native binary/machine code.

-------------------------------------------------------------------------------

INSTALLATION
------------

Python Dependencies:
textual>=0.40.0
pywebview>=4.0.0

Windows:
1. Extract the zip file.
2. Right-click `install.bat` and select Run as Administrator. (This automatically sets up the global `jack` command for
you.)

macOS / Linux:
1. Extract the zip file.
2. Open your terminal in the extracted folder.
3. Run the installer: ./install.sh

USAGE
-----
1. Compile a project (produces .vm files):
   jack <path_to_project_folder>

2. Visualize the Syntax Tree (AST):
   jack <path_to_project_folder> --viz-ast

3. Inspect Symbol Tables & Semantic Analysis:
   jack <path_to_project_folder> --viz-checker

FOLDER CONTENTS
---------------
- bin/      : Compiler binaries for Windows, macOS, and Linux.
- os/       : The Jack Standard Library (Math, Array, etc.).
- tools/    : Visualization scripts (Python + D3.js).
- JackCode/ : Example Jack programs.