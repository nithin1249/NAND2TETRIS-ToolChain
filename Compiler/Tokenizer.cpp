//
// Created by Nithin Kondabathini on 23/11/2025.
//

#include "Tokenizer.h"
#include <fstream>
#include <stdexcept>
#include <cctype>

namespace nand2tetris::jack {
	Tokenizer::Tokenizer(const std::string &filePath):fileName(filePath){
		loadFile(filePath);
		skipWhitespaceAndComments();
		currentToken=nextToken();
	}

	void Tokenizer::loadFile(const std::string &filePath) {
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
		} else {
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
		if (!hasMoreTokens()) {
			return;
		}
		skipWhitespaceAndComments();
		if (pos>=src.size()) {
			currentToken=std::make_unique<EofToken>(line, column);
			return;
		}
		currentToken=nextToken();
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

		std::size_t tokenLine   = line;
		std::size_t tokenColumn = column;
		const char c =src[pos];

		//symbol
		const std::string symbols="{}()[].,;+-*/&|<>=~";
		if (symbols.find(c) != std::string::npos) {
			advanceChar();
			return std::make_unique<TextToken>(TokenType::SYMBOL, std::string(1,c),tokenLine,tokenColumn);
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

		std::string value;
		while (pos<src.size() && src[pos]!='"') {
			const char c = src[pos];
			if (c == '\n' || c == '\r') {
				errorAt(tokenline, tokencolumn, "Newline in string constant");
			}
			value.push_back(c);
			advanceChar();
		}
		if (pos>=src.size()) {
			errorAt(tokenline, tokencolumn, "Unterminated string constant");
		}
		advanceChar();//skip closing of strings "
		return std::make_unique<TextToken>(TokenType::STRING_CONST,value,tokenline, tokencolumn);
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
			value=value*10+(src[pos]-'0');
			advanceChar();
		}
		if (value < 0 || value > 32767) {
			errorAt(tokenline,tokencolumn,"Integer constant out of Jack range (0–32767)");
		}

		return std::make_unique<IntToken>(value,tokenline,tokencolumn);
	}

	bool Tokenizer::isKeywordString(const std::string& s, Keyword& outKw) {
		using K=Keyword;
		if (s=="class"){outKw=K::CLASS; return true;}
		if (s=="method"){outKw=K::METHOD; return true;}
		if (s=="function"){outKw=K::FUNCTION; return true;}
		if (s=="constructor"){outKw=K::CONSTRUCTOR; return true;}
		if (s=="int"){outKw=K::INT; return true;}
		if (s=="char"){outKw=K::CHAR; return true;}
		if (s=="boolean"){outKw=K::BOOLEAN; return true;}
		if (s=="void"){outKw=K::VOID; return true;}
		if (s=="var"){outKw=K::VAR; return true;}
		if (s=="static"){outKw=K::STATIC; return true;}
		if (s=="field"){outKw=K::FIELD; return true;}
		if (s=="let"){outKw=K::LET; return true;}
		if (s=="do"){outKw=K::DO; return true;}
		if (s=="if"){outKw=K::IF; return true;}
		if (s=="else"){outKw=K::ELSE; return true;}
		if (s=="while"){outKw=K::WHILE; return true;}
		if (s=="return"){outKw=K::RETURN; return true;}
		if (s=="true"){outKw=K::TRUE_; return true;}
		if (s=="false"){outKw=K::FALSE_; return true;}
		if (s=="null"){outKw=K::NULL_; return true;}
		if (s=="this"){outKw=K::THIS_; return true;}

		return false;
	}

	std::unique_ptr<Token> Tokenizer::readIdentifierOrKeyword(const std::size_t tokenline, const std::size_t tokencolumn) {
		std::string s;
		while (pos<src.size()) {
			const char c=src[pos];
			if (std::isalnum(static_cast<unsigned char>(c))||c=='_') {
				s.push_back(c);
				advanceChar();
			} else break;
		}

		Keyword kw;
		if (isKeywordString(s,kw)) {
			return std::make_unique<KeywordToken>(kw,tokenline,tokencolumn);
		} else {
			return std::make_unique<TextToken>(TokenType::IDENTIFIER, s,tokenline,tokencolumn);
		}
	}

	[[noreturn]] void Tokenizer::errorAt(const std::size_t errLine, const std::size_t errColumn,const std::string& message) const {
		const std::string full =
			fileName + ":" +
			std::to_string(errLine) + ":" +
			std::to_string(errColumn) + ": " +
			message;
		throw std::runtime_error(full);
	}

	[[noreturn]]void Tokenizer::errorHere(const std::string& message) const {
		errorAt(line, column, message);
	}

}
