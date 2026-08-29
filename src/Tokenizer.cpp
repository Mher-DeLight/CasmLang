#include "../include/Tokenizer.h"
#include "ErrorHandler.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
#define ucharcast(x) static_cast<unsigned char>(x)
#define next                                                                                       \
    advance();                                                                                     \
    continue

namespace ocarlang {
namespace fs = std::filesystem;

void Tokenizer::advance(int amount) {
    for (int i = 0; i < amount; i++) {
        if (peek(i) == '\n') {
            column = 1;
            row++;
        } else {
            column++;
        }
    }
    cursor += amount;
}
char Tokenizer::peek(int offset) const {
    if (!is_valid_position(cursor + offset))
        return '\0';
    return code[cursor + offset];
}
char Tokenizer::current() const {
    assert_validity(cursor);
    return code[cursor];
}
bool Tokenizer::is_valid_position(const int pos) const {
    return (pos >= 0) && (pos < code.size());
}
void Tokenizer::assert_validity(const int pos, const std::string& msg) const {
    if (!is_valid_position(pos)) {
        panic(msg.empty() ? std::string("Position " + std::to_string(pos) + " is invalid.") : msg);
    }
}
void Tokenizer::token(const TokenType tkn, const std::string& lexeme_, SourceLocation lct) {
    tokens.push_back(ocarlang::Token(tkn, lexeme_, lct));
}
bool Tokenizer::eof() {
    return cursor >= code.size();
}
bool Tokenizer::is_identifier(const std::string& str) {
    return !word_table.contains(str);
}
TokenType Tokenizer::get_word_type(const std::string& str, bool allow_identifiers) {
    if (word_table.contains(str)) {
        return word_table.at(str);
    } else {
        if (allow_identifiers)
            return TokenType::Identifier;
        else
            return TokenType::None;
    }
    return TokenType::None;
}
std::vector<ocarlang::Token>& Tokenizer::get_tokens() {
    return tokens;
}

void Tokenizer::pretty_print(std::ostream& os) {
    os << "=== TOKENS ===\n";
    for (auto& tkn : tokens) {
        os << "Token(" << (int)tkn.type << ", ";
        os << "\"" << tkn.lexeme << "\") ";
        os << tkn.location.row << ":" << tkn.location.column << "\n";
    }
}
bool Tokenizer::is_character(const char c) {
    return (std::string_view(".,:;-><+-/=!&*(){}[]#|").find(c) != std::string_view::npos);
}

void Tokenizer::parse_comment() {
    while (!eof() && (current() != '\n')) {
        advance();
    }
}

void Tokenizer::parse_multiline_comment() {
    while (!eof() && !(current() == '*' && peek() == '/'))
        advance();
    if (eof())
        return;
    // encountered the ending "*/" sequence
    advance(2);
}

void Tokenizer::tokenize(const std::string& c) {
    code = c;
    cursor = 0;
    row = 1;
    column = 1;

    while (cursor < code.size()) {
        if (!is_valid_position(cursor))
            break;

        if (current() == '\n') {
            next;
        }
        if (current() == ' ') {
            next;
        }
        if (current() == '/' && peek() == '/') {
            parse_comment();
            continue;
        }
        if (current() == '/' && peek() == '*') {
            parse_multiline_comment();
            continue;
        }
        if (current() == '#') {
            prcs_process();
            continue;
        }

        SourceLocation location(row, column);
        if (std::isalpha(current()) || current() == '_') {
            std::string str;

            while (!eof() && (std::isalnum(ucharcast(current())) || current() == '_')) {
                assert_validity(cursor);
                str += current();
                advance();
            }

            if (str == "__END_OF_FILE__") {
                token(TokenType::EndOfFile, "", location);
                return;
            }
            token(get_word_type(str), str, location);
            continue;
        } else if (current() == '@') { // direct conversion "operator"
            advance();
            std::string str;
            while (!eof() && current() != '\n') {
                if (current() == '/' && peek() == '/') {
                    parse_comment();
                    break;
                } else if (current() == '/' && peek() == '*') {
                    parse_multiline_comment();
                    break;
                }

                assert_validity(cursor);
                str += current();
                advance();
            }

            token(TokenType::AsmInstruction, str, location);
            continue;
        } else if (is_character(current())) {
            std::string str;

            while (!eof() && (is_character(ucharcast(current())))) {
                assert_validity(cursor);
                str += current();
                advance();
            }

            while (!str.empty()) {
                if (word_table.contains(str))
                    break;

                advance(-1);
                str.pop_back();
            }

            if (!word_table.contains(str))
                panic("invalid character string \"" + str + "\"");

            token(get_word_type(str, false), str, location);
            continue;
        } else if (std::isdigit(current())) {
            std::string str;

            bool found_dot = false;
            while (!eof() &&
                   (std::isdigit(ucharcast(current())) || (!found_dot && current() == '.'))) {
                assert_validity(cursor);
                str += current();
                if (current() == '.')
                    found_dot = true;
                advance();
            }

            token(found_dot ? TokenType::FloatLiteral : TokenType::IntegerLiteral, str, location);
            continue;

        } else if (current() == '\"') {
            std::string str;
            advance();

            while (!eof() && (current() != '\"')) {
                if (current() == '\\') {
                    advance();
                    assert_validity(cursor);
                    if (current() == 'n') {
                        str += "\\n";
                        advance();
                        continue;
                    } else if (current() == 't') {
                        str += "\\t";
                        advance();
                        continue;
                    }
                    str += current();
                    advance();
                    continue;
                }
                assert_validity(cursor);
                str += current();
                advance();
            }

            token(TokenType::StringLiteral, str, location);
        } else if (current() == '\'') {
            advance();
            token(TokenType::CharLiteral, std::string(1, current()), location);
            advance();
        } else {
            panic("Invalid character '" + std::string(1, current()) + "' at " +
                  std::to_string(row) + ":" + std::to_string(column));
        }
        next;
    }

    token(TokenType::EndOfFile, "\0", SourceLocation(row, column));
}
} // namespace ocarlang