#include <utility>
#include <type_traits>
#include <functional>
#include <array>

namespace using_cpp11
{
    template <typename T, template <typename> class ... requirements_list>
    using requirements_t = std::conjunction<requirements_list<T>...>;
}
namespace using_cpp17
{
    template <typename T>
    constexpr static const auto can_fluctuate = not std::is_const_v<T::value>;
}