#include "../dasmig/biodatagen.hpp"
#include <iostream>
#include <string>

int main()
{
    dasmig::bdg gen;

    // Try auto-probe, then explicit tiers, then raw paths.
    if (!gen.load(dasmig::dataset::lite) &&
        !gen.load(dasmig::dataset::full))
    {
        gen.load(std::filesystem::path{"resources/lite"});
    }

    if (!gen.has_data())
    {
        std::cerr << "Could not load biodata.\n";
        return 1;
    }

    std::cout << "Loaded " << gen.country_count() << " countries.\n\n";

    // Generate biodata for specific countries.
    for (const auto* code : {"US", "BR", "JP", "NG", "DE"})
    {
        auto b = gen.get_biodata(code);
        std::cout << code << ": " << b.to_string()
                  << "  sex=" << dasmig::biodata::sex_str(b.bio_sex)
                  << "\n";
    }

    // Generate from a random country.
    std::cout << "\nRandom:\n";
    for (int i = 0; i < 5; ++i)
    {
        auto b = gen.get_biodata();
        std::cout << "  " << b.country_code << ": " << b << "\n";
    }

    // Deterministic replay.
    std::cout << "\nDeterministic replay:\n";
    auto b1 = gen.get_biodata("US", std::uint64_t{42});
    auto b2 = gen.get_biodata("US", std::uint64_t{42});
    std::cout << "  seed 42 → " << b1.to_string() << "\n";
    std::cout << "  seed 42 → " << b2.to_string()
              << (b1.height_cm == b2.height_cm ? "  match" : "  mismatch")
              << "\n";

    // Sex-specific generation.
    std::cout << "\nSex-specific:\n";
    for (int i = 0; i < 3; ++i)
    {
        auto m = gen.get_biodata("US", dasmig::sex::male);
        std::cout << "  Male:   " << m.height_cm << "cm "
                  << m.weight_kg << "kg\n";
    }
    for (int i = 0; i < 3; ++i)
    {
        auto f = gen.get_biodata("US", dasmig::sex::female);
        std::cout << "  Female: " << f.height_cm << "cm "
                  << f.weight_kg << "kg\n";
    }

    // Seed the engine for a deterministic sequence.
    std::cout << "\nDeterministic sequence (seed=100):\n";
    gen.seed(100);
    for (int i = 0; i < 3; ++i)
    {
        auto b = gen.get_biodata("JP");
        std::cout << "  " << b << "\n";
    }
    gen.unseed();

    // Enum label helpers.
    std::cout << "\nEnum labels:\n";
    auto b = gen.get_biodata("IN");
    std::cout << "  Eyes: "
              << dasmig::biodata::eye_color_str(b.eyes) << "\n";
    std::cout << "  Hair: "
              << dasmig::biodata::hair_color_str(b.hair) << "\n";
    std::cout << "  Skin: "
              << dasmig::biodata::skin_type_str(b.skin) << "\n";
    std::cout << "  Blood: "
              << dasmig::biodata::blood_type_str(b.blood) << "\n";
    std::cout << "  Hand: "
              << dasmig::biodata::handedness_str(b.hand) << "\n";

    return 0;
}
