#ifndef SHIFT_TYPE_INFO_H_
#define SHIFT_TYPE_INFO_H_ 1

#include "compiler/parsing/parser_fwd.h"

#include <string>

namespace shift::compiler::analyzing {
    struct type_info {
        constexpr explicit type_info(const parsing::shift_class* clazz) : m_class(clazz) {}

        inline const parsing::shift_class* get_class() const { return m_class; }

        std::string get_fqn() const;

        virtual std::string get_printable_fqn() const;

        static const type_info unresolved;
    protected:
        const parsing::shift_class* m_class{ nullptr };
    };

    constexpr type_info type_info::unresolved{ nullptr };

    struct class_type_info {
        const parsing::shift_class* base{ nullptr };
    };

    struct dimension_type_info : type_info {
        constexpr explicit array_type_info(const parsing::shift_class* array_class, const parsing::shift_class* clazz) : type_info
                                                                                                                             (array_class) {}

        std::string get_printable_fqn() const override;

    private:
        const parsing::shift_class* m_arra_clazz{ nullptr };
    };
}

#endif //SHIFT_TYPE_INFO_H_
