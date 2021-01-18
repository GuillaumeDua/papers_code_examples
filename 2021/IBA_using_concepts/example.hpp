#include <concepts>
#include <type_traits>
#include <algorithm>

namespace using_contracts::concepts
{
    template <typename T>
    concept animal = requires(T & value) {
        value.behave();
    };
    template <typename T>
    concept vertebrate = animal<T> && requires(const T & value) {
        value.spine;
    };

    template <typename predator_type, typename prey_type>
    concept predator_of = requires(predator_type & value) {
        { value.hunt(std::declval<prey_type&>()) };
        { std::declval<prey_type&>().hunted_by(std::declval<const predator_type &>()) };
    };
    template <typename prey_type, typename predator_type>
    concept prey_of = predator_of<predator_type, prey_type>;

    template <typename T>
    concept gendered = requires(T value) {
        { std::is_enum_v<decltype(T::gender_value)> };
        { T::gender_value } -> std::same_as<const typename T::gender_type>;
    };
    template <typename T>
    concept female = gendered<T> && requires(T) {
        { T::gender_value == decltype(T::gender_value)::female } -> std::convertible_to<bool>;
        { std::conditional_t
            <
                T::gender_value == decltype(T::gender_value)::female,
                std::true_type,
                std::false_type
            >{} } -> std::same_as<std::true_type>;
    };

    template <typename T>
    concept iterable = requires(T value) {
        { value.begin() } -> std::same_as<decltype(value.end())>;
    };
    template <typename T>
    concept has_udders = requires(const T value) {
        { value.udders } -> iterable;
        { std::size(value.udders) > 0 };
    };
    template <typename T>
    concept has_constant_temperature = requires(T & value) {
        { value.temperature } -> std::convertible_to<int>;
        std::is_const_v<decltype(std::declval<T&>().temperature)>;
    };

    template <typename T>
    concept mammal = vertebrate<T>
        && has_constant_temperature<T>
        && gendered<T>
        && requires(T & value) {
        {
            not female<T> ||
            (female<T> && has_udders<T>)
        };
        { value.breathe() };
    };

    // template <class T, class prey_type>
    // concept feline = mammal<T> && predator_of<T, prey_type>;
    // template <class T, class predator_type>
    // concept rodent = mammal<T> && prey_of<T, predator_type>;
}
namespace using_contracts::traits
{
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
}
namespace using_contracts::sample
{   // ---------- Impls
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
        struct impl<gender_arg, std::enable_if_t<traits::is_female_value<gender_arg>>>
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

    template <
        class T,
        auto gender_value,
        template <auto> typename gender_specifier = gender_specifications_t
    >
    requires (not concepts::gendered<T>)
    auto animal_factory()
    {   // replace lambda in unevaluated context
        static_assert(concepts::gendered<gender_specifier<gender_value>>);
        struct type : T, gender_specifier<gender_value>
        {};
        static_assert(concepts::gendered<type>);
        return type{};
    };
    template <
        class T,
        auto gender_value,
        template <auto> typename gender_specifier = gender_specifications_t
    >
    requires (not concepts::gendered<T>)
    using animal_type = decltype(animal_factory<T, gender_value, gender_specifier>());

    struct mouse_model
    {
        enum gender_type { male, female };

        // prey requirements ...
        template <typename predator_type>
        void hunted_by(const predator_type &)
        {
            static_assert(concepts::prey_of<mouse_model, predator_type>);
        }

        // animal requirements ...
        void behave(){}

        // vertebrate requirements ...
        struct spine_type{};
        spine_type spine;

        // mammals requirements ...
        void breathe(){}
        // has_constant_temperature
        const int temperature = 35;
    };
    using male_mouse = animal_type<mouse_model, mouse_model::male>;
    using female_mouse = animal_type<mouse_model, mouse_model::female>;

    struct cat_model
    {
        // gendered requirements ...
        enum gender_type { male, female };

        // animal requirements ...
        void behave(){}

        // vertebrate requirements ...
        struct spine_type{};
        spine_type spine;

        // predator requirements ...
        template <typename prey_type>
        void hunt(prey_type &)
        {
            static_assert(concepts::predator_of<cat_model, prey_type>);
        }

        // has_constant_temperature
        const int temperature = 37;

        // mammals requirements ...
        void breathe(){}
    };
    using male_cat = animal_type<cat_model, cat_model::male>;
    using female_cat = animal_type<cat_model, cat_model::female>;

    template <class feline_type>
        requires
            concepts::predator_of<feline_type, male_mouse> &&
            concepts::mammal<feline_type>
    void hunt_male_mouse(feline_type & some_feline)
    {
        male_mouse some_male_mouse;
        static_assert(concepts::mammal<male_mouse>);
        some_feline.hunt(some_male_mouse);
    }

    void test()
    {
        {
            auto some_female_cat = animal_factory<cat_model, cat_model::female>();
            static_assert(decltype(some_female_cat)::gender_value == cat_model::female);
            static_assert(concepts::female<decltype(some_female_cat)>);
            hunt_male_mouse(some_female_cat);
        }
        {
            auto some_male_cat = animal_factory<cat_model, cat_model::male>();
            static_assert(decltype(some_male_cat)::gender_value == cat_model::male);
            static_assert(not concepts::female<decltype(some_male_cat)>);
            hunt_male_mouse(some_male_cat);
        }
    }
}

// todo : CRTP on models
