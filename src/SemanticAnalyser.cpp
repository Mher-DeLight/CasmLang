#include "../include/SemanticAnalyser.h"
#include "../include/ErrorHandler.h"
#include <algorithm>
#include <iostream>

template <typename T> using uq = std::unique_ptr<T>;

// == HELPERS ==
std::unique_ptr<ScopeBlock> SemanticAnalyser::hand_over_AST() {
    return std::move(ast);
}
void SemanticAnalyser::load_ast(uq<ScopeBlock> ast_) {
    ast = std::move(ast_);
}
void SemanticAnalyser::analyse() {
    enter_scope();

    ast->accept(*this);
    exit_scope();
}
void SemanticAnalyser::semaPanic(const std::string& msg, SourceLocation src) {
    if (src.row == -1) {
        panic("[SEMANTIC ANALYSIS PANIC] " + msg);
        return;
    }

    panic("[SEMANTIC ANALYSIS PANIC] " + msg + " [AT " + std::to_string(src.row) + ":" +
          std::to_string(src.column) + "]");
}
ExpressionInfo SemanticAnalyser::analyseExpression(Expression* expr) {
    if (auto lt = dynamic_cast<StringLiteral*>(expr)) {
        return ExpressionInfo(false, true);
    } else if (auto lt = dynamic_cast<IntegerLiteral*>(expr)) {
        return ExpressionInfo(false, true);
    } else if (auto lt = dynamic_cast<FloatLiteral*>(expr)) {
        return ExpressionInfo(false, true);
    } else if (auto lt = dynamic_cast<BooleanLiteral*>(expr)) {
        return ExpressionInfo(false, true);
    } else if (auto lt = dynamic_cast<VoidLiteral*>(expr)) {
        return ExpressionInfo(false, true);
    } else if (auto var = dynamic_cast<VariableReference*>(expr)) {
        return ExpressionInfo(true, false);
    } else if (auto func = dynamic_cast<RoutineCallExpr*>(expr)) {
        const Symbol* symbol = getSymbol(func->identifier);
        if (symbol == nullptr)
            semaPanic("undeclared function \"" + func->identifier + "\"", func->location);
        return ExpressionInfo(false, true);
    } else if (auto bexpr = dynamic_cast<BinaryExpression*>(expr)) {
        auto leftInfo = analyseExpression(bexpr->left.get());
        auto rightInfo = analyseExpression(bexpr->right.get());

        return ExpressionInfo(false, true);
    } else if (auto uxpr = dynamic_cast<UnaryExpression*>(expr)) {
        uxpr->value->accept(*this);
        if ((uxpr->op == UnaryOperation::DECREMENT || uxpr->op == UnaryOperation::INCREMENT ||
             uxpr->op == UnaryOperation::NEGATE)) {
            semaPanic("cannot decrement/increment/negate non-integer/float value");
        }
        return ExpressionInfo(false, true);
    } else {
        semaPanic("invalid expression", expr->location);
    }
    return ExpressionInfo();
}

void SemanticAnalyser::enter_scope() {
    if (stack.size() > 0)
        stack.push_back(std::make_unique<Scope>(stack.back().get()));
    else
        stack.push_back(std::make_unique<Scope>(nullptr));
}
void SemanticAnalyser::exit_scope() {
    stack.pop_back();
}
void SemanticAnalyser::addSymbol(const Symbol& symbol) {
    stack.back()->symbols[symbol.identifier] = std::make_unique<Symbol>(symbol);
}
bool SemanticAnalyser::symbolExists(const std::string& identifier) const {
    Scope* scp = stack.back().get();
    while (scp != nullptr) {
        if (scp->symbols.contains(identifier))
            return true;

        scp = scp->parent;
    }
    return false;
}
const Symbol* SemanticAnalyser::getSymbol(const std::string& identifier) const {
    Scope* scp = stack.back().get();
    while (scp != nullptr) {
        if (scp->symbols.contains(identifier))
            return scp->symbols[identifier].get();
        if (scp->parent == nullptr)
            break;
        scp = scp->parent;
    }
    return nullptr;
}
const Symbol* SemanticAnalyser::getSymbolFromMemory(const std::string& memname) const {
    Scope* scp = stack.back().get();
    while (scp != nullptr) {
        for (auto& pair : scp->symbols) {
            if (pair.second->memoryName->name == memname) {
                return pair.second.get();
            }
        }
        if (scp->parent == nullptr)
            break;
        scp = scp->parent;
    }
    return nullptr;
}
void SemanticAnalyser::removeSymbol(const std::string& identifier) {
    Scope* scp = stack.back().get();
    while (scp != nullptr) {
        if (scp->symbols.contains(identifier)) {
            scp->symbols.erase(identifier);
            return;
        }
        scp = scp->parent;
    }
    semaPanic("Internal, cannot erase symbol \"" + identifier + "\"; it does not exist");
}

// == VISIT ==
void SemanticAnalyser::visit(ScopeBlock& node) {
    enter_scope();
    for (auto& child : node.children) {
        child->accept(*this);
    }
    exit_scope();
}
void SemanticAnalyser::visit(StringLiteral& node) {
    // literally what could go wrong in a string literal (famous last words)
}
void SemanticAnalyser::visit(FloatLiteral& node) {}
void SemanticAnalyser::visit(IntegerLiteral& node) {}
void SemanticAnalyser::visit(BooleanLiteral& node) {}
void SemanticAnalyser::visit(VoidLiteral& node) {}
void SemanticAnalyser::visit(BinaryExpression& node) {
    analyseExpression(&node);
}
void SemanticAnalyser::visit(RoutineCallExpr& node) {
    if (!symbolExists(node.identifier)) {
        semaPanic("cannot reference function \"" + node.identifier + "\"; it does not exist.",
                  node.location);
    }
    if (getSymbol(node.identifier)->kind != SymbolKind::Routine) {
        semaPanic("\"" + node.identifier + "\" is used as a function even though it is a variable",
                  node.location);
    }

    auto paramCount = getSymbol(node.identifier)->paramCount;
    if (node.args.size() < paramCount) {
        semaPanic("less arguments than requested");
    }
    if (node.args.size() > paramCount) {
        semaPanic("more arguments than requested");
    }
}
void SemanticAnalyser::visit(VariableReference& node) {
    if (!symbolExists(node.identifier)) {
        semaPanic("cannot reference variable \"" + node.identifier + "\"; it does not exist.",
                  node.location);
    }
    if (getSymbol(node.identifier)->kind != SymbolKind::Variable) {
        semaPanic("\"" + node.identifier + "\" is used as a variable even though it is a function",
                  node.location);
    }
    MemoryName* memory = getSymbol(node.identifier)->memoryName;
    if (memory == nullptr) {
        semaPanic("cannot refernce variable \"" + node.identifier +
                      "\" which is not assigned to any memory",
                  node.location);
    }
    node.name = memory->name;
    analyseExpression(&node);
}
void SemanticAnalyser::visit(VariableReassignment& node) {
    if (!symbolExists(node.identifier)) {
        semaPanic("cannot reassign variable \"" + node.identifier + "\"; it does not exist.",
                  node.location);
    }
    if (getSymbol(node.identifier)->kind != SymbolKind::Variable) {
        semaPanic("\"" + node.identifier + "\" is used as a variable even though it is a function",
                  node.location);
    }
    node.memory->accept(*this);
    MemoryName* memory = getSymbol(node.identifier)->memoryName;
    if (memory == nullptr) {
        semaPanic("Could not resolve memory name for identifier \"" + node.identifier + "\"",
                  node.location);
    }
    node.memory = copyMemoryInformation(memory);
    node.value->accept(*this);
}
void SemanticAnalyser::visit(VariableDefinition& node) {
    node.memory->accept(*this);
    auto othermem = getSymbolFromMemory(node.memory->name);
    if (othermem) {
        semaPanic("cannot reassign memory \"" + node.memory->name + "\" to \"" + node.identifier +
                  "\"; it is already taken by \"" + othermem->identifier + "\"");
    }
    node.value->accept(*this);
    addSymbol(Symbol(node.identifier, SymbolKind::Variable, 0, node.memory.get(), &node));
}
void SemanticAnalyser::visit(UnaryExpression& node) {
    node.value->accept(*this);
}
void SemanticAnalyser::visit(RoutineCallStmt& node) {
    if (!symbolExists(node.identifier)) {
        semaPanic("cannot reference function \"" + node.identifier + "\"; it does not exist.",
                  node.location);
    }
    auto symbol = getSymbol(node.identifier);
    if (symbol->kind != SymbolKind::Routine) {
        semaPanic("\"" + node.identifier + "\" is used as a function even though it is a variable",
                  node.location);
    }

    auto paramCount = symbol->paramCount;
    if (node.args.size() < paramCount) {
        semaPanic("less arguments than requested");
    }
    if (node.args.size() > paramCount) {
        semaPanic("more arguments than requested");
    }
    symbol->source->accept(*this);
}
void SemanticAnalyser::visit(RoutineDefinition& node) {
    addSymbol(Symbol(node.identifier, SymbolKind::Routine, 0, nullptr, &node));
    node.scope->accept(*this);
}
void SemanticAnalyser::visit(SectionDefinition& node) {
    addSymbol(Symbol(node.identifier, SymbolKind::Routine, 0, nullptr, &node));
}
void SemanticAnalyser::visit(Global& node) {
    if (!symbolExists(node.identifier))
        semaPanic("cannot make identifier \"" + node.identifier + "\" global; it is not declared",
                  node.location);

    if (getSymbol(node.identifier)->kind != SymbolKind::Routine) {
        semaPanic("cannot use 'global' on symbol \"" + node.identifier + "\"; it is not a routine");
    }
}
void SemanticAnalyser::visit(RoutineDeclaration& node) {
    addSymbol(Symbol(node.identifier, SymbolKind::Routine, 0, nullptr, &node));
}
void SemanticAnalyser::visit(AsmInstruction& node) {}
void SemanticAnalyser::visit(DeleteSymbol& node) {
    if (!symbolExists(node.identifier))
        semaPanic("cannot delete \"" + node.identifier + "\"; it is not declared");

    auto kind = getSymbol(node.identifier)->kind;
    removeSymbol(node.identifier);
}
void SemanticAnalyser::visit(FreeMemory& node) {
    auto symbol = getSymbolFromMemory(node.memoryName->name);

    if (!symbol) {
        semaPanic("Cannot free memory \"" + node.memoryName->name + "\"; it is not occupied");
        return;
    }

    removeSymbol(symbol->identifier);
}
void SemanticAnalyser::visit(IfStatement& node) {
    // the condition is an enum, nothing could possibly go wrong
    // (right?)

    node.scope->accept(*this);
}
void SemanticAnalyser::visit(Compare& node) {
    std::vector<Expression*> cdr = {node.left.get(), node.right.get()};

    for (auto& child : cdr) { // no need to copy logic, iterate through childreen
        if (auto memname = dynamic_cast<MemoryName*>(child)) {
            // == VARIABLE REFERENCE ==
            if (auto var = dynamic_cast<VariableReference*>(child)) {
                if (!symbolExists(var->identifier))
                    semaPanic("can't compare variable \"" + var->identifier +
                                  "\"; it is not declared",
                              var->location);
                auto symbol = getSymbol(var->identifier);
                if (symbol->kind != SymbolKind::Variable)
                    semaPanic("can't compare symbol \"" + var->identifier +
                                  "\"; it is not a variable",
                              var->location);

                memname->name = getSymbol(var->identifier)->memoryName->name;
            }
            // == REGISTER NAME ==
            else if (auto reg = dynamic_cast<RegisterName*>(child)) {
                memname->name = reg->name;
            }
        }
        // ====== INTEGER LITERAL ======
        else if (auto reg = dynamic_cast<IntegerLiteral*>(child)) {
        } else {
            semaPanic("invalid comparison node in compare", child->location);
        }
    }
}
void SemanticAnalyser::visit(RegisterName& node) {
    if (std::find(registers.begin(), registers.end(), node.name) == registers.end()) {
        semaPanic("invalid register name \"" + node.name + "\"", node.location);
    }
}
void SemanticAnalyser::visit(SyscallStatement& node) {}
void SemanticAnalyser::visit(ArithmeticOperation& node) {
    node.left->accept(*this);
    node.right->accept(*this);
}
void SemanticAnalyser::visit(RawAssignment& node) {
    node.left->accept(*this);
    node.right->accept(*this);
}
void SemanticAnalyser::visit(RawLabel& node) {
    auto symbol = getSymbol(node.labelname);
    if (symbol)
        semaPanic("cannot defined label \"" + node.labelname +
                  "\"; another symbol with the same name is already declared, and labels do not "
                  "support shadowing");

    addSymbol(Symbol(node.labelname, SymbolKind::Label, 0, nullptr, &node));
}
void SemanticAnalyser::visit(JumpStatement& node) {
    auto symbol = getSymbol(node.labelname);
    if (!symbol)
        semaPanic("cannot jump to label \"" + node.labelname + "\"; it is not defined");

    if (symbol->kind != SymbolKind::Label)
        semaPanic("cannot jump to symbol \"" + symbol->identifier + "\"; it is not a label");
}
void SemanticAnalyser::visit(WhileLoop& node) {
    node.comparison->accept(*this);
    node.scope->accept(*this);
}
void SemanticAnalyser::visit(SwapStatement& node) {
    node.left->accept(*this);
    node.right->accept(*this);
}
