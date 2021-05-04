#include <concepts>
#include <type_traits>
#include <algorithm>

namespace mp
{
    template <bool evaluation>
    using if_t = std::conditional_t<evaluation, std::true_type, std::false_type>;

    template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
    template<class... Ts> overload(Ts...) -> overload<Ts...>;
}
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
        { std::decay_t<decltype(T::gender_value)>{} } -> std::same_as<typename T::gender_type>;
    };
    template <typename T>
    concept female = gendered<T> && requires(T) {
        { T::gender_value == decltype(T::gender_value)::female } -> std::convertible_to<bool>;
        { mp::if_t<T::gender_value == decltype(T::gender_value)::female>{} } -> std::same_as<std::true_type>;
    };
    template <typename T>
    concept male = gendered<T> && requires(T) {
        { T::gender_value == decltype(T::gender_value)::male } -> std::convertible_to<bool>;
        { mp::if_t<T::gender_value == decltype(T::gender_value)::male>{} } -> std::same_as<std::true_type>;
    };
    template <typename T1, typename T2>
    concept same_species = std::same_as<typename T1::species_type, typename T2::species_type>;
    template <typename T1, typename T2>
    concept can_copulate =
        same_species<T1, T2> &&
        gendered<T1> &&
        gendered<T2> && 
        ((male<T1> && female<T2>) || (female<T1> && male<T2>))
    ;

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
        static_assert(std::is_enum_v<decltype(arg)>);
        using type = impl<arg>;
    };
    template <auto gender_arg>
    using gender_specifications_t = typename gender_specifications<gender_arg>::type;

    template <
        class species,
        auto gender_value,
        template <auto> typename gender_specifier = gender_specifications_t
    >
    requires
        (not concepts::gendered<species>) &&
        concepts::gendered<gender_specifier<gender_value>>
    auto animal_factory()
    {   // replace lambda in unevaluated context
        struct type : species, gender_specifier<gender_value>
        {
            using species_type = species;
        };
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

    struct mouse_species
    {
        enum genders { male, female };

        // prey requirements ...
        template <typename predator_type>
        void hunted_by(const predator_type &)
        {
            static_assert(concepts::prey_of<decltype(*this), predator_type>);
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
    using male_mouse = animal_type<mouse_species, mouse_species::male>;
    static_assert(concepts::mammal<male_mouse>);
    using female_mouse = animal_type<mouse_species, mouse_species::female>;
    static_assert(concepts::mammal<female_mouse>);

    struct cat_species
    {
        // gendered requirements ...
        enum genders { male, female };
        // animal requirements ...
        void behave(){}
        // vertebrate requirements ...
        struct spine_type{};
        spine_type spine;

        // predator requirements ...
        template <typename prey_type>
        void hunt(prey_type &)
        {
            static_assert(concepts::predator_of<decltype(*this), prey_type>);
        }

        // has_constant_temperature
        const int temperature = 37;
        // mammals requirements ...
        void breathe(){}
    };
    using male_cat = animal_type<cat_species, cat_species::male>;
    static_assert(concepts::mammal<male_cat>);
    using female_cat = animal_type<cat_species, cat_species::female>;
    static_assert(concepts::mammal<female_cat>);

    template <class feline_type>
        requires
            concepts::predator_of<feline_type, mouse_species> &&
            concepts::mammal<feline_type>
    constexpr void hunt_male_mouse(feline_type & some_feline)
    {
        male_mouse some_male_mouse;
        static_assert(concepts::mammal<male_mouse>);
        some_feline.hunt(some_male_mouse);
    }

    void test()
    {
        {
            auto some_female_cat = animal_factory<cat_species, cat_species::female>();
            static_assert(decltype(some_female_cat)::gender_value == cat_species::female);
            static_assert(concepts::female<decltype(some_female_cat)>);
            hunt_male_mouse(some_female_cat);
        }
        {
            auto some_male_cat = animal_factory<cat_species, cat_species::male>();
            static_assert(decltype(some_male_cat)::gender_value == cat_species::male);
            static_assert(not concepts::female<decltype(some_male_cat)>);
            hunt_male_mouse(some_male_cat);
        }
    }
}

#include <variant>
#include <array>
#include <ranges>

#include <iostream>
#include <typeinfo>
namespace using_contracts::sample
{
    template <using_contracts::concepts::animal ... animal_type>
    static constexpr auto animal_collection_v = std::array<std::variant<animal_type...>, sizeof...(animal_type)>{ animal_type{}...};

    void simulation()
    {
        auto animals_collection_value = animal_collection_v<female_cat, male_cat, female_mouse, male_mouse>;

        const auto behaviors = mp::overload
        {
            []<concepts::animal T, concepts::animal U>(T&, U&)
                requires
                    concepts::can_copulate<T,U>
            {
                // copulate
                std::cout << "copulate :\n\t" << typeid(T).name() << "\nand\n\t" << typeid(U).name() << '\n';
            },
            []<concepts::animal T, concepts::animal U>(T & T_value, U & U_value)
                requires
                    concepts::predator_of<T,U> ||
                    concepts::predator_of<U,T>
            {
                std::cout << "hunt :\n\t" << typeid(T).name() << "\nand\n\t" << typeid(U).name() << '\n';

                if constexpr (concepts::predator_of<T,U>)
                    T_value.hunt(U_value);
                if constexpr (concepts::predator_of<U, T>)
                    U_value.hunt(T_value);
            },
            [](auto & arg1, auto & arg2)
            {
                // ignore each others
                std::cout << "ignore :\n\t" << typeid(arg1).name() << "\nand\n\t" << typeid(arg2).name() << '\n';
            }
        };
        for (auto & animal_value : animals_collection_value)
        {
            for (auto & other_animal : animals_collection_value | std::views::filter([&animal_value](auto & rhs) {
                return &animal_value != &rhs;
            }))
            {
                std::visit(behaviors, animal_value, other_animal);
            }
        }
    }
}

// todo : CRTP on models

auto main() -> int
{
    using_contracts::sample::simulation();
}
