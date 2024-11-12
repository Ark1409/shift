#ifndef SHIFT_NAME_INFO_H_
#define SHIFT_NAME_INFO_H_ 1

#include "compiler/analyzing/analyzer_fwd.h"
#include "compiler/parsing/parser_fwd.h"

#include "utils/utils.h"

#include <variant>
#include <optional>

namespace shift::compiler::analyzing {
    struct name_info {
        const std::variant<const parsing::shift_module*, const parsing::shift_class*, const parsing::shift_function*,
                           const parsing::shift_variable*> name;
        const std::vector<const parsing::shift_function*> conversions;

        type_info* get_type() const;
    };
}

#endif //SHIFT_NAME_INFO_H_
