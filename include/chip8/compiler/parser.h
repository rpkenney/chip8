#pragma once

#include <chip8/compiler/lexer.h>
#include <chip8/compiler/ast.h>
#include <memory>
#include <string>

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    // Parse the entire program
    std::shared_ptr<Program> parse();

private:
    std::vector<Token> tokens_;
    size_t current_;

    // Token management
    Token peek() const;
    Token previous() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(const std::vector<TokenType>& types);
    Token consume(TokenType type, const std::string& message);
    bool isAtEnd() const;

    // Error handling
    void error(const std::string& message);
    void synchronize();

    // Parsing functions (top-down)
    std::shared_ptr<VariableDecl> parseVariableDecl();
    std::shared_ptr<SpriteDecl> parseSpriteDecl();
    std::shared_ptr<FunctionDef> parseFunctionDef();

    std::shared_ptr<Statement> parseStatement();
    std::shared_ptr<Statement> parseIfStatement();
    std::shared_ptr<Statement> parseWhileStatement();
    std::shared_ptr<Statement> parseReturnStatement();
    std::shared_ptr<Statement> parseBreakStatement();
    std::shared_ptr<Statement> parseContinueStatement();
    std::shared_ptr<Statement> parseExpressionStatement();
    std::vector<std::shared_ptr<Statement>> parseBlock();

    // Expression parsing (Pratt-style)
    std::shared_ptr<Expression> parseExpression(int min_precedence = 0);
    std::shared_ptr<Expression> parseAtom();
    std::shared_ptr<Expression> parsePostfix(std::shared_ptr<Expression> expr);

    // Operator precedence
    int getPrecedence(TokenType type) const;
    bool isRightAssociative(TokenType type) const;
};
