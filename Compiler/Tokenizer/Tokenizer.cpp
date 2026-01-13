//
// Created by Nithin Kondabathini on 23/11/2025.
//

#include "Tokenizer.h"
#include <fstream>
#include <stdexcept>
#include <unordered_map>

namespace nand2tetris::jack {
	Tokenizer::Tokenizer(const std::string &filePath):fileName(filePath){
		loadFile(filePath);
		currentToken=fetchNext();
	}

	std::unique_ptr<Token> Tokenizer::fetchNext() {
		skipWhitespaceAndComments();
		return nextToken();
	}

	const Token& Tokenizer::peek() {
		if (!peekToken) {
			peekToken = fetchNext(); //
		}

		return *peekToken;
	}

	void Tokenizer::loadFile(const std::string &filePath) {
		// Check for .jack extension
		if (filePath.length() < 5 || filePath.substr(filePath.length() - 5) != ".jack") {
			throw std::runtime_error("Invalid file extension. Expected a .jack file: " + filePath);
		}
		std::ifstream in(filePath);
		if (!in) {
			throw std::runtime_error("Cannot open Jack file: "+filePath);
		}
		src.assign((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
		pos=0;
		line=1;
		column=1;
	}
	void Tokenizer::advanceChar() {
		if (pos >= src.size()) {
			return;
		}
		const char c = src[pos++];
		if (c == '\n') {
			++line;
			column = 1;
		} else if (c=='\r') {
			//do nothing
		}else {
			++column;
		}
	}

	bool Tokenizer::hasMoreTokens() const {
		// No token yet? Then we assume there *will* be tokens.
		if (!currentToken) {
			return true;
		}
		return currentToken->getType()!=TokenType::END_OF_FILE;
	}

	void Tokenizer::advance() {
		if (peekToken) {
			currentToken = std::move(peekToken); //
		}

		if (!hasMoreTokens()) {
			return;
		}
		currentToken = fetchNext(); //

	}

	void Tokenizer::skipWhitespaceAndComments() {
		while (pos<src.size()) {
			const char c = src[pos];

			//white space
			if (std::isspace(static_cast<unsigned char>(c))) {
				advanceChar();
				continue;
			}

			//line comment: //
			if (c=='/'&& pos+1<src.size() && src[pos+1]=='/') {
				advanceChar();
				advanceChar();
				while (pos < src.size() && src[pos] != '\n') {
					advanceChar();
				}
				continue;
			}

			//block comment /*...*/
			if (c=='/' && pos+1<src.size() && src[pos+1]=='*') {
				advanceChar();
				advanceChar();
				while (pos+1<src.size() && !(src[pos]=='*' && src[pos+1]=='/')) {
					advanceChar();
				}

				if (pos + 1 >= src.size()) {
					errorHere("Unterminated block comment");
				}
				advanceChar();
				advanceChar();
				continue;
			}

			break; // non-comment non-whitespace
		}
	}

	std::unique_ptr<Token> Tokenizer::nextToken() {
		if (pos>=src.size()) {
			return std::make_unique<EofToken>(line, column);
		}

		std::size_t tokenLine= line;
		std::size_t tokenColumn = column;
		const char c =src[pos];

		//symbol
		constexpr std::string symbols="{}()[].,;+-*/&|<>=~";
		if (symbols.find(c) != std::string_view::npos) {
			std::string_view symView=std::string_view(src).substr(pos,1);
			advanceChar();
			return std::make_unique<TextToken>(TokenType::SYMBOL,symView,tokenLine,tokenColumn);
		}

		//string
		if (c == '"') {
			return readString(tokenLine,tokenColumn);
		}

		//integer constant
		if (std::isdigit(static_cast<unsigned char>(c))) {
			return readNumber(tokenLine,tokenColumn);
		}

		//identifier or keyword
		if (std::isalpha(static_cast<unsigned char>(c))||c=='_') {
			return readIdentifierOrKeyword(tokenLine,tokenColumn);
		}

		errorHere( "Unexpected character: '" + std::string(1,c)+"'");
	}

	std::unique_ptr<Token> Tokenizer::readString(const std::size_t tokenline, const std::size_t tokencolumn) {
		advanceChar(); //skip opening of strings "

		const std::size_t start=pos;
		while (pos < src.size() && src[pos] != '"') {
			if (src[pos] == '\n' || src[pos] == '\r') errorAt(tokenline, tokencolumn, "Newline in string");
			advanceChar();
		}

		std::string_view val = std::string_view(src).substr(start, pos - start);

		if (pos>=src.size()) {
			errorAt(tokenline, tokencolumn, "Unterminated string constant");
		}
		advanceChar();//skip closing of strings "
		return std::make_unique<TextToken>(TokenType::STRING_CONST,val,tokenline, tokencolumn);
	}

	const Token& Tokenizer::current() const {
		if (!currentToken) {
			throw std::logic_error("Tokenizer::current() called with no current token");
		}
		return *currentToken;
	}

	std::unique_ptr<Token> Tokenizer::readNumber(const std::size_t tokenline, const std::size_t tokencolumn) {
		int value=0;

		while (pos<src.size()&&std::isdigit(static_cast<unsigned char>(src[pos]))) {
			const int digit = src[pos] - '0';
			// Check for overflow BEFORE it happens
			// If value > 3276, the next multiplication (value * 10) will be >= 32770
			// If value == 3276 and the digit > 7, it will be > 32767
			if (value > 3276 || (value == 3276 && digit > 7)) {
				errorAt(tokenline, tokencolumn, "Integer constant too large (max 32767)");
			}

			value = value * 10 + digit;
			advanceChar();
		}

		return std::make_unique<IntToken>(value,tokenline,tokencolumn);
	}

	bool Tokenizer::isKeywordString(const std::string_view s, Keyword& outKw) {
		static const std::unordered_map<std::string_view, Keyword> keywordMap = {
			{"class",       Keyword::CLASS},
			{"method",      Keyword::METHOD},
			{"function",    Keyword::FUNCTION},
			{"constructor", Keyword::CONSTRUCTOR},
			{"int",         Keyword::INT},
			{"boolean",     Keyword::BOOLEAN},
			{"char",        Keyword::CHAR},
			{"void",        Keyword::VOID},
			{"var",         Keyword::VAR},
			{"static",      Keyword::STATIC},
			{"field",       Keyword::FIELD},
			{"let",         Keyword::LET},
			{"do",          Keyword::DO},
			{"if",          Keyword::IF},
			{"else",        Keyword::ELSE},
			{"while",       Keyword::WHILE},
			{"return",      Keyword::RETURN},
			{"true",        Keyword::TRUE_},
			{"false",       Keyword::FALSE_},
			{"null",        Keyword::NULL_},
			{"this",        Keyword::THIS_}
		};

		const auto it = keywordMap.find(s);
		if (it != keywordMap.end()) {
			outKw = it->second;
			return true;
		}

		return false;
	}

	std::unique_ptr<Token> Tokenizer::readIdentifierOrKeyword(const std::size_t tokenline, const std::size_t tokencolumn) {
		const std::size_t start=pos;
		while (pos < src.size() && (std::isalnum(static_cast<unsigned char>(src[pos])) || src[pos] == '_')) {
			advanceChar();
		}
		// Create a view of the segment we just scanned
		std::string_view s = std::string_view(src).substr(start, pos - start);

		Keyword kw;
		if (isKeywordString(s,kw)) {
			return std::make_unique<KeywordToken>(kw,tokenline,tokencolumn);
		}

		return std::make_unique<TextToken>(TokenType::IDENTIFIER, s,tokenline,tokencolumn);
	}

	[[noreturn]] void Tokenizer::errorAt(const std::size_t errLine, const std::size_t errColumn,const std::string_view
		message) const {
		const std::string full =
			fileName + ":" +
			std::to_string(errLine) + ":" +
			std::to_string(errColumn) + ": " +
			std::string(message);
		throw std::runtime_error(full);
	}

	[[noreturn]]void Tokenizer::errorHere(const std::string_view message) const {
		errorAt(line, column, message);
	}

}
