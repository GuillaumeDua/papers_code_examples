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
