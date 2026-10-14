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
        auto b = gen.get_biodata({.country = code});
        std::cout << b.to_string() << "\n";
    }

    // Generate from a random country.
    std::cout << "\nRandom:\n";
    for (int i = 0; i < 5; ++i)
    {
        auto b = gen.get_biodata({});
        std::cout << "  " << b << "\n";
    }

    // Deterministic replay.
    std::cout << "\nDeterministic replay:\n";
    auto b1 = gen.get_biodata({.country = "US"}, std::uint64_t{42});
    auto b2 = gen.get_biodata({.country = "US"}, std::uint64_t{42});
    std::cout << "  seed 42 -> " << b1.to_string() << "\n";
    std::cout << "  seed 42 -> " << b2.to_string()
              << (b1.height_cm == b2.height_cm ? "  match" : "  mismatch")
              << "\n";

    // Sex-specific generation.
    std::cout << "\nSex-specific:\n";
    for (int i = 0; i < 3; ++i)
    {
        auto m = gen.get_biodata({.country = "US", .bio_sex = dasmig::sex::male});
        std::cout << "  Male:   " << m.height_cm << "cm "
                  << m.weight_kg << "kg\n";
    }
    for (int i = 0; i < 3; ++i)
    {
        auto f = gen.get_biodata({.country = "US", .bio_sex = dasmig::sex::female});
        std::cout << "  Female: " << f.height_cm << "cm "
                  << f.weight_kg << "kg\n";
    }

    std::cout << "\nDeterministic sequence (explicit seeds 100-102):\n";
    const dasmig::biodata_request request{.country = "JP"};
    for (std::uint64_t seed = 100; seed < 103; ++seed)
    {
        auto b = gen.get_biodata(request, seed);
        std::cout << "  " << b << "\n";
    }

    std::cout << "\nWith age:\n  "
              << gen.get_biodata({.country = "BR", .bio_sex = dasmig::sex::female, .age = 67}, 42)
              << "\n";

    // Enum label helpers.
    std::cout << "\nEnum labels:\n";
    auto b = gen.get_biodata({.country = "IN"});
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

    // JSON with the replay signature (seed and data hash).
    std::cout << "\nJSON:\n  " << gen.get_biodata({.country = "BR"}, std::uint64_t{7}).to_json() << "\n";

    // Extend the data with a layer: make Brazilians much taller.
    const auto& base = gen.layers().front();
    std::cout << "\nBase data: " << base.source << " (" << base.rows << " rows)\n";
    const std::string layer =
        "country_code\tcountry_name\tmale_height_cm\tmale_height_sd\tfemale_height_cm\t"
        "female_height_sd\tmale_bmi_mean\tfemale_bmi_mean\tbmi_sd\teye_blue\teye_intermediate\t"
        "eye_brown\thair_black\thair_brown\thair_blond\thair_red\tskin_I\tskin_II\tskin_III\t"
        "skin_IV\tskin_V\tskin_VI\tblood_O_pos\tblood_A_pos\tblood_B_pos\tblood_AB_pos\t"
        "blood_O_neg\tblood_A_neg\tblood_B_neg\tblood_AB_neg\tleft_handed_pct\n"
        "BR\tBrazil\t195\t6.5\t182\t6\t26\t26\t4.5\t10\t20\t70\t30\t45\t20\t5\t"
        "5\t15\t30\t30\t15\t5\t40\t30\t20\t5\t2\t1\t1\t1\t10\n";
    const auto& info = gen.extend_from_string(layer, "tall-brazil");
    std::cout << "Layer " << info.source << ": " << info.replaced << " replaced, "
              << info.added << " added\n";
    std::cout << "  " << gen.get_biodata({.country = "BR"}, std::uint64_t{7}) << "\n";

    return 0;
}
