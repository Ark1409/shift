#include <gtest/gtest.h>
#include <utility>

#include "compiler/shift_argument_parser.h"

TEST(ShiftArgumentParser, ShouldParseWarningArgument) {
    shift::compiler::error_handler error_handler;

    shift::compiler::argument_parser arg_parser(&error_handler, { SHIFT_FLAG_WARNING });
    arg_parser.parse();

    EXPECT_EQ(error_handler.get_error_count(), 0);
    EXPECT_TRUE(error_handler.is_warning());
    EXPECT_TRUE(arg_parser.has_flag(shift::compiler::argument_parser::flags::FLAG_WARNINGS));
}

TEST(ShiftArgumentParser, ShouldParseWerrorArgument) {
    shift::compiler::error_handler error_handler;

    {
        shift::compiler::argument_parser arg_parser(&error_handler, { SHIFT_FLAG_WERROR });
        arg_parser.parse();

        EXPECT_EQ(error_handler.get_error_count(), 0);
        EXPECT_TRUE(error_handler.is_werror());
        EXPECT_TRUE(arg_parser.has_flag(shift::compiler::argument_parser::flags::FLAG_WERROR));
    }
    {
        shift::compiler::argument_parser arg_parser(&error_handler, { "fsad.fsad ", SHIFT_FLAG_WERROR });
        arg_parser.parse();

        EXPECT_TRUE(error_handler.is_werror());
        EXPECT_TRUE(arg_parser.has_flag(shift::compiler::argument_parser::flags::FLAG_WERROR));
    }

    {
        shift::compiler::argument_parser arg_parser(&error_handler, { "fsad.fsad", SHIFT_FLAG_WERROR, "-tests" });
        arg_parser.parse();

        EXPECT_TRUE(error_handler.is_werror());
        EXPECT_TRUE(arg_parser.has_flag(shift::compiler::argument_parser::flags::FLAG_WERROR));
    }

    {
        shift::compiler::argument_parser arg_parser(&error_handler, { "fsad.fsad q", SHIFT_FLAG_WERROR, "-warnings" });
        arg_parser.parse();

        EXPECT_TRUE(error_handler.is_werror());
        EXPECT_TRUE(arg_parser.has_flag(shift::compiler::argument_parser::flags::FLAG_WERROR));
    }
}

TEST(ShiftArgumentParser, ShouldParseHelpArgument) {
    shift::compiler::error_handler error_handler;

    {
        shift::compiler::argument_parser arg_parser(&error_handler, { SHIFT_FLAG_HELP });
        arg_parser.parse();

        EXPECT_EQ(error_handler.get_error_count(), 0);
        EXPECT_TRUE(arg_parser.has_flag(shift::compiler::argument_parser::flags::FLAG_HELP));
    }
    {
        shift::compiler::argument_parser arg_parser(&error_handler, { "-werror", SHIFT_FLAG_HELP, "test.shift" });
        arg_parser.parse();

        EXPECT_EQ(error_handler.get_error_count(), 0);
        EXPECT_TRUE(arg_parser.has_flag(shift::compiler::argument_parser::flags::FLAG_HELP));
    }

    {
        shift::compiler::argument_parser arg_parser(&error_handler, { SHIFT_FLAG_HELP, "test.shift" });
        arg_parser.parse();

        EXPECT_EQ(error_handler.get_error_count(), 0);
        EXPECT_TRUE(arg_parser.has_flag(shift::compiler::argument_parser::flags::FLAG_HELP));
    }

    {
        shift::compiler::argument_parser arg_parser(&error_handler, { "test.shift", SHIFT_FLAG_HELP });
        arg_parser.parse();

        EXPECT_EQ(error_handler.get_error_count(), 0);
        EXPECT_TRUE(arg_parser.has_flag(shift::compiler::argument_parser::flags::FLAG_HELP));
    }

    {
        shift::compiler::argument_parser arg_parser(&error_handler, { "test.shift", SHIFT_FLAG_HELP, "help.shift" });
        arg_parser.parse();

        EXPECT_EQ(error_handler.get_error_count(), 0);
        EXPECT_TRUE(arg_parser.has_flag(shift::compiler::argument_parser::flags::FLAG_HELP));
    }
}

TEST(ShiftArgumentParser, ShouldParseLibArgument) {
    shift::compiler::error_handler error_handler;

    {
        shift::compiler::argument_parser arg_parser(&error_handler, { SHIFT_FLAG_LIB, "help.shift", SHIFT_FLAG_LIB, "C:/users/test.shift" });
        arg_parser.parse();

        EXPECT_EQ(error_handler.get_error_count(), 0);
        EXPECT_TRUE(arg_parser.get_flags() == 0x0 || arg_parser.get_flags() == shift::compiler::argument_parser::flags::FLAG_HELP);
    }

    {
        shift::compiler::argument_parser arg_parser(&error_handler, { SHIFT_FLAG_LIB, "help.shift", SHIFT_FLAG_LIB, "C:/users/test.shift", "test.shift" });
        arg_parser.parse();

        EXPECT_EQ(error_handler.get_error_count(), 0);
        EXPECT_TRUE(arg_parser.get_flags() == 0x0 || arg_parser.get_flags() == shift::compiler::argument_parser::flags::FLAG_HELP);
    }

    {
        shift::compiler::argument_parser arg_parser(&error_handler, { SHIFT_FLAG_LIB, "help.shift", "test.shift", SHIFT_FLAG_LIB, "C:/users/test.shift" });
        arg_parser.parse();

        EXPECT_EQ(error_handler.get_error_count(), 0);
        EXPECT_TRUE(arg_parser.get_flags() == 0x0 || arg_parser.get_flags() == shift::compiler::argument_parser::flags::FLAG_HELP);
    }

    {
        shift::compiler::argument_parser arg_parser(&error_handler, { "test.shift", SHIFT_FLAG_LIB, "C:/users/test.shift" });
        arg_parser.parse();

        EXPECT_EQ(error_handler.get_error_count(), 0);
        EXPECT_TRUE(arg_parser.get_flags() == 0x0 || arg_parser.get_flags() == shift::compiler::argument_parser::flags::FLAG_HELP);
    }

    {
        shift::compiler::argument_parser arg_parser(&error_handler, { "test.shift", SHIFT_FLAG_LIB, "C:/users/test.shift", SHIFT_FLAG_LIB, "C:/users/test.shift" });
        arg_parser.parse();

        EXPECT_EQ(error_handler.get_error_count(), 0);
        EXPECT_TRUE(arg_parser.get_flags() == 0x0 || arg_parser.get_flags() == shift::compiler::argument_parser::flags::FLAG_HELP);
    }
}

TEST(ShiftArgumentParser, ShouldParseLibPathArgument) {
    shift::compiler::error_handler error_handler;
    error_handler.set_print_warnings();

    {
        shift::compiler::argument_parser arg_parser(&error_handler, { SHIFT_FLAG_LIB_PATH, "./help.shift" });
        arg_parser.parse();

        EXPECT_TRUE(error_handler.get_error_count() > 0 || error_handler.get_warning_count() > 0);
        EXPECT_TRUE(arg_parser.get_flags() == 0x0 || arg_parser.get_flags() == shift::compiler::argument_parser::flags::FLAG_HELP);
        
        error_handler.get_messages().clear();
    }

    {
        shift::compiler::argument_parser arg_parser(&error_handler, { SHIFT_FLAG_LIB_PATH, "./help.shift", "test.shift" });
        arg_parser.parse();

        EXPECT_TRUE(error_handler.get_error_count() > 0 || error_handler.get_warning_count() > 0);
        EXPECT_TRUE(arg_parser.get_flags() == 0x0 || arg_parser.get_flags() == shift::compiler::argument_parser::flags::FLAG_HELP);
        
        error_handler.get_messages().clear();
    }

    {
        shift::compiler::argument_parser arg_parser(&error_handler, { "test.shift", SHIFT_FLAG_LIB_PATH, "./help.shift" });
        arg_parser.parse();

        EXPECT_TRUE(error_handler.get_error_count() > 0 || error_handler.get_warning_count() > 0);
        EXPECT_TRUE(arg_parser.get_flags() == 0x0 || arg_parser.get_flags() == shift::compiler::argument_parser::flags::FLAG_HELP);
        
        error_handler.get_messages().clear();
    }

    {
        shift::compiler::argument_parser arg_parser(&error_handler, { "test.shift", SHIFT_FLAG_LIB_PATH, "./help.shift", SHIFT_FLAG_LIB_PATH, "./help.shift" });
        arg_parser.parse();

        EXPECT_TRUE(error_handler.get_error_count() > 0 || error_handler.get_warning_count() > 0);
        EXPECT_TRUE(arg_parser.get_flags() == 0x0 || arg_parser.get_flags() == shift::compiler::argument_parser::flags::FLAG_HELP);
        
        error_handler.get_messages().clear();
    }
    {
        shift::compiler::argument_parser arg_parser(&error_handler, { SHIFT_FLAG_LIB_PATH, "./", SHIFT_FLAG_LIB_PATH });
        arg_parser.parse();

        EXPECT_NE(error_handler.get_error_count(), 0);
        EXPECT_EQ(error_handler.get_warning_count(), 0);
        EXPECT_TRUE(arg_parser.get_flags() == 0x0 || arg_parser.get_flags() == shift::compiler::argument_parser::flags::FLAG_HELP);

        error_handler.get_messages().clear();
    }

    {
        shift::compiler::argument_parser arg_parser(&error_handler, { SHIFT_FLAG_LIB_PATH, "./", SHIFT_FLAG_LIB_PATH, "test.shift" });
        arg_parser.parse();

        EXPECT_GT(error_handler.get_warning_count(), 0);
        EXPECT_TRUE(arg_parser.get_flags() == 0x0 || arg_parser.get_flags() == shift::compiler::argument_parser::flags::FLAG_HELP);
        error_handler.get_messages().clear();
    }

}