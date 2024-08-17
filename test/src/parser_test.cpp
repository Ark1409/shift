#include <gtest/gtest.h>

#include "compiler/shift_compiler.h"

using namespace shift::compiler;

using namespace std::string_view_literals;

TEST(ShiftParser, ShouldParseModuleDeclaration) {
    error_handler handler;
    {
        tokenizer t(&handler, R"(module test_module;)");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        EXPECT_EQ(handler.get_error_count(), 0);
        EXPECT_EQ(p.get_module().name, "test_module"sv);
    }
}

TEST(ShiftParser, ShouldErrorOnMultipleModuleDeclarations) {
    error_handler handler;
    {
        tokenizer t(&handler, R"(module test_module; module test_module2;)");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        EXPECT_NE(handler.get_error_count(), 0);
    }
}

TEST(ShiftParser, ShouldErrorOnFunctionDeclarationWithoutModule) {
    error_handler handler;
    {
        tokenizer t(&handler, R"(void test_function() {})");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        EXPECT_NE(handler.get_error_count(), 0);
    }
}

TEST(ShiftParser, ShouldErrorOnVariableDeclarationWithoutModule) {
    error_handler handler;
    {
        tokenizer t(&handler, R"(int v = 3;)");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        EXPECT_NE(handler.get_error_count(), 0);
    }
}

TEST(ShiftParser, ShouldErrorOnClassDeclarationWithoutModule) {
    error_handler handler;
    {
        tokenizer t(&handler, R"(class test_class {})");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        EXPECT_NE(handler.get_error_count(), 0);
    }
}