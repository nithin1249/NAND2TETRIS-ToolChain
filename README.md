# Nand2Tetris: Jack Compiler 

![Language](https://img.shields.io/badge/language-C%2B%2B17-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey)
![Architecture](https://img.shields.io/badge/architecture-Modular-orange.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)

A high-performance, cross-platform compiler for the Jack programming language.

This project goes beyond the standard Nand2Tetris course requirements by implementing a **modern, modular compilation pipeline**. It serves as a robust Frontend Compiler, currently configured to translate high-level `.jack` source code into optimized Virtual Machine (`.vm`) intermediate code, but designed to target any architecture.

**[📥 Download Latest Release](https://github.com/nithin1249/NAND2TETRIS-ToolChain/releases)** 

---

## 🚀 Key Features & Performance

* **🧩 Modular Backend Architecture:** The compiler is architected with strict separation of concerns. The Code Generator is a swappable module; as long as the Interface is respected, the compiler can be retargeted to output WebAssembly, LLVM IR, or native binary without touching the frontend.
* **⚡ Zero-Copy String Processing:** Utilizes `std::string_view` throughout the Tokenizer and Parser to eliminate redundant memory allocations, significantly reducing heap usage during compilation.
* **🧵 Parallel Compilation:** Implements multi-threading (via `std::future`) to compile classes in parallel, utilizing all available CPU cores.
* **🔍 Semantic Analysis:** Includes a dedicated semantic pass that validates type safety, variable scope, and class existence *before* code generation.
* **🛠 Visualization Suite:** Built-in tools to visualize the Abstract Syntax Tree (AST) and inspect the Global Symbol Registry in real-time.

---

## 🏗 Architectural Pipeline

The compiler follows a four-stage pipeline designed for extensibility:

### 1. Tokenization (Lexical Analysis)
* **Mechanism:** A custom, regex-free state machine.
* **Detail:** Scans the source code character-by-character to produce tokens. By avoiding standard regex libraries, the tokenizer achieves maximum throughput with minimal overhead.

### 2. Parsing (Syntax Analysis)
* **Mechanism:** Recursive Descent Parser with LL(1) lookahead.
* **Detail:** Constructs a full Abstract Syntax Tree (AST) using `std::unique_ptr`. This stage captures the *intent* of the code in a format that is completely independent of the final output language.

### 3. Semantic Analysis (The "Modern" Layer)
* **Mechanism:** Global Symbol Registry & Scope Checking.
* **Detail:** A dedicated pass builds symbol tables for all classes, methods, and variables. It catches complex errors like "undefined variable" or "type mismatch" that simple one-pass compilers miss.

### 4. Modular Code Generation (The Interface)
* **Mechanism:** AST Traversal $\rightarrow$ Interface $\rightarrow$ VM Emission.
* **Detail:** The compiler defines a strict **Code Generation Interface**. The current implementation plugs into this interface to emit Hack VM code. However, this module can be swapped entirely to target different virtual machines or hardware architectures while reusing 100% of the frontend analysis.

---

## 🔮 Future Roadmap: The Binary Backend

Because of the **Modular Architecture** described above, our roadmap involves swapping the current "VM Writer" module with a "Native Writer" module.

**In Development:**
We are working on a backend extension that translates the parsed AST directly into **Native Machine Code** (Binary). This demonstrates the power of the modular design: transforming the tool from a VM-translator into a true native compiler.

---

## 📦 Installation

### 1. Download
Get the `JackCompiler_Release.zip` from the **[Releases Page](https://github.com/nithin1249/NAND2TETRIS-ToolChain/releases)**.

Python Dependencies:
1. textual>=0.40.0
2. pywebview>=4.0.0

### 2. FOLDER CONTENTS of ZIP File:
- bin/      : Compiler binaries for Windows, macOS, and Linux.
- os/       : The Jack Standard Library (Math, Array, etc.).
- tools/    : Visualization scripts (Python + D3.js).
- JackCode/ : Example Jack programs.


### 3. Install

#### Windows
1. Extract the zip file.
2. Double-click **`install.bat`**.
3. Follow the prompt to add the installation folder to your `PATH`.

#### macOS / Linux
1. Extract the zip file.
2. Open your terminal in the extracted folder.
3. Run the installer:
   ```bash
   chmod +x install.sh
   ./install.sh

### 4. USAGE

1. Compile a project (produces .vm files):
   jack <path_to_project_folder>

2. Visualize the Syntax Tree (AST):
   jack <path_to_project_folder> --viz-ast

3. Inspect Symbol Tables & Semantic Analysis:
   jack <path_to_project_folder> --viz-checker

