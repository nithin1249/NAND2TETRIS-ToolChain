//
// Created by Nithin Kondabathini on 23/11/2025.
//

#include "Tokenizer.h"
#include <fstream>
#include <stdexcept>

namespace nand2tetris::jack {
	Tokenizer::Tokenizer(const std::string &filePath) {
		loadFile(filePath);
		currentToken.type=TokenType::END_OF_FILE;
	}

	void Tokenizer::loadFile(const std::string &filePath) {
		std::ifstream in(filePath);
		if (!in) {
			throw std::runtime_error("Cannot open Jack file: "+filePath);
		}
		src.assign((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
		pos=0;
	}

	bool Tokenizer::hasMoreTokens() const {
		return pos<src.size() || currentToken.type!=TokenType::END_OF_FILE;
	}

	void Tokenizer::advance() {
		skipWhitespaceAndComments();
		if (pos>=src.size()) {
			currentToken.type = TokenType::END_OF_FILE;
			currentToken.text.clear();
			return;
		}
		currentToken=nextToken();
	}

	void Tokenizer::skipWhitespaceAndComments() {
		while (pos<src.size()) {
			const char c = src[pos];

			//white space
			if (std::isspace(static_cast<unsigned char>(c))) {
				++pos;
				continue;
			}

			//line comment: //
			if (c=='/'&& pos+1<src.size() && src[pos+1]=='/') {
				pos+=2;
				while (pos < src.size() && src[pos] != '\n') ++pos;
				continue;
			}

			//block comment /*...*/
			if (c=='/' && pos+1<src.size() && src[pos+1]=='*') {
				pos+=2;
				while (pos<src.size() && !(src[pos]=='*' && src[pos+1]=='/')) ++pos;
				continue;
			}

			break; // non-comment non-whitespace
		}
	}

	Token Tokenizer::nextToken() {
		const char c =src[pos];

		//symbol
		const std::string symbols="{}()[].,;+-*/&|<>=~";
		if (symbols.find(c) != std::string::npos) {
			Token t;
			t.type = TokenType::SYMBOL;
			t.text = std::string(1, c);
			++pos;
			return t;
		}

		//string
		if (c == '"') {
			return readString();
		}

		//integer constant
		if (std::isdigit(static_cast<unsigned char>(c))) {
			return readNumber();
		}

		//identifier or keyword
		if (std::isalpha(static_cast<unsigned char>(c))||c=='_') {
			return readIdentifierOrKeyword();
		}

		throw std::runtime_error("Unexpected character in Jack source");
	}

	Token Tokenizer::readString() {
		Token t;
		t.type=TokenType::STRING_CONST;
		++pos; //skip opening of strings "

		std::string value;
		while (pos<src.size() && src[pos]!='"') {
			value.push_back(src[pos]);
			++pos;
		}
		if (pos>=src.size()) {
			throw std::runtime_error("Unterminated string constant");
		}
		++pos;//skip closing of strings "
		t.text=value;
		return t;
	}

	const Token &Tokenizer::current() const {
		return currentToken;
	}

	Token Tokenizer::readNumber() {
		Token t;
		t.type=TokenType::INT_CONST;
		int value=0;

		while (pos<src.size()&&std::isdigit(static_cast<unsigned char>(src[pos]))) {
			value=value*10+(src[pos]-'0');
			++pos;
		}
		if (value < 0 || value > 32767) {
			throw std::runtime_error("Integer constant out of Jack range (0–32767)");
		}
		t.intVal=value;
		t.text=std::to_string(value);
		return t;
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

	Token Tokenizer::readIdentifierOrKeyword() {
		std::string s;
		while (pos<src.size()) {
			const char c=src[pos];
			if (std::isalnum(static_cast<unsigned char>(c))||c=='_') {
				s.push_back(c);
				++pos;
			} else break;
		}

		Token t;
		Keyword kw;
		if (isKeywordString(s,kw)) {
			t.type=TokenType::KEYWORD;
			t.keyword=kw;
			t.text=s;

		}else {
			t.type=TokenType::IDENTIFIER;
			t.text=s;
		}

		return t;
	}
}
