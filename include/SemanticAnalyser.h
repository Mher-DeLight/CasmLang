#pragma once
#include "Common.h"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

template <typename T> using uq = std::unique_ptr<T>;

enum class SymbolKind { Variable, Routine, Label };
struct Symbol {
    std::string identifier;
    SymbolKind kind;
    MemoryName* memoryName;
    Statement* source;
    int paramCount = 0;

    Symbol(const std::string& identifier_, SymbolKind kind_, int paramCount_ = 0,
           MemoryName* memoryName_ = nullptr, Statement* source_ = nullptr)
        : identifier(identifier_), kind(kind_), paramCount(paramCount_), memoryName(memoryName_),
          source(source_) {}
};

class SemanticAnalyser : public Visitor {
    std::string emptystring = ""; // referenced later
    uq<ScopeBlock> ast = nullptr;
    struct Scope {
        Scope* parent = nullptr;
        std::unordered_map<std::string, uq<Symbol>> symbols;

        explicit Scope(Scope* parent_) : parent(parent_) {}
    };
    std::vector<uq<Scope>> stack;

    std::unique_ptr<MemoryName> copyMemoryInformation(MemoryName* source) {
        if (auto var = dynamic_cast<VariableReference*>(source)) {
            return std::make_unique<VariableReference>(source->location, source->name);
        } else if (auto var = dynamic_cast<RegisterName*>(source)) {
            return std::make_unique<RegisterName>(source->location, source->name);
        }

        semaPanic("internal, cannot find valid conversion type for MemoryName*", source->location);
        return std::make_unique<VariableReference>(source->location, "INVALID_CONVERSION_INTERNAL");
    }

    void enter_scope();
    void exit_scope();
    void addSymbol(const Symbol& symbol);
    void removeSymbol(const std::string& identifier);
    void semaPanic(const std::string& msg, SourceLocation src = SourceLocation());

    bool symbolExists(const std::string& identifier) const;
    const Symbol* getSymbol(const std::string& identifier) const;
    const Symbol* getSymbolFromMemory(const std::string& memName) const;

    ExpressionInfo analyseExpression(Expression* expr);

public:
    void visit(ScopeBlock& node) override;
    void visit(StringLiteral& node) override;
    void visit(FloatLiteral& node) override;
    void visit(IntegerLiteral& node) override;
    void visit(BooleanLiteral& node) override;
    void visit(VoidLiteral& node) override;
    void visit(VariableDefinition& node) override;
    void visit(BinaryExpression& node) override;
    void visit(RoutineCallExpr& node) override;
    void visit(VariableReference& node) override;
    void visit(UnaryExpression& node) override;
    void visit(VariableReassignment& node) override;
    void visit(RoutineCallStmt& node) override;
    void visit(RoutineDefinition& node) override;
    void visit(SectionDefinition& node) override;
    void visit(Global& node) override;
    void visit(RoutineDeclaration& node) override;
    void visit(AsmInstruction& node) override;
    void visit(DeleteSymbol& node) override;
    void visit(FreeMemory& node) override;
    void visit(IfStatement& node) override;
    void visit(Compare& node) override;
    void visit(RegisterName& node) override;
    void visit(SyscallStatement& node) override;
    void visit(ArithmeticOperation& node) override;
    void visit(RawAssignment& node) override;
    void visit(RawLabel& node) override;
    void visit(JumpStatement& node) override;
    void visit(WhileLoop& node) override;
    void visit(SwapStatement& node) override;

    std::unique_ptr<ScopeBlock> hand_over_AST();
    void load_ast(uq<ScopeBlock> ast_);
    void analyse();
};