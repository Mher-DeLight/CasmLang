#pragma once
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

enum class TokenType {
    None,

    Identifier,
    StringLiteral,
    IntegerLiteral,
    FloatLiteral,
    CharLiteral,
    BoolLiteral,
    NullLiteral,

    AsmInstruction,

    LParen,
    RParen,
    LBrace,
    RBrace,
    Comma,
    Semicolon,
    Colon,
    EqualSign,
    Minus,
    SnailSign,
    ArithmeticAssign,

    KeywordRegister,
    KeywordRoutine,
    KeywordSection,
    KeywordGlobal,
    KeywordNoreturn,
    KeywordDelete,
    KeywordFree,
    KeywordIf,
    KeywordCompareCond,
    KeywordCompare,
    KeywordSyscall,
    KeywordArithmeticOperation,
    KeywordRaw,
    KeywordJump,
    KeywordWhile,
    KeywordSwap,

    EndOfFile
};
enum class AssignOperation {
    ASSIGN,
    ADDASSIGN,
    SUBTRACTASSIGN,
    MULTIPLYASSIGN,
    DIVIDEASSIGN,
    POWERASSIGN
};
enum class ComparativeConditions { EQ, NEQ, LT, LTEQ, GT, GTEQ };
enum class BinaryOperation { ADD, SUBTRACT, MULTIPLY, DIVIDE };
enum class UnaryOperation { COMPLEMENT, DECREMENT, INCREMENT, NEGATE };

class SourceLocation {
public:
    int row = -1;
    int column = -1;

    SourceLocation(int r, int c) : row(r), column(c) {}
    SourceLocation() = default;
};
const std::unordered_map<std::string, TokenType> word_table{
    {{"", TokenType::None},
     {"rtn", TokenType::KeywordRoutine},
     {"sect", TokenType::KeywordSection},
     {"global", TokenType::KeywordGlobal},
     {"noret", TokenType::KeywordNoreturn},
     {"delete", TokenType::KeywordDelete},
     {"free", TokenType::KeywordFree},
     {"if", TokenType::KeywordIf},
     {"compare", TokenType::KeywordCompare},
     {"syscall", TokenType::KeywordSyscall},
     {"raw", TokenType::KeywordRaw},
     {"jump", TokenType::KeywordJump},
     {"while", TokenType::KeywordWhile},
     {"swap", TokenType::KeywordSwap},

     {"greater", TokenType::KeywordCompareCond},
     {"greater_equal", TokenType::KeywordCompareCond},
     {"less", TokenType::KeywordCompareCond},
     {"less_equal", TokenType::KeywordCompareCond},
     {"equal", TokenType::KeywordCompareCond},
     {"not_equal", TokenType::KeywordCompareCond},

     {"rax", TokenType::KeywordRegister},
     {"rbx", TokenType::KeywordRegister},
     {"rcx", TokenType::KeywordRegister},
     {"rdx", TokenType::KeywordRegister},
     {"rsi", TokenType::KeywordRegister},
     {"rdi", TokenType::KeywordRegister},
     {"rbp", TokenType::KeywordRegister},
     {"rsp", TokenType::KeywordRegister},
     {"r8", TokenType::KeywordRegister},
     {"r9", TokenType::KeywordRegister},
     {"r10", TokenType::KeywordRegister},
     {"r11", TokenType::KeywordRegister},
     {"r12", TokenType::KeywordRegister},
     {"r13", TokenType::KeywordRegister},
     {"r14", TokenType::KeywordRegister},
     {"r15", TokenType::KeywordRegister},

     {"true", TokenType::BoolLiteral},
     {"false", TokenType::BoolLiteral},

     {"+=", TokenType::ArithmeticAssign},
     {"-=", TokenType::ArithmeticAssign},
     {"*=", TokenType::ArithmeticAssign},

     {"__END_OF_FILE__", TokenType::EndOfFile},
     {"(", TokenType::LParen},
     {")", TokenType::RParen},
     {"{", TokenType::LBrace},
     {"}", TokenType::RBrace},
     {":", TokenType::Colon},
     {";", TokenType::Semicolon},
     {"-", TokenType::Minus},
     {",", TokenType::Comma},
     {"@", TokenType::SnailSign},
     {"=", TokenType::EqualSign}}};
struct ExpressionInfo {
    bool isLValue = false;
    bool isConstant = false;

    ExpressionInfo(bool isLValue_ = false, bool isConstant_ = false)
        : isLValue(isLValue_), isConstant(isConstant_) {}
};
const std::array<std::string, 16> registers = {{
    "rax",
    "rbx",
    "rcx",
    "rdx",
    "rsi",
    "rdi",
    "rbp", // base pointer
    "rsp", // stack pointer
    "r8",
    "r9",
    "r10",
    "r11",
    "r12",
    "r13",
    "r14",
    "r15",
}};

class Visitor;
class PrettyPrinter;

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void accept(Visitor& visitor) = 0;
};
class ScopeBlock : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> children;

    ScopeBlock(std::vector<std::unique_ptr<ASTNode>> children_) : children(std::move(children_)) {}
    ScopeBlock() = default;
    void accept(Visitor& visitor) override;
};

class Expression : public ASTNode {
public:
    SourceLocation location;

    virtual ~Expression() = default;
    virtual void accept(Visitor& visitor) = 0;

    Expression(const SourceLocation& lct) : location(lct) {}
};
class Literal : public Expression {
public:
    virtual ~Literal() = default;

    Literal(const SourceLocation& lct) : Expression(lct) {}
    virtual void accept(Visitor& visitor) = 0;
};
class StringLiteral : public Literal {
public:
    std::string string;

    StringLiteral(const SourceLocation& lct, const std::string str = "")
        : Literal(lct), string(str) {}
    void accept(Visitor& visitor) override;
};
class FloatLiteral : public Literal {
public:
    float number;

    FloatLiteral(const SourceLocation& lct, const float number_) : Literal(lct), number(number_) {}
    void accept(Visitor& visitor) override;
};
class IntegerLiteral : public Literal {
public:
    int number;

    IntegerLiteral(const SourceLocation& lct, const float number_)
        : Literal(lct), number(number_) {}
    void accept(Visitor& visitor) override;
};
class BooleanLiteral : public Literal {
public:
    bool state;

    BooleanLiteral(const SourceLocation& lct, const bool state_) : Literal(lct), state(state_) {}
    void accept(Visitor& visitor) override;
};
class VoidLiteral : public Literal {
public:
    void accept(Visitor& visitor) override;
};

class MemoryName : public Literal {
public:
    std::string name; // annotated by the semantic analyser
    virtual ~MemoryName() = default;
    virtual void accept(Visitor& visitor) = 0;

    MemoryName(const SourceLocation& lct, const std::string& name_ = "")
        : Literal(lct), name(name_) {}
};
class BinaryExpression : public Expression {
public:
    std::unique_ptr<Expression> left;
    BinaryOperation op;
    std::unique_ptr<Expression> right;

    void accept(Visitor& visitor) override;
    BinaryExpression(const SourceLocation& src, std::unique_ptr<Expression> l, BinaryOperation op_,
                     std::unique_ptr<Expression> r)
        : Expression(src), left(std::move(l)), op(op_), right(std::move(r)) {}
};
class RoutineCallExpr : public Expression {
public:
    std::string identifier;
    std::vector<std::unique_ptr<Expression>> args;

    void accept(Visitor& visitor) override;
    RoutineCallExpr(const SourceLocation& src, const std::string& identifier_,
                    std::vector<std::unique_ptr<Expression>> args_)
        : Expression(src), identifier(identifier_), args(std::move(args_)) {}
};
class VariableReference : public MemoryName {
public:
    std::string identifier;

    void accept(Visitor& visitor) override;
    VariableReference(const SourceLocation& src, const std::string& identifier_)
        : MemoryName(src), identifier(identifier_) {}
};
class RegisterName : public MemoryName {
public:
    void accept(Visitor& visitor) override;
    RegisterName(const SourceLocation& src, const std::string& registerName_)
        : MemoryName(src, registerName_) {}
};
class UnaryExpression : public Expression {
public:
    std::unique_ptr<Expression> value;
    UnaryOperation op;

    void accept(Visitor& visitor) override;
    UnaryExpression(const SourceLocation& src, std::unique_ptr<Expression> value_,
                    UnaryOperation op_)
        : Expression(src), value(std::move(value_)), op(op_) {}
};

class Statement : public ASTNode {
public:
    SourceLocation location;

    virtual ~Statement() = default;
    virtual void accept(Visitor& visitor) = 0;

    Statement(const SourceLocation& lct) : location(lct) {}
    Statement() {}
};
class ConditionalStatement : public Statement {
public:
    virtual ~ConditionalStatement() = default;
    virtual void accept(Visitor& visitor) = 0;

    ConditionalStatement(const SourceLocation& lct) : Statement(lct) {}
    ConditionalStatement() : Statement() {}
};
class VariableDefinition : public Statement {
public:
    std::string identifier;
    std::unique_ptr<MemoryName> memory;
    std::unique_ptr<Expression> value;
    void accept(Visitor& visitor) override;

    VariableDefinition(SourceLocation& src, std::unique_ptr<MemoryName> memory_,
                       const std::string& identifier_, std::unique_ptr<Expression> value_)
        : Statement(src), memory(std::move(memory_)), identifier(identifier_),
          value(std::move(value_)) {}
};
class VariableReassignment : public Statement {
public:
    std::string identifier;
    std::unique_ptr<MemoryName> memory;
    std::unique_ptr<Expression> value;

    void accept(Visitor& visitor) override;

    VariableReassignment(SourceLocation& src, const std::string& identifier_,
                         std::unique_ptr<Expression> value_)
        : Statement(src), identifier(identifier_), value(std::move(value_)) {}
};
class RoutineCallStmt : public Statement {
public:
    std::string identifier;
    std::vector<std::unique_ptr<Expression>> args;

    void accept(Visitor& visitor) override;

    RoutineCallStmt(SourceLocation& src, const std::string& identifier_,
                    std::vector<std::unique_ptr<Expression>> args_)
        : Statement(src), identifier(identifier_), args(std::move(args_)) {}
};
class RoutineDefinition : public Statement {
public:
    std::string identifier;
    std::unique_ptr<ScopeBlock> scope;
    bool hasRet = true;

    void accept(Visitor& visitor) override;

    RoutineDefinition(const std::string& identifier_, std::unique_ptr<ScopeBlock> scope_ = nullptr,
                      bool hasRet_ = true)
        : identifier(identifier_), scope(std::move(scope_)), hasRet(hasRet_) {}
};
class SectionDefinition : public Statement {
public:
    std::string identifier;

    void accept(Visitor& visitor) override;

    SectionDefinition(const std::string& identifier_) : identifier(identifier_) {}
};
class Global : public Statement {
public:
    std::string identifier;

    void accept(Visitor& visitor) override;

    Global(const std::string& identifier_) : identifier(identifier_) {}
};
class RoutineDeclaration : public Statement {
public:
    std::string identifier;

    RoutineDeclaration(const std::string& identifier_) : identifier(identifier_) {}

    void accept(Visitor& visitor) override;
};
class AsmInstruction : public Statement {
public:
    std::string instruction;
    void accept(Visitor& visitor) override;

    AsmInstruction(const std::string& instruction_) : instruction(instruction_) {}
};
class DeleteSymbol : public Statement {
public:
    std::string identifier;
    void accept(Visitor& visitor) override;

    DeleteSymbol(const std::string& identifier_) : identifier(identifier_) {}
};
class FreeMemory : public Statement {
public:
    std::unique_ptr<MemoryName> memoryName;
    void accept(Visitor& visitor) override;

    FreeMemory(std::unique_ptr<MemoryName> memoryName_) : memoryName(std::move(memoryName_)) {}
};
class IfStatement : public ConditionalStatement {
public:
    ComparativeConditions cond;
    std::unique_ptr<ScopeBlock> scope;
    void accept(Visitor& visitor) override;

    IfStatement(SourceLocation& lct, ComparativeConditions cond_,
                std::unique_ptr<ScopeBlock> scope_)
        : ConditionalStatement(lct), cond(cond_), scope(std::move(scope_)) {}
};
class Compare : public Statement {
public:
    std::unique_ptr<MemoryName> left;
    std::unique_ptr<Expression> right;
    void accept(Visitor& visitor) override;

    Compare(const SourceLocation& lct, std::unique_ptr<MemoryName> left_,
            std::unique_ptr<Expression> right_)
        : Statement(lct), left(std::move(left_)), right(std::move(right_)) {}
};
class SyscallStatement : public Statement {
public:
    void accept(Visitor& visitor) override;

    SyscallStatement(const SourceLocation& lct) : Statement(lct) {}
};
class ArithmeticOperation : public Statement {
public:
    std::unique_ptr<MemoryName> left;
    BinaryOperation operation;
    std::unique_ptr<Expression> right;

    ArithmeticOperation(const SourceLocation& lct, std::unique_ptr<MemoryName> left_,
                        BinaryOperation op, std::unique_ptr<Expression> right_)
        : Statement(lct), left(std::move(left_)), operation(op), right(std::move(right_)) {}
    void accept(Visitor& visitor) override;
};
class RawAssignment : public Statement {
public:
    std::unique_ptr<MemoryName> left;
    std::unique_ptr<Expression> right;

    RawAssignment(const SourceLocation& lct, std::unique_ptr<MemoryName> left_,
                  std::unique_ptr<Expression> right_)
        : Statement(lct), left(std::move(left_)), right(std::move(right_)) {}
    void accept(Visitor& visitor) override;
};
class RawLabel : public Statement {
public:
    std::string labelname;

    RawLabel(const SourceLocation& lct, const std::string& labelname_)
        : Statement(lct), labelname(labelname_) {}
    void accept(Visitor& visitor) override;
};
class JumpStatement : public Statement {
public:
    std::string labelname;

    JumpStatement(const SourceLocation& lct, const std::string& labelname_)
        : Statement(lct), labelname(labelname_) {}
    void accept(Visitor& visitor) override;
};
class WhileLoop : public Statement {
public:
    std::unique_ptr<Compare> comparison;
    ComparativeConditions cond;
    // the user is not exposed to the actual comparison and conditions
    std::unique_ptr<ScopeBlock> scope;

    WhileLoop(std::unique_ptr<Compare> comparison_, ComparativeConditions cond_,
              std::unique_ptr<ScopeBlock> scope_)
        : comparison(std::move(comparison_)), cond(cond_), scope(std::move(scope_)) {}
    void accept(Visitor& visitor) override;
};
class SwapStatement : public Statement {
public:
    std::unique_ptr<MemoryName> left;
    std::unique_ptr<MemoryName> right;

    SwapStatement(const SourceLocation& lct, std::unique_ptr<MemoryName> left_,
                  std::unique_ptr<MemoryName> right_)
        : Statement(lct), left(std::move(left_)), right(std::move(right_)) {}
    void accept(Visitor& visitor) override;
};

class Visitor {
public:
    virtual ~Visitor() = default;

    virtual void visit(ScopeBlock& node) = 0;
    virtual void visit(StringLiteral& node) = 0;
    virtual void visit(FloatLiteral& node) = 0;
    virtual void visit(IntegerLiteral& node) = 0;
    virtual void visit(BooleanLiteral& node) = 0;
    virtual void visit(VoidLiteral& node) = 0;
    virtual void visit(VariableDefinition& node) = 0;
    virtual void visit(BinaryExpression& node) = 0;
    virtual void visit(RoutineCallExpr& node) = 0;
    virtual void visit(VariableReference& node) = 0;
    virtual void visit(RegisterName& node) = 0;
    virtual void visit(UnaryExpression& node) = 0;
    virtual void visit(VariableReassignment& node) = 0;
    virtual void visit(RoutineCallStmt& node) = 0;
    virtual void visit(RoutineDefinition& node) = 0;
    virtual void visit(SectionDefinition& node) = 0;
    virtual void visit(Global& node) = 0;
    virtual void visit(RoutineDeclaration& node) = 0;
    virtual void visit(AsmInstruction& node) = 0;
    virtual void visit(DeleteSymbol& node) = 0;
    virtual void visit(FreeMemory& node) = 0;
    virtual void visit(IfStatement& node) = 0;
    virtual void visit(Compare& node) = 0;
    virtual void visit(SyscallStatement& node) = 0;
    virtual void visit(ArithmeticOperation& node) = 0;
    virtual void visit(RawAssignment& node) = 0;
    virtual void visit(RawLabel& node) = 0;
    virtual void visit(JumpStatement& node) = 0;
    virtual void visit(WhileLoop& node) = 0;
    virtual void visit(SwapStatement& node) = 0;
};
class PrettyPrinter : public Visitor {
private:
    int indent = 0;
    std::ostream& stream;
    void printIndent();

public:
    PrettyPrinter(std::ostream& stream_) : stream(stream_) {}

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
};

inline void ScopeBlock::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void StringLiteral::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void FloatLiteral::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void IntegerLiteral::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void BooleanLiteral::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void VoidLiteral::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void VariableDefinition::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void BinaryExpression::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void RoutineCallExpr::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void VariableReference::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void UnaryExpression::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void VariableReassignment::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void RoutineCallStmt::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void RoutineDefinition::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void SectionDefinition::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void Global::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void RoutineDeclaration::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void AsmInstruction::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void DeleteSymbol::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void FreeMemory::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void IfStatement::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void Compare::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void RegisterName::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void SyscallStatement::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void ArithmeticOperation::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void RawAssignment::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void RawLabel::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void JumpStatement::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void WhileLoop::accept(Visitor& visitor) {
    visitor.visit(*this);
}
inline void SwapStatement::accept(Visitor& visitor) {
    visitor.visit(*this);
}
