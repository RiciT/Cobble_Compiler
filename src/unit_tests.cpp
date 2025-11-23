#include <variant>
#include <string>
#include <vector>
#include <gtest/gtest.h>

#include "lexing/tokenization.hpp"
#include "parsing/parser.hpp"
#include "analysis/type_checker.hpp"
#include "codegen/generation.hpp"
#include "diagnostics/error_handler.hpp"

#pragma region Tokenizer Tests
class TokenizerTest : public ::testing::Test {
protected:
    static std::vector<Token> tokenize(const std::string& src) {
        Tokenizer tokenizer(src);
        return tokenizer.tokenize();
    }
};

// TEST_F(TokenizerTest, PerformanceCheck)
// {
//     const std::string src = "def int x = 10 + 20 * 30; if (x) { exit(0); }";
//
//     // Run it 100,000 times
//     for(int i = 0; i < 100000; ++i) {
//         Tokenizer t(src);
//         t.tokenize(); // Discard result
//     }
// }

TEST_F(TokenizerTest, IdentifiesKeywords) {
    const auto tokens = tokenize("exit def func if else return while print");

    EXPECT_EQ(tokens.size(), 8);
    EXPECT_EQ(tokens[0].type, TokenType::exit_);
    EXPECT_EQ(tokens[1].type, TokenType::def_);
    EXPECT_EQ(tokens[2].type, TokenType::func_);
    EXPECT_EQ(tokens[3].type, TokenType::if_);
    EXPECT_EQ(tokens[4].type, TokenType::else_);
    EXPECT_EQ(tokens[5].type, TokenType::return_);
    EXPECT_EQ(tokens[6].type, TokenType::while_);
    EXPECT_EQ(tokens[7].type, TokenType::print_);
}

TEST_F(TokenizerTest, IdentifiesLiteralsAndIdentifiers) {
    const auto tokens = tokenize("123 myVar 456");

    EXPECT_EQ(tokens.size(), 3);

    EXPECT_EQ(tokens[0].type, TokenType::int_lit);
    EXPECT_EQ(tokens[0].value.value(), "123");

    EXPECT_EQ(tokens[1].type, TokenType::ident);
    EXPECT_EQ(tokens[1].value.value(), "myVar");

    EXPECT_EQ(tokens[2].type, TokenType::int_lit);
    EXPECT_EQ(tokens[2].value.value(), "456");
}

TEST_F(TokenizerTest, IdentifiesOperators) {
    const auto tokens = tokenize("+ - * / = == != >= <= > < ( ) { } ; , [ ]");

    EXPECT_EQ(tokens.size(), 19);
    EXPECT_EQ(tokens[0].type, TokenType::plus_sign);
    EXPECT_EQ(tokens[5].type, TokenType::equals_equals);
    EXPECT_EQ(tokens[17].type, TokenType::open_bracket);
}

TEST_F(TokenizerTest, IgnoresComments) {
    const std::string src = R"(
        // This is a comment
        exit // inline comment
        /* Multi
           line
           comment */
        ;
    )";
    const auto tokens = tokenize(src);

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0].type, TokenType::exit_);
    EXPECT_EQ(tokens[1].type, TokenType::semi);
}
#pragma endregion

#pragma region Parser Tests
class ParserTest : public ::testing::Test {
protected:
    struct ParseResult {
        std::optional<NodeProgram> prog;
        ErrorHandler handler;
        //we must return the parser to keep the ArenaAllocator alive
        //otherwise AST nodes become dangling pointers
        std::shared_ptr<Parser> parser_ptr;
    };

    static ParseResult parse(const std::string& src) {
        Tokenizer tokenizer(src);
        std::vector<Token> tokens = tokenizer.tokenize();

        ErrorHandler handler;
        //using shared_ptr to keep parser alive in the result struct
        const auto parser = std::make_shared<Parser>(std::move(tokens), handler);
        const auto prog = parser->parse_prog();

        //copy handler state (errors) before returning
        //note: real error strings are copied in ErrorHandler, so this is safe.
        return { prog, handler, parser };
    }
};

TEST_F(ParserTest, ParsesExitStatement) {
    const auto result = parse("exit(0);");

    ASSERT_TRUE(result.prog.has_value());
    EXPECT_FALSE(result.handler.has_errors());
    ASSERT_EQ(result.prog->stmts.size(), 1);

    const NodeStmt* stmt = result.prog->stmts[0];
    EXPECT_TRUE(std::holds_alternative<NodeStmtExit*>(stmt->stmt));
}

TEST_F(ParserTest, ParsesVariableDefinition) {
    const auto result = parse("def int x = 10;");

    ASSERT_TRUE(result.prog.has_value());
    EXPECT_FALSE(result.handler.has_errors());

    const NodeStmt* stmt = result.prog->stmts[0];
    ASSERT_TRUE(std::holds_alternative<NodeStmtDef*>(stmt->stmt));

    const auto def = std::get<NodeStmtDef*>(stmt->stmt);
    EXPECT_EQ(def->ident.value.value(), "x");
    EXPECT_EQ(def->type.base, BaseType::int_);
}

TEST_F(ParserTest, EnforcesOperatorPrecedence) {
    //1 + 2 * 3 should be parsed as 1 + (2 * 3)
    const auto result = parse("def int x = 1 + 2 * 3;");
    ASSERT_TRUE(result.prog.has_value());

    const auto stmt = std::get<NodeStmtDef*>(result.prog->stmts[0]->stmt);
    const auto expr = stmt->expr.value(); // NodeExpr*

    ASSERT_TRUE(std::holds_alternative<NodeBinExpr*>(expr->expr));
    const auto binExpr = std::get<NodeBinExpr*>(expr->expr);
    ASSERT_TRUE(std::holds_alternative<NodeBinExprAdd*>(binExpr->bin_expr));
}

TEST_F(ParserTest, DetectsSyntaxErrors) {
    //missing semicolon
    const auto result = parse("exit(0)");

    EXPECT_TRUE(result.handler.has_errors());
}
#pragma endregion

#pragma region TypeChecker Tests
class TypeCheckerTest : public ParserTest {
protected:
    static ErrorHandler check(const std::string& src) {
        auto parse_res = parse(src);
        if (!parse_res.prog.has_value()) {
            return parse_res.handler;
        }

        //we create a new error handler for the type checker
        //to isolate analysis errors from parsing warnings (if any)
        ErrorHandler tc_handler;
        TypeChecker checker(parse_res.prog.value(), tc_handler);
        checker.analyse_program();
        return tc_handler;
    }
};

TEST_F(TypeCheckerTest, AcceptsValidCode) {
    const std::string src = R"(
        def int x = 10;
        def int y = x + 5;
        if (y > 10) {
            exit(0);
        }
    )";
    const ErrorHandler handler = check(src);
    EXPECT_FALSE(handler.has_errors());
}

TEST_F(TypeCheckerTest, DetectsTypeMismatchInAssignment) {
    const ErrorHandler handler = check("def int x = true;");
    EXPECT_TRUE(handler.has_errors());
}

TEST_F(TypeCheckerTest, DetectsUndeclaredVariables) {
    const ErrorHandler handler = check("x = 10;"); // x never defined
    EXPECT_TRUE(handler.has_errors());
}

TEST_F(TypeCheckerTest, DetectsRedeclaration) {
    const std::string src = R"(
        def int x = 1;
        def int x = 2;
    )";
    const ErrorHandler handler = check(src);
    EXPECT_TRUE(handler.has_errors());
}

TEST_F(TypeCheckerTest, EnforcesScopeRules) {
    const std::string src = R"(
        if (true) {
            def int inner = 5;
        }
        inner = 10; // Should fail, inner is out of scope
    )";
    const ErrorHandler handler = check(src);
    EXPECT_TRUE(handler.has_errors());
}

TEST_F(TypeCheckerTest, DetectsArgumentMismatch) {
    const std::string src = R"(
        func int add(def int a, def int b) {
            return a + b;
        }
        add(1); // Missing one arg
    )";
    const ErrorHandler handler = check(src);
    EXPECT_TRUE(handler.has_errors());
}
#pragma endregion

#pragma region Generator Tests
class GeneratorTest : public ParserTest {
protected:
    static std::string generate(const std::string& src) {
        const auto result = parse(src);
        if (!result.prog.has_value() || result.handler.has_errors()) {
            return "PARSE_ERROR";
        }

        //we assume valid types for generation test (skipping type checker here)
        Generator generator(result.prog.value());
        return generator.generate_program();
    }
};

TEST_F(GeneratorTest, GeneratesBasicExit) {
    const std::string asm_code = generate("exit(0);");

    EXPECT_TRUE(asm_code.find("global _start") != std::string::npos);
    EXPECT_TRUE(asm_code.find("_start:") != std::string::npos);

    EXPECT_TRUE(asm_code.find("mov rax, 60") != std::string::npos); // sys_exit
    EXPECT_TRUE(asm_code.find("syscall") != std::string::npos);
}

TEST_F(GeneratorTest, GeneratesVariableAssignment) {
    const std::string asm_code = generate("def int x = 5;");

    EXPECT_TRUE(asm_code.find("push") != std::string::npos);
}
#pragma endregion

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}