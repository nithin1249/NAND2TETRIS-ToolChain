#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <future>
#include <mutex>
#include <filesystem>
#include <chrono>
#include <fstream>
#include <thread>
#include <functional>
#include <cstdlib>



#include "Compiler/Tokenizer/Tokenizer.h"
#include "Compiler/Parser/Parser.h"
#include "Compiler/Parser/AST.h"
#include "Compiler/SemanticAnalyser/GlobalRegistry.h"
#include "Compiler/SemanticAnalyser/SemanticAnalyser.h"
#include "Compiler/CodeGenerator/CodeGenerator.h"


#ifdef _WIN32
	#include <windows.h>
	#include <psapi.h>
	#include <process.h>
#else
	#include <sys/resource.h>
	#include <unistd.h>
#endif

using namespace nand2tetris::jack;
namespace fs = std::filesystem;

// Global mutex for thread-safe console logging
std::mutex consoleMutex;

// Thread-safe logging function
void log(const std::string& msg) {
	std::scoped_lock lock(consoleMutex);
	std::cout << msg << std::endl;
}

// Memory usage helper (Peak RSS in MB)
// Returns the peak resident set size (memory usage) of the process.
double getPeakMemoryMB() {
#ifdef _WIN32
	PROCESS_MEMORY_COUNTERS pmc;
	if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
		// PeakWorkingSetSize is in bytes, convert to MB
		return static_cast<double>(pmc.PeakWorkingSetSize) / (1024.0 * 1024.0);
	}
	return 0.0;
#else
	struct rusage rusage;
	getrusage(RUSAGE_SELF, &rusage);
	#ifdef __APPLE__
		return static_cast<double>(rusage.ru_maxrss) / (1024.0 * 1024.0);
	#else
		return static_cast<double>(rusage.ru_maxrss) / 1024.0; // Linux uses KB
	#endif
#endif
}

// This struct holds the entire lifecycle state of a single .jack file.
// It keeps the Tokenizer (source string owner), AST, and SymbolTable alive.
struct CompilationUnit {
	std::string filePath;
	std::unique_ptr<Tokenizer> tokenizer;
	std::unique_ptr<ClassNode> ast;
	std::shared_ptr<SymbolTable> symbolTable;
};

// Job 1: Parse
// Reads the file, tokenizes it, and builds the AST.
// Also registers the class and its methods into the GlobalRegistry.
CompilationUnit parseJob(const std::string& filePath, GlobalRegistry* registry) {
	auto tokenizer = std::make_unique<Tokenizer>(filePath);
	const auto symbolTable = std::make_shared<SymbolTable>();
	Parser parser(*tokenizer, *registry);
	auto ast = parser.parse();
	log("[Parsed]    " + filePath);
	return {filePath, std::move(tokenizer), std::move(ast),symbolTable};
};

// Job 2: Analyze
// Performs semantic analysis (type checking, scope resolution) on the AST.
void analyzeJob(const CompilationUnit& unit, const GlobalRegistry* registry) {
	if (!unit.ast) return; // Skip if parse failed
	SemanticAnalyser analyser(*registry);
	analyser.analyseClass(*unit.ast,*unit.symbolTable);
	log("[Verified]  " + unit.filePath);
}

// Job 3: Compile
// Generates VM code from the AST and writes it to a .vm file.
void compileJob(const CompilationUnit& unit, const GlobalRegistry* registry) {
	if (!unit.ast) return;

	fs::path p(unit.filePath);
	const fs::path outputPath = p.replace_extension(".vm");

	std::ofstream out(outputPath);
	if (!out) {
		throw std::runtime_error("Could not open output file: " + outputPath.string());
	}

	CodeGenerator generator(*registry, out,*unit.symbolTable);
	generator.compileClass(*unit.ast);

	log("[Generated] " + outputPath.string());
}

// Validates that the Main class has a static void main() function.
// This is the entry point of a Jack program.
void validateMainEntry(const GlobalRegistry& registry) {
	try {
		// 1. Fetch the signature from the registry
		const auto sig = registry.getSignature("Main", "main");

		// 2. Check: Must be Static (Function)
		if (!sig.isStatic) {
			throw std::runtime_error("Error: 'Main.main' must be a static function, not a method or constructor.");
		}

		// 3. Check: Must return Void
		if (sig.returnType != "void") {
			throw std::runtime_error("Error: 'Main.main' must have a 'void' return type.");
		}

	} catch (const std::exception& e) {
		// If getSignature throws (e.g., class or method not found), catch it here
		throw std::runtime_error("Error: Verification failed for 'Main.main'.\nDetails: " + std::string(e.what()));
	}
}

// Helper to find the 'tools' directory for visualization scripts.
std::string getToolsDir() {
	fs::path homeDir;

	// Check Env Vars
#ifdef _WIN32
	const char* userProfile = std::getenv("USERPROFILE");
	if (userProfile) homeDir = userProfile;
#else
	const char* home = std::getenv("HOME");
	if (home) homeDir = home;
#endif

	// Check Installed Location (~/.jack_toolchain/tools)
	if (!homeDir.empty()) {
		const fs::path installedTools = homeDir / ".jack_toolchain" / "tools";
		if (fs::exists(installedTools)) return installedTools.string();
	}

	return "";
}

// Helper to get a temporary file path.
fs::path getTempPath(const std::string& filename) {
	try {
		return fs::temp_directory_path() / filename;
	} catch (...) {
		return fs::path(filename); // Fallback to local dir
	}
}

// Launches the unified visualization dashboard (Registry + Symbol Tables).
void runUnifiedViz(const GlobalRegistry& registry, const std::vector<CompilationUnit>& units) {
	// 1. Dump Registry to Temp
	std::string regPath = getTempPath("jack_unified_reg.json").string();
	registry.dumpToJSON(regPath);

	// 2. Dump Symbol Tables for all files
	std::vector<std::string> symPaths;
	for (const auto& unit : units) {
		if (!unit.symbolTable) continue;

		// Create a unique filename for each symbol table
		size_t h = std::hash<std::string>{}(unit.filePath);
		std::string name = fs::path(unit.filePath).stem().string();
		std::string path = "/tmp/jack_sym_" + name + "_" + std::to_string(h) + ".json";

		unit.symbolTable->dumpToJSON(name, path);
		symPaths.push_back(path);
	}

	// 3. Locate the Python Script
	std::string toolsDir = getToolsDir();
	if (toolsDir.empty()) {
		std::cerr << "Error: 'tools' folder not found. Cannot launch visualization." << std::endl;
		return;
	}

	fs::path script = fs::path(toolsDir) / "unified_viz.py";
	std::string absScriptPath = fs::absolute(script).string();

	// 4. Construct Command
	std::string cmd;
	#ifdef _WIN32
		cmd = "python \"" + absScriptPath + "\" --registry \"" + regPath + "\"";
	#else
		cmd = "python3 \"" + absScriptPath + "\" --registry \"" + regPath + "\"";
	#endif

	if (!symPaths.empty()) {
		cmd += " --symbols";
		for (const auto& p : symPaths) cmd += " \"" + p + "\"";
	}

	// 5. Run (Blocks until you close the dashboard)
	std::system(cmd.c_str());

	// 6. Cleanup Temp Files
	if (fs::exists(regPath)) fs::remove(regPath);
	for (const auto& p : symPaths) if (fs::exists(p)) fs::remove(p);
}

// Launches the AST visualization tool for all compiled units.
void runBatchAstViz(const std::vector<CompilationUnit>& units) {
	std::string toolsDir = getToolsDir();
	if (toolsDir.empty()) {
		std::cerr << "Error: 'tools' folder not found." << std::endl;
		return;
	}

	fs::path scriptPath = fs::path(toolsDir) / "jack_viz.py";
	std::string absScriptPath = fs::absolute(scriptPath).string();

	std::vector<std::string> tempFiles;
	std::string pyArgs = "";

	// 1. Generate ALL XML files
	for (const auto& unit : units) {
		if (!unit.ast) continue;

		// Create unique filename
		size_t pathHash = std::hash<std::string>{}(unit.filePath);
		std::string niceName = fs::path(unit.filePath).stem().string();
		std::string xmlFilename = niceName + "_" + std::to_string(pathHash) + ".xml";

		fs::path xmlPath = getTempPath(xmlFilename);

		std::cout << "Generated: " << xmlPath.string() << std::endl;

		std::ofstream xmlFile(xmlPath);
		unit.ast->printXml(xmlFile, 0);
		xmlFile.close();

		tempFiles.push_back(xmlPath.string());

		pyArgs += " \"" + xmlPath.string() + "\"";
	}
	if (tempFiles.empty()) return;

	// 2. Build the command
	std::string cmd;
	#ifdef _WIN32
		// Windows: start /b (background)
		cmd = "start /b python \"" + absScriptPath + "\"" + pyArgs;
	#else
		// Linux/Mac: python3, rm, &
		std::string cleanupCmd = "rm -f";
		for (const auto& f : tempFiles) cleanupCmd += " \"" + f + "\"";
		cmd = "(python3 \"" + absScriptPath + "\"" + pyArgs + " && " + cleanupCmd + ") &";
	#endif

	std::system(cmd.c_str());
}

int main(int argc, char* argv[]) {
	// Optimization: Disable C-style I/O synchronization for speed
	std::ios_base::sync_with_stdio(false);

	if (argc < 2) {
		std::cerr << "Usage: JackCompiler <file.jack or directory>" << std::endl;
		return 1;
	}

	try {
		const auto startTotal = std::chrono::high_resolution_clock::now();

		std::vector<std::string> userFiles;

		bool vizAst = false;
		bool vizSymbols = false;
		// Iterate through ALL command line arguments
		for (int i = 1; i < argc; ++i) {
			std::string arg = argv[i];
			if (arg == "--viz-ast") {
				vizAst = true;
				continue;
			}
			if (arg == "--viz-checker") {
				vizSymbols = true;
				continue;
			}

			fs::path inputPathArg = arg;

			if (!fs::exists(inputPathArg)) {
				std::cerr << "Error: Path does not exist: " << inputPathArg << std::endl;
				return 1;
			}

			if (inputPathArg.extension() != ".jack") {
				std::cerr << "Error: Invalid file type. Only .jack files are allowed." << std::endl;
				std::cerr << "Offending file: " << inputPathArg << std::endl;
				return 1;
			}

			userFiles.push_back(fs::absolute(inputPathArg).string());
		}

		if (userFiles.empty()) {
			std::cerr << "No files provided." << std::endl;
			return 1;
		}

		// Check for Main.jack
		bool hasMain = false;
		for (const auto& file : userFiles) {
			if (fs::path(file).filename() == "Main.jack") {
				hasMain = true;
				break;
			}
		}

		if (!hasMain) {
			std::cerr << "\nError: Compilation Failed." << std::endl;
			std::cerr << "Reason: Missing 'Main.jack'" << std::endl;
			std::cerr << "The list of files to compile must include the Main class." << std::endl;
			return 1;
		}


		GlobalRegistry registry;

		// --- PHASE 1: PARSING ---
		const auto startParse = std::chrono::high_resolution_clock::now();
		std::vector<std::future<CompilationUnit>> parseTasks;

		parseTasks.reserve(userFiles.size());
		for (const auto& f : userFiles) {
			parseTasks.push_back(std::async(std::launch::async, parseJob, f, &registry));
		}

		std::vector<CompilationUnit> units;
		for (auto& t : parseTasks) {
			auto unit = t.get();
			if (unit.ast) units.push_back(std::move(unit));
		}
		const auto endParse = std::chrono::high_resolution_clock::now();

		// Validate Entry Point
		validateMainEntry(registry);


		// --- PHASE 2: SEMANTIC ANALYSIS ---
		const auto startAnalyze = std::chrono::high_resolution_clock::now();
		std::vector<std::future<void>> analysisTasks;

		analysisTasks.reserve(units.size());
		for (const auto& unit : units) {
			analysisTasks.push_back(std::async(std::launch::async, analyzeJob, std::ref(unit), &registry));
		}

		for (auto& t : analysisTasks) {
			t.get();
		}
		const auto endAnalyze = std::chrono::high_resolution_clock::now();

		// --- PHASE 3: CODE GENERATION ---
		const auto startCodeGen = std::chrono::high_resolution_clock::now();
		std::vector<std::future<void>> compileTasks;

		compileTasks.reserve(units.size());
		for (auto& unit : units) {
			compileTasks.push_back(std::async(std::launch::async, compileJob, std::ref(unit), &registry));
		}

		for (auto& t : compileTasks) {
			t.get();
		}
		const auto endCodeGen = std::chrono::high_resolution_clock::now();
		const auto endTotal = std::chrono::high_resolution_clock::now();

		// --- REPORT ---
		std::cout << "\n========================================" << std::endl;
		std::cout << " BUILD SUCCESSFUL" << std::endl;
		std::cout << "========================================" << std::endl;
		std::cout << " Files Compiled: " << units.size() << std::endl;
		std::cout << " Parsing:        " << std::chrono::duration<double, std::milli>(endParse - startParse).count() << " ms" << std::endl;
		std::cout << " Static Analysis:" << std::chrono::duration<double, std::milli>(endAnalyze - startAnalyze).count() << " ms" << std::endl;
		std::cout << " Code Gen:       " << std::chrono::duration<double, std::milli>(endCodeGen - startCodeGen).count() << " ms" << std::endl;
		std::cout << " Total Time:     " << std::chrono::duration<double, std::milli>(endTotal - startTotal).count() << " ms" << std::endl;
		std::cout << " Peak Memory:    " << getPeakMemoryMB() << " MB" << std::endl;
		std::cout << "========================================" << std::endl;

		// --- VISUALIZATION ---
		if (vizAst) {
			runBatchAstViz(units);
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}


		if (vizSymbols) {
			runUnifiedViz(registry, units);
		}

	}catch (const std::exception& e) {
		std::cerr << "\n COMPILATION FAILED" << std::endl;
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}