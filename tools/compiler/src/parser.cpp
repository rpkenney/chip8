#include "parser.h"
#include <iostream>
#include <stdexcept>

Parser::Parser(const std::vector<Token>& tokens)
    : tokens_(tokens), current_(0) {}

std::shared_ptr<Program> Parser::parse() {
    auto program = std::make_shared<Program>();

    while (!isAtEnd()) {
        try {
            if (check(TokenType::UINT8)) {
                program->declarations.push_back(parseVariableDecl());
            } else if (check(TokenType::SPRITE)) {
                program->declarations.push_back(parseSpriteDecl());
            } else if (check(TokenType::VOID)) {
                program->functions.push_back(parseFunctionDef());
            } else {
                throw std::runtime_error("Expected declaration or function definition");
            }
        } catch (const std::exception& e) {
            error(e.what());
            synchronize();
        }
    }

    return program;
}

Token Parser::peek() const {
    return tokens_[current_];
}

Token Parser::previous() const {
    return tokens_[current_ - 1];
}

Token Parser::advance() {
    if (!isAtEnd()) current_++;
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(const std::vector<TokenType>& types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw std::runtime_error(message + " at line " + std::to_string(peek().line));
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::EOF_TOKEN;
}

void Parser::error(const std::string& message) {
    std::cerr << "Parse error: " << message << " at line " << peek().line << std::endl;
}

void Parser::synchronize() {
    advance();

    while (!isAtEnd()) {
        if (previous().type == TokenType::SEMICOLON) return;

        switch (peek().type) {
            case TokenType::UINT8:
            case TokenType::SPRITE:
            case TokenType::VOID:
                return;
            default:
                break;
        }

        advance();
    }
}

std::shared_ptr<VariableDecl> Parser::parseVariableDecl() {
    consume(TokenType::UINT8, "Expected 'uint8'");

    std::vector<std::string> names;
    names.push_back(consume(TokenType::IDENTIFIER, "Expected variable name").lexeme);

    while (match(TokenType::COMMA)) {
        names.push_back(consume(TokenType::IDENTIFIER, "Expected variable name").lexeme);
    }

    std::shared_ptr<Expression> initializer = nullptr;
    if (match(TokenType::ASSIGN)) {
        initializer = parseExpression();
    }

    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    return std::make_shared<VariableDecl>(names, initializer);
}

std::shared_ptr<SpriteDecl> Parser::parseSpriteDecl() {
    consume(TokenType::SPRITE, "Expected 'sprite'");
    std::string name = consume(TokenType::IDENTIFIER, "Expected sprite name").lexeme;
    consume(TokenType::LBRACE, "Expected '{' before sprite data");

    auto sprite = std::make_shared<SpriteDecl>(name);

    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        std::string byte_val = consume(TokenType::INTEGER, "Expected hex byte").lexeme;
        sprite->bytes.push_back(byte_val);

        if (!match(TokenType::COMMA)) {
            break;
        }
    }

    consume(TokenType::RBRACE, "Expected '}' after sprite data");
    return sprite;
}

std::shared_ptr<FunctionDef> Parser::parseFunctionDef() {
    consume(TokenType::VOID, "Expected 'void'");
    std::string name = consume(TokenType::IDENTIFIER, "Expected function name").lexeme;
    consume(TokenType::LPAREN, "Expected '(' after function name");
    consume(TokenType::RPAREN, "Expected ')' after parameter list");

    auto func = std::make_shared<FunctionDef>(name);
    func->body = parseBlock();

    return func;
}

std::shared_ptr<Statement> Parser::parseStatement() {
    if (match(TokenType::IF)) {
        return parseIfStatement();
    }
    if (match(TokenType::WHILE)) {
        return parseWhileStatement();
    }
    if (match(TokenType::RETURN)) {
        return parseReturnStatement();
    }
    if (match(TokenType::BREAK)) {
        return parseBreakStatement();
    }
    if (match(TokenType::CONTINUE)) {
        return parseContinueStatement();
    }

    return parseExpressionStatement();
}

std::shared_ptr<Statement> Parser::parseIfStatement() {
    consume(TokenType::LPAREN, "Expected '(' after 'if'");
    auto condition = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after if condition");

    auto if_stmt = std::make_shared<IfStatement>();
    if_stmt->condition = condition;
    if_stmt->thenBranch = parseBlock();

    if (match(TokenType::ELSE)) {
        if_stmt->elseBranch = parseBlock();
    }

    return if_stmt;
}

std::shared_ptr<Statement> Parser::parseWhileStatement() {
    consume(TokenType::LPAREN, "Expected '(' after 'while'");
    auto condition = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after while condition");

    auto while_stmt = std::make_shared<WhileStatement>();
    while_stmt->condition = condition;
    while_stmt->body = parseBlock();

    return while_stmt;
}

std::shared_ptr<Statement> Parser::parseReturnStatement() {
    std::shared_ptr<Expression> value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        value = parseExpression();
    }
    consume(TokenType::SEMICOLON, "Expected ';' after return");
    return std::make_shared<ReturnStatement>(value);
}

std::shared_ptr<Statement> Parser::parseBreakStatement() {
    consume(TokenType::SEMICOLON, "Expected ';' after 'break'");
    return std::make_shared<BreakStatement>();
}

std::shared_ptr<Statement> Parser::parseContinueStatement() {
    consume(TokenType::SEMICOLON, "Expected ';' after 'continue'");
    return std::make_shared<ContinueStatement>();
}

std::shared_ptr<Statement> Parser::parseExpressionStatement() {
    auto expr = parseExpression();

    if (match(TokenType::ASSIGN)) {
        // It's an assignment
        if (auto ident = std::dynamic_pointer_cast<Identifier>(expr)) {
            auto value = parseExpression();
            consume(TokenType::SEMICOLON, "Expected ';' after assignment");
            return std::make_shared<Assignment>(ident->name, value);
        } else {
            throw std::runtime_error("Invalid assignment target");
        }
    }

    consume(TokenType::SEMICOLON, "Expected ';' after expression statement");
    return std::make_shared<ExpressionStatement>(expr);
}

std::vector<std::shared_ptr<Statement>> Parser::parseBlock() {
    consume(TokenType::LBRACE, "Expected '{' before block");

    std::vector<std::shared_ptr<Statement>> stmts;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        stmts.push_back(parseStatement());
    }

    consume(TokenType::RBRACE, "Expected '}' after block");
    return stmts;
}

std::shared_ptr<Expression> Parser::parseExpression(int min_precedence) {
    auto left = parseAtom();

    while (true) {
        int prec = getPrecedence(peek().type);
        if (prec == 0 || prec < min_precedence) break;  // Not a binary operator

        TokenType op_type = peek().type;
        Token op_token = peek();
        advance();

        auto right = parseExpression(prec + (isRightAssociative(op_type) ? 0 : 1));
        left = std::make_shared<BinaryOp>(op_token.lexeme, left, right);
    }

    return left;
}

std::shared_ptr<Expression> Parser::parseAtom() {
    if (match(TokenType::INTEGER)) {
        return std::make_shared<Literal>(previous().lexeme);
    }

    if (match(TokenType::IDENTIFIER)) {
        std::string name = previous().lexeme;

        // Check for function call
        if (check(TokenType::LPAREN)) {
            auto call = std::make_shared<FunctionCall>(name);
            advance();  // consume '('

            if (!check(TokenType::RPAREN)) {
                do {
                    call->args.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }

            consume(TokenType::RPAREN, "Expected ')' after arguments");
            return call;
        }

        return std::make_shared<Identifier>(name);
    }

    // Built-in function calls
    if (match({TokenType::DRAW_SPRITE, TokenType::CLEAR_SCREEN, TokenType::SET_DELAY,
               TokenType::GET_DELAY, TokenType::SET_SOUND, TokenType::WAIT_KEY,
               TokenType::GET_KEY})) {
        std::string name = previous().lexeme;
        auto call = std::make_shared<FunctionCall>(name);

        consume(TokenType::LPAREN, "Expected '(' after function name");
        if (!check(TokenType::RPAREN)) {
            do {
                call->args.push_back(parseExpression());
            } while (match(TokenType::COMMA));
        }
        consume(TokenType::RPAREN, "Expected ')' after arguments");

        return call;
    }

    // Unary operators
    if (match({TokenType::MINUS, TokenType::BANG})) {
        std::string op = previous().lexeme;
        auto operand = parseAtom();
        return std::make_shared<UnaryOp>(op, operand);
    }

    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }

    throw std::runtime_error("Expected expression");
}

int Parser::getPrecedence(TokenType type) const {
    switch (type) {
        case TokenType::OR:
            return 1;
        case TokenType::AND:
            return 2;
        case TokenType::EQ:
        case TokenType::NE:
            return 3;
        case TokenType::LT:
        case TokenType::GT:
        case TokenType::LE:
        case TokenType::GE:
            return 4;
        case TokenType::PLUS:
        case TokenType::MINUS:
            return 5;
        case TokenType::STAR:
        case TokenType::SLASH:
        case TokenType::PERCENT:
            return 6;
        default:
            return 0;
    }
}

bool Parser::isRightAssociative(TokenType type) const {
    return false;  // All binary operators are left-associative for MVP
}
