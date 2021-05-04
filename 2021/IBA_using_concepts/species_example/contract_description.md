# Contract example : feline

## Concepts tree

- mammal
  - Animal vertébré
    - Animal
      - livingthing
        - hétérotrophes : feeds on organic substances
        - vertebrate
          - has_spine
      - Constant temperature
      - Breathe using lungs
      - [skipped] : central nervous system developed
      - females carry udders
- predator
     Hunt(prey)
- prey
     Hunted(predator)

## Examples

Animal examples

- spineless : mosquitos, molluscs (snails, ...)
- not feline : dogs
- not const temperature : snakes
- no udders : birds

## Points

- static polymorphism
- type erasure
- zero-cost abstration
- no tight coupling (classes, CMake targets, ...)
  - share contracts, not classes
  - no forward-declarations

## todos

- detection idiom using std::void_t
- std::enable_if vs static_assert => error messages

```cpp
namespace 
{
     template <typename T>
     void do_stuffs()
     {
          static_assert(SomeTrait_v<T>, "Requirement unmet : SomeTrait<T>");
     }
}
```

- check perfs
- check generated assembly