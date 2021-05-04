#include <type_traits>

namespace SFINAE_synthaxes::example_1
{
    template <typename T>
    using IsNotReference = typename std::enable_if<!std::is_reference<T>::value>::type;

    template <typename T, typename = IsNotReference<T>>
    void do_stuff(T && arg)
    {}
    void test()
    {
        do_stuff(int{42});
        int i = 42;
        // do_stuff(i); // error: no matching function for call to 'do_stuff(int&)'
    }
}
namespace SFINAE_synthaxes::example_2
{
    template <typename T>
    struct IsNotReference : std::negation<std::is_reference<T>>{ };
    template <typename T>
    constexpr static auto IsNotReference_v = IsNotReference<T>::value;

    template <typename T, typename = std::enable_if_t<IsNotReference_v<T>>>
    void do_stuff(T && arg)
    {}
     void test()
    {
        do_stuff(int{42});
        int i = 42;
        // do_stuff(i); // error: no matching function for call to 'do_stuff(int&)'
    }
}