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

namespace impl_1
{
    enum gender_type { male, female, unknown };
    template <gender_type gender_value>
    struct gender_specifications
    {
    };
    template <>
    struct gender_specifications<gender_type::female>
    {
    };
}
namespace impl_2
{
    template <typename T, typename T::gender_type gender_arg>
    class gender_specifications
    {
        template <typename T::gender_type value>
        struct impl
        {
            constexpr static auto gender_value = value;
        };
        template <>
        struct impl<T::gender_type::female>
        {
            constexpr static auto gender_value = T::gender_type::female;
        };

    public:
        using type = impl<gender_arg>;
    };
    template <typename T, typename T::gender_type gender_arg>
    using gender_specifications_t = typename gender_specifications<T, gender_arg>::type; 
}
namespace impl_3
{
    template <auto gender_arg>
    class gender_specifications
    {
        using gender_arg_t = decltype(gender_arg);
        static_assert(std::is_enum_v<gender_arg_t>);

        template <gender_arg_t value>
        struct impl
        {
            using gender_type = gender_arg_t;
            constexpr static auto gender_value = gender_arg;
        };
        template <>
        struct impl<gender_arg_t::female>
        {
            using gender_type = gender_arg_t;
            constexpr static auto gender_value = gender_type::female;
        };

    public:
        using type = impl<gender_arg>;
    };
    template <auto gender_arg>
    using gender_specifications_t = typename gender_specifications<gender_arg>::type;
}


enum gender_type { male, female, unknown };

struct rat
{
};
struct unicorn
{
    enum gender_type { female };
};

template <class T, auto gender_value>
auto animal_factory()
{
    struct type : T, impl_3::gender_specifications_t<gender_value>
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
        const auto female_rat = animal_factory<unicorn, unicorn::female>();
        static_assert(concepts::female<decltype(female_rat)>);
    }
}
