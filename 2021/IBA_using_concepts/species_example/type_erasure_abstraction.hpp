#include <concepts>
#include <type_traits>
#include <utility>
#include <memory>

namespace concepts
{
    template <typename T>
    concept animal = requires(T & value) {
        value.behave();
    };
}

namespace type_erasure_abstractions
{
    struct animal
    {
        template <concepts::animal T>
        animal(T && arg)
        : value_accessor{ std::make_unique<wrapper<T>>(std::forward<decltype(arg)>(arg)) }
        {}

        void behave()
        {
            value_accessor->behave();
        }

    private:
        struct model
        {
            virtual ~model() = 0;
            virtual void behave() = 0;
        };
        template <typename T>
        struct wrapper : model
        {
            wrapper(T && arg)
            : value{std::forward<decltype(arg)>(arg)}
            {}
            T value;
            void behave(){ value.behave(); }
        };
        std::unique_ptr<model> value_accessor;
    };
}
static_assert(concepts::animal<type_erasure_abstractions::animal>);


#include <iostream>

struct cat
{
    void behave() { std::cout << "meow\n"; }
};
struct dog
{
    void behave() { std::cout << "woof\n"; }
};

#include <vector>

auto main() -> int
{
    auto animals = std::vector<type_erasure_abstractions::animal>{};
    animals.emplace_back(cat{});
    animals.emplace_back(dog{});

}