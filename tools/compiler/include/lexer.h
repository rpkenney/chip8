#pragma once

#include <string>
#include <vector>
#include <memory>

// Token types for the CHIP-8 compiler language
enum class TokenType {
    // End of file
    EOF_TOKEN,

    // Literals
    INTEGER,
    IDENTIFIER,

    // Keywords
    UINT8,
    IF,
    ELSE,
    WHILE,
    FOR,
    BREAK,
    CONTINUE,
    SPRITE,
    VOID,
    RETURN,

    // Built-in functions
    DRAW_SPRITE,
    CLEAR_SCREEN,
    SET_DELAY,
    GET_DELAY,
    SET_SOUND,
    WAIT_KEY,
    GET_KEY,

    // Operators
    PLUS,       // +
    MINUS,      // -
    STAR,       // *
    SLASH,      // /
    PERCENT,    // %
    ASSIGN,     // =
    EQ,         // ==
    NE,         // !=
    LT,         // <
    GT,         // >
    LE,         // <=
    GE,         // >=
    BANG,       // !
    AND,        // &&
    OR,         // ||

    // Punctuation
    LBRACE,     // {
    RBRACE,     // }
    LPAREN,     // (
    RPAREN,     // )
    COMMA,      // ,
    SEMICOLON,  // ;

    // Error
    ERROR,
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;

    Token(TokenType type, const std::string& lexeme, int line, int column)
        : type(type), lexeme(lexeme), line(line), column(column) {}
};

class Lexer {
public:
    explicit Lexer(const std::string& source);

    // Scan all tokens from the source
    std::vector<Token> scanTokens();

    // Peek at the next token without consuming it
    Token peekToken() const;

    // Consume and return the next token
    Token nextToken();

    // Check if we're at EOF
    bool isAtEnd() const;

private:
    std::string source_;
    size_t current_;
    int line_;
    int column_;
    std::vector<Token> tokens_;
    size_t token_index_;

    // Helper methods
    char peek() const;
    char peekNext() const;
    char advance();
    void skipWhitespace();
    void skipComment();
    Token scanNumber();
    Token scanIdentifierOrKeyword();
    Token makeToken(TokenType type, const std::string& lexeme);
    Token errorToken(const std::string& message);
};
