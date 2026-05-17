#pragma once

#include <string>
#include <vector>
#include <memory>

// Forward declarations for AST nodes
struct ASTNode {
    virtual ~ASTNode() = default;
    virtual std::string toString(int indent = 0) const = 0;
};

// Expressions
struct Expression : public ASTNode {};

struct Literal : public Expression {
    std::string value;  // "42", "0xFF", etc.

    Literal(const std::string& val) : value(val) {}
    std::string toString(int indent = 0) const override;
};

struct Identifier : public Expression {
    std::string name;

    Identifier(const std::string& n) : name(n) {}
    std::string toString(int indent = 0) const override;
};

struct BinaryOp : public Expression {
    std::string op;
    std::shared_ptr<Expression> left;
    std::shared_ptr<Expression> right;

    BinaryOp(const std::string& o, std::shared_ptr<Expression> l, std::shared_ptr<Expression> r)
        : op(o), left(l), right(r) {}
    std::string toString(int indent = 0) const override;
};

struct UnaryOp : public Expression {
    std::string op;
    std::shared_ptr<Expression> operand;

    UnaryOp(const std::string& o, std::shared_ptr<Expression> opnd)
        : op(o), operand(opnd) {}
    std::string toString(int indent = 0) const override;
};

struct FunctionCall : public Expression {
    std::string name;
    std::vector<std::shared_ptr<Expression>> args;

    FunctionCall(const std::string& n) : name(n) {}
    std::string toString(int indent = 0) const override;
};

// Statements
struct Statement : public ASTNode {
    int line = 0;  // 1-based source line from lexer (for listings)
};

struct ExpressionStatement : public Statement {
    std::shared_ptr<Expression> expr;

    ExpressionStatement(std::shared_ptr<Expression> e) : expr(e) {}
    std::string toString(int indent = 0) const override;
};

struct Assignment : public Statement {
    std::string name;
    std::shared_ptr<Expression> value;

    Assignment(const std::string& n, std::shared_ptr<Expression> v) : name(n), value(v) {}
    std::string toString(int indent = 0) const override;
};

struct IfStatement : public Statement {
    std::shared_ptr<Expression> condition;
    std::vector<std::shared_ptr<Statement>> thenBranch;
    std::vector<std::shared_ptr<Statement>> elseBranch;

    std::string toString(int indent = 0) const override;
};

struct WhileStatement : public Statement {
    std::shared_ptr<Expression> condition;
    std::vector<std::shared_ptr<Statement>> body;

    std::string toString(int indent = 0) const override;
};

struct ReturnStatement : public Statement {
    std::shared_ptr<Expression> value;  // can be null

    ReturnStatement(std::shared_ptr<Expression> v = nullptr) : value(v) {}
    std::string toString(int indent = 0) const override;
};

struct BreakStatement : public Statement {
    std::string toString(int indent = 0) const override;
};

struct ContinueStatement : public Statement {
    std::string toString(int indent = 0) const override;
};

// Declarations
struct Declaration : public ASTNode {};

struct VariableDecl : public Declaration {
    std::vector<std::string> names;  // uint8 x, y; → {"x", "y"}
    std::shared_ptr<Expression> initializer;  // can be null

    VariableDecl(const std::vector<std::string>& n, std::shared_ptr<Expression> init = nullptr)
        : names(n), initializer(init) {}
    std::string toString(int indent = 0) const override;
};

struct SpriteDecl : public Declaration {
    std::string name;
    std::vector<std::string> bytes;  // sprite data as hex strings

    SpriteDecl(const std::string& n) : name(n) {}
    std::string toString(int indent = 0) const override;
};

struct FunctionDef : public Declaration {
    std::string name;
    std::vector<std::shared_ptr<Statement>> body;

    FunctionDef(const std::string& n) : name(n) {}
    std::string toString(int indent = 0) const override;
};

// Top-level program
struct Program : public ASTNode {
    std::vector<std::shared_ptr<Declaration>> declarations;
    std::vector<std::shared_ptr<FunctionDef>> functions;

    std::string toString(int indent = 0) const override;
};
