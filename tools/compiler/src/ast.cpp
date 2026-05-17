#include <chip8/compiler/ast.h>
#include <sstream>

static std::string indent_str(int indent) {
    return std::string(indent * 2, ' ');
}

std::string Literal::toString(int indent) const {
    return indent_str(indent) + "Literal(" + value + ")";
}

std::string Identifier::toString(int indent) const {
    return indent_str(indent) + "Identifier(" + name + ")";
}

std::string BinaryOp::toString(int indent) const {
    std::ostringstream oss;
    oss << indent_str(indent) << "BinaryOp(" << op << ")\n";
    oss << left->toString(indent + 1) << "\n";
    oss << right->toString(indent + 1);
    return oss.str();
}

std::string UnaryOp::toString(int indent) const {
    std::ostringstream oss;
    oss << indent_str(indent) << "UnaryOp(" << op << ")\n";
    oss << operand->toString(indent + 1);
    return oss.str();
}

std::string FunctionCall::toString(int indent) const {
    std::ostringstream oss;
    oss << indent_str(indent) << "FunctionCall(" << name << ")\n";
    for (const auto& arg : args) {
        oss << arg->toString(indent + 1) << "\n";
    }
    return oss.str();
}

std::string ExpressionStatement::toString(int indent) const {
    return indent_str(indent) + "ExpressionStmt\n" + expr->toString(indent + 1);
}

std::string Assignment::toString(int indent) const {
    std::ostringstream oss;
    oss << indent_str(indent) << "Assignment(" << name << ")\n";
    oss << value->toString(indent + 1);
    return oss.str();
}

std::string IfStatement::toString(int indent) const {
    std::ostringstream oss;
    oss << indent_str(indent) << "IfStatement\n";
    oss << indent_str(indent + 1) << "Condition:\n";
    oss << condition->toString(indent + 2) << "\n";
    oss << indent_str(indent + 1) << "Then:\n";
    for (const auto& stmt : thenBranch) {
        oss << stmt->toString(indent + 2) << "\n";
    }
    if (!elseBranch.empty()) {
        oss << indent_str(indent + 1) << "Else:\n";
        for (const auto& stmt : elseBranch) {
            oss << stmt->toString(indent + 2) << "\n";
        }
    }
    return oss.str();
}

std::string WhileStatement::toString(int indent) const {
    std::ostringstream oss;
    oss << indent_str(indent) << "WhileStatement\n";
    oss << indent_str(indent + 1) << "Condition:\n";
    oss << condition->toString(indent + 2) << "\n";
    oss << indent_str(indent + 1) << "Body:\n";
    for (const auto& stmt : body) {
        oss << stmt->toString(indent + 2) << "\n";
    }
    return oss.str();
}

std::string ReturnStatement::toString(int indent) const {
    std::ostringstream oss;
    oss << indent_str(indent) << "ReturnStatement";
    if (value) {
        oss << "\n" << value->toString(indent + 1);
    }
    return oss.str();
}

std::string BreakStatement::toString(int indent) const {
    return indent_str(indent) + "BreakStatement";
}

std::string ContinueStatement::toString(int indent) const {
    return indent_str(indent) + "ContinueStatement";
}

std::string VariableDecl::toString(int indent) const {
    std::ostringstream oss;
    oss << indent_str(indent) << "VariableDecl(";
    for (size_t i = 0; i < names.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << names[i];
    }
    oss << ")";
    if (initializer) {
        oss << "\n" << initializer->toString(indent + 1);
    }
    return oss.str();
}

std::string SpriteDecl::toString(int indent) const {
    std::ostringstream oss;
    oss << indent_str(indent) << "SpriteDecl(" << name << ", " << bytes.size() << " bytes)";
    return oss.str();
}

std::string FunctionDef::toString(int indent) const {
    std::ostringstream oss;
    oss << indent_str(indent) << "FunctionDef(" << name << ")\n";
    for (const auto& stmt : body) {
        oss << stmt->toString(indent + 1) << "\n";
    }
    return oss.str();
}

std::string Program::toString(int indent) const {
    std::ostringstream oss;
    oss << indent_str(indent) << "Program\n";
    oss << indent_str(indent + 1) << "Declarations:\n";
    for (const auto& decl : declarations) {
        oss << decl->toString(indent + 2) << "\n";
    }
    oss << indent_str(indent + 1) << "Functions:\n";
    for (const auto& func : functions) {
        oss << func->toString(indent + 2) << "\n";
    }
    return oss.str();
}
