#include <type_traits>
#include <concepts>

namespace concepts
{
    template <typename T>
    concept gendered = requires(T value) {
        { std::is_enum_v<decltype(T::gender_value)> };
        { T::gender_value } -> std::same_as<const typename T::gender_type>;
    };
    template <typename T>
    concept female = gendered<T> && requires(T) {
        T::gender_value == decltype(T::gender_value)::female;
    };
    template <typename T>
    concept not_female = gendered<T> && not female<T>;
}

template <typename, class = void>
struct has_female_member : std::false_type { };
template <typename T>
struct has_female_member<T, std::void_t<decltype(T::female)>> : std::true_type { };
template <typename T>
constexpr static auto has_female_member_v = has_female_member<T>::value;

template <
    auto value,
    typename = std::enable_if_t<std::is_enum_v<decltype(value)>>>
constexpr static auto is_female_value = [](auto arg)
{
    if constexpr (has_female_member_v<decltype(arg)>)
        return value == decltype(arg)::female;
    return false;
}(value);

template <auto arg>
class gender_specifications
{
    template <auto gender_arg, typename = void>
    struct impl
    {
        using gender_type = decltype(gender_arg);
        static_assert(std::is_enum_v<gender_type>);
        constexpr static auto gender_value = gender_arg;
    };
    template <auto gender_arg>
    struct impl<gender_arg, std::enable_if_t<is_female_value<gender_arg>>>
    {
        using gender_type = decltype(gender_arg);
        static_assert(std::is_enum_v<gender_type>);
        constexpr static auto gender_value = gender_arg;
    };
public :
    using type = impl<arg>;
};

template <auto gender_arg>
using gender_specifications_t = typename gender_specifications<gender_arg>::type;

enum gender_type { male, female, unknown };
struct rat
{
};
struct unicorn
{
    enum gender_type { hybrid };
};

template <
    class T,
    auto gender_value,
    template <auto> typename gender_specifier = gender_specifications_t
>
    requires (not concepts::gendered<T>)
auto animal_factory()
{
    static_assert(concepts::gendered<gender_specifier<gender_value>>);
    struct type : T, gender_specifier<gender_value>
    {};
    static_assert(concepts::gendered<type>);
    return type{};
};

void test()
{
    {
        const auto female_rat = animal_factory<rat, female>();
        static_assert(concepts::female<decltype(female_rat)>);
    }
    {
        const auto female_unicorn = animal_factory<unicorn, unicorn::hybrid>();
        static_assert(not concepts::female<decltype(female_unicorn)>);
    }
}



