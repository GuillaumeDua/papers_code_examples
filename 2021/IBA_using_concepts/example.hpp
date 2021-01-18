#include <utility>
#include <type_traits>
#include <functional>
#include <array>

namespace using_contracts
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
        { T::gender_value } -> std::same_as<const typename T::gender_type &>;
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

    template <class T, class prey_type>
    concept feline = mammal<T> && predator_of<T, prey_type>;

    // ---------- Impls

    template <class T, class predator_type>
    concept rodent = mammal<T> && prey_of<T, predator_type>;

    struct mouse
    {
        enum gender_type { unknown };
        constexpr static auto gender_value = gender_type::unknown;

        // prey requirements ...
        template <typename predator_type>
        void hunted_by(const predator_type &)
        {
            static_assert(prey_of<mouse, predator_type>);
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

    struct cat
    {
        // gendered requirements ...
        enum gender_type {male, female};
        constexpr static auto gender_value = gender_type::male;

        // animal requirements ...
        void behave(){}

        // vertebrate requirements ...
        struct spine_type{};
        spine_type spine;

        // predator requirements ...
        template <typename prey_type>
        void hunt(prey_type &)
        {
            static_assert(predator_of<cat, prey_type>);
        }

        // has_constant_temperature
        const int temperature = 37;

        // mammals requirements ...
        void breathe(){}

    };
    static_assert(rodent<mouse, cat>);
    static_assert(feline<cat, mouse>);

    template <feline<mouse> feline_type>
    void use_feline(feline_type some_feline)
    {
        mouse some_mouse;
        some_feline.hunt(some_mouse);
    }

    void test()
    {
        cat some_cat;
        use_feline(some_cat);
    }
}