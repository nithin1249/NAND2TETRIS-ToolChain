//
// Created by Nithin Kondabathini on 26/1/2026.
//

#include "VMWriter.h"

namespace nand2tetris::jack {
	VMWriter::VMWriter(std::ostream &out):out(out) {};

	// ReSharper disable once CppMemberFunctionMayBeStatic
	std::string VMWriter::segmentToString(const Segment seg) { // NOLINT(*-convert-member-functions-to-static)
		switch (seg) {
			case Segment::CONST: return "constant";
			case Segment::ARG:     return "argument";
			case Segment::LOCAL:   return "local";
			case Segment::STATIC:  return "static";
			case Segment::THIS:    return "this";
			case Segment::THAT:    return "that";
			case Segment::POINTER: return "pointer";
			case Segment::TEMP:    return "temp";
		}
		return "unknown";
	}
	// ReSharper disable once CppMemberFunctionMayBeStatic
	std::string VMWriter::commandToString(const Command cmd) {// NOLINT(*-convert-member-functions-to-static)
		switch (cmd) {
			case Command::ADD: return "add";
			case Command::SUB: return "sub";
			case Command::NEG: return "neg";
			case Command::EQ:  return "eq";
			case Command::GT:  return "gt";
			case Command::LT:  return "lt";
			case Command::AND: return "and";
			case Command::OR:  return "or";
			case Command::NOT: return "not";
		}
		return "unknown";
	}

	void VMWriter::writePush(const Segment segment, const int index) {
		out << "push " << segmentToString(segment) << " " << index << "\n";
	}

	void VMWriter::writePop(const Segment segment, const int index) {
		out << "pop " << segmentToString(segment) << " " << index << "\n";
	}

	void VMWriter::writeArithmetic(Command command) {
		out << commandToString(command) << "\n";
	}

	void VMWriter::writeLabel(const std::string &label) const {
		out << "label " << label << "\n";
	}

	void VMWriter::writeGoto(const std::string &label) const {
		out << "goto " << label << "\n";
	}

	void VMWriter::writeIf(const std::string& label) const {
		out << "if-goto " << label << "\n";
	}

	void VMWriter::writeCall(const std::string& name, int nArgs) const {
		out << "call " << name << " " << nArgs << "\n";
	}

	void VMWriter::writeFunction(const std::string& name, int nLocals) const {
		out << "function " << name << " " << nLocals << "\n";
	}

	void VMWriter::writeReturn() const {
		out << "return\n";
	}

	void VMWriter::writeStringConstant(const std::string_view& str) {
		// 1. Push length of string
		writePush(Segment::CONST, static_cast<int>(str.length()));

		// 2. Call String.new(length) -> Returns string object pointer
		writeCall("String.new", 1);

		// 3. Append characters one by one
		for (const char c : str) {
			// Push the character code
			writePush(Segment::CONST, static_cast<int>(c));
			// Call String.appendChar(this, char)
			// Note: appendChar returns 'this', so the stack stays valid for the next call
			writeCall("String.appendChar", 2);
		}
	}


}
