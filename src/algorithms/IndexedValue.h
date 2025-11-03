#pragma once
#include <utility>

namespace houdini::tools::utils {
    template<class T, class TIndex>
    struct Indexed {
        T value;
        TIndex index{};

        Indexed() = default;
        template<class U, class V>
        Indexed(U&& value, V&& index)
            : value(std::forward<U>(value)),
              index(std::forward<V>(index)) {}

        explicit operator T() const {
            return value;
        }
    };
}
