//
// Created by Nithin Kondabathini on 26/1/2026.
//

#ifndef NAND2TETRIS_VM_WRITER_H
#define NAND2TETRIS_VM_WRITER_H

#include <ostream>
#include <string>
namespace nand2tetris::jack {

	enum class Segment {CONST, ARG, LOCAL, STATIC, THIS, THAT, POINTER, TEMP};

	enum class Command {ADD, SUB, NEG, EQ, GT, LT, AND, OR, NOT};
	class VMWriter {
		public:
			explicit VMWriter(std::ostream &out);
			~VMWriter()=default;

			void writePush(Segment segment, int index);
			void writePop(Segment segment, int index);
			void writeArithmetic(Command command);

			void writeLabel(const std::string& label) const;
			void writeGoto(const std::string& label) const;
			void writeIf(const std::string& label) const;

			void writeCall(const std::string& name, int nArgs) const;
			void writeFunction(const std::string& name, int nLocals) const;
			void writeReturn() const;

			void writeStringConstant(const std::string_view& str);

		private:
			std::ostream& out;
			std::string segmentToString(Segment seg) ;
			std::string commandToString(Command cmd) ;

	};
}


#endif //NAND2TETRIS_VM_WRITER_H