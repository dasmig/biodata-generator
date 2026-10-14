#include "catch_amalgamated.hpp"
#include "../dasmig/biodatagen.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <format>
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

    if (!lite_ok || !full_ok)
    {
        SKIP("One or both dataset tiers not found — skipping");
    }

    REQUIRE(gen_lite.country_count() > 0);
    REQUIRE(gen_full.country_count() > 0);
    REQUIRE(gen_full.country_count() > gen_lite.country_count());
}

#ifndef DASMIG_BIODATA_EMBED // compiled-in data is never missing
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
#endif

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

TEST_CASE("get_biodata throws with informative message", "[generation]")
{
    auto gen = make_loaded_generator();
    REQUIRE_THROWS_WITH(gen.get_biodata("ZZ"),
                        Catch::Matchers::ContainsSubstring("ZZ"));
}

TEST_CASE("get_biodata throws for empty country code", "[generation]")
{
    auto gen = make_loaded_generator();
    REQUIRE_THROWS_AS(gen.get_biodata(""), std::invalid_argument);
}

TEST_CASE("load replaces previously loaded data", "[loading]")
{
    dasmig::bdg gen;
    bool lite_ok = gen.load(dasmig::dataset::lite);
    if (!lite_ok) { SKIP("lite tier not found"); }
    auto count_lite = gen.country_count();
    REQUIRE(count_lite > 0);

    bool full_ok = gen.load(dasmig::dataset::full);
    if (!full_ok) { SKIP("full tier not found"); }
    auto count_full = gen.country_count();

    // Full replaces lite, not merges.
    REQUIRE(count_full > count_lite);
    REQUIRE(count_full < count_lite + count_lite); // not doubled
}

TEST_CASE("get_biodata(sex) picks varied countries", "[generation][sex]")
{
    auto gen = make_loaded_generator();
    gen.seed(0);

    std::set<std::string> countries;
    for (int i = 0; i < 200; ++i)
    {
        auto b = gen.get_biodata(dasmig::sex::male);
        countries.insert(b.country_code);
    }

    REQUIRE(countries.size() > 10);
    gen.unseed();
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

    REQUIRE(s.find("US") != std::string::npos);
    REQUIRE(s.find("male") != std::string::npos);
    REQUIRE(s.find("cm") != std::string::npos);
    REQUIRE(s.find("kg") != std::string::npos);
    REQUIRE(s.find("BMI=") != std::string::npos);
    REQUIRE(s.find("eyes=") != std::string::npos);
    REQUIRE(s.find("hair=") != std::string::npos);
    REQUIRE(s.find("skin=") != std::string::npos);
    REQUIRE(s.find("blood=") != std::string::npos);
}

// ---------------------------------------------------------------------------
// Known answers (rule 2): the same seed and data give these exact results on
// every compiler and platform. A change here is a change of output and needs
// a major version.
// ---------------------------------------------------------------------------

TEST_CASE("known answers for fixed seeds", "[kat]")
{
    dasmig::bdg gen;
    gen.load(std::filesystem::path{"resources/lite"});
    if (!gen.has_data()) gen.load(std::filesystem::path{"../resources/lite"});
    REQUIRE(gen.has_data());
    REQUIRE(gen.data_hash() == 0x076d859c658fef72ULL);

    REQUIRE(gen.get_biodata("US", std::uint64_t{1}).to_json() ==
            R"({"country":"US","sex":"male","height_cm":173.5172709253195,"weight_kg":91.22083570504105,)"
            R"("bmi":30.297628050803606,"eyes":"blue","hair":"blond","skin":"III","blood":"O+","hand":"right",)"
            R"("signature":{"seed":"0000000000000001","data":"076d859c658fef72"}})");
    REQUIRE(gen.get_biodata("JP", dasmig::sex::female, std::uint64_t{2}).to_json() ==
            R"({"country":"JP","sex":"female","height_cm":158.17896069415235,"weight_kg":44.525357578480246,)"
            R"("bmi":17.79549121600056,"eyes":"brown","hair":"black","skin":"III","blood":"A+","hand":"right",)"
            R"("signature":{"seed":"0000000000000002","data":"076d859c658fef72"}})");
    REQUIRE(gen.get_biodata(std::uint64_t{3}).to_json() ==
            R"({"country":"JM","sex":"male","height_cm":176.8561684380176,"weight_kg":100.39633798702847,)"
            R"("bmi":32.09796123745295,"eyes":"intermediate","hair":"black","skin":"IV","blood":"O+","hand":"right",)"
            R"("signature":{"seed":"0000000000000003","data":"076d859c658fef72"}})");
    REQUIRE(gen.get_biodata(dasmig::sex::male, std::uint64_t{4}).to_json() ==
            R"({"country":"MT","sex":"male","height_cm":164.46793267482911,"weight_kg":63.551146441403866,)"
            R"("bmi":23.49421412356942,"eyes":"brown","hair":"brown","skin":"IV","blood":"O+","hand":"left",)"
            R"("signature":{"seed":"0000000000000004","data":"076d859c658fef72"}})");

    std::uint64_t h = 0xcbf29ce484222325ULL;
    for (std::uint64_t s = 0; s < 10000; ++s)
    {
        for (const char c : gen.get_biodata(s).to_json())
        {
            h ^= static_cast<unsigned char>(c);
            h *= 0x100000001b3ULL;
        }
    }
    REQUIRE(h == 0x7b8667dd6cf9dbdfULL);
}

// ---------------------------------------------------------------------------
// JSON and signatures
// ---------------------------------------------------------------------------

TEST_CASE("to_json and signature", "[json]")
{
    auto gen = make_loaded_generator();
    const auto b = gen.get_biodata("BR", std::uint64_t{99});
    REQUIRE(b.signature().seed == 99);
    REQUIRE(b.seed() == 99);
    REQUIRE(b.signature().data == gen.data_hash());
    const auto json = b.to_json();
    REQUIRE(json.starts_with(R"({"country":"BR","sex":)"));
    REQUIRE(json.find(R"("signature":{"seed":"0000000000000063")") != std::string::npos);

    dasmig::core::json_writer w;
    w.begin_array();
    b.to_json(w);
    w.end_array();
    REQUIRE(w.str() == "[" + json + "]");
}

TEST_CASE("forcing the sex leaves the other draws unchanged", "[generation][sex]")
{
    auto gen = make_loaded_generator();
    int compared = 0;
    for (std::uint64_t s = 0; s < 200; ++s)
    {
        const auto free = gen.get_biodata("US", s);
        const auto forced = gen.get_biodata("US", free.bio_sex, s);
        REQUIRE(forced.to_json() == free.to_json());
        ++compared;
    }
    REQUIRE(compared == 200);
}

// ---------------------------------------------------------------------------
// Extending data (rule 8)
// ---------------------------------------------------------------------------

namespace
{

std::string row_for(const dasmig::bdg& /*unused*/, std::string_view code, double male_height)
{
    // A full row in the biodata.tsv format.
    return std::format("{}\tTest\t{}\t6.5\t160\t6\t25\t25\t4.5\t10\t20\t70\t40\t40\t15\t5\t"
                       "5\t10\t20\t30\t20\t15\t40\t30\t20\t5\t2\t1\t1\t1\t10\n",
                       code, male_height);
}

const std::string header =
    "country_code\tcountry_name\tmale_height_cm\tmale_height_sd\tfemale_height_cm\t"
    "female_height_sd\tmale_bmi_mean\tfemale_bmi_mean\tbmi_sd\teye_blue\teye_intermediate\t"
    "eye_brown\thair_black\thair_brown\thair_blond\thair_red\tskin_I\tskin_II\tskin_III\t"
    "skin_IV\tskin_V\tskin_VI\tblood_O_pos\tblood_A_pos\tblood_B_pos\tblood_AB_pos\t"
    "blood_O_neg\tblood_A_neg\tblood_B_neg\tblood_AB_neg\tleft_handed_pct\n";

} // namespace

TEST_CASE("extend overrides, adds and removes countries", "[extend]")
{
    auto gen = make_loaded_generator();
    const auto base_count = gen.country_count();
    const auto base_hash = gen.data_hash();
    const auto before = gen.get_biodata("JP", dasmig::sex::male, std::uint64_t{7});

    const auto& info = gen.extend_from_string(header + row_for(gen, "JP", 190.0) + row_for(gen, "QQ", 170.0),
                                              "giants.tsv");
    REQUIRE(info.replaced == 1);
    REQUIRE(info.added == 1);
    REQUIRE(gen.country_count() == base_count + 1);
    REQUIRE(gen.data_hash() != base_hash);
    REQUIRE(gen.layers().size() == 2);

    const auto after = gen.get_biodata("JP", dasmig::sex::male, std::uint64_t{7});
    REQUIRE(after.height_cm > before.height_cm + 10.0);
    REQUIRE(after.signature().data == gen.data_hash());
    REQUIRE(gen.get_biodata("QQ").country_code == "QQ");

    gen.extend_from_string("country_code\tdisabled\nQQ\t1\n", "remove.tsv");
    REQUIRE(gen.country_count() == base_count);
    REQUIRE_THROWS_AS(gen.get_biodata("QQ"), std::invalid_argument);

    // Reloading the base drops every layer.
    REQUIRE(gen.load(dasmig::dataset::lite));
    REQUIRE(gen.data_hash() == base_hash);
    REQUIRE(gen.get_biodata("JP", dasmig::sex::male, std::uint64_t{7}).to_json() == before.to_json());
}

TEST_CASE("a bad layer is rejected and leaves the data unchanged", "[extend][errors]")
{
    auto gen = make_loaded_generator();
    const auto hash = gen.data_hash();
    const auto count = gen.country_count();

    std::string bad_number = header + row_for(gen, "JP", 175.0) + row_for(gen, "KR", 175.0);
    bad_number.replace(bad_number.rfind("175"), 3, "tall");
    REQUIRE_THROWS_WITH(gen.extend_from_string(bad_number, "bad.tsv"),
                        "bad.tsv:3: column 'male_height_cm': expected a number, got 'tall'");

    std::string zero_eyes = header + row_for(gen, "JP", 175.0);
    zero_eyes.replace(zero_eyes.find("\t10\t20\t70\t"), 10, "\t0\t0\t0\t");
    REQUIRE_THROWS_AS(gen.extend_from_string(zero_eyes, "zero.tsv"), dasmig::core::tsv_error);

    REQUIRE_THROWS_AS(gen.extend_from_string("country_code\tmale_height_cm\nJP\t180\n", "partial.tsv"),
                      dasmig::core::tsv_error); // required columns missing
    REQUIRE_THROWS_AS(gen.extend_from_string(header + row_for(gen, "JP", 175.0), "clash.tsv",
                                             dasmig::core::layer_mode::add),
                      dasmig::core::tsv_error); // JP already loaded

    REQUIRE(gen.data_hash() == hash);
    REQUIRE(gen.country_count() == count);
    REQUIRE(gen.layers().size() == 1);
}

#ifdef DASMIG_BIODATA_EMBED
TEST_CASE("compiled-in data needs no files and matches them", "[embed]")
{
    const auto old_cwd = std::filesystem::current_path();
    const auto tmp = std::filesystem::temp_directory_path() / "bdg_embed_dir";
    std::filesystem::create_directories(tmp);
    std::filesystem::current_path(tmp);
    dasmig::bdg gen;
    const bool loaded = gen.load(dasmig::dataset::lite);
    std::filesystem::current_path(old_cwd);
    std::filesystem::remove_all(tmp);

    REQUIRE(loaded);
    REQUIRE(gen.layers().front().source == "embedded:lite/biodata.tsv");
    REQUIRE(gen.data_hash() == 0x076d859c658fef72ULL); // same as the file
    REQUIRE(gen.get_biodata("US", std::uint64_t{1}).to_json().find(R"("height_cm":173.5172709253195)") !=
            std::string::npos);
}
#endif

// ---------------------------------------------------------------------------
// Age
// ---------------------------------------------------------------------------

TEST_CASE("options without an age match the plain overloads", "[age]")
{
    auto gen = make_loaded_generator();
    for (std::uint64_t s = 0; s < 100; ++s)
    {
        REQUIRE(gen.get_biodata({.country = "US"}, s).to_json() == gen.get_biodata("US", s).to_json());
        REQUIRE(gen.get_biodata(dasmig::biodata_options{}, s).to_json() == gen.get_biodata(s).to_json());
    }
}

TEST_CASE("options fix country, sex and age", "[age]")
{
    auto gen = make_loaded_generator();
    const auto b = gen.get_biodata({.country = "JP", .bio_sex = dasmig::sex::female, .age = 34}, 7);
    REQUIRE(b.country_code == "JP");
    REQUIRE(b.bio_sex == dasmig::sex::female);
    REQUIRE(b.age == 34);
    REQUIRE(b.to_json().find(R"("age":34,"signature")") != std::string::npos);
    REQUIRE_THROWS_AS(gen.get_biodata({.country = "US", .age = 121}, 1), std::invalid_argument);
}

TEST_CASE("age shapes height, BMI and hair", "[age]")
{
    auto gen = make_loaded_generator();
    double child = 0, adult = 0, elder = 0, child_bmi = 0, adult_bmi = 0;
    int grey_young = 0, grey_old = 0;
    constexpr int n = 400;
    for (std::uint64_t s = 0; s < n; ++s)
    {
        const auto c = gen.get_biodata({.country = "US", .bio_sex = dasmig::sex::male, .age = 6}, s);
        const auto a = gen.get_biodata({.country = "US", .bio_sex = dasmig::sex::male, .age = 30}, s);
        const auto o = gen.get_biodata({.country = "US", .bio_sex = dasmig::sex::male, .age = 85}, s);
        child += c.height_cm; adult += a.height_cm; elder += o.height_cm;
        child_bmi += c.bmi; adult_bmi += a.bmi;
        REQUIRE(c.weight_kg < a.weight_kg);
        if (a.hair == dasmig::hair_color::grey) ++grey_young;
        if (o.hair == dasmig::hair_color::grey) ++grey_old;
    }
    REQUIRE(child / n < 0.75 * adult / n);
    REQUIRE(elder / n < adult / n);
    REQUIRE(child_bmi / n < adult_bmi / n);
    REQUIRE(grey_old > 3 * grey_young);
    REQUIRE(gen.get_biodata({.country = "US", .age = 10}, 1).hair != dasmig::hair_color::grey);
}
