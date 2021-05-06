// IBA -> with contracts IN CODE
//  - no more reading the documentation + error message bloat

// Why ? Flexible designs + powerful (specialization of cases, etc.)

// concepts vs SFINAE std::enable_if_t on type traits
// static_assert -> both ! (easier portability)
// => error messages examples

// --- concepts definitions

#include <concepts>
namespace concepts::cpp20
{
    template <typename T>
    concept can_behave = requires(T value)
    {
        value.behave();
    };

    template <typename T>
    concept has_hp_getter = requires(const T value)
    {
        { value.get_hp() } -> std::convertible_to<unsigned int>;
    };

    template <typename T>
    concept entity =
        can_behave<T> and
        has_hp_getter<T>
    ;
}

// template <concept_name>
// or template <typename T> requires clause -> more than 1 template parameter for concept

#include <type_traits>
#include <utility>
namespace concepts::cpp17
{
    // detection idiom
    template <typename T, typename = void>
    struct can_behave : std::false_type{};
    template <typename T>
    struct can_behave<T, std::void_t<decltype(std::declval<T>().behave())>>
    : std::true_type{};

    // detection idiom + return value check
    template <typename T, typename = void>
    struct has_hp_getter : std::false_type{};
    template <typename T>
    struct has_hp_getter<T, std::void_t<decltype(std::declval<const T>().get_hp())>>
    : std::is_convertible<decltype(std::declval<const T>().get_hp()), unsigned int>{};

    template <typename T>
    struct is_entity : std::conjunction<
        can_behave<T>,
        has_hp_getter<T>
    >
    {};
}

struct entity_implementation
{
    void behave(){}
    std::size_t get_hp() const { return 42; }
};
static_assert(concepts::cpp20::entity<entity_implementation>);
static_assert(concepts::cpp17::is_entity<entity_implementation>::value);

namespace usage::cpp20
{
    using namespace concepts::cpp20;
    template <entity entity_type>
    void use_entity(entity_type &&)
    {}

    void usage()
    {
        use_entity(entity_implementation{});
    }
}

namespace usage::cpp17
{
    using namespace concepts::cpp17;

    template <
        typename entity_type,
        typename = std::enable_if_t<concepts::cpp17::is_entity<entity_type>::value>
    >
    void use_entity(entity_type &&)
    {}

    void usage()
    {
        use_entity(entity_implementation{});
    }
}

// --- Type erasure
#include <memory>
namespace type_erasure::cpp17
{
    struct any_entity
    {
        template <typename T> // C++20 : template <concepts::cpp2::entity or requires clause
        any_entity(T && arg)
        : value_accessor{ std::make_unique<wrapper<T>>(std::forward<decltype(arg)>(arg)) }
        {
            static_assert(concepts::cpp17::is_entity<T>::value);
        }

        void behave() {
            value_accessor->behave();
        }
        auto get_hp() const {
            return value_accessor->get_hp();
        }

    private:
        struct model
        {
            virtual ~model() = default;
            virtual void behave() = 0;
            virtual unsigned int get_hp() const = 0;
        };
        template <typename T>
        struct wrapper : model
        {
            wrapper(T && arg)
            : value{std::forward<decltype(arg)>(arg)}
            {}
            ~wrapper() override {}
            void behave() override { value.behave(); }
            unsigned int get_hp() const override { return value.get_hp(); }
        private:
            T value;
        };
        std::unique_ptr<model> value_accessor;
    };

    static_assert(concepts::cpp17::is_entity<type_erasure::cpp17::any_entity>::value);
}

namespace usage
{
    struct hero
    {
        void behave(){}
        auto get_hp() const -> unsigned int { return 100; }
    };
    struct monster
    {
        monster(unsigned int hp_arg)
        : hp{hp_arg}
        {}
        void behave()
        {
            hp -= 1;
        }
        auto get_hp() const { return hp; }

    private:
        unsigned int hp = 13;
    };
}

#include <vector>
#include <numeric>
namespace usage::cpp17
{
    auto use_entity_type_erasure()
    {
        using namespace type_erasure::cpp17;
        using namespace usage;

        using collection_type = std::vector<any_entity>;

        collection_type entity_collection;
        entity_collection.emplace_back(hero{});
        entity_collection.emplace_back(monster{42});

        for (auto & element : entity_collection)
        {
            element.behave();
        }
        return std::accumulate(
            std::cbegin(entity_collection),
            std::cend(entity_collection),
            0,
            [](auto intermediate_sum, const decltype(entity_collection)::value_type & element){
                return element.get_hp() + intermediate_sum;
            }
        );
    }
}

#include <variant>
namespace usage::cpp20
{
    template <concepts::cpp20::entity ... entities_type>
    using entity_variant = std::variant<entities_type...>;

    auto use_entity_type_erasure()
    {
        using element_type = entity_variant<hero, monster>;
        using collection_type = std::vector<element_type>;

        auto entity_collection = collection_type{
            hero{},
            element_type{42}
        };

        const auto behave_visitor = [](auto & any_entity){
            any_entity.behave();
        };
        for (auto & element : entity_collection)
        {
            std::visit(behave_visitor, element);
        }
        return std::accumulate(
            std::cbegin(entity_collection),
            std::cend(entity_collection),
            0,
            [](auto intermediate_sum, const decltype(entity_collection)::value_type & element){
                return
                    std::visit([](const auto e){
                        return e.get_hp();
                    }, element) + intermediate_sum;
            }
        );
    }
}

// --- Bonus : flexible contracts

template <bool condition>
using if_t = std::conditional_t<condition, std::true_type, std::false_type>;

namespace flexible_concepts::cpp20
{
    // check constexper value equality in concepts
    template <typename T>
    // = (T::difficulty_value == decltype(T::difficulty_value)::legendary)>{})
    concept is_legendary = requires(T) {
        // BAD : T::difficulty_value == decltype(T::difficulty_value)::legendary;
        { if_t<(T::difficulty_value == decltype(T::difficulty_value)::legendary)>{} } -> std::same_as<std::true_type>;
    };

    template <typename T>
    concept has_difficulty_level = requires(T) {
        T::difficulty_value;
    };
}
#include <iostream>
namespace flexible_concepts::cpp20::usage
{
    enum difficulty{
        weak, average, hard, legendary
    };
    
    template <difficulty difficulty_arg>
    struct dungeon_monster
    {
        constexpr static auto difficulty_value = difficulty_arg;
    };

    template <flexible_concepts::cpp20::has_difficulty_level ... Ts>
    using entity = std::variant<Ts...>;

    struct unicorn
    {
        enum custom_difficulty{
            legendary
        };
        constexpr static auto difficulty_value = custom_difficulty::legendary;
    };
    template <std::size_t id>
    struct skeleton {
        enum custom_difficulty{
            not_that_hard,
            above_average
        };
        constexpr static auto difficulty_value = (
            id % 2 == 0
            ? custom_difficulty::not_that_hard
            : custom_difficulty::above_average
        );
    };
    struct boss{};

    template <class... Ts> struct overload : Ts... { using Ts::operator()...; };
    template <class... Ts> overload(Ts...) -> overload<Ts...>;

    void use()
    {
        auto visitor = overload{
            //[]<flexible_concepts::cpp20::is_legendary T>(const T &){
            [](boss &&){
                std::cout << "boss\n";
            },
            [](flexible_concepts::cpp20::is_legendary auto &&){
                std::cout << "legendary\n";
            },
            [](auto &&){
                std::cout << "NOT legendary nor a boss\n";
            }
        };

        visitor(boss{});
        visitor(dungeon_monster<difficulty::weak>{});
        visitor(dungeon_monster<difficulty::legendary>{});
        visitor(unicorn{});
        visitor(42);
    }
}

// using accessor = detect A or B => A::smthg, B::smthg
// or if-constexpr

namespace function_contract
{
    struct monster {
        using hp_type = unsigned int;
        hp_type hp{0};
    };

    namespace cpp20
    {
        template <typename F>
        concept monster_generator = requires(F) {
            { std::declval<F>()(monster::hp_type{}) } -> std::convertible_to<monster>;
        };
    }
    namespace cpp17
    {
        template <typename F, typename = void>
        struct is_monster_generator : std::false_type{};
        template <typename F>
        struct is_monster_generator<F, std::void_t<decltype(std::declval<F>()(monster::hp_type{}))>>
        : std::is_convertible<decltype(std::declval<F>()(monster::hp_type{})), monster>
        {};
    }

    namespace usage
    {
        auto generate_monster(monster::hp_type hp_value)
        {
            struct impl : monster{} value{hp_value};
            return value;
        }
        constexpr auto monster_generator = [](monster::hp_type hp_value){
            return monster{ hp_value };
        };

        static_assert(cpp20::monster_generator<decltype(generate_monster)>);
        static_assert(cpp20::monster_generator<decltype(monster_generator)>);

        static_assert(cpp17::is_monster_generator<decltype(generate_monster)>::value);
        static_assert(cpp17::is_monster_generator<decltype(monster_generator)>::value);
    }
}

#include <iostream>
auto main() -> int
{
    std::cout
        << "cpp17 : " << usage::cpp17::use_entity_type_erasure() << '\n'
        << "cpp20 : " << usage::cpp20::use_entity_type_erasure() << '\n'
        ;
    flexible_concepts::cpp20::usage::use();
}
