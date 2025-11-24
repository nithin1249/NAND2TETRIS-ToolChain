//
// Created by Nithin Kondabathini on 23/11/2025.
//

#ifndef NAND2TETRIS_TOKEN_H
#define NAND2TETRIS_TOKEN_H

#include<string>
#include <utility>
#include <cstddef>

namespace nand2tetris::jack {
	enum class TokenType {
		KEYWORD,
		SYMBOL,
		IDENTIFIER,
		INT_CONST,
		STRING_CONST,
		END_OF_FILE
	};

	enum class Keyword {
		CLASS,METHOD,FUNCTION,CONSTRUCTOR,
		INT, BOOLEAN, CHAR, VOID,
		VAR, STATIC, FIELD, LET, DO, IF,
		ELSE, WHILE, RETURN, TRUE_, FALSE_, NULL_, THIS_

	};
	inline const char* keywordToString(const Keyword kw) {
		using K = Keyword;
		switch (kw) {
			case K::CLASS:       return "class";
			case K::METHOD:      return "method";
			case K::FUNCTION:    return "function";
			case K::CONSTRUCTOR: return "constructor";
			case K::INT:         return "int";
			case K::CHAR:        return "char";
			case K::BOOLEAN:     return "boolean";
			case K::VOID:        return "void";
			case K::VAR:         return "var";
			case K::STATIC:      return "static";
			case K::FIELD:       return "field";
			case K::LET:         return "let";
			case K::DO:          return "do";
			case K::IF:          return "if";
			case K::ELSE:        return "else";
			case K::WHILE:       return "while";
			case K::RETURN:      return "return";
			case K::TRUE_:       return "true";
			case K::FALSE_:      return "false";
			case K::NULL_:       return "null";
			case K::THIS_:       return "this";
		}
		return "<unknown>";
	}


	struct Token {
		private:
			TokenType type=TokenType::END_OF_FILE;
			std::size_t line;
			std::size_t column;

		public:
			explicit Token(const TokenType t, const std::size_t line, const std::size_t column):type(t),line(line),column(column){}
			TokenType getType() const {return type;}
			std::size_t getLine() const { return line; }
			std::size_t getColumn() const { return column; }

			// For debugging / error messages
			virtual std::string toString() const = 0;

			virtual ~Token() = default;
	};

	struct TextToken final : public Token {
		private:
			std::string text;

		public:
			TextToken(const TokenType t, std::string text,
				const std::size_t line, const std::size_t column ):Token(t,line,column),text(std::move(text)){};
			std::string getText() const {return text;}
			std::string toString() const override {return text;}
	};

	struct IntToken final : public Token {
		private:
			int intVal=0;
		public:
			explicit IntToken(const int val,
				const std::size_t line, const std::size_t column):Token(TokenType::INT_CONST,line, column), intVal(val){}
			int getInt() const {return intVal;}
			std::string toString() const override {return std::to_string(intVal);}
	};

	struct KeywordToken final : public Token {
		private:
			Keyword keyword{};
		public:
			explicit KeywordToken(const Keyword keyword,
				const std::size_t line, const std::size_t column):Token(TokenType::KEYWORD,line,column), keyword(keyword){}
			Keyword getKeyword() const {return keyword;}
			std::string toString() const override {return keywordToString(keyword);}
	};

	struct EofToken final : public Token {
		EofToken(const std::size_t line, const std::size_t column) : Token(TokenType::END_OF_FILE,line,column) {}
		std::string toString() const override {return {"<EOF>"};}
	};

}

#endif