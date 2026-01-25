//
// Created by Nithin Kondabathini on 24/1/2026.
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

#include "../../Compiler/Tokenizer/Tokenizer.h"
#include "../../Compiler/Parser/Parser.h"
#include "../../Compiler/Parser/AST.h"
#include "../../Compiler/SemanticAnalyser/GlobalRegistry.h"
#include "../../Compiler/SemanticAnalyser/SemanticAnalyser.h"

using namespace nand2tetris::jack;
namespace fs = std::filesystem;

std::mutex consoleMutex;

void log(const std::string& msg) {
    std::scoped_lock lock(consoleMutex);
    std::cout << msg << std::endl;
}

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


CompilationUnit parseJob(const std::string& filePath, GlobalRegistry* registry) {
    auto tokenizer = std::make_unique<Tokenizer>(filePath);
    Parser parser(*tokenizer,*registry);
    auto ast = parser.parse();

    if (ast) {
        registry->registerClass(ast->getClassName());
    }

    log("[Parsed] " + filePath);
    return {std::move(tokenizer), std::move(ast), filePath};
}

void analyzeJob(const ClassNode* ast, const GlobalRegistry* registry) {
    SemanticAnalyser analyser(*registry);
    analyser.analyseClass(*ast);
    log("[Verified] class " + std::string(ast->getClassName()));
}

int main(const int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./jack_semantic <files...>" << std::endl;
        return 1;
    }

    try {
        const auto startTotal = std::chrono::high_resolution_clock::now();

        GlobalRegistry registry;

        std::vector<std::string> userFiles;
        fs::path inputPathArg = argv[1];
        for (int i = 1; i < argc; ++i) {
            userFiles.emplace_back(argv[i]);
        }

        std::vector<std::future<CompilationUnit>> userTasks;
        // Phase 1: Parse
        const auto startParse = std::chrono::high_resolution_clock::now();
        userTasks.reserve(userFiles.size());
            for (const auto& f : userFiles) {
                userTasks.push_back(std::async(std::launch::async, parseJob, f, &registry));
            }


        std::vector<CompilationUnit> units;
        std::vector<ClassNode*> userASTs;

        for (auto& t : userTasks) {
            auto unit = t.get();
            userASTs.push_back(unit.ast.get());
            units.push_back(std::move(unit));
        }
        const auto endParse = std::chrono::high_resolution_clock::now();

        // Phase 2: Analyze
        const auto startAnalyze = std::chrono::high_resolution_clock::now();
        std::vector<std::future<void>> analysisTasks;
        analysisTasks.reserve(userASTs.size());
        for (const auto* ast : userASTs) {
            analysisTasks.push_back(std::async(std::launch::async, analyzeJob, ast, &registry));
        }

        for (auto& t : analysisTasks) t.get();
        const auto endAnalyze = std::chrono::high_resolution_clock::now();

        const auto endTotal = std::chrono::high_resolution_clock::now();

        std::cout << "Build Complete." << std::endl;
        std::cout << "Parsing Time:  " << std::chrono::duration<double, std::milli>(endParse - startParse).count() << " ms" << std::endl;
        std::cout << "Analysis Time: " << std::chrono::duration<double, std::milli>(endAnalyze - startAnalyze).count() << " ms" << std::endl;
        std::cout << "Total Time:    " << std::chrono::duration<double, std::milli>(endTotal - startTotal).count() << " ms" << std::endl;
        std::cout << "Peak Memory:   " << getPeakMemoryMB() << " MB" << std::endl;

        // DUMP THE REGISTRY
        fs::path jsonPath;
        std::string jsonFilename = inputPathArg.stem().string() + "_registry.json"; // e.g., StressTest_registry.json

        if (fs::is_directory(inputPathArg)) {
            // If input was a directory 'MyDir', file becomes 'MyDir/MyDir_registry.json'
            jsonPath = inputPathArg / jsonFilename;
        } else {
            // If input was 'tests/MyFile.jack', file becomes 'tests/MyFile_registry.json'
            jsonPath = inputPathArg.parent_path() / jsonFilename;
        }

        registry.dumpToJSON(jsonPath.string());
        std::cout << "[Debug] Registry dumped to: " << jsonPath.string() << std::endl;

        // --- AUTO-LAUNCH PYTHON VISUALIZER ---
        fs::path scriptPath = "../tools/global_registry_viz.py";
        if (!fs::exists(scriptPath)) {
            scriptPath = "tools/global_registry_viz.py"; // Fallback for root execution
        }

        if (fs::exists(scriptPath)) {
            std::cout << "[UI] Launching Visualizer..." << std::endl;
            // Command: python3 <script> <json_path>
            std::string cmd = "python3 \"" + scriptPath.string() + "\" \"" + jsonPath.string() + "\"";
            std::system(cmd.c_str());
        } else {
            std::cout << "[UI] Warning: Could not find " << scriptPath << ". Visualizer skipped." << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}