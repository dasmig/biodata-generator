#include "catch_amalgamated.hpp"
#include "../dasmig/biodatagen.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static dasmig::bdg make_loaded_generator()
{
    dasmig::bdg gen;
    if (!gen.load(dasmig::dataset::lite) &&
        !gen.load(dasmig::dataset::full))
    {
        for (const auto& candidate :
             {"resources/lite",
              "../resources/lite",
              "biodata-generator/resources/lite"})
        {
            if (std::filesystem::is_directory(candidate))
            {
                gen.load(candidate);
                break;
            }
        }
    }
    REQUIRE(gen.has_data());
    return gen;
}

// ---------------------------------------------------------------------------
// Loading
// ---------------------------------------------------------------------------

TEST_CASE("load - populates biodata from resource directory", "[loading]")
{
    auto gen = make_loaded_generator();
    REQUIRE(gen.country_count() > 50);
}

TEST_CASE("load - non-existent path is a no-op", "[loading]")
{
    dasmig::bdg gen;
    gen.load("does_not_exist_dir");
    REQUIRE_FALSE(gen.has_data());
    REQUIRE(gen.country_count() == 0);
}

TEST_CASE("load - file path (not directory) is a no-op", "[loading]")
{
    dasmig::bdg gen;
    gen.load("dasmig/biodatagen.hpp");
    REQUIRE_FALSE(gen.has_data());
}

// ---------------------------------------------------------------------------
// Dataset tier loading
// ---------------------------------------------------------------------------

TEST_CASE("load(dataset::lite) loads fewer than full", "[loading][tier]")
{
    dasmig::bdg gen_lite;
    dasmig::bdg gen_full;

    bool lite_ok = gen_lite.load(dasmig::dataset::lite);
    bool full_ok = gen_full.load(dasmig::dataset::full);

    if (lite_ok && full_ok)
    {
        REQUIRE(gen_lite.country_count() > 0);
        REQUIRE(gen_full.country_count() > 0);
        REQUIRE(gen_full.country_count() > gen_lite.country_count());
    }
}

TEST_CASE("load(dataset) returns false for missing tier", "[loading][tier]")
{
    dasmig::bdg gen;
    auto old_cwd = std::filesystem::current_path();
    auto tmp = std::filesystem::temp_directory_path() / "bdg_empty_dir";
    std::filesystem::create_directories(tmp);
    std::filesystem::current_path(tmp);

    REQUIRE_FALSE(gen.load(dasmig::dataset::lite));
    REQUIRE_FALSE(gen.load(dasmig::dataset::full));
    REQUIRE_FALSE(gen.has_data());

    std::filesystem::current_path(old_cwd);
    std::filesystem::remove_all(tmp);
}

// ---------------------------------------------------------------------------
// Generation — basic
// ---------------------------------------------------------------------------

TEST_CASE("get_biodata(cca2) returns valid biodata", "[generation]")
{
    auto gen = make_loaded_generator();
    auto b = gen.get_biodata("US");

    REQUIRE(b.country_code == "US");
    REQUIRE(b.height_cm > 100.0);
    REQUIRE(b.height_cm < 230.0);
    REQUIRE(b.weight_kg > 20.0);
    REQUIRE(b.weight_kg < 300.0);
    REQUIRE(b.bmi > 14.0);
    REQUIRE(b.bmi < 55.0);
    REQUIRE(b.seed() != 0);
}

TEST_CASE("get_biodata() returns any valid biodata", "[generation]")
{
    auto gen = make_loaded_generator();
    auto b = gen.get_biodata();

    REQUIRE_FALSE(b.country_code.empty());
    REQUIRE(b.height_cm > 100.0);
    REQUIRE(b.weight_kg > 20.0);
}

TEST_CASE("get_biodata with seed is deterministic", "[generation][seed]")
{
    auto gen = make_loaded_generator();
    auto b1 = gen.get_biodata("BR", std::uint64_t{42});
    auto b2 = gen.get_biodata("BR", std::uint64_t{42});

    REQUIRE(b1.height_cm == b2.height_cm);
    REQUIRE(b1.weight_kg == b2.weight_kg);
    REQUIRE(b1.bmi == b2.bmi);
    REQUIRE(b1.eyes == b2.eyes);
    REQUIRE(b1.hair == b2.hair);
    REQUIRE(b1.skin == b2.skin);
    REQUIRE(b1.blood == b2.blood);
    REQUIRE(b1.hand == b2.hand);
    REQUIRE(b1.bio_sex == b2.bio_sex);
}

TEST_CASE("get_biodata seed replay", "[generation][seed]")
{
    auto gen = make_loaded_generator();
    auto b1 = gen.get_biodata("JP");
    auto b2 = gen.get_biodata("JP", std::uint64_t{b1.seed()});

    REQUIRE(b1.height_cm == b2.height_cm);
    REQUIRE(b1.bmi == b2.bmi);
    REQUIRE(b1.eyes == b2.eyes);
}

TEST_CASE("get_biodata throws when empty", "[generation]")
{
    dasmig::bdg gen;
    REQUIRE_THROWS_AS(gen.get_biodata("US"), std::runtime_error);
}

TEST_CASE("get_biodata(seed) throws when empty", "[generation]")
{
    const dasmig::bdg gen;
    REQUIRE_THROWS_AS(gen.get_biodata(42), std::runtime_error);
}

TEST_CASE("get_biodata throws for unknown country", "[generation]")
{
    auto gen = make_loaded_generator();
    REQUIRE_THROWS_AS(gen.get_biodata("ZZ"), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// Sex distribution
// ---------------------------------------------------------------------------

TEST_CASE("both sexes are generated", "[generation][sex]")
{
    auto gen = make_loaded_generator();
    gen.seed(0);

    int males = 0;
    int females = 0;

    for (int i = 0; i < 500; ++i)
    {
        auto b = gen.get_biodata("US");
        if (b.bio_sex == dasmig::sex::male) ++males;
        else ++females;
    }

    REQUIRE(males > 100);
    REQUIRE(females > 100);
    gen.unseed();
}

TEST_CASE("sex-specific generation forces sex", "[generation][sex]")
{
    auto gen = make_loaded_generator();
    gen.seed(0);

    for (int i = 0; i < 100; ++i)
    {
        auto m = gen.get_biodata("US", dasmig::sex::male);
        REQUIRE(m.bio_sex == dasmig::sex::male);

        auto f = gen.get_biodata("US", dasmig::sex::female);
        REQUIRE(f.bio_sex == dasmig::sex::female);
    }
    gen.unseed();
}

TEST_CASE("sex-specific deterministic generation", "[generation][sex][seed]")
{
    auto gen = make_loaded_generator();
    auto b1 = gen.get_biodata("DE", dasmig::sex::female,
                               std::uint64_t{99});
    auto b2 = gen.get_biodata("DE", dasmig::sex::female,
                               std::uint64_t{99});

    REQUIRE(b1.bio_sex == dasmig::sex::female);
    REQUIRE(b2.bio_sex == dasmig::sex::female);
    REQUIRE(b1.height_cm == b2.height_cm);
    REQUIRE(b1.bmi == b2.bmi);
}

// ---------------------------------------------------------------------------
// Height distribution
// ---------------------------------------------------------------------------

TEST_CASE("height varies across generations", "[generation][height]")
{
    auto gen = make_loaded_generator();
    gen.seed(0);

    double min_h = 999.0;
    double max_h = 0.0;

    for (int i = 0; i < 500; ++i)
    {
        auto b = gen.get_biodata("US");
        if (b.height_cm < min_h) min_h = b.height_cm;
        if (b.height_cm > max_h) max_h = b.height_cm;
    }

    REQUIRE(max_h - min_h > 20.0); // reasonable spread
    gen.unseed();
}

TEST_CASE("male heights are taller on average than female", "[generation][height]")
{
    auto gen = make_loaded_generator();
    gen.seed(0);

    double sum_m = 0.0;
    double sum_f = 0.0;

    for (int i = 0; i < 500; ++i)
    {
        auto m = gen.get_biodata("US", dasmig::sex::male);
        auto f = gen.get_biodata("US", dasmig::sex::female);
        sum_m += m.height_cm;
        sum_f += f.height_cm;
    }

    REQUIRE(sum_m / 500.0 > sum_f / 500.0);
    gen.unseed();
}

// ---------------------------------------------------------------------------
// BMI and weight
// ---------------------------------------------------------------------------

TEST_CASE("weight is derived from height and BMI", "[generation][weight]")
{
    auto gen = make_loaded_generator();
    gen.seed(0);

    for (int i = 0; i < 100; ++i)
    {
        auto b = gen.get_biodata("US");
        double h_m = b.height_cm / 100.0;
        double expected_w = b.bmi * h_m * h_m;
        REQUIRE(std::abs(b.weight_kg - expected_w) < 0.01);
    }
    gen.unseed();
}

TEST_CASE("BMI is within plausible range", "[generation][bmi]")
{
    auto gen = make_loaded_generator();
    gen.seed(0);

    for (int i = 0; i < 500; ++i)
    {
        auto b = gen.get_biodata("US");
        REQUIRE(b.bmi >= 14.0);
        REQUIRE(b.bmi <= 55.0);
    }
    gen.unseed();
}

// ---------------------------------------------------------------------------
// Eye colour distribution
// ---------------------------------------------------------------------------

TEST_CASE("eye colour distribution is plausible", "[generation][eye]")
{
    auto gen = make_loaded_generator();
    gen.seed(0);

    std::unordered_map<int, int> counts;
    for (int i = 0; i < 1000; ++i)
    {
        auto b = gen.get_biodata("US");
        counts[static_cast<int>(b.eyes)]++;
    }

    // US should have all three categories.
    REQUIRE(counts.size() >= 2);
    // Brown should be most common for US.
    REQUIRE(counts[static_cast<int>(dasmig::eye_color::brown)] > 200);
    gen.unseed();
}

// ---------------------------------------------------------------------------
// Hair colour distribution
// ---------------------------------------------------------------------------

TEST_CASE("hair colour distribution is plausible", "[generation][hair]")
{
    auto gen = make_loaded_generator();
    gen.seed(0);

    std::set<int> seen;
    for (int i = 0; i < 1000; ++i)
    {
        auto b = gen.get_biodata("US");
        seen.insert(static_cast<int>(b.hair));
    }

    // US should produce at least 3 hair colours.
    REQUIRE(seen.size() >= 3);
    gen.unseed();
}

// ---------------------------------------------------------------------------
// Skin type distribution
// ---------------------------------------------------------------------------

TEST_CASE("skin type is within range I-VI", "[generation][skin]")
{
    auto gen = make_loaded_generator();
    gen.seed(0);

    for (int i = 0; i < 500; ++i)
    {
        auto b = gen.get_biodata("US");
        auto val = static_cast<int>(b.skin);
        REQUIRE(val >= 1);
        REQUIRE(val <= 6);
    }
    gen.unseed();
}

// ---------------------------------------------------------------------------
// Blood type distribution
// ---------------------------------------------------------------------------

TEST_CASE("blood type distribution is non-degenerate", "[generation][blood]")
{
    auto gen = make_loaded_generator();
    gen.seed(0);

    std::set<int> seen;
    for (int i = 0; i < 1000; ++i)
    {
        auto b = gen.get_biodata("US");
        seen.insert(static_cast<int>(b.blood));
    }

    // Should see most blood types for US.
    REQUIRE(seen.size() >= 5);
    gen.unseed();
}

// ---------------------------------------------------------------------------
// Handedness distribution
// ---------------------------------------------------------------------------

TEST_CASE("both handedness values are generated", "[generation][hand]")
{
    auto gen = make_loaded_generator();
    gen.seed(0);

    int left = 0;
    int right = 0;

    for (int i = 0; i < 500; ++i)
    {
        auto b = gen.get_biodata("US");
        if (b.hand == dasmig::handedness::left) ++left;
        else ++right;
    }

    REQUIRE(left > 10);
    REQUIRE(right > 200);
    gen.unseed();
}

// ---------------------------------------------------------------------------
// String conversion
// ---------------------------------------------------------------------------

TEST_CASE("implicit string conversion returns summary", "[conversion]")
{
    auto gen = make_loaded_generator();
    auto b = gen.get_biodata("US", std::uint64_t{42});
    std::string s = b;
    REQUIRE(s == b.to_string());
    REQUIRE_FALSE(s.empty());
}

TEST_CASE("ostream operator outputs summary", "[conversion]")
{
    auto gen = make_loaded_generator();
    auto b = gen.get_biodata("US", std::uint64_t{42});
    std::ostringstream oss;
    oss << b;
    REQUIRE(oss.str() == b.to_string());
}

// ---------------------------------------------------------------------------
// Enum to string
// ---------------------------------------------------------------------------

TEST_CASE("eye_color_str returns valid labels", "[enum]")
{
    using dasmig::biodata;
    REQUIRE(biodata::eye_color_str(dasmig::eye_color::blue) == "blue");
    REQUIRE(biodata::eye_color_str(dasmig::eye_color::intermediate)
            == "intermediate");
    REQUIRE(biodata::eye_color_str(dasmig::eye_color::brown) == "brown");
}

TEST_CASE("hair_color_str returns valid labels", "[enum]")
{
    using dasmig::biodata;
    REQUIRE(biodata::hair_color_str(dasmig::hair_color::black) == "black");
    REQUIRE(biodata::hair_color_str(dasmig::hair_color::brown) == "brown");
    REQUIRE(biodata::hair_color_str(dasmig::hair_color::blond) == "blond");
    REQUIRE(biodata::hair_color_str(dasmig::hair_color::red) == "red");
}

TEST_CASE("skin_type_str returns valid labels", "[enum]")
{
    using dasmig::biodata;
    REQUIRE(biodata::skin_type_str(dasmig::skin_type::I) == "I");
    REQUIRE(biodata::skin_type_str(dasmig::skin_type::VI) == "VI");
}

TEST_CASE("blood_type_str returns valid labels", "[enum]")
{
    using dasmig::biodata;
    REQUIRE(biodata::blood_type_str(dasmig::blood_type::O_pos) == "O+");
    REQUIRE(biodata::blood_type_str(dasmig::blood_type::AB_neg) == "AB-");
}

TEST_CASE("handedness_str returns valid labels", "[enum]")
{
    using dasmig::biodata;
    REQUIRE(biodata::handedness_str(dasmig::handedness::right) == "right");
    REQUIRE(biodata::handedness_str(dasmig::handedness::left) == "left");
}

TEST_CASE("sex_str returns valid labels", "[enum]")
{
    using dasmig::biodata;
    REQUIRE(biodata::sex_str(dasmig::sex::male) == "male");
    REQUIRE(biodata::sex_str(dasmig::sex::female) == "female");
}

// ---------------------------------------------------------------------------
// Seed accessors
// ---------------------------------------------------------------------------

TEST_CASE("biodata::seed() returns non-zero after generation", "[seed]")
{
    auto gen = make_loaded_generator();
    auto b = gen.get_biodata("US");
    REQUIRE(b.seed() != 0);
}

// ---------------------------------------------------------------------------
// Seeding
// ---------------------------------------------------------------------------

TEST_CASE("seed/unseed produces deterministic then random", "[seed]")
{
    auto gen = make_loaded_generator();

    gen.seed(100);
    auto a = gen.get_biodata("DE");
    auto b = gen.get_biodata("DE");

    gen.seed(100);
    auto a2 = gen.get_biodata("DE");
    auto b2 = gen.get_biodata("DE");

    REQUIRE(a.height_cm == a2.height_cm);
    REQUIRE(b.height_cm == b2.height_cm);
    REQUIRE(a.bmi == a2.bmi);
    REQUIRE(b.bmi == b2.bmi);

    gen.unseed();
}

TEST_CASE("seed is chainable", "[seed]")
{
    auto gen = make_loaded_generator();
    auto& ref = gen.seed(42);
    REQUIRE(&ref == &gen);
    gen.unseed();
}

TEST_CASE("unseed is chainable", "[seed]")
{
    auto gen = make_loaded_generator();
    auto& ref = gen.unseed();
    REQUIRE(&ref == &gen);
}

// ---------------------------------------------------------------------------
// Multiple countries
// ---------------------------------------------------------------------------

TEST_CASE("can generate for many different countries", "[generation]")
{
    auto gen = make_loaded_generator();

    for (const auto* code : {"US", "BR", "JP", "NG", "DE", "IN", "CN",
                              "AU", "ZA", "MX", "FR", "GB"})
    {
        auto b = gen.get_biodata(code);
        REQUIRE(b.country_code == code);
        REQUIRE(b.height_cm > 100.0);
        REQUIRE(b.height_cm < 230.0);
    }
}

// ---------------------------------------------------------------------------
// Country-level differences
// ---------------------------------------------------------------------------

TEST_CASE("different countries produce different average heights",
          "[generation][country]")
{
    auto gen = make_loaded_generator();
    gen.seed(0);

    // Netherlands is one of the tallest, India is shorter on average.
    double nl_sum = 0.0;
    double in_sum = 0.0;
    static constexpr int N = 200;

    for (int i = 0; i < N; ++i)
    {
        auto nl = gen.get_biodata("NL", dasmig::sex::male);
        auto in_ = gen.get_biodata("IN", dasmig::sex::male);
        nl_sum += nl.height_cm;
        in_sum += in_.height_cm;
    }

    REQUIRE(nl_sum / N > in_sum / N);
    gen.unseed();
}

// ---------------------------------------------------------------------------
// Random country seed is deterministic
// ---------------------------------------------------------------------------

TEST_CASE("get_biodata(seed) selects same country deterministically",
          "[generation][seed]")
{
    auto gen = make_loaded_generator();
    auto b1 = gen.get_biodata(std::uint64_t{9999});
    auto b2 = gen.get_biodata(std::uint64_t{9999});

    REQUIRE(b1.country_code == b2.country_code);
    REQUIRE(b1.height_cm == b2.height_cm);
    REQUIRE(b1.bmi == b2.bmi);
    REQUIRE(b1.bio_sex == b2.bio_sex);
}

// ---------------------------------------------------------------------------
// Random country with sex
// ---------------------------------------------------------------------------

TEST_CASE("get_biodata(sex) from random country forces sex",
          "[generation][sex]")
{
    auto gen = make_loaded_generator();
    gen.seed(0);

    for (int i = 0; i < 100; ++i)
    {
        auto m = gen.get_biodata(dasmig::sex::male);
        REQUIRE(m.bio_sex == dasmig::sex::male);
        REQUIRE_FALSE(m.country_code.empty());

        auto f = gen.get_biodata(dasmig::sex::female);
        REQUIRE(f.bio_sex == dasmig::sex::female);
    }
    gen.unseed();
}

TEST_CASE("get_biodata(sex, seed) is deterministic",
          "[generation][sex][seed]")
{
    auto gen = make_loaded_generator();
    auto b1 = gen.get_biodata(dasmig::sex::male, std::uint64_t{777});
    auto b2 = gen.get_biodata(dasmig::sex::male, std::uint64_t{777});

    REQUIRE(b1.country_code == b2.country_code);
    REQUIRE(b1.height_cm == b2.height_cm);
    REQUIRE(b1.bio_sex == dasmig::sex::male);
}

// ---------------------------------------------------------------------------
// to_string format
// ---------------------------------------------------------------------------

TEST_CASE("to_string contains expected fields", "[conversion]")
{
    auto gen = make_loaded_generator();
    auto b = gen.get_biodata("US", std::uint64_t{42});
    auto s = b.to_string();

    REQUIRE(s.find("cm") != std::string::npos);
    REQUIRE(s.find("kg") != std::string::npos);
    REQUIRE(s.find("BMI=") != std::string::npos);
    REQUIRE(s.find("eyes=") != std::string::npos);
    REQUIRE(s.find("hair=") != std::string::npos);
    REQUIRE(s.find("skin=") != std::string::npos);
    REQUIRE(s.find("blood=") != std::string::npos);
}
