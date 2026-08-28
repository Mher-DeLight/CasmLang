#pragma once
#include "Common.h"
#include <string>
#include <vector>

namespace ocarlang {

class Token {
public:
    TokenType type;
    std::string lexeme;
    SourceLocation location;

    Token(const TokenType type_, const std::string& lexeme_, const SourceLocation& source_location_)
        : type(type_), lexeme(lexeme_), location(source_location_) {}
};

class Tokenizer {
private:
    std::string code;
    std::vector<Token> tokens;

    int cursor = 0;
    int row = 1;
    int column = 1;

    TokenType get_word_type(const std::string& str, bool allow_identifiers = true);
    bool is_valid_position(const int pos) const;
    void assert_validity(const int pos, const std::string& error_message = "") const;
    bool is_identifier(const std::string& str);
    bool eof();
    void advance(int amount = 1);
    char peek(int offset = 1) const;
    void token(const TokenType type, const std::string& lexeme_ = "",
               SourceLocation src = SourceLocation());
    char current() const;
    bool is_character(const char c);

    void parse_comment();
    void parse_multiline_comment();

    // == PREPROCESSOR ==
    void prcs_process();
    void prcs_process_include();
    void prcs_process_noheader();

public:
    bool makeHeader = true;
    void tokenize(const std::string& c);
    std::vector<Token>& get_tokens();
    void pretty_print(std::ostream& os);
};
} // namespace ocarlang