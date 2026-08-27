#include "../include/Parser.h"
#include "../include/Common.h"
#include "../include/ErrorHandler.h"
#include "Tokenizer.h"
#include <algorithm>
#include <iostream>

void Parser::load_tokens(std::vector<ocarlang::Token> tkns) {
    tokens = std::move(tkns);
}
std::unique_ptr<ScopeBlock> Parser::hand_over_AST() {
    return std::move(entry_point);
}

void Parser::parse() {
    cursor = 0;
    entry_point = parseScope(false);
}
std::unique_ptr<ScopeBlock> Parser::parseScope(bool require_brackets) {
    std::unique_ptr<ScopeBlock> scope = std::make_unique<ScopeBlock>();

    if (require_brackets)
        eat(TokenType::LBrace, "expected '{' on scope entry");

    while (!isEnd()) {
        std::unique_ptr<ASTNode> node;
        if (check(TokenType::LBrace)) {
            node = parseScope(true); // arbitrary scoping
        } else {
            node = parseStatement();
        }
        if (node != nullptr) {
            scope->children.push_back(std::move(node));
        }

        if (match(TokenType::EndOfFile))
            break;
        if (require_brackets && match(TokenType::RBrace))
            break;
    }
    return std::move(scope);
}

// == PARSE FUNCTIONS ==
std::unique_ptr<Statement> Parser::parseStatement() {
    // TODO: change this to a switch statement
    if (match(TokenType::Semicolon))
        return nullptr;
    else if (check(TokenType::KeywordRegister))
        return parseVariableDeclaration();
    else if (check(TokenType::Identifier)) {
        if (peek(1).type == TokenType::LParen)
            return parseRoutineCallStmt();
        if (peek(1).type == TokenType::ArithmeticAssign)
            return parseArithmeticOperation();
        if (peek(1).type == TokenType::Colon)
            return parseRawLabel();
        return parseVariableReassignment();
    } else if (check(TokenType::KeywordRoutine)) {
        if (peek(4).type == TokenType::Semicolon)
            return parseRoutineDeclaration();
        return parseRoutineDefinition();
    } else if (check(TokenType::KeywordSection))
        return parseSectionDefinition();
    else if (check(TokenType::KeywordGlobal))
        return parseGlobal();
    else if (check(TokenType::AsmInstruction)) { // not worth its own function
        auto instr = peek().lexeme;
        advance();
        return std::make_unique<AsmInstruction>(instr);
    } else if (check(TokenType::KeywordDelete))
        return parseDeleteVar();
    else if (check(TokenType::KeywordFree))
        return parseFreeMemory();
    else if (check(TokenType::KeywordIf))
        return parseIfStatement();
    else if (check(TokenType::KeywordCompare))
        return parseCompareStatement();
    else if (check(TokenType::KeywordSyscall))
        return parseSyscall();
    else if (check(TokenType::KeywordRaw))
        return parseRawAssignment();
    else if (check(TokenType::KeywordJump))
        return parseJumpStatement();
    else if (check(TokenType::KeywordWhile))
        return parseWhileLoop();
    else if (check(TokenType::KeywordSwap))
        return parseSwapStatement();
    else {
        parserPanic("invalid token \"" + peek().lexeme + "\"", peek().location);
        return nullptr;
    }
}
std::unique_ptr<VariableDefinition> Parser::parseVariableDeclaration() {
    auto tkn = peek();
    std::unique_ptr<MemoryName> memname = parseMemoryName();

    std::string identifier =
        eat(TokenType::Identifier, "expected identifier for variable declaration").lexeme;

    eat(TokenType::EqualSign, "expected equals sign for variable declaration");

    std::unique_ptr<Expression> value = parseExpression();
    eat(TokenType::Semicolon, "expected semi colon after variable declaration");

    return std::make_unique<VariableDefinition>(tkn.location, std::move(memname), identifier,
                                                std::move(value));
}
std::unique_ptr<RoutineCallExpr> Parser::parseFunctionCallExpr() {
    ocarlang::Token tkn = eat(TokenType::Identifier, "expected identifier for function call");
    std::string identifier = tkn.lexeme;
    eat(TokenType::LParen, "expected '(' after identifier for function call");
    std::vector<std::unique_ptr<Expression>> args;

    while (!isEnd() && !match(TokenType::RParen)) {
        args.push_back(parseExpression());
        if (match(TokenType::RParen))
            break;
        if (isEnd()) {
            parserPanic("unexpected eof token before closing parenthesis of function call");
            break;
        }

        eat(TokenType::Comma);
    }

    return std::make_unique<RoutineCallExpr>(tkn.location, identifier, std::move(args));
}
std::unique_ptr<VariableReassignment> Parser::parseVariableReassignment() {
    ocarlang::Token tkn =
        eat(TokenType::Identifier, "expected identifier for variable reassignment");
    std::string identifier = tkn.lexeme;

    eat(TokenType::EqualSign, "expected equal sign after identifier in variable reassignment");
    std::unique_ptr<VariableReassignment> ptr =
        std::make_unique<VariableReassignment>(tkn.location, identifier, parseExpression());

    eat(TokenType::Semicolon, "expected semicolon after statement");
    return std::move(ptr);
}
std::unique_ptr<RoutineCallStmt> Parser::parseRoutineCallStmt() {
    ocarlang::Token tkn = eat(TokenType::Identifier, "expected identifier for routine call");
    std::string identifier = tkn.lexeme;

    eat(TokenType::LParen, "expected '(' after identifier for routine call");
    std::vector<std::unique_ptr<Expression>> args;

    while (!isEnd() && !match(TokenType::RParen)) {
        args.push_back(parseExpression());
        if (match(TokenType::RParen))
            break;
        if (isEnd()) {
            parserPanic("unexpected eof token before closing parenthesis of routine call");
            break;
        }

        eat(TokenType::Comma, "expected comma between routine arguments");
    }

    eat(TokenType::Semicolon, "expected semi colon after routine call");
    return std::make_unique<RoutineCallStmt>(tkn.location, identifier, std::move(args));
}
std::unique_ptr<SectionDefinition> Parser::parseSectionDefinition() {
    ocarlang::Token tkn =
        eat(TokenType::KeywordSection, "expected keyword 'sect' for section definition");

    std::string type_string = tkn.lexeme;

    std::string identifier = eat(TokenType::Identifier, "expected section identifier").lexeme;
    eat(TokenType::LParen, "expected '(' after identifier for section definition");
    eat(TokenType::RParen, "expected ')' after identifier for section definition");

    return std::make_unique<SectionDefinition>(identifier);
}
std::unique_ptr<Global> Parser::parseGlobal() {
    auto tkn = eat(TokenType::KeywordGlobal, "expected keyword 'global' for global definition");
    std::string identifier = eat(TokenType::Identifier, "expected identifier for global").lexeme;
    eat(TokenType::LParen, "expected function identifier for global");
    eat(TokenType::RParen, "expected function identifier for global");
    eat(TokenType::Semicolon, "expected semicolon after global statement");
    return std::make_unique<Global>(identifier);
}
std::unique_ptr<RoutineDefinition> Parser::parseRoutineDefinition() {
    ocarlang::Token tkn =
        eat(TokenType::KeywordRoutine, "expected keyword 'rtn' for routine definition");

    std::string type_string = tkn.lexeme;

    std::string identifier = eat(TokenType::Identifier, "expected routine identifier").lexeme;
    eat(TokenType::LParen, "expected '(' after identifier for routine definition");
    eat(TokenType::RParen, "expected ')' after identifier for routine definition");
    bool hasReturn = !match(TokenType::KeywordNoreturn);
    auto scope = parseScope(true);

    return std::make_unique<RoutineDefinition>(identifier, std::move(scope), hasReturn);
}
std::unique_ptr<RoutineDeclaration> Parser::parseRoutineDeclaration() {
    ocarlang::Token tkn =
        eat(TokenType::KeywordRoutine, "expected keyword 'rtn' for routine declaration");

    std::string type_string = tkn.lexeme;

    std::string identifier = eat(TokenType::Identifier, "expected routine identifier").lexeme;
    eat(TokenType::LParen, "expected '(' after identifier for routine declaration");
    eat(TokenType::RParen, "expected ')' after identifier for routine declaration");
    eat(TokenType::Semicolon, "expected semicolon after forward routine declaration");

    return std::make_unique<RoutineDeclaration>(identifier);
}
std::unique_ptr<DeleteSymbol> Parser::parseDeleteVar() {
    auto tkn = eat(TokenType::KeywordDelete, "expected keyword 'delete' for delete statement");
    auto identifier = eat(TokenType::Identifier, "expected identifier after 'delete'").lexeme;
    eat(TokenType::Semicolon, "expected semicolon afted delete statement");

    return std::make_unique<DeleteSymbol>(identifier);
}
std::unique_ptr<FreeMemory> Parser::parseFreeMemory() {
    auto tkn = eat(TokenType::KeywordFree, "expected keyword 'free' for free statement");
    auto mem = parseMemoryName();
    if (auto varref = dynamic_cast<VariableReference*>(mem.get()))
        parserPanic("cannot free variable reference to \"" + varref->identifier +
                    "\"; consider using 'delete'");
    eat(TokenType::Semicolon, "expected semicolon afted free statement");

    return std::make_unique<FreeMemory>(std::move(mem));
}
std::unique_ptr<MemoryName> Parser::parseMemoryName() {
    ocarlang::Token tkn = peek();
    if (check(TokenType::KeywordRegister)) {
        tkn = eat(TokenType::KeywordRegister, "internal, expected register for memory name");
        return std::make_unique<RegisterName>(tkn.location, tkn.lexeme);
    } else if (check(TokenType::Identifier)) {
        tkn = eat(TokenType::Identifier, "internal, expected identifier for memory name");
        return std::make_unique<VariableReference>(tkn.location, tkn.lexeme);
    } else {
        parserPanic("expected variable identifier or register name for memory name", tkn.location);
    }
    return std::make_unique<VariableReference>(SourceLocation(), ""); // to quiet down the warnings
}
std::unique_ptr<IfStatement> Parser::parseIfStatement() {
    auto tkn = eat(TokenType::KeywordIf, "expectede keyword 'if' for if statement");
    auto cond_string = eat(TokenType::KeywordCompareCond).lexeme;
    auto scope = parseScope(true);

    using CC = ComparativeConditions;
    CC cond;
    if (cond_string == "greater")
        cond = CC::GT;
    else if (cond_string == "greater_equal")
        cond = CC::GTEQ;
    else if (cond_string == "less")
        cond = CC::LT;
    else if (cond_string == "less_equal")
        cond = CC::LTEQ;
    else if (cond_string == "equal")
        cond = CC::EQ;
    else if (cond_string == "not_equal")
        cond = CC::NEQ;
    else
        parserPanic("invalid condition \"" + cond_string + "\"", tkn.location);

    return std::make_unique<IfStatement>(tkn.location, cond, std::move(scope));
}
std::unique_ptr<Compare> Parser::parseCompareStatement() {
    auto tkn = eat(TokenType::KeywordCompare, "expected keyword 'compare' for compare statement");
    auto left = parseMemoryName();
    eat(TokenType::Comma, "expected comma after first memory name in 'compare' statement");
    auto right = parseExpression();

    return std::make_unique<Compare>(tkn.location, std::move(left), std::move(right));
}
std::unique_ptr<SyscallStatement> Parser::parseSyscall() {
    auto tkn = eat(TokenType::KeywordSyscall, "expected keyword 'syscall' for syscall statement");
    return std::make_unique<SyscallStatement>(tkn.location);
}
std::unique_ptr<ArithmeticOperation> Parser::parseArithmeticOperation() {
    auto left = eat(TokenType::Identifier, "expected identifier on the left");
    auto op = eat(TokenType::ArithmeticAssign,
                  "expected arithmetic assignment operator for arithmetic operation")
                  .lexeme;
    auto right = parseExpression();

    using BO = BinaryOperation;
    BO op_enum;
    switch (op.at(0)) {
        case '+':
            op_enum = BO::ADD;
            break;
        case '-':
            op_enum = BO::SUBTRACT;
            break;
        case '*':
            op_enum = BO::MULTIPLY;
            break;
        case '/':
            op_enum = BO::DIVIDE;
            break;
        default:
            break;
    }

    auto left_node = std::make_unique<VariableReference>(left.location, left.lexeme);
    return std::make_unique<ArithmeticOperation>(left.location, std::move(left_node), op_enum,
                                                 std::move(right));
}
std::unique_ptr<RawAssignment> Parser::parseRawAssignment() {
    auto tkn = eat(TokenType::KeywordRaw, "expected keyword 'raw' for raw assignment");

    auto mem = parseMemoryName();
    eat(TokenType::EqualSign, "expected equal sign after memory name in raw assignment");
    auto right = parseExpression();
    eat(TokenType::Semicolon, "expected semicolon after raw assignment");
    return std::make_unique<RawAssignment>(tkn.location, std::move(mem), std::move(right));
}
std::unique_ptr<RawLabel> Parser::parseRawLabel() {
    auto tkn = eat(TokenType::Identifier, "expected label identifier for label definition");
    eat(TokenType::Colon, "expected colon after identifier for label definition");

    return std::make_unique<RawLabel>(tkn.location, tkn.lexeme);
}
std::unique_ptr<JumpStatement> Parser::parseJumpStatement() {
    auto tkn = eat(TokenType::KeywordJump, "expected keyword 'jump' for jump statement");
    auto identifier =
        eat(TokenType::Identifier, "expected label identifier for jump statement").lexeme;

    eat(TokenType::Semicolon, "expected semicolon after jump statement");
    return std::make_unique<JumpStatement>(tkn.location, identifier);
}
std::unique_ptr<WhileLoop> Parser::parseWhileLoop() {
    auto tkn = eat(TokenType::KeywordWhile, "expected keyword 'while' for while loop");
    eat(TokenType::LParen, "expected '(' after 'while' in while loop");

    SourceLocation conditionLocation = peek().location;
    auto left = parseMemoryName();
    auto cond_string =
        eat(TokenType::KeywordCompareCond, "expected comparison condition in while loop").lexeme;
    auto right = parseExpression();

    eat(TokenType::RParen, "expected ')' after while loop comparison");

    auto scope = parseScope(true);

    // done parsing, now we'll emit the actual nodes
    using CC = ComparativeConditions;
    CC cond;
    {
        if (cond_string == "greater")
            cond = CC::GT;
        else if (cond_string == "greater_equal")
            cond = CC::GTEQ;
        else if (cond_string == "less")
            cond = CC::LT;
        else if (cond_string == "less_equal")
            cond = CC::LTEQ;
        else if (cond_string == "equal")
            cond = CC::EQ;
        else if (cond_string == "not_equal")
            cond = CC::NEQ;
        else
            parserPanic("invalid condition \"" + cond_string + "\"", tkn.location);
    }

    auto comparison =
        std::make_unique<Compare>(conditionLocation, std::move(left), std::move(right));
    return std::make_unique<WhileLoop>(std::move(comparison), cond, std::move(scope));
}
std::unique_ptr<SwapStatement> Parser::parseSwapStatement() {
    auto tkn = eat(TokenType::KeywordSwap, "expected keyword 'swap' for swap statement");
    auto left = parseMemoryName();
    match(TokenType::Comma); // optional
    auto right = parseMemoryName();

    eat(TokenType::Semicolon, "expected semicolon after swap statement");
    return std::make_unique<SwapStatement>(tkn.location, std::move(left), std::move(right));
}

// == EXPRESSION PARSERS ==
std::unique_ptr<Expression> Parser::parseExpression() {
    return parseFactor();
}
std::unique_ptr<Expression> Parser::parseFactor() {
    auto tkn = peek();
    if (check(TokenType::LParen)) {
        eat(TokenType::LParen, "expected '('");
        auto expr = parseExpression();
        eat(TokenType::RParen, "expected ')'");
        return std::move(expr);
    } else if (check(TokenType::Identifier)) {
        std::string identifier = eat(TokenType::Identifier, "expected identifier").lexeme;
        if (match(TokenType::LParen)) {
            advance(-2);
            return parseFunctionCallExpr();
        }
        return std::make_unique<VariableReference>(tkn.location, identifier);
    }
    return parseLiteral();
}
std::unique_ptr<Literal> Parser::parseLiteral() {
    auto token = peek();
    std::unique_ptr<Literal> returnNode;
    bool doAdvance = true;

    if (token.type == TokenType::StringLiteral) {
        returnNode = std::make_unique<StringLiteral>(token.location, token.lexeme);
    } else if (token.type == TokenType::BoolLiteral &&
               (token.lexeme == "true" || token.lexeme == "false")) {
        returnNode = std::make_unique<BooleanLiteral>(token.location, token.lexeme == "true");
    } else if (token.type == TokenType::IntegerLiteral) {
        returnNode = std::make_unique<IntegerLiteral>(token.location, std::stoi(token.lexeme));
    } else if (token.type == TokenType::FloatLiteral) {
        returnNode = std::make_unique<FloatLiteral>(token.location, std::stof(token.lexeme));
    } else if (isMemory(true)) {
        returnNode = parseMemoryName();
        doAdvance = false; // parseMemoryName() already eats the token
    } else {
        parserPanic("invalid token " + token.lexeme, token.location);
    }

    if (doAdvance)
        advance();
    return std::move(returnNode);
}
std::unique_ptr<IntegerLiteral> Parser::parseInteger() {
    bool negative = false;
    if (match(TokenType::Minus)) {
        negative = true;
    }

    ocarlang::Token tkn = eat(TokenType::IntegerLiteral, "expected integer literal");
    return std::make_unique<IntegerLiteral>(tkn.location, std::stoi(tkn.lexeme));
}
std::unique_ptr<FloatLiteral> Parser::parseFloat() {
    bool negative = false;
    if (match(TokenType::Minus)) {
        negative = true;
    }

    ocarlang::Token tkn = eat(TokenType::FloatLiteral, "expected float literal");
    return std::make_unique<FloatLiteral>(tkn.location, std::stof(tkn.lexeme));
}
std::unique_ptr<StringLiteral> Parser::parseString() {
    ocarlang::Token tkn = eat(TokenType::StringLiteral, "expected a string literal");
    return std::make_unique<StringLiteral>(tkn.location, tkn.lexeme);
}

// == HELPERS ==
ocarlang::Token& Parser::peek(int offset) {
    if (cursor + offset > tokens.size()) {
        parserPanic("cannot peek into token number " + std::to_string(cursor + offset) +
                    "; such token does not exist.");
    }
    return tokens[cursor + offset];
}
ocarlang::Token& Parser::previous(int offset) {
    if (cursor - offset > tokens.size()) {
        parserPanic("cannot peek into previous token number " + std::to_string(cursor - offset) +
                    "; such token does not exist.");
    }
    return tokens[cursor - offset];
}
bool Parser::isEnd() {
    return check(TokenType::EndOfFile);
}
bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    } else {
        return false;
    }
}
bool Parser::check(TokenType type) {
    if (peek().type == type) {
        return true;
    }
    return false;
}
void Parser::advance(int offset) {
    if ((cursor + offset > tokens.size()) && offset > 0) {
        parserPanic("cannot advance to position " + std::to_string(cursor + offset) +
                    "; position is out of bounds.");
    }
    if ((cursor + offset < 0) && offset < 0) {
        parserPanic("cannot advance to position " + std::to_string(cursor + offset) +
                    "; position is out of bounds.");
    }
    cursor += offset;
}
ocarlang::Token Parser::eat(TokenType type, const std::string& msg) {
    if (!check(type))
        parserPanic(msg, peek().location);

    ocarlang::Token t = peek();
    advance();
    return t;
}
void Parser::parserPanic(const std::string& msg, const SourceLocation& src) {
    panic("[PARSER PANIC] " + msg + " [AT " + std::to_string(src.row) + ":" +
          std::to_string(src.column) + "]");
}
bool Parser::isMemory(bool allowVars) {
    auto tkn = peek();
    if (std::find(registers.begin(), registers.end(), tkn.lexeme) != registers.end()) {
        return true;
    }
    if (tkn.type == TokenType::Identifier && allowVars) {
        // assume the identifier is a variable
        // let's just hope semantic analysis will catch it otherwise
        return true;
    }
    return false;
}