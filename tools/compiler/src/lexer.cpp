#include <chip8/compiler/lexer.h>
#include <cctype>
#include <unordered_map>

Lexer::Lexer(const std::string& source)
    : source_(source), current_(0), line_(1), column_(1), token_index_(0) {}

std::vector<Token> Lexer::scanTokens() {
    while (!isAtEnd()) {
        skipWhitespace();
        if (isAtEnd()) break;

        char c = peek();

        // Single-character tokens
        if (c == '{') {
            tokens_.push_back(makeToken(TokenType::LBRACE, "{"));
            advance();
        } else if (c == '}') {
            tokens_.push_back(makeToken(TokenType::RBRACE, "}"));
            advance();
        } else if (c == '(') {
            tokens_.push_back(makeToken(TokenType::LPAREN, "("));
            advance();
        } else if (c == ')') {
            tokens_.push_back(makeToken(TokenType::RPAREN, ")"));
            advance();
        } else if (c == ',') {
            tokens_.push_back(makeToken(TokenType::COMMA, ","));
            advance();
        } else if (c == ';') {
            tokens_.push_back(makeToken(TokenType::SEMICOLON, ";"));
            advance();
        } else if (c == '+') {
            tokens_.push_back(makeToken(TokenType::PLUS, "+"));
            advance();
        } else if (c == '-') {
            tokens_.push_back(makeToken(TokenType::MINUS, "-"));
            advance();
        } else if (c == '*') {
            tokens_.push_back(makeToken(TokenType::STAR, "*"));
            advance();
        } else if (c == '/') {
            if (peekNext() == '/') {
                // Line comment
                skipComment();
            } else {
                tokens_.push_back(makeToken(TokenType::SLASH, "/"));
                advance();
            }
        } else if (c == '%') {
            tokens_.push_back(makeToken(TokenType::PERCENT, "%"));
            advance();
        } else if (c == '=') {
            advance();
            if (peek() == '=') {
                advance();
                tokens_.push_back(makeToken(TokenType::EQ, "=="));
            } else {
                tokens_.push_back(makeToken(TokenType::ASSIGN, "="));
            }
        } else if (c == '!') {
            advance();
            if (peek() == '=') {
                advance();
                tokens_.push_back(makeToken(TokenType::NE, "!="));
            } else {
                tokens_.push_back(makeToken(TokenType::BANG, "!"));
            }
        } else if (c == '<') {
            advance();
            if (peek() == '=') {
                advance();
                tokens_.push_back(makeToken(TokenType::LE, "<="));
            } else {
                tokens_.push_back(makeToken(TokenType::LT, "<"));
            }
        } else if (c == '>') {
            advance();
            if (peek() == '=') {
                advance();
                tokens_.push_back(makeToken(TokenType::GE, ">="));
            } else {
                tokens_.push_back(makeToken(TokenType::GT, ">"));
            }
        } else if (c == '&') {
            advance();
            if (peek() == '&') {
                advance();
                tokens_.push_back(makeToken(TokenType::AND, "&&"));
            } else {
                tokens_.push_back(errorToken("Unexpected character '&'"));
            }
        } else if (c == '|') {
            advance();
            if (peek() == '|') {
                advance();
                tokens_.push_back(makeToken(TokenType::OR, "||"));
            } else {
                tokens_.push_back(errorToken("Unexpected character '|'"));
            }
        } else if (std::isdigit(c)) {
            tokens_.push_back(scanNumber());
        } else if (std::isalpha(c) || c == '_') {
            tokens_.push_back(scanIdentifierOrKeyword());
        } else {
            tokens_.push_back(errorToken(std::string("Unexpected character '") + c + "'"));
            advance();
        }
    }

    tokens_.push_back(Token(TokenType::EOF_TOKEN, "", line_, column_));
    return tokens_;
}

Token Lexer::peekToken() const {
    if (token_index_ < tokens_.size()) {
        return tokens_[token_index_];
    }
    return Token(TokenType::EOF_TOKEN, "", line_, column_);
}

Token Lexer::nextToken() {
    if (token_index_ < tokens_.size()) {
        return tokens_[token_index_++];
    }
    return Token(TokenType::EOF_TOKEN, "", line_, column_);
}

bool Lexer::isAtEnd() const {
    return current_ >= source_.length();
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source_[current_];
}

char Lexer::peekNext() const {
    if (current_ + 1 >= source_.length()) return '\0';
    return source_[current_ + 1];
}

char Lexer::advance() {
    char c = peek();
    current_++;
    if (c == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    return c;
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else {
            break;
        }
    }
}

void Lexer::skipComment() {
    // Skip '//' and the rest of the line
    if (peek() == '/' && peekNext() == '/') {
        advance(); // skip first /
        advance(); // skip second /
        while (!isAtEnd() && peek() != '\n') {
            advance();
        }
    }
}

Token Lexer::scanNumber() {
    size_t start = current_;
    int start_col = column_;

    // Check for hex literal (0x...)
    if (peek() == '0' && peekNext() == 'x') {
        advance(); // skip '0'
        advance(); // skip 'x'
        while (!isAtEnd() && std::isxdigit(peek())) {
            advance();
        }
    } else {
        // Decimal
        while (!isAtEnd() && std::isdigit(peek())) {
            advance();
        }
    }

    std::string lexeme = source_.substr(start, current_ - start);
    return Token(TokenType::INTEGER, lexeme, line_, start_col);
}

Token Lexer::scanIdentifierOrKeyword() {
    size_t start = current_;
    int start_col = column_;

    while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_')) {
        advance();
    }

    std::string lexeme = source_.substr(start, current_ - start);

    // Check if it's a keyword
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"uint8", TokenType::UINT8},
        {"void", TokenType::VOID},
        {"if", TokenType::IF},
        {"else", TokenType::ELSE},
        {"while", TokenType::WHILE},
        {"for", TokenType::FOR},
        {"break", TokenType::BREAK},
        {"continue", TokenType::CONTINUE},
        {"sprite", TokenType::SPRITE},
        {"return", TokenType::RETURN},
        {"draw_sprite", TokenType::DRAW_SPRITE},
        {"clear_screen", TokenType::CLEAR_SCREEN},
        {"set_delay", TokenType::SET_DELAY},
        {"get_delay", TokenType::GET_DELAY},
        {"set_sound", TokenType::SET_SOUND},
        {"wait_key", TokenType::WAIT_KEY},
        {"get_key", TokenType::GET_KEY},
    };

    auto it = keywords.find(lexeme);
    TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;

    return Token(type, lexeme, line_, start_col);
}

Token Lexer::makeToken(TokenType type, const std::string& lexeme) {
    return Token(type, lexeme, line_, column_ - static_cast<int>(lexeme.length()));
}

Token Lexer::errorToken(const std::string& message) {
    return Token(TokenType::ERROR, message, line_, column_);
}
