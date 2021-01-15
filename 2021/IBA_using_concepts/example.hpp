#include <utility>
#include <type_traits>
#include <functional>

// SFINAE, detection idiom, contracts
//
// Contract example : feline
// - default constructible
// - mammal
//      Animal vertébré
//          Animal
//              êtres vivants
//              hétérotrophes : se nourrissent de substances organiques
//          vertebrate
//              has_spine
//      température constante
//      respirant par des poumons
//      [skipped] : système nerveux central développé
//      les femelles portent des mamelles
// - predator
//      Hunt(prey)
// - prey
//      Hunted(predator)

// Animal examples
//  - spineless : mosquitos, molluscs (snails, ...)
//  - Not feline : dogs
//  - not const temperature : snakes
//  - no udders : birds

// Points :
// - static polymorphism
// - type erasure
// - zero-cost abstration
// - no tight coupling (classes, CMake targets, ...)
//  - share contracts, not classes
//  - no forward-declarations

// todo : check perfs

namespace using_inheritance
{
    struct animal
    {
        virtual void behave() = 0;
    };
    class vertebrate : animal
    {
        struct spine_type{} spine;
    };
    enum gender_type { male, female };
    struct gendered
    {
        const gender_type gender;
    };

    struct predator;
    struct prey
    {
        virtual void hunted_by(const predator&) = 0;
    };
    struct predator
    {
        virtual void hunt(prey&) = 0;
    };

    struct mammal : vertebrate, gendered
    {
        const int temperature;

        const auto has_udders()
        {
            return gender == gender_type::female;
        } 

        void breathe()
        {
            lungs.use();
        }

    private:
        struct lungs_type{ void use(){}; } lungs;
    };
    struct feline : mammal, predator
    {};
}
namespace using_cpp11
{
    template <typename T, template <typename> class ... requirements_list>
    using requirements_t = std::conjunction<requirements_list<T>...>;
}
namespace using_cpp17
{
    template <typename T>
    constexpr static const auto can_fluctuate = std::is_const_v<T::value>;

    // gennder-specific : crtp ?
}
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

    template <typename T, typename predator_type>
    concept prey = requires(T & value) {
        { value.hunted_by(std::declval<const predator_type&>()) };
        { std::declval<predator_type&>().hunt(value) };
    };
    template <typename T, typename prey_type>
    concept predator = requires(T & value) {
        { value.hunt(std::declval<prey_type&>()) };
        { std::declval<prey_type&>().hunted_by(std::declval<const T &>()) };
    };

    template <typename T>
    concept gendered = requires(T) {
        T::gender_value;
    };
    template <typename T>
    concept female = gendered<T> && requires(T) {
        std::is_enum_v<decltype(T::gender_value)>;
        T::gender_value == decltype(T::gender_value)::female;
    };

    template <typename T>
    concept iterable = requires(T value) {
        { value.begin() } -> std::same_as<decltype(value.end())>;
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
        { not female<T> || (female<T> && value.udders -> iterable) }; // or std::array::size > 0
        { value.breathe() };
    };

    template <class T, class prey_type>
    concept feline = mammal<T> && predator<T, prey_type>;

    // ---------- Impls

    template <class T, class predator_type>
    concept rodent = mammal<T> && prey<T, predator_type>;

    // todo : crtp -> gender_specific

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
        template <rodent<cat> rodent_type>
        void hunt(rodent_type &){}

        // has_constant_temperature
        const int temperature = 37;

        // mammals requirements ...

    };

}


namespace impl_1
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
namespace impl_2
{
    template <typename T>
    struct IsNotReference : std::negation<std::is_reference<T>>{ };
    template <typename T>
    constexpr static auto IsNotReference_v = IsNotReference<T>::value;

    template <typename T, typename = std::enable_if_t<IsNotReference_v<T>>>
    void do_stuff(T && arg)
    {}

    // template< class, class = void>
    // struct IsNotReference : std::false_type { };
    //  template <class T>
    // struct IsNotReference<T, std::void_t<typename T::type>> : std::true_type { };
}