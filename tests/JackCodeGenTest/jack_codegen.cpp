//
// Created by Nithin Kondabathini on 27/1/2026.
//

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <future>
#include <mutex>
#include <filesystem>
#include <chrono>
#include <sys/resource.h>
#include <fstream>

#include "../../Compiler/Tokenizer/Tokenizer.h"
#include "../../Compiler/Parser/Parser.h"
#include "../../Compiler/Parser/AST.h"
#include "../../Compiler/SemanticAnalyser/GlobalRegistry.h"
#include "../../Compiler/SemanticAnalyser/SemanticAnalyser.h"
#include "../../Compiler/CodeGenerator//CodeGenerator.h"

using namespace nand2tetris::jack;
namespace fs = std::filesystem;

std::mutex consoleMutex;

// Thread-safe logging
void log(const std::string& msg) {
    std::scoped_lock lock(consoleMutex);
    std::cout << msg << std::endl;
}

// Memory usage helper
double getPeakMemoryMB() {
    struct rusage rusage;
    getrusage(RUSAGE_SELF, &rusage);
#ifdef __APPLE__
    return static_cast<double>(rusage.ru_maxrss) / (1024.0 * 1024.0);
#else
    return (double)rusage.ru_maxrss / 1024.0;
#endif
}

struct CompilationUnit {
    std::unique_ptr<Tokenizer> tokenizer;
    std::unique_ptr<ClassNode> ast;
    std::string filePath;
};

// --- JOB 1: PARSE (Produces AST & Updates Registry) ---
CompilationUnit parseJob(const std::string& filePath, GlobalRegistry* registry) {
    auto tokenizer = std::make_unique<Tokenizer>(filePath);
    Parser parser(*tokenizer, *registry);
    auto ast = parser.parse();
    log("[Parsed] " + filePath);
    return {std::move(tokenizer), std::move(ast), filePath};
}

// --- JOB 2: ANALYZE (Read-Only Checks) ---
void analyzeJob(const ClassNode* ast, const GlobalRegistry* registry) {
    SemanticAnalyser analyser(*registry);
    analyser.analyseClass(*ast);
    log("[Verified] class " + std::string(ast->getClassName()));
}

// --- JOB 3: CODE GEN (Writes .vm files) ---
void compileJob(const ClassNode* ast, const std::string& inputPath, const GlobalRegistry* registry) {
    // 1. Determine Output Path: "Folder/MyClass.jack" -> "Folder/MyClass.vm"
    fs::path p(inputPath);
    fs::path outputPath = p.replace_extension(".vm");

    // 2. Open unique Output Stream for this thread
    std::ofstream out(outputPath);
    if (!out) {
        throw std::runtime_error("Could not open output file: " + outputPath.string());
    }

    // 3. Generate VM Code
    CodeGenerator generator(*registry, out);
    generator.compileClass(*ast);

    log("[Generated] " + outputPath.string());
}

int main(int argc, char* argv[]) {
    // Speed up C++ I/O
    std::ios_base::sync_with_stdio(false);

    if (argc < 2) {
        std::cerr << "Usage: ./JackCompiler <files...>" << std::endl;
        return 1;
    }

    try {
        const auto startTotal = std::chrono::high_resolution_clock::now();

        // 1. Collect Files (Simplified)
        std::vector<std::string> userFiles;
        for (int i = 1; i < argc; ++i) {
            userFiles.emplace_back(argv[i]);
        }

        GlobalRegistry registry;

        const auto startParse = std::chrono::high_resolution_clock::now();
        std::vector<std::future<CompilationUnit>> parseTasks;

        parseTasks.reserve(userFiles.size());
        for (const auto& f : userFiles) {
            parseTasks.push_back(std::async(std::launch::async, parseJob, f, &registry));
        }

        std::vector<CompilationUnit> units;
        for (auto& t : parseTasks) {
            auto unit = t.get();
            if (unit.ast) {
                units.push_back(std::move(unit));
            }
        }
        const auto endParse = std::chrono::high_resolution_clock::now();


        const auto startAnalyze = std::chrono::high_resolution_clock::now();
        std::vector<std::future<void>> analysisTasks;

        analysisTasks.reserve(units.size());
        for (const auto& unit : units) {
            analysisTasks.push_back(std::async(std::launch::async, analyzeJob, unit.ast.get(), &registry));
        }

        for (auto& t : analysisTasks) t.get();
        const auto endAnalyze = std::chrono::high_resolution_clock::now();


        const auto startCodeGen = std::chrono::high_resolution_clock::now();
        std::vector<std::future<void>> compileTasks;

        compileTasks.reserve(units.size());
        for (const auto& unit : units) {
            compileTasks.push_back(std::async(std::launch::async, compileJob, unit.ast.get(), unit.filePath, &registry));
        }

        for (auto& t : compileTasks) t.get();
        const auto endCodeGen = std::chrono::high_resolution_clock::now();
        const auto endTotal = std::chrono::high_resolution_clock::now();

        // --- REPORTING ---
        std::cout << "\n========================================" << std::endl;
        std::cout << " Compilation Successful." << std::endl;
        std::cout << " Files:         " << units.size() << std::endl;
        std::cout << " Parsing:       " << std::chrono::duration<double, std::milli>(endParse - startParse).count() << " ms" << std::endl;
        std::cout << " Analysis:      " << std::chrono::duration<double, std::milli>(endAnalyze - startAnalyze).count() << " ms" << std::endl;
        std::cout << " Code Gen:      " << std::chrono::duration<double, std::milli>(endCodeGen - startCodeGen).count() << " ms" << std::endl;
        std::cout << " Total Time:    " << std::chrono::duration<double, std::milli>(endTotal - startTotal).count() << " ms" << std::endl;
        std::cout << " Peak Memory:   " << getPeakMemoryMB() << " MB" << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Compiler Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}