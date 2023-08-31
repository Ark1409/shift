#include <gtest/gtest.h>
#include <utility>

#include "compiler/shift_argument_parser.h"

TEST(ShiftArgumentParser, CanParseWarningArgument) {
    shift::compiler::error_handler error_handler;

    shift::compiler::argument_parser arg_parser(&error_handler, { SHIFT_FLAG_WARNING });
    arg_parser.parse();

    EXPECT_EQ(error_handler.get_error_count(), 0);
    EXPECT_TRUE(error_handler.is_warning());
    EXPECT_TRUE(arg_parser.has_flag(shift::compiler::argument_parser::flags::FLAG_WARNINGS));
}

TEST(ShiftArgumentParser, CanParseWerrorArgument) {
    shift::compiler::error_handler error_handler;

    shift::compiler::argument_parser arg_parser(&error_handler, { SHIFT_FLAG_WERROR });
    arg_parser.parse();

    EXPECT_EQ(error_handler.get_error_count(), 0);
    EXPECT_TRUE(error_handler.is_werror());
    EXPECT_TRUE(arg_parser.has_flag(shift::compiler::argument_parser::flags::FLAG_WERROR));
}

TEST(ShiftArgumentParser, CanParseHelpArgument) {
    shift::compiler::error_handler error_handler;

    shift::compiler::argument_parser arg_parser(&error_handler, { SHIFT_FLAG_HELP });
    arg_parser.parse();

    EXPECT_EQ(error_handler.get_error_count(), 0);
    EXPECT_TRUE(arg_parser.has_flag(shift::compiler::argument_parser::flags::FLAG_HELP));
}