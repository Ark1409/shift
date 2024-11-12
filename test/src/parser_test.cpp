#include <gtest/gtest.h>

#include "compiler/shift_compiler.h"
#include "utils/utils.h"

using namespace shift;
using namespace shift::compiler;
using namespace shift::utils;

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

TEST(ShiftParser, ShouldErrorOnInvalidModuleName) {
    error_handler handler;
    {
        tokenizer t(&handler, R"(module test_module.;)");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        EXPECT_NE(handler.get_error_count(), 0);
        handler.clear_messages();
    }
    {
        tokenizer t(&handler, R"(module .test_module;)");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        EXPECT_NE(handler.get_error_count(), 0);
        handler.clear_messages();
    }
    {
        tokenizer t(&handler, R"(module operator;)");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        EXPECT_NE(handler.get_error_count(), 0);
        handler.clear_messages();
    }
    {
        tokenizer t(&handler, R"(module test.operator.test;)");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        EXPECT_NE(handler.get_error_count(), 0);
        handler.clear_messages();
    }
    {
        tokenizer t(&handler, R"(module test.operator;)");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        EXPECT_NE(handler.get_error_count(), 0);
        handler.clear_messages();
    }
    {
        tokenizer t(&handler, R"(module test.operator.;)");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        EXPECT_NE(handler.get_error_count(), 0);
        handler.clear_messages();
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
        handler.clear_messages();
    }
}

TEST(ShiftParser, ShouldErrorOnInvalidFunctionName) {
    error_handler handler;
    {
        tokenizer t(&handler, R"(module test; void operator() {})");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        EXPECT_NE(handler.get_error_count(), 0);
        handler.clear_messages();
    }
    {
        tokenizer t(&handler, R"(module test; void void() {})");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        EXPECT_NE(handler.get_error_count(), 0);
        handler.clear_messages();
    }

    {
        tokenizer t(&handler, R"(module test; void mv() {})");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        EXPECT_NE(handler.get_error_count(), 0);
        handler.clear_messages();
    }

    {
        tokenizer t(&handler, R"(module test; void +() {})");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        EXPECT_NE(handler.get_error_count(), 0);
        handler.clear_messages();
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

TEST(ShiftParser, ShouldErrorOnInvalidVariableName) {
    error_handler handler;
    {
        tokenizer t(&handler, R"(module test;
        int operator = 3;
        int + = 3;
        int operator+ = 3;
        int module = 3;)");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        EXPECT_GE(handler.get_error_count(), 4);
        handler.clear_messages();
    }
}

TEST(ShiftParser, ShouldErrorOnInvalidType) {
    error_handler handler;
    {
        tokenizer t(&handler, R"(module test;
        int. v = 3;
        int.[] v = 3;
        int[]. v = 3;
        int.test.*[] v = 3;
        .int.test.*[] v = 3;
        _5fdas[*] v = 3;)");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        EXPECT_GE(handler.get_error_count(), 6);
        handler.clear_messages();
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

TEST(ShiftParser, ShouldParseVariableDeclaration) {
    error_handler handler;
    {
        tokenizer t(&handler, R"(module test;
        int v = 3;
        shift.long[]* v1 = func("SS", 2) + 3;
        clazz var3; 
        clazz* var4 = null;
        shift.string[][] s = a[0][1+2];)");

        t.tokenize();

        parser p(&handler, &t);
        p.parse();

        EXPECT_EQ(handler.get_error_count(), 0);

        auto var_it = p.get_variables().begin();

        {
            auto& var = *var_it;
            EXPECT_EQ(var.type.name.name.to_string(), "int"sv);
            EXPECT_EQ(*var.name, "v"sv);
            EXPECT_EQ(var.value.type, token::type::INTEGER_LITERAL);
            EXPECT_EQ(var.value.begin->get_token_type(), token::type::INTEGER_LITERAL);
            ++var_it;
        }
        {
            auto& var = *var_it;
            EXPECT_EQ(var.type.name.name.to_string(), "shift.long"sv);
            {
                auto dim_it = var.type.dimensions.begin();
                {
                    auto& dim = *dim_it;
                    EXPECT_EQ(dim.type, shift_type::dimension::dimension_type::array);
                    EXPECT_EQ(dim.count, 1);
                    ++dim_it;
                }
                {
                    auto& dim = *dim_it;
                    EXPECT_EQ(dim.type, shift_type::dimension::dimension_type::pointer);
                    EXPECT_EQ(dim.count, 1);
                    ++dim_it;
                }
            }
            EXPECT_EQ(*var.name, "v1"sv);
            {
                EXPECT_EQ(var.value.type, token::type::PLUS);
                {
                    auto& left = *var.value.get_left();
                    EXPECT_TRUE(left.is_function_call());
                    {
                        const auto& call_obj = *left.get_function_call_object();
                        EXPECT_EQ(call_obj.type, token::type::IDENTIFIER);
                        EXPECT_EQ(call_obj.begin->get_token_type(), token::type::IDENTIFIER);
                        EXPECT_EQ(call_obj.begin->get_data(), "func"sv);
                    }
                    {
                        const auto& args = left.get_function_call_arguments();
                        EXPECT_EQ(args.size(), 2);
                        {
                            auto arg_it = args.begin();
                            {
                                auto& arg = *arg_it;
                                EXPECT_EQ(arg.type, token::type::STRING_LITERAL);
                                EXPECT_EQ(arg.begin->get_token_type(), token::type::STRING_LITERAL);
                                EXPECT_TRUE(utils::contains(arg.begin->get_data(), "SS"sv));
                                ++arg_it;

                            }
                            {
                                auto& arg = *arg_it;
                                EXPECT_EQ(arg.type, token::type::INTEGER_LITERAL);
                                EXPECT_EQ(arg.begin->get_token_type(), token::type::INTEGER_LITERAL);
                                EXPECT_EQ(arg.begin->get_data(), "2"sv);
                                ++arg_it;
                            }
                        }
                    }
                }
                {
                    auto& right = *var.value.get_right();
                    EXPECT_EQ(right.type, token::type::INTEGER_LITERAL);
                    EXPECT_EQ(right.begin->get_token_type(), token::type::INTEGER_LITERAL);
                    EXPECT_EQ(right.begin->get_data(), "3"sv);
                }
            }
            ++var_it;
        }
        {
            auto& var = *var_it;
            EXPECT_EQ(var.type.name.name.to_string(), "clazz"sv);
            EXPECT_EQ(*var.name, "var3"sv);
            EXPECT_EQ(var.value.type, token::type::NULL_TOKEN);
            EXPECT_TRUE(var.value.empty() || var.value.begin->get_token_type() == token::type::SEMICOLON);
            EXPECT_TRUE(var.type.dimensions.empty());
            ++var_it;
        }
        {
            auto& var = *var_it;
            EXPECT_EQ(var.type.name.name.to_string(), "clazz"sv);
            {
                auto dim_it = var.type.dimensions.begin();
                {
                    auto& dim = *dim_it;
                    EXPECT_EQ(dim.type, shift_type::dimension::dimension_type::pointer);
                    ++dim_it;
                }
            }
            EXPECT_EQ(*var.name, "var4"sv);
            EXPECT_EQ(var.value.type, token::type::IDENTIFIER);
            EXPECT_EQ(var.value.begin->get_data(), "null"sv);
            ++var_it;
        }
        {
            auto& var = *var_it;
            EXPECT_EQ(var.type.name.name.to_string(), "shift.string"sv);
            {
                auto dim_it = var.type.dimensions.begin();
                {
                    auto& dim = *dim_it;
                    EXPECT_EQ(dim.type, shift_type::dimension::dimension_type::array);
                    EXPECT_EQ(dim.count, 2);
                    ++dim_it;
                }
            }
            EXPECT_EQ(*var.name, "s"sv);
            {
                auto& value = var.value;
                EXPECT_TRUE(value.is_array());
                {
                    auto& array_object = *value.get_array_object();
                    EXPECT_EQ(array_object.type, token::type::IDENTIFIER);
                    EXPECT_EQ(array_object.begin->get_token_type(), token::type::IDENTIFIER);
                    EXPECT_TRUE(utils::contains(array_object.begin->get_data(), "a"sv));
                }
                {
                    auto array_dimension_it = value.get_array_dimensions().begin();
                    {
                        auto& indexer_expr = *array_dimension_it->get_array_indexer_expression();
                        EXPECT_EQ(indexer_expr.type, token::type::INTEGER_LITERAL);
                        EXPECT_EQ(indexer_expr.begin->get_token_type(), token::type::INTEGER_LITERAL);
                        EXPECT_EQ(indexer_expr.begin->get_data(), "0"sv);
                        ++array_dimension_it;
                    }
                    {
                        auto& indexer_expr = *array_dimension_it->get_array_indexer_expression();
                        EXPECT_EQ(indexer_expr.type, token::type::PLUS);
                        {
                            auto& left = *indexer_expr.get_left();
                            EXPECT_EQ(left.type, token::type::INTEGER_LITERAL);
                            EXPECT_EQ(left.begin->get_token_type(), token::type::INTEGER_LITERAL);
                            EXPECT_EQ(left.begin->get_data(), "1"sv);
                        }
                        {
                            auto& right = *indexer_expr.get_right();
                            EXPECT_EQ(right.type, token::type::INTEGER_LITERAL);
                            EXPECT_EQ(right.begin->get_token_type(), token::type::INTEGER_LITERAL);
                            EXPECT_EQ(right.begin->get_data(), "2"sv);
                        }
                    }
                }
            }
        }
    }
}

TEST(ShiftParser, ShouldParseFunctionDeclaration) {
    error_handler handler;
    {
        tokenizer t(&handler, R"(module test;
        void test_function() {}
        shift.long[]** test_function1(int a, ref shift.long b) { return null; }
        ref shift.string test_function2(shift.string, imut shift.string[][]*  b) { return (*test_function1(3,4))[5+3*i] + "h"[2]; }
        )");
        t.tokenize();

        parser p(&handler, &t);
        p.parse();

        {
            EXPECT_EQ(handler.get_error_count(), 0);
            handler.print();
            if (handler.get_error_count() != 0) {
                return;
            }
        }

        auto func_it = p.get_functions().begin();

        {
            auto& func = *func_it;
            EXPECT_EQ(func.return_type.name.name.to_string(), "void"sv);
            EXPECT_EQ(func.name.to_string(), "test_function"sv);
            EXPECT_TRUE(func.parameters.empty());
            EXPECT_TRUE(func.statements.empty());
            EXPECT_EQ(func.module_->to_string(), "test"sv);
            ++func_it;
        }
        {
            auto& func = *func_it;
            EXPECT_EQ(func.return_type.name.name.to_string(), "shift.long"sv);
            {
                ASSERT_EQ(func.return_type.dimensions.size(), 2);
                auto dim_it = func.return_type.dimensions.begin();
                {
                    auto& dim = *dim_it;
                    EXPECT_EQ(dim.type, shift_type::dimension::dimension_type::array);
                    EXPECT_EQ(dim.count, 1);
                    ++dim_it;
                }
                {
                    auto& dim = *dim_it;
                    EXPECT_EQ(dim.type, shift_type::dimension::dimension_type::pointer);
                    EXPECT_EQ(dim.count, 2);
                    ++dim_it;
                }
            }
            EXPECT_EQ(func.name.to_string(), "test_function1"sv);
            EXPECT_EQ(func.module_->to_string(), "test"sv);
            {
                ASSERT_EQ(func.parameters.size(), 2);

                auto param_it = func.parameters.begin();
                {
                    auto& [param_name, param_var] = *param_it;
                    EXPECT_EQ(param_var.type.name.name.to_string(), "int"sv);
                    EXPECT_EQ(param_var.name->get_data(), "a"sv);
                    EXPECT_EQ(param_name, "a"sv);
                    EXPECT_TRUE(param_var.type.dimensions.empty());
                    EXPECT_EQ(param_var.type.shift_mods, shift_mods::NONE);
                    ++param_it;
                }
                {
                    auto& [param_name, param_var] = *param_it;
                    EXPECT_EQ(param_var.type.name.name.to_string(), "shift.long"sv);
                    EXPECT_TRUE(param_var.type.dimensions.empty());
                    EXPECT_EQ(param_var.name->get_data(), "b"sv);
                    EXPECT_EQ(param_name, "b"sv);
                    EXPECT_EQ(param_var.type.ref_type, shift_type::reference_type::ref);
                    ++param_it;
                }
            }
            {
                ASSERT_EQ(func.statements.size(), 1);
                auto stmt = func.statements.begin();
                {
                    EXPECT_EQ(stmt->type, shift_statement::statement_type::return_);

                    auto& return_expr = stmt->get_return_statement();
                    EXPECT_EQ(return_expr.type, token::type::IDENTIFIER);
                    EXPECT_TRUE(return_expr.begin->is_null());

                    ++stmt;
                }
            }
            ++func_it;
        }
        {
            auto& func = *func_it;
            EXPECT_EQ(func.return_type.name.name.to_string(), "shift.string"sv);
            EXPECT_EQ(func.name.to_string(), "test_function2"sv);
            EXPECT_EQ(func.module_->to_string(), "test"sv);
            {
                ASSERT_EQ(func.parameters.size(), 2);

                auto param_it = func.parameters.begin();
                {
                    auto& [param_name, param_var] = *param_it;
                    EXPECT_EQ(param_var.type.name.name.to_string(), "shift.string"sv);
                    EXPECT_TRUE(param_var.type.dimensions.empty());
                    EXPECT_EQ(param_var.type.shift_mods, shift_mods::NONE);
                    ++param_it;
                }
                {
                    auto& [param_name, param_var] = *param_it;
                    EXPECT_EQ(param_var.type.name.name.to_string(), "shift.string"sv);
                    {
                        ASSERT_EQ(param_var.type.dimensions.size(), 2);
                        auto dim_it = param_var.type.dimensions.begin();
                        {
                            auto& dim = *dim_it;
                            EXPECT_EQ(dim.type, shift_type::dimension::dimension_type::array);
                            EXPECT_EQ(dim.count, 2);
                            ++dim_it;
                        }
                        {
                            auto& dim = *dim_it;
                            EXPECT_EQ(dim.type, shift_type::dimension::dimension_type::pointer);
                            EXPECT_EQ(dim.count, 1);
                            ++dim_it;
                        }
                    }
                    EXPECT_EQ(param_var.name->get_data(), "b"sv);
                    EXPECT_EQ(param_name, "b"sv);
                    EXPECT_EQ(param_var.type.shift_mods, shift_mods::IMUT);
                    ++param_it;
                }
            }
            {
                ASSERT_EQ(func.statements.size(), 1);
                auto stmt = func.statements.begin();
                {
                    EXPECT_EQ(stmt->type, shift_statement::statement_type::return_);

                    auto& return_expr = stmt->get_return_statement();

                    ASSERT_EQ(return_expr.type, token::type::PLUS);
                    {
                        auto& left = *return_expr.get_left();

                        ASSERT_TRUE(left.is_array());

                        {
                            auto& array_object = *left.get_array_object();
                            ASSERT_EQ(array_object.type, token::type::LEFT_BRACKET);
                            ASSERT_TRUE(array_object.is_bracket());

                            {
                                auto& bracket_expr = *array_object.get_bracket_expression();

                                ASSERT_EQ(bracket_expr.type, token::type::STAR);
                                ASSERT_TRUE(bracket_expr.has_right());

                                {
                                    auto& deref_expr = *bracket_expr.get_right();

                                    ASSERT_TRUE(deref_expr.is_function_call());

                                    {
                                        auto& call_obj = *deref_expr.get_function_call_object();
                                        ASSERT_TRUE(!call_obj.empty());
                                        EXPECT_EQ(call_obj.type, token::type::IDENTIFIER);
                                        EXPECT_EQ(call_obj.begin->get_data(), "test_function1"sv);
                                    }

                                    {
                                        ASSERT_EQ(deref_expr.get_function_call_arguments().size(), 2);
                                        auto arg_it = deref_expr.get_function_call_arguments().begin();
                                        {
                                            auto& arg = *arg_it;
                                            EXPECT_EQ(arg.type, token::type::INTEGER_LITERAL);
                                            EXPECT_EQ(arg.begin->get_data(), "3"sv);
                                            ++arg_it;
                                        }
                                        {
                                            auto& arg = *arg_it;
                                            EXPECT_EQ(arg.type, token::type::INTEGER_LITERAL);
                                            EXPECT_EQ(arg.begin->get_data(), "4"sv);
                                            ++arg_it;
                                        }
                                    }
                                }
                            }
                        }
                        {
                            ASSERT_EQ(left.get_array_dimensions().size(), 1);

                            auto array_dimension_it = left.get_array_dimensions().begin();

                            {
                                auto& indexer_expr = *array_dimension_it->get_array_indexer_expression();

                                ASSERT_EQ(indexer_expr.type, token::type::PLUS);

                                {
                                    auto& left = *indexer_expr.get_left();
                                    EXPECT_EQ(left.type, token::type::INTEGER_LITERAL);
                                    EXPECT_EQ(left.begin->get_data(), "5"sv);
                                }

                                {
                                    auto& right = *indexer_expr.get_right();
                                    ASSERT_TRUE(right.has_left() && right.has_right());
                                    EXPECT_EQ(right.type, token::type::STAR);

                                    {
                                        auto& right_left = *right.get_left();
                                        EXPECT_EQ(right_left.type, token::type::INTEGER_LITERAL);
                                        EXPECT_EQ(right_left.begin->get_data(), "3"sv);
                                    }

                                    {
                                        auto& right_right = *right.get_right();
                                        EXPECT_EQ(right_right.type, token::type::IDENTIFIER);
                                        EXPECT_EQ(right_right.begin->get_data(), "i"sv);
                                    }
                                }

                                ++array_dimension_it;
                            }
                        }


                    }
                    {
                        auto& right = *return_expr.get_right();
                        ASSERT_TRUE(right.is_array());

                        auto& array_object = *right.get_array_object();

                        EXPECT_EQ(array_object.type, token::type::STRING_LITERAL);
                        ASSERT_TRUE(!array_object.empty());
                        EXPECT_EQ(array_object.begin->get_data(), "\"h\""sv);

                        ASSERT_EQ(right.get_array_dimensions().size(), 1);
                        auto array_dimension_it = right.get_array_dimensions().begin();
                        {
                            auto& indexer_expr = *array_dimension_it->get_array_indexer_expression();
                            EXPECT_EQ(indexer_expr.type, token::type::INTEGER_LITERAL);
                            EXPECT_EQ(indexer_expr.begin->get_data(), "2"sv);
                            ++array_dimension_it;
                        }
                    }

                    ++stmt;
                }
            }
        }
    }

    {
        tokenizer t(&handler, R"(module test;
            class clazz {
                void func() {}
                extern shift.int[]* func2(long a, string[] b);
                imut ref shift.string func3(ref shift.string a, imut shift.string b) { }
            }
        )");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        {
            EXPECT_EQ(handler.get_error_count(), 0);
            handler.print();
            if (handler.get_error_count() != 0) {
                return;
            }
        }


    }

}

TEST(ShiftParser, ShouldParseExternFunctionDeclaration) {
    error_handler handler;
    {
        tokenizer t(&handler, R"(module test;
            extern void _testfunc();

            extern int testfunc(int a, int b);

            extern ref test[] do_something(ref int);

            extern int[]* helper() {}
        )");

        t.tokenize();

        parser p(&handler, &t);
        p.parse();

        ASSERT_GE(handler.get_error_count(), 1);

        handler.clear_messages();

        ASSERT_EQ(p.get_functions().size(), 4);
        {
            auto func_it = p.get_functions().begin();

            {
                auto& func = *func_it;
                EXPECT_EQ(func.return_type.name.name.to_string(), "void"sv);
                EXPECT_EQ(func.name.to_string(), "_testfunc"sv);
                EXPECT_TRUE(func.parameters.empty());
                EXPECT_TRUE(func.statements.empty());
                EXPECT_EQ(func.module_->to_string(), "test"sv);
                EXPECT_EQ(func.shift_mods, shift_mods::EXTERN);
                ++func_it;
            }
            {
                auto& func = *func_it;
                EXPECT_EQ(func.return_type.name.name.to_string(), "int"sv);
                EXPECT_EQ(func.name.to_string(), "testfunc"sv);
                EXPECT_EQ(func.module_->to_string(), "test"sv);
                EXPECT_EQ(func.shift_mods, shift_mods::EXTERN);
                {
                    ASSERT_EQ(func.parameters.size(), 2);

                    auto param_it = func.parameters.begin();
                    {
                        auto& [param_name, param_var] = *param_it;
                        EXPECT_EQ(param_var.type.name.name.to_string(), "int"sv);
                        EXPECT_EQ(param_var.name->get_data(), "a"sv);
                        EXPECT_EQ(param_name, "a"sv);
                        EXPECT_TRUE(param_var.type.dimensions.empty());
                        EXPECT_EQ(param_var.type.shift_mods, shift_mods::NONE);
                        ++param_it;
                    }
                    {
                        auto& [param_name, param_var] = *param_it;
                        EXPECT_EQ(param_var.type.name.name.to_string(), "int"sv);
                        EXPECT_TRUE(param_var.type.dimensions.empty());
                        EXPECT_EQ(param_var.name->get_data(), "b"sv);
                        EXPECT_EQ(param_name, "b"sv);
                        EXPECT_EQ(param_var.type.shift_mods, shift_mods::NONE);
                        ++param_it;
                    }
                }
                EXPECT_TRUE(func.statements.empty());
                ++func_it;
            }
            {
                auto& func = *func_it;
                EXPECT_EQ(func.return_type.name.name.to_string(), "test"sv);
                {
                    ASSERT_EQ(func.return_type.dimensions.size(), 1);
                    auto dim_it = func.return_type.dimensions.begin();
                    {
                        auto& dim = *dim_it;
                        EXPECT_EQ(dim.type, shift_type::dimension::dimension_type::array);
                        EXPECT_EQ(dim.count, 1);
                        ++dim_it;
                    }
                }
                EXPECT_EQ(func.name.to_string(), "do_something"sv);
                EXPECT_EQ(func.module_->to_string(), "test"sv);
                EXPECT_EQ(func.shift_mods, shift_mods::EXTERN);
                {
                    ASSERT_EQ(func.parameters.size(), 1);

                    auto param_it = func.parameters.begin();
                    {
                        auto& [param_name, param_var] = *param_it;
                        EXPECT_EQ(param_var.type.name.name.to_string(), "int"sv);
                        EXPECT_EQ(param_var.type.shift_mods, shift_mods::NONE);
                        EXPECT_EQ(param_var.type.ref_type, shift_type::reference_type::ref);
                        ++param_it;
                    }
                }
                EXPECT_TRUE(func.statements.empty());
                ++func_it;
            }
            {
                auto& func = *func_it;
                EXPECT_EQ(func.return_type.name.name.to_string(), "int"sv);
                {
                    ASSERT_EQ(func.return_type.dimensions.size(), 2);
                    auto dim_it = func.return_type.dimensions.begin();
                    {
                        auto& dim = *dim_it;
                        EXPECT_EQ(dim.type, shift_type::dimension::dimension_type::array);
                        EXPECT_EQ(dim.count, 1);
                        ++dim_it;
                    }
                    {
                        auto& dim = *dim_it;
                        EXPECT_EQ(dim.type, shift_type::dimension::dimension_type::pointer);
                        EXPECT_EQ(dim.count, 1);
                        ++dim_it;
                    }
                }
                EXPECT_EQ(func.name.to_string(), "helper"sv);
                EXPECT_EQ(func.module_->to_string(), "test"sv);
                EXPECT_EQ(func.shift_mods, shift_mods::EXTERN);
                EXPECT_TRUE(func.parameters.empty());
                {
                    EXPECT_TRUE(func.statements.empty());
                }
            }
        }
    }
}

TEST(ShiftParser, ShouldParseUseStatement) {
    error_handler handler;
    {
        tokenizer t(&handler, R"(module test.sub;
            use test;
            use test.sub1;
            use _test.getters.home;
        )");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        {
            EXPECT_EQ(handler.get_error_count(), 0);
            handler.print();
            if (handler.get_error_count() != 0) {
                return;
            }
        }

        ASSERT_EQ(p.get_global_uses().size(), 3);

        auto use_it = p.get_global_uses().begin();

        {
            auto& use = *use_it;
            EXPECT_EQ(use.to_string(), "test"sv);
            ++use_it;
        }
        {
            auto& use = *use_it;
            EXPECT_EQ(use.to_string(), "test.sub1"sv);
            ++use_it;
        }
        {
            auto& use = *use_it;
            EXPECT_EQ(use.to_string(), "_test.getters.home"sv);
            ++use_it;
        }
    }
    {
        tokenizer t(&handler, R"(
            use test;
            use test.sub1;
            use _test.getters.home;
        )");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        EXPECT_EQ(handler.get_error_count(), 0);

        ASSERT_EQ(p.get_global_uses().size(), 3);

        auto use_it = p.get_global_uses().begin();

        {
            auto& use = *use_it;
            EXPECT_EQ(use.to_string(), "test"sv);
            ++use_it;
        }
        {
            auto& use = *use_it;
            EXPECT_EQ(use.to_string(), "test.sub1"sv);
            ++use_it;
        }
        {
            auto& use = *use_it;
            EXPECT_EQ(use.to_string(), "_test.getters.home"sv);
            ++use_it;
        }
    }
    {
        tokenizer t(&handler, R"(module test.sub;
            class test_class {
                use test3;
            }
        )");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        EXPECT_EQ(handler.get_error_count(), 0);

        ASSERT_EQ(p.get_classes().size(), 1);

        auto class_it = p.get_classes().begin();

        {
            auto& class_ = *class_it;
            EXPECT_EQ(class_.name->get_data(), "test_class"sv);
            EXPECT_EQ(class_.module_->to_string(), "test.sub"sv);
            ASSERT_EQ(class_.use_statements.size(), 1);

            auto use_it = class_.use_statements.begin();

            {
                auto& use = *use_it;
                EXPECT_EQ(use.to_string(), "test3"sv);
                ++use_it;
            }
        }

    }
}

TEST(ShiftParser, ShouldParseConstructorDeclaration) {
    error_handler handler;
    {
        tokenizer t(&handler, R"(module test;
            class test_class {
                constructor() {}
                public constructor(int a, ref int b) {}
                extern constructor(int a, imut int b, int[]**[] c);
            }
        )");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        {
            EXPECT_EQ(handler.get_error_count(), 0);
            handler.print();
            if (handler.get_error_count() != 0) {
                return;
            }
        }

        ASSERT_EQ(p.get_classes().size(), 1);

        auto class_it = p.get_classes().begin();

        {
            auto& class_ = *class_it;
            EXPECT_EQ(class_.name->get_data(), "test_class"sv);
            EXPECT_EQ(class_.module_->to_string(), "test"sv);
            {
                ASSERT_EQ(class_.functions.size(), 3);

                auto ctor_it = class_.functions.begin();

                {
                    auto& ctor = *ctor_it;
                    EXPECT_TRUE(ctor.parameters.empty());
                    EXPECT_TRUE(ctor.statements.empty());
                    EXPECT_EQ(ctor.shift_mods, shift_mods::NONE);
                    ++ctor_it;
                }
                {
                    auto& ctor = *ctor_it;
                    {
                        ASSERT_EQ(ctor.parameters.size(), 2);

                        auto param_it = ctor.parameters.begin();
                        {
                            auto& [param_name, param_var] = *param_it;
                            EXPECT_EQ(param_var.type.name.name.to_string(), "int"sv);
                            EXPECT_EQ(param_var.name->get_data(), "a"sv);
                            EXPECT_EQ(param_name, "a"sv);
                            EXPECT_TRUE(param_var.type.dimensions.empty());
                            EXPECT_EQ(param_var.type.shift_mods, shift_mods::NONE);
                            ++param_it;
                        }
                        {
                            auto& [param_name, param_var] = *param_it;
                            EXPECT_EQ(param_var.type.name.name.to_string(), "int"sv);
                            EXPECT_TRUE(param_var.type.dimensions.empty());
                            EXPECT_EQ(param_var.name->get_data(), "b"sv);
                            EXPECT_EQ(param_name, "b"sv);
                            EXPECT_EQ(param_var.type.shift_mods, shift_mods::NONE);
                            EXPECT_EQ(param_var.type.ref_type, shift_type::reference_type::ref);
                            ++param_it;
                        }
                    }
                    EXPECT_TRUE(ctor.statements.empty());
                    EXPECT_EQ(ctor.shift_mods, shift_mods::PUBLIC);
                    ++ctor_it;
                }
                {
                    auto& ctor = *ctor_it;
                    {
                        ASSERT_EQ(ctor.parameters.size(), 3);

                        auto param_it = ctor.parameters.begin();
                        {
                            auto& [param_name, param_var] = *param_it;
                            EXPECT_EQ(param_var.type.name.name.to_string(), "int"sv);
                            EXPECT_EQ(param_var.name->get_data(), "a"sv);
                            EXPECT_EQ(param_name, "a"sv);
                            EXPECT_TRUE(param_var.type.dimensions.empty());
                            EXPECT_EQ(param_var.type.shift_mods, shift_mods::NONE);
                            ++param_it;
                        }
                        {
                            auto& [param_name, param_var] = *param_it;
                            EXPECT_EQ(param_var.type.name.name.to_string(), "int"sv);
                            EXPECT_TRUE(param_var.type.dimensions.empty());
                            EXPECT_EQ(param_var.name->get_data(), "b"sv);
                            EXPECT_EQ(param_name, "b"sv);
                            EXPECT_EQ(param_var.type.shift_mods, shift_mods::IMUT);
                            ++param_it;
                        }
                        {
                            auto& [param_name, param_var] = *param_it;
                            EXPECT_EQ(param_var.type.name.name.to_string(), "int"sv);
                            {
                                ASSERT_EQ(param_var.type.dimensions.size(), 3);
                                auto dim_it = param_var.type.dimensions.begin();
                                {
                                    auto& dim = *dim_it;
                                    EXPECT_EQ(dim.type, shift_type::dimension::dimension_type::array);
                                    EXPECT_EQ(dim.count, 1);
                                    ++dim_it;
                                }
                                {
                                    auto& dim = *dim_it;
                                    EXPECT_EQ(dim.type, shift_type::dimension::dimension_type::pointer);
                                    EXPECT_EQ(dim.count, 2);
                                    ++dim_it;
                                }
                                {
                                    auto& dim = *dim_it;
                                    EXPECT_EQ(dim.type, shift_type::dimension::dimension_type::array);
                                    EXPECT_EQ(dim.count, 1);
                                    ++dim_it;
                                }
                            }
                            EXPECT_EQ(param_var.name->get_data(), "c"sv);
                            EXPECT_EQ(param_name, "c"sv);
                            EXPECT_EQ(param_var.type.shift_mods, shift_mods::NONE);
                            ++param_it;
                        }
                    }
                    EXPECT_TRUE(ctor.statements.empty());
                    EXPECT_EQ(ctor.shift_mods, shift_mods::EXTERN);
                    ++ctor_it;
                }

            }
        }
    }
}

TEST(ShiftParser, ShouldParseBinaryOperator) {
    error_handler handler;
    {
        tokenizer t(&handler, R"(module test;
            int v = i + 3;
        )");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        {
            EXPECT_EQ(handler.get_error_count(), 0);
            handler.print();
            if (handler.get_error_count() != 0) { return; }
        }

        auto var_it = p.get_variables().begin();
        {
            EXPECT_EQ(var_it->type.name.name.to_string(), "int");
            EXPECT_EQ(var_it->name->get_data(), "v");
            {
                ASSERT_EQ(var_it->value.type, token::type::PLUS);
                {
                    const auto& left = *var_it->value.get_left();
                    EXPECT_EQ(left.type, token::type::IDENTIFIER);
                    EXPECT_EQ(left.to_string(), "i");
                }
                {
                    const auto& right = *var_it->value.get_right();
                    EXPECT_EQ(right.type, token::type::INTEGER_LITERAL);
                    EXPECT_EQ(right.to_string(), "3");
                }
            }
            ++var_it;
        }

        handler.clear_messages();
    }
}

TEST(ShiftParser, ShouldParsePrefixAndSuffixOperator) {
    error_handler handler;

    {
        tokenizer t(&handler, R"(module test;
            int v = ++a;
            int v1 = --(b+3*9);
            int** v2 = !c++;
            int v3 = ~d("h")+!e[0];
            int[] v4 = *f[2];
        )");
        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        {
            EXPECT_EQ(handler.get_error_count(), 0);
            handler.print();
            if (handler.get_error_count() != 0) {
                return;
            }
        }

        ASSERT_EQ(p.get_variables().size(), 5);

        auto var_it = p.get_variables().begin();

        {
            auto& var = *var_it;
            EXPECT_EQ(var.type.name.name.to_string(), "int"sv);
            EXPECT_EQ(*var.name, "v"sv);
            {
                auto& value = var.value;
                EXPECT_EQ(value.type, token::type::PLUS_PLUS);
                ASSERT_TRUE(value.has_right());
                {
                    auto& right = *value.get_right();
                    EXPECT_EQ(right.type, token::type::IDENTIFIER);
                    ASSERT_TRUE(!right.empty());
                    EXPECT_EQ(right.begin->get_data(), "a"sv);
                }
            }
            ++var_it;
        }
        {
            auto& var = *var_it;
            EXPECT_EQ(var.type.name.name.to_string(), "int"sv);
            EXPECT_EQ(*var.name, "v1"sv);
            {
                auto& value = var.value;
                EXPECT_EQ(value.type, token::type::MINUS_MINUS);
                ASSERT_TRUE(value.has_right());
                {
                    auto& value_right = *value.get_right();
                    EXPECT_TRUE(value_right.is_bracket());
                    ASSERT_NE(value_right.get_bracket_expression(), nullptr);
                    {
                        auto& bracket_expr = *value_right.get_bracket_expression();
                        EXPECT_EQ(bracket_expr.type, token::type::PLUS);
                        ASSERT_TRUE(bracket_expr.has_left());
                        {
                            auto& left = *bracket_expr.get_left();
                            EXPECT_EQ(left.type, token::type::IDENTIFIER);
                            EXPECT_EQ(left.to_string(), "b"sv);
                        }
                        ASSERT_TRUE(bracket_expr.has_right());
                        {
                            auto& right = *bracket_expr.get_right();
                            EXPECT_EQ(right.type, token::type::MULTIPLY);
                            ASSERT_TRUE(right.has_left());
                            {
                                auto& right_left = *right.get_left();
                                EXPECT_EQ(right_left.type, token::type::INTEGER_LITERAL);
                                EXPECT_EQ(right_left.to_string(), "3"sv);
                            }
                            ASSERT_TRUE(right.has_right());
                            {
                                auto& right_right = *right.get_right();
                                EXPECT_EQ(right_right.type, token::type::INTEGER_LITERAL);
                                EXPECT_EQ(right_right.to_string(), "9"sv);
                            }
                        }
                    }

                }
            }
            ++var_it;
        }
        {
            auto& var = *var_it;
            EXPECT_EQ(var.type.name.name.to_string(), "int"sv);

            ASSERT_EQ(var.type.dimensions.size(), 1);
            {
                auto dim_it = var.type.dimensions.begin();
                {
                    auto& dim = *dim_it;
                    EXPECT_EQ(dim.type, shift_type::dimension::dimension_type::pointer);
                    EXPECT_EQ(dim.count, 2);
                    ++dim_it;
                }
            }
            EXPECT_EQ(*var.name, "v2"sv);
            {
                auto& value = var.value;
                EXPECT_EQ(value.type, token::type::NOT);
                ASSERT_TRUE(value.has_right());
                {
                    auto& right = *value.get_right();
                    EXPECT_EQ(right.type, token::type::PLUS_PLUS);
                    ASSERT_TRUE(right.has_left());
                    {
                        auto& right_left = *right.get_left();
                        EXPECT_EQ(right_left.type, token::type::IDENTIFIER);
                        EXPECT_EQ(right_left.to_string(), "c"sv);
                    }
                }
            }
            ++var_it;
        }
        {
            auto& var = *var_it;
            EXPECT_EQ(var.type.name.name.to_string(), "int"sv);
            EXPECT_EQ(*var.name, "v3"sv);
            {
                auto& value = var.value;
                EXPECT_EQ(value.type, token::type::PLUS);
                ASSERT_TRUE(value.has_left());
                {
                    auto& left = *value.get_left();
                    EXPECT_EQ(left.type, token::type::FLIP_BITS);
                    ASSERT_TRUE(left.has_right());
                    {
                        auto& right = *left.get_right();
                        EXPECT_TRUE(right.is_function_call());
                        ASSERT_NE(right.get_function_call_object(), nullptr);
                        {
                            auto& call_obj = *right.get_function_call_object();
                            EXPECT_EQ(call_obj.type, token::type::IDENTIFIER);
                            EXPECT_EQ(call_obj.to_string(), "d"sv);
                        }
                        ASSERT_EQ(right.get_function_call_arguments().size(), 1);
                        {
                            auto arg_it = right.get_function_call_arguments().begin();
                            {
                                auto& arg = *arg_it;
                                EXPECT_EQ(arg.type, token::type::STRING_LITERAL);
                                EXPECT_EQ(arg.to_string(), "\"h\""sv);

                                ++arg_it;
                            }
                        }

                    }
                }
                ASSERT_TRUE(value.has_right());
                {
                    auto& right = *value.get_right();
                    EXPECT_EQ(right.type, token::type::NOT);
                    ASSERT_TRUE(right.has_right());
                    {
                        auto& right_right = *right.get_right();
                        EXPECT_TRUE(right_right.is_array());
                        ASSERT_NE(right_right.get_array_object(), nullptr);
                        {
                            auto& array_object = *right_right.get_array_object();
                            EXPECT_EQ(array_object.type, token::type::IDENTIFIER);
                            EXPECT_EQ(array_object.to_string(), "e"sv);
                        }
                        ASSERT_EQ(right_right.get_array_dimensions().size(), 1);
                        {
                            auto array_dim_it = right_right.get_array_dimensions().begin();
                            {
                                auto& indexer_expr = *array_dim_it->get_array_indexer_expression();
                                EXPECT_EQ(indexer_expr.type, token::type::INTEGER_LITERAL);
                                EXPECT_EQ(indexer_expr.to_string(), "0"sv);

                                ++array_dim_it;
                            }
                        }
                    }
                }
            }
            ++var_it;
        }
        {
            auto& var = *var_it;
            EXPECT_EQ(var.type.name.name.to_string(), "int"sv);
            {
                ASSERT_EQ(var.type.dimensions.size(), 1);
                auto dim_it = var.type.dimensions.begin();
                {
                    auto& dim = *dim_it;
                    EXPECT_EQ(dim.type, shift_type::dimension::dimension_type::array);
                    EXPECT_EQ(dim.count, 1);
                    ++dim_it;
                }
            }
            EXPECT_EQ(*var.name, "v4"sv);
            {
                auto& value = var.value;
                EXPECT_EQ(value.type, token::type::STAR);
                ASSERT_TRUE(value.has_right());
                {
                    auto& right = *value.get_right();
                    EXPECT_TRUE(right.is_array());
                    ASSERT_NE(right.get_array_object(), nullptr);
                    {
                        auto& array_object = *right.get_array_object();
                        EXPECT_EQ(array_object.type, token::type::IDENTIFIER);
                        EXPECT_EQ(array_object.to_string(), "f"sv);
                    }
                    ASSERT_EQ(right.get_array_dimensions().size(), 1);
                    {
                        auto array_dim_it = right.get_array_dimensions().begin();
                        {
                            auto& indexer_expr = *array_dim_it->get_array_indexer_expression();
                            EXPECT_EQ(indexer_expr.type, token::type::INTEGER_LITERAL);
                            EXPECT_EQ(indexer_expr.to_string(), "2"sv);

                            ++array_dim_it;
                        }
                    }
                }
            }
            ++var_it;
        }
    }

    {
        tokenizer t(&handler, R"(module test;
            bool b = !a++;
            bool b2 = ++b--;
            bool b3 = !~--c++--;
        )");

        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        {
            EXPECT_EQ(handler.get_error_count(), 0);
            handler.print();
            if (handler.get_error_count() != 0) {
                return;
            }
        }

        ASSERT_EQ(p.get_variables().size(), 3);

        auto var_it = p.get_variables().begin();

        {
            auto& var = *var_it;
            EXPECT_EQ(var.type.name.name.to_string(), "bool"sv);
            EXPECT_EQ(*var.name, "b"sv);
            {
                auto& value = var.value;
                EXPECT_EQ(value.type, token::type::NOT);
                ASSERT_TRUE(value.has_right());
                {
                    auto& right = *value.get_right();
                    EXPECT_EQ(right.type, token::type::PLUS_PLUS);
                    ASSERT_TRUE(right.has_left());
                    {
                        auto& right_left = *right.get_left();
                        EXPECT_EQ(right_left.type, token::type::IDENTIFIER);
                        EXPECT_EQ(right_left.to_string(), "a"sv);
                    }
                }
            }
            ++var_it;
        }
        {
            auto& var = *var_it;
            EXPECT_EQ(var.type.name.name.to_string(), "bool"sv);
            EXPECT_EQ(*var.name, "b2"sv);
            {
                auto& value = var.value;
                EXPECT_EQ(value.type, token::type::MINUS_MINUS);
                ASSERT_TRUE(value.has_left());
                {
                    auto& value_left = *value.get_left();
                    EXPECT_EQ(value_left.type, token::type::PLUS_PLUS);
                    ASSERT_TRUE(value_left.has_right());
                    {
                        auto& value_left_right = *value_left.get_right();
                        EXPECT_EQ(value_left_right.type, token::type::IDENTIFIER);
                        EXPECT_EQ(value_left_right.to_string(), "b"sv);
                    }
                }
            }
            ++var_it;
        }
        {
            auto& var = *var_it;
            EXPECT_EQ(var.type.name.name.to_string(), "bool"sv);
            EXPECT_EQ(*var.name, "b3"sv);
            {
                auto& value = var.value;
                EXPECT_EQ(value.type, token::type::NOT);
                ASSERT_TRUE(value.has_right());
                {
                    auto& right = *value.get_right();
                    EXPECT_EQ(right.type, token::type::FLIP_BITS);
                    ASSERT_TRUE(right.has_right());
                    {
                        auto& right_right = *right.get_right();
                        EXPECT_EQ(right_right.type, token::type::MINUS_MINUS);
                        ASSERT_TRUE(right_right.has_left());
                        {
                            auto& right_right_left = *right_right.get_left();
                            EXPECT_EQ(right_right_left.type, token::type::PLUS_PLUS);
                            ASSERT_TRUE(right_right_left.has_left());
                            {
                                auto& right_right_left_left = *right_right_left.get_left();
                                EXPECT_EQ(right_right_left_left.type, token::type::MINUS_MINUS);
                                ASSERT_TRUE(right_right_left_left.has_right());
                                {
                                    auto& right_right_left_right = *right_right_left_left.get_right();
                                    EXPECT_EQ(right_right_left_right.type, token::type::IDENTIFIER);
                                    EXPECT_EQ(right_right_left_right.to_string(), "c"sv);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    {
        tokenizer t(&handler, R"(module test;
            bool b3 = c--!;
        )");

        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        {
            EXPECT_GE(handler.get_error_count(), 1);
            // handler.print();
        }

        if (p.get_variables().size() == 1) {
            auto var_it = p.get_variables().begin();
            {
                auto& var = *var_it;
                EXPECT_EQ(var.type.name.name.to_string(), "bool"sv);
                EXPECT_EQ(*var.name, "b3"sv);
            }
        }

        handler.clear_messages();
    }

    {
        tokenizer t(&handler, R"(module test;
            bool b3 = c--~;
        )");

        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        {
            EXPECT_GE(handler.get_error_count(), 1);
            // handler.print();
        }

        if (p.get_variables().size() == 1) {
            auto var_it = p.get_variables().begin();
            {
                auto& var = *var_it;
                EXPECT_EQ(var.type.name.name.to_string(), "bool"sv);
                EXPECT_EQ(*var.name, "b3"sv);
            }
        }

        handler.clear_messages();
    }

    {
        tokenizer t(&handler, R"(module test;
            bool b3 = c~;
        )");

        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        {
            EXPECT_GE(handler.get_error_count(), 1);
            // handler.print();
        }

        if (p.get_variables().size() == 1) {
            auto var_it = p.get_variables().begin();
            {
                auto& var = *var_it;
                EXPECT_EQ(var.type.name.name.to_string(), "bool"sv);
                EXPECT_EQ(*var.name, "b3"sv);
            }
        }

        handler.clear_messages();
    }

    {
        tokenizer t(&handler, R"(module test;
            bool b3 = c+;
        )");

        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        {
            EXPECT_GE(handler.get_error_count(), 1);
            // handler.print();
        }

        if (p.get_variables().size() == 1) {
            auto var_it = p.get_variables().begin();
            {
                auto& var = *var_it;
                EXPECT_EQ(var.type.name.name.to_string(), "bool"sv);
                EXPECT_EQ(*var.name, "b3"sv);
            }
        }

        handler.clear_messages();
    }
    {
        tokenizer t(&handler, R"(module test;
            bool b3 = c-;
        )");

        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        {
            EXPECT_GE(handler.get_error_count(), 1);
            // handler.print();
        }

        if (p.get_variables().size() == 1) {
            auto var_it = p.get_variables().begin();
            {
                auto& var = *var_it;
                EXPECT_EQ(var.type.name.name.to_string(), "bool"sv);
                EXPECT_EQ(*var.name, "b3"sv);
            }
        }

        handler.clear_messages();
    }
    {
        tokenizer t(&handler, R"(module test;
            bool b3 = -c-;
        )");

        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        {
            EXPECT_GE(handler.get_error_count(), 1);
            // handler.print();
        }

        if (p.get_variables().size() == 1) {
            auto var_it = p.get_variables().begin();
            {
                auto& var = *var_it;
                EXPECT_EQ(var.type.name.name.to_string(), "bool"sv);
                EXPECT_EQ(*var.name, "b3"sv);
            }
        }

        handler.clear_messages();
    }
    { // No unary plus operator

        tokenizer t(&handler, R"(module test . test;
            bool __b3__ = +c;
        )");

        t.tokenize();
        parser p(&handler, &t);
        p.parse();

        {
            EXPECT_GE(handler.get_error_count(), 1);
            // handler.print();
        }

        if (p.get_variables().size() == 1) {
            auto var_it = p.get_variables().begin();
            {
                auto& var = *var_it;
                EXPECT_EQ(var.type.name.name.to_string(), "bool"sv);
                EXPECT_EQ(*var.name, "__b3__"sv);
                EXPECT_EQ(var.get_fqn(), "test.test.__b3__");
            }
        }
        handler.clear_messages();
    }
}

TEST(ShiftParser, ShouldParseIfStatement) {
    error_handler handler;

    {
        tokenizer t(&handler, R"()");
        t.tokenize();

        parser p(&handler, &t);
        p.parse();

        handler.clear_messages();
    }
}
