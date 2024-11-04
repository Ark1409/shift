#include <gtest/gtest.h>

#include "compiler/shift_compiler.h"

using namespace shift::compiler;

using namespace std::string_view_literals;

TEST(ShiftTokenizer, ShouldTokenizeIdentifiers) {
    error_handler handler;
    {
        tokenizer t(&handler, R"(
        module _9 _9a_ m9_ z99932 _m9 _mi_ hufsiigyya _4qyg23bufqerbvyuisacyb8wqbuvequ3fgt3q8yqwefas
        _9996963854675372452743952387956234759286t7348972512 _843895379634__
        a7659843923506547899783579283c a zz m cc _ __
    )");
        t.tokenize();

        EXPECT_EQ(handler.get_error_count(), 0);
        for (auto& token : t.get_tokens()) {
            EXPECT_EQ(token.get_token_type(), token::type::IDENTIFIER);
        }
    }

    {
        tokenizer t(&handler, R"(9 $ %%! @ # $ % ^ & * ( ) + - = { } [ ] | \ : ; "" ' ' < > , . ? / ~ 1 2 3 4 5 6 7 8 0
    )");
        t.tokenize();

        EXPECT_NE(handler.get_error_count(), 0);
        for (auto& token : t.get_tokens()) {
            EXPECT_NE(token.get_token_type(), token::type::IDENTIFIER);
        }

        handler.get_messages().clear();
    }
}

TEST(ShiftTokenizer, ShouldTokenizeNumbers) {
    error_handler handler;

    {
        tokenizer t(&handler, R"(1571892437598324 5234758932457982437 589732967349287689352 597423 53942 75345 3
         34 53 3566 474854865
    )");

        t.tokenize();

        EXPECT_EQ(handler.get_error_count(), 0);
        for (auto& token : t.get_tokens()) {
            EXPECT_TRUE(token.is_number_literal());
        }
    }

    {
        tokenizer t(&handler, R"(1571892437598324.f 5234758932457982437.f 589732967349287689352.02f 597.423F 539.42F 753.45f .3F
         3.4f .53f 35.66f 0.474854865F 2f
    )");

        t.tokenize();

        EXPECT_EQ(handler.get_error_count(), 0);
        for (auto& token : t.get_tokens()) {
            EXPECT_EQ(token.get_token_type(), token::type::FLOAT_LITERAL);
        }
    }

    {
        tokenizer t(&handler, R"(1571892437598324.d 5234758932457982437.D 589732967349287689352.02d 597423D 53942.D 75345d 3.d
         3.4D .53d 35.66d 0.474854865d .53d
    )");

        t.tokenize();

        EXPECT_EQ(handler.get_error_count(), 0);
        for (auto& token : t.get_tokens()) {
            EXPECT_EQ(token.get_token_type(), token::type::DOUBLE_LITERAL);
        }
    }

    {
        tokenizer t(&handler, R"(0xfc9 0x232 0x67253748523 0xabffde3421 0xabdcfee358382cce
    )");

        t.tokenize();

        EXPECT_EQ(handler.get_error_count(), 0);
        for (auto& token : t.get_tokens()) {
            EXPECT_EQ(token.get_token_type(), token::type::HEX_LITERAL);
        }
    }

    {
        tokenizer t(&handler, R"(0b11001011010 0b1111111111111111111 0b100000000000000 0b000000000 0b11110101011111
    )");

        t.tokenize();

        EXPECT_EQ(handler.get_error_count(), 0);
        for (auto& token : t.get_tokens()) {
            EXPECT_EQ(token.get_token_type(), token::type::BINARY_LITERAL);
        }
    }

    {
        tokenizer t(&handler, R"(11001011010 1b1111111111111111111 2b100000000000000 0b200200000 0b11110101011111
    )");

        t.tokenize();

        EXPECT_NE(handler.get_error_count(), 0);
        for (auto& token : t.get_tokens()) {
            if (token.get_token_type() == token::type::BINARY_LITERAL)
                break;
            EXPECT_NE(token.get_token_type(), token::type::BINARY_LITERAL);
        }

        handler.get_messages().clear();
    }
}


TEST(ShiftTokenizer, ShouldTokenizeStringLiterals) {
    error_handler handler;

    {
        tokenizer t(&handler, R"("Hello World" "" " " "     " "   \t\v  " "Hello World\n" "Hello World\t" "Hello World\r" "Hello World\f" "Hello World\v"
        "Hello World\a" "Hello World\b" "Hello World\'" "Hello World\\" "Hello World\"" 
    )");

        t.tokenize();

        EXPECT_EQ(handler.get_error_count(), 0);
        for (auto& token : t.get_tokens()) {
            EXPECT_EQ(token.get_token_type(), token::type::STRING_LITERAL);
        }

        EXPECT_EQ((-- -- --t.get_tokens().end())->get_data(), "\"Hello World\\'\""sv);
        EXPECT_EQ((-- --t.get_tokens().end())->get_data(), "\"Hello World\\\\\""sv);
        EXPECT_EQ(t.get_tokens().back().get_data(), "\"Hello World\\\"\""sv);
        EXPECT_EQ((++t.get_tokens().begin())->get_data(), "\"\""sv);
        EXPECT_EQ((++ ++t.get_tokens().begin())->get_data(), "\" \""sv);
        EXPECT_EQ((++ ++ ++t.get_tokens().begin())->get_data(), "\"     \""sv);
        EXPECT_EQ((++ ++ ++ ++t.get_tokens().begin())->get_data(), "\"   \\t\\v  \""sv);
    }
    {
        tokenizer t(&handler, R"("
        "

        ""
    )");

        t.tokenize();

        EXPECT_NE(handler.get_error_count(), 0);
    }
}

TEST(ShiftTokenizer, ShouldTokenizeCharLiterals) {
    error_handler handler;

    {
        tokenizer t(&handler, R"('a' 'b' 'c' 'd' 'e' 'f' 'g' 'h' 'i' 'j' 'k' 'l' 'm' 'n' 'o' 'p' 'q' 'r' 's' 't' 'u' 'v' 'w' 'x' 'y' 'z'
        'A' 'B' 'C' 'D' 'E' 'F' 'G' 'H' 'I' 'J' 'K' 'L' 'M' 'N' 'O' 'P' 'Q' 'R' 'S' 'T' 'U' 'V' 'W' 'X' 'Y' 'Z'
        '0' '1' '2' '3' '4' '5' '6' '7' '8' '9'
        ' ' '\t' '\v' '\n' '\r' '\f' '\a' '\b' '\'' '\"' '\\'
    )");

        t.tokenize();

        EXPECT_EQ(handler.get_error_count(), 0);
        for (auto& token : t.get_tokens()) {
            EXPECT_EQ(token.get_token_type(), token::type::CHAR_LITERAL);
        }

        EXPECT_EQ((-- -- --t.get_tokens().end())->get_data(), "'\\''"sv);
        EXPECT_EQ((-- --t.get_tokens().end())->get_data(), "'\\\"'"sv);
        EXPECT_EQ(t.get_tokens().back().get_data(), "'\\\\'"sv);

    }

    {
        tokenizer t(&handler, R"('
        '
        ''
        '\'
        )");

        t.tokenize();
        EXPECT_GE(handler.get_error_count(), 3);
    }
}

TEST(ShiftTokenizer, VerifyTokenizerUtilities) {
    error_handler handler;

    {
        tokenizer t(&handler, R"(
        public class file {
            static use shift.io._internal;
            public constructor() {}

            public constructor(int) {}

            public constructor(const string path) { this._path = path; }

            public string get_path() { return this._path;}

            private string _path;

            public const intern.[] test (, int){ while const(true) this.base.run()_; }

            private static void name = "Hello World inside\nmy home";
        }
    )");

        t.tokenize();

        EXPECT_EQ(handler.get_error_count(), 0);

        {
            const auto& tok = t.token_after(file_indexer{ 3, 23 });
            EXPECT_EQ(tok.get_token_type(), token::type::IDENTIFIER);
            EXPECT_EQ(tok.get_data(), "shift"sv);
        }

        {
            const auto& tok = t.token_before(file_indexer{ 13, 5 });
            EXPECT_EQ(tok.get_token_type(), token::type::SEMICOLON);
            EXPECT_EQ(tok.get_data(), ";"sv);
        }

        {
            const auto& tok = t.token_at(file_indexer{ 16, 40 });
            EXPECT_EQ(tok.get_token_type(), token::type::STRING_LITERAL);
            EXPECT_EQ(tok.get_data(), "\"Hello World inside\\nmy home\""sv);
        }
    }
}