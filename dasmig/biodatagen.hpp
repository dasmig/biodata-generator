#ifndef DASMIG_BIODATAGEN_HPP
#define DASMIG_BIODATAGEN_HPP

#include "random.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <ostream>
#include <random>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

/// @file biodatagen.hpp
/// @brief Biodata generator library — procedural human physical
///        characteristics generation for C++23.
/// @author Diego Dasso Migotto (diegomigotto at hotmail dot com)
/// @see See doc/usage.md for the narrative tutorial.

namespace dasmig
{

/// @brief Dataset size tier for resource loading.
#ifndef DASMIG_DATASET_DEFINED
#define DASMIG_DATASET_DEFINED
enum class dataset : std::uint8_t
{
    lite, ///< ~111 countries with best data coverage.
    full  ///< ~197 countries (gaps filled with regional defaults).
};
#endif

/// @brief Biological sex for biodata generation.
#ifndef DASMIG_SEX_DEFINED
#define DASMIG_SEX_DEFINED
enum class sex : std::uint8_t
{
    male,
    female
};
#endif

/// @brief Eye colour categories.
enum class eye_color : std::uint8_t
{
    blue,
    intermediate, ///< Green, hazel, amber.
    brown
};

/// @brief Hair colour categories.
enum class hair_color : std::uint8_t
{
    black,
    brown,
    blond,
    red
};

/// @brief Fitzpatrick skin-type scale (I–VI).
enum class skin_type : std::uint8_t
{
    I = 1,
    II,
    III,
    IV,
    V,
    VI
};

/// @brief ABO/Rh blood-type groups.
enum class blood_type : std::uint8_t
{
    O_pos,
    A_pos,
    B_pos,
    AB_pos,
    O_neg,
    A_neg,
    B_neg,
    AB_neg
};

/// @brief Handedness.
enum class handedness : std::uint8_t
{
    right,
    left
};

/// @brief Return type for biodata generation, holding all physical
///        characteristics.
///
/// Supports implicit conversion to std::string (summary line) and
/// streaming via operator<<.
class biodata
{
  public:
    std::string country_code;     ///< ISO 3166-1 alpha-2 code.
    sex bio_sex{sex::male};       ///< Biological sex.
    double height_cm{0.0};        ///< Height in centimetres.
    double weight_kg{0.0};        ///< Weight in kilograms.
    double bmi{0.0};              ///< Body mass index (kg/m²).
    eye_color eyes{eye_color::brown};       ///< Eye colour.
    hair_color hair{hair_color::black};     ///< Hair colour.
    skin_type skin{skin_type::III};         ///< Fitzpatrick skin type.
    blood_type blood{blood_type::O_pos};    ///< Blood type (ABO/Rh).
    handedness hand{handedness::right};     ///< Handedness.

    /// @brief Retrieve the random seed used to generate this biodata.
    [[nodiscard]] std::uint64_t seed() const { return _seed; }

    /// @brief Human-readable summary string.
    [[nodiscard]] std::string to_string() const
    {
        return std::format("{} {} {:.1f}cm {:.1f}kg BMI={:.1f} "
                           "eyes={} hair={} skin={} blood={} {}",
                           country_code, sex_str(bio_sex),
                           height_cm, weight_kg, bmi,
                           eye_color_str(eyes),
                           hair_color_str(hair),
                           skin_type_str(skin),
                           blood_type_str(blood),
                           handedness_str(hand));
    }

    /// @brief Implicit conversion to std::string.
    /// @return Summary string.
    operator std::string() const // NOLINT(hicpp-explicit-conversions)
    {
        return to_string();
    }

    /// @brief Stream the summary to an output stream.
    friend std::ostream& operator<<(std::ostream& os, const biodata& b)
    {
        os << b.to_string();
        return os;
    }

    // -- Enum to string helpers -------------------------------------------

    /// @brief Eye colour label.
    [[nodiscard]] static std::string_view eye_color_str(eye_color c)
    {
        switch (c)
        {
        case eye_color::blue:         return "blue";
        case eye_color::intermediate: return "intermediate";
        case eye_color::brown:        return "brown";
        }
        return "unknown";
    }

    /// @brief Hair colour label.
    [[nodiscard]] static std::string_view hair_color_str(hair_color c)
    {
        switch (c)
        {
        case hair_color::black: return "black";
        case hair_color::brown: return "brown";
        case hair_color::blond: return "blond";
        case hair_color::red:   return "red";
        }
        return "unknown";
    }

    /// @brief Skin type label.
    [[nodiscard]] static std::string_view skin_type_str(skin_type t)
    {
        switch (t)
        {
        case skin_type::I:   return "I";
        case skin_type::II:  return "II";
        case skin_type::III: return "III";
        case skin_type::IV:  return "IV";
        case skin_type::V:   return "V";
        case skin_type::VI:  return "VI";
        }
        return "unknown";
    }

    /// @brief Blood type label.
    [[nodiscard]] static std::string_view blood_type_str(blood_type t)
    {
        switch (t)
        {
        case blood_type::O_pos:  return "O+";
        case blood_type::A_pos:  return "A+";
        case blood_type::B_pos:  return "B+";
        case blood_type::AB_pos: return "AB+";
        case blood_type::O_neg:  return "O-";
        case blood_type::A_neg:  return "A-";
        case blood_type::B_neg:  return "B-";
        case blood_type::AB_neg: return "AB-";
        }
        return "unknown";
    }

    /// @brief Handedness label.
    [[nodiscard]] static std::string_view handedness_str(handedness h)
    {
        switch (h)
        {
        case handedness::right: return "right";
        case handedness::left:  return "left";
        }
        return "unknown";
    }

    /// @brief Biological sex label.
    [[nodiscard]] static std::string_view sex_str(sex s)
    {
        switch (s)
        {
        case sex::male:   return "male";
        case sex::female: return "female";
        }
        return "unknown";
    }

  private:
    std::uint64_t _seed{0};
    friend class bdg;
};

/// @brief Biodata generator that produces demographically plausible
///        human physical characteristics using country-level
///        population statistics.
///
/// The generation pipeline:
/// 1. Select biological sex (50/50 unless forced).
/// 2. Sample height from a Gaussian (country/sex mean and SD).
/// 3. Sample BMI from a log-normal distribution.
/// 4. Derive weight from height and BMI.
/// 5. Sample eye colour, hair colour, skin type from categorical
///    distributions.
/// 6. Sample blood type from ABO/Rh distribution.
/// 7. Sample handedness from country-level left-handedness rate.
///
/// Can be used as a singleton via instance() or constructed
/// independently.
///
/// @par Thread safety
/// Each instance is independent.  Concurrent calls to get_biodata()
/// on the **same** instance require external synchronisation.
class bdg
{
  public:
    /// @brief Default constructor — creates an empty generator with no
    ///        data.
    bdg() = default;

    bdg(const bdg&) = delete;
    bdg& operator=(const bdg&) = delete;
    bdg(bdg&&) noexcept = default;
    bdg& operator=(bdg&&) noexcept = default;
    ~bdg() = default;

    /// @brief Access the global singleton instance.
    ///
    /// Auto-probes common resource paths on first access.
    static bdg& instance()
    {
        static bdg inst{auto_probe_tag{}};
        return inst;
    }

    // -- Generation -------------------------------------------------------

    /// @brief Generate random biodata for a specific country.
    /// @param cca2 ISO 3166-1 alpha-2 country code (e.g. "US", "BR").
    /// @throws std::runtime_error If no data has been loaded.
    /// @throws std::invalid_argument If the country code is unknown.
    [[nodiscard]] biodata get_biodata(std::string_view cca2)
    {
        return generate_(lookup_entry_(cca2), draw_seed_(), {});
    }

    /// @brief Generate deterministic biodata for a specific country.
    [[nodiscard]] biodata get_biodata(std::string_view cca2,
                                      std::uint64_t call_seed) const
    {
        return generate_(lookup_entry_(cca2), call_seed, {});
    }

    /// @brief Generate random biodata from a random country.
    /// @throws std::runtime_error If no data has been loaded.
    [[nodiscard]] biodata get_biodata()
    {
        return get_biodata(draw_seed_());
    }

    /// @brief Generate deterministic biodata from a random country.
    [[nodiscard]] biodata get_biodata(std::uint64_t call_seed) const
    {
        return get_biodata(pick_random_cca2_(call_seed), call_seed);
    }

    // -- Sex-specific generation ------------------------------------------

    /// @brief Generate random biodata with a predetermined sex.
    [[nodiscard]] biodata get_biodata(std::string_view cca2, sex bio_sex)
    {
        return generate_(lookup_entry_(cca2), draw_seed_(),
                         {.fix_sex = true, .forced_sex = bio_sex});
    }

    /// @brief Generate deterministic biodata with a predetermined sex.
    [[nodiscard]] biodata get_biodata(std::string_view cca2, sex bio_sex,
                                      std::uint64_t call_seed) const
    {
        return generate_(lookup_entry_(cca2), call_seed,
                         {.fix_sex = true, .forced_sex = bio_sex});
    }

    /// @brief Generate random biodata from a random country with a
    ///        predetermined sex.
    [[nodiscard]] biodata get_biodata(sex bio_sex)
    {
        auto seed = draw_seed_();
        return generate_(lookup_entry_(pick_random_cca2_(seed)), seed,
                         {.fix_sex = true, .forced_sex = bio_sex});
    }

    /// @brief Generate deterministic biodata from a random country with
    ///        a predetermined sex.
    [[nodiscard]] biodata get_biodata(sex bio_sex,
                                      std::uint64_t call_seed) const
    {
        return get_biodata(pick_random_cca2_(call_seed), bio_sex,
                           call_seed);
    }

    // -- Seeding ----------------------------------------------------------

    /// @brief Seed the internal random engine for deterministic
    ///        sequences.
    bdg& seed(std::uint64_t seed_value)
    {
        _engine.seed(seed_value);
        return *this;
    }

    /// @brief Reseed the engine with a non-deterministic source.
    bdg& unseed()
    {
        _engine.seed(std::random_device{}());
        return *this;
    }

    // -- Data management --------------------------------------------------

    /// @brief Check whether any data has been loaded.
    [[nodiscard]] bool has_data() const { return !_entries.empty(); }

    /// @brief Return the number of loaded countries.
    [[nodiscard]] std::size_t country_count() const
    {
        return _entries.size();
    }

    /// @brief Load biodata from a resource directory.
    ///
    /// Expects the directory to contain: biodata.tsv.
    ///
    /// Replaces any previously loaded data.
    void load(const std::filesystem::path& dir)
    {
        if (!std::filesystem::is_directory(dir))
        {
            return;
        }

        _entries.clear();
        load_biodata_(dir / "biodata.tsv");
        rebuild_indices_();
    }

    /// @brief Load a specific dataset tier from auto-probed paths.
    [[nodiscard]] bool load(dataset tier)
    {
        std::string_view sub = (tier == dataset::full) ? "full" : "lite";
        auto found = std::ranges::find_if(
            probe_bases_, [&](std::string_view base) {
                auto d = std::filesystem::path{base} / sub;
                return std::filesystem::is_regular_file(
                    d / "biodata.tsv");
            });
        if (found != probe_bases_.end())
        {
            load(std::filesystem::path{*found} / sub);
            return true;
        }
        return false;
    }

  private:
    // -- Internal data structures -----------------------------------------

    struct entry
    {
        std::string cca2;
        std::string name;

        // Height Gaussian parameters.
        double male_height_mean{170.0};
        double male_height_sd{7.0};
        double female_height_mean{157.0};
        double female_height_sd{6.5};

        // BMI parameters.
        double male_bmi_mean{24.5};
        double female_bmi_mean{25.0};
        double bmi_sd{4.5};

        // Eye colour distribution (blue, intermediate, brown).
        std::array<double, 3> eye_weights{0.0, 5.0, 95.0};

        // Hair colour distribution (black, brown, blond, red).
        std::array<double, 4> hair_weights{90.0, 8.0, 1.0, 1.0};

        // Skin type distribution (I..VI).
        std::array<double, 6> skin_weights{2.0, 8.0, 18.0, 30.0,
                                           28.0, 14.0};

        // Blood type distribution (O+ A+ B+ AB+ O- A- B- AB-).
        std::array<double, 8> blood_weights{37.0, 28.0, 20.0, 5.0,
                                            4.0, 3.0, 2.0, 1.0};

        // Handedness.
        double left_handed_pct{10.6};
    };

    std::unordered_map<std::string, entry> _entries;

    // Ordered cca2 list for random-country selection.
    std::vector<std::string> _cca2_order;
    mutable std::uniform_int_distribution<std::size_t> _country_uniform;

    static constexpr unsigned seed_shift_{32U};

    static constexpr std::array<std::string_view, 3> probe_bases_{
        "resources", "../resources", "biodata-generator/resources"};

    std::mt19937_64 _engine{std::random_device{}()};

    struct auto_probe_tag {};

    explicit bdg(auto_probe_tag /*tag*/)
    {
        auto found = std::ranges::find_if(
            probe_bases_, [](std::string_view p) {
                return std::filesystem::exists(p) &&
                       std::filesystem::is_directory(p);
            });
        if (found != probe_bases_.end())
        {
            const std::filesystem::path base{*found};
            auto lite = base / "lite";
            auto full = base / "full";
            if (std::filesystem::is_regular_file(
                    lite / "biodata.tsv"))
            {
                load(lite);
            }
            else if (std::filesystem::is_regular_file(
                         full / "biodata.tsv"))
            {
                load(full);
            }
        }
    }

    // -- Helpers ----------------------------------------------------------

    std::uint64_t draw_seed_()
    {
        return static_cast<std::uint64_t>(_engine());
    }

    [[nodiscard]] const entry& lookup_entry_(
        std::string_view cca2) const
    {
        if (_entries.empty())
        {
            throw std::runtime_error(
                "No biodata loaded. Call load() first.");
        }
        auto it = _entries.find(std::string{cca2});
        if (it == _entries.end())
        {
            throw std::invalid_argument(
                std::format("Unknown country code: {}", cca2));
        }
        return it->second;
    }

    /// Pick a random country code deterministically from the seed.
    [[nodiscard]] const std::string& pick_random_cca2_(
        std::uint64_t call_seed) const
    {
        if (_entries.empty())
        {
            throw std::runtime_error(
                "No biodata loaded. Call load() first.");
        }
        effolkronium::random_local rng;
        rng.seed(static_cast<std::mt19937::result_type>(
            call_seed ^ (call_seed >> seed_shift_)));
        return _cca2_order[_country_uniform(rng.engine())]; // NOLINT
    }

    // Optional constraints passed to generate_().
    struct gen_opts_
    {
        bool fix_sex{false};
        sex forced_sex{sex::male};
    };

    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    [[nodiscard]] biodata generate_(const entry& e,
                                    std::uint64_t call_seed,
                                    const gen_opts_& opts) const
    {
        effolkronium::random_local rng;
        rng.seed(static_cast<std::mt19937::result_type>(
            call_seed ^ (call_seed >> seed_shift_)));

        biodata b;
        b._seed = call_seed;
        b.country_code = e.cca2;

        // 1. Sex — fixed or 50/50.
        //    Note: when fix_sex is true the Bernoulli draw is skipped,
        //    so the RNG sequence diverges from an unfixed call with the
        //    same seed that happened to land on the same sex.
        if (opts.fix_sex)
        {
            b.bio_sex = opts.forced_sex;
        }
        else
        {
            std::bernoulli_distribution sex_dist(0.5);
            b.bio_sex = sex_dist(rng.engine()) ? sex::female
                                               : sex::male;
        }

        // 2. Height — Gaussian from country/sex mean and SD.
        const double h_mean = (b.bio_sex == sex::male)
                                  ? e.male_height_mean
                                  : e.female_height_mean;
        const double h_sd = (b.bio_sex == sex::male)
                                ? e.male_height_sd
                                : e.female_height_sd;
        std::normal_distribution<double> height_dist(h_mean, h_sd);
        b.height_cm = std::clamp(height_dist(rng.engine()),
                                 h_mean - 4.0 * h_sd,
                                 h_mean + 4.0 * h_sd);

        // 3. BMI — log-normal to model right-skewed distribution.
        const double bmi_mean = (b.bio_sex == sex::male)
                                    ? e.male_bmi_mean
                                    : e.female_bmi_mean;
        const double bmi_sd = e.bmi_sd;
        // Convert mean/SD to log-normal parameters.
        const double bmi_var = bmi_sd * bmi_sd;
        const double ln_sigma2 =
            std::log1p(bmi_var / (bmi_mean * bmi_mean));
        const double ln_mu =
            std::log(bmi_mean) - 0.5 * ln_sigma2;
        const double ln_sigma = std::sqrt(ln_sigma2);
        std::lognormal_distribution<double> bmi_dist(ln_mu, ln_sigma);
        b.bmi = std::clamp(bmi_dist(rng.engine()), 14.0, 55.0);

        // 4. Weight = BMI × (height_m)².
        const double height_m = b.height_cm / 100.0;
        b.weight_kg = b.bmi * height_m * height_m;

        // 5. Eye colour — categorical.
        std::discrete_distribution<unsigned> eye_dist(
            e.eye_weights.begin(), e.eye_weights.end());
        b.eyes = static_cast<eye_color>(eye_dist(rng.engine()));

        // 6. Hair colour — categorical.
        std::discrete_distribution<unsigned> hair_dist(
            e.hair_weights.begin(), e.hair_weights.end());
        b.hair = static_cast<hair_color>(hair_dist(rng.engine()));

        // 7. Skin type — categorical (Fitzpatrick I–VI).
        std::discrete_distribution<unsigned> skin_dist(
            e.skin_weights.begin(), e.skin_weights.end());
        b.skin = static_cast<skin_type>(
            skin_dist(rng.engine()) + 1); // enum starts at 1

        // 8. Blood type — categorical (ABO/Rh).
        std::discrete_distribution<unsigned> blood_dist(
            e.blood_weights.begin(), e.blood_weights.end());
        b.blood = static_cast<blood_type>(blood_dist(rng.engine()));

        // 9. Handedness — Bernoulli.
        std::bernoulli_distribution hand_dist(
            e.left_handed_pct / 100.0);
        b.hand = hand_dist(rng.engine()) ? handedness::left
                                         : handedness::right;

        return b;
    }

    // -- Loading ----------------------------------------------------------

    // Locale-independent double parser via std::from_chars.
    static double parse_double_(std::string_view str,
                                double fallback = 0.0)
    {
        if (str.empty()) { return fallback; }
        double val{};
        auto [ptr, ec] =
            std::from_chars(str.data(),
                            str.data() + str.size(), val); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return ec == std::errc{} ? val : fallback;
    }

    static std::vector<std::string> split_tab_(std::string_view line)
    {
        std::vector<std::string> fields;
        fields.reserve(32);
        for (auto part : line | std::views::split('\t'))
        {
            fields.emplace_back(std::ranges::begin(part),
                                std::ranges::end(part));
        }
        return fields;
    }

    void load_biodata_(const std::filesystem::path& path)
    {
        if (!std::filesystem::is_regular_file(path)) return;

        std::ifstream file{path};
        if (!file.is_open()) return;

        std::string line;
        if (!std::getline(file, line)) return; // skip header

        // Header (31 columns):
        // country_code country_name
        // male_height_cm male_height_sd female_height_cm female_height_sd
        // male_bmi_mean female_bmi_mean bmi_sd
        // eye_blue eye_intermediate eye_brown
        // hair_black hair_brown hair_blond hair_red
        // skin_I skin_II skin_III skin_IV skin_V skin_VI
        // blood_O_pos blood_A_pos blood_B_pos blood_AB_pos
        // blood_O_neg blood_A_neg blood_B_neg blood_AB_neg
        // left_handed_pct
        static constexpr std::size_t min_fields{31};

        while (std::getline(file, line))
        {
            if (line.empty()) continue;
            if (line.back() == '\r') line.pop_back();

            auto f = split_tab_(line);
            if (f.size() < min_fields) continue;

            entry e;
            e.cca2 = std::move(f[0]);
            e.name = std::move(f[1]);

            // Height.
            e.male_height_mean   = parse_double_(f[2], 170.0);
            e.male_height_sd     = parse_double_(f[3], 7.0);
            e.female_height_mean = parse_double_(f[4], 157.0);
            e.female_height_sd   = parse_double_(f[5], 6.5);

            // BMI.
            e.male_bmi_mean   = parse_double_(f[6], 24.5);
            e.female_bmi_mean = parse_double_(f[7], 25.0);
            e.bmi_sd          = parse_double_(f[8], 4.5);

            // Eye colour.
            e.eye_weights = {
                parse_double_(f[9]),   // blue
                parse_double_(f[10]),  // intermediate
                parse_double_(f[11])   // brown
            };

            // Hair colour.
            e.hair_weights = {
                parse_double_(f[12]),  // black
                parse_double_(f[13]),  // brown
                parse_double_(f[14]),  // blond
                parse_double_(f[15])   // red
            };

            // Skin type (I–VI).
            e.skin_weights = {
                parse_double_(f[16]),  // I
                parse_double_(f[17]),  // II
                parse_double_(f[18]),  // III
                parse_double_(f[19]),  // IV
                parse_double_(f[20]),  // V
                parse_double_(f[21])   // VI
            };

            // Blood type.
            e.blood_weights = {
                parse_double_(f[22]),  // O+
                parse_double_(f[23]),  // A+
                parse_double_(f[24]),  // B+
                parse_double_(f[25]),  // AB+
                parse_double_(f[26]),  // O-
                parse_double_(f[27]),  // A-
                parse_double_(f[28]),  // B-
                parse_double_(f[29])   // AB-
            };

            // Handedness.
            e.left_handed_pct = parse_double_(f[30], 10.6);

            std::string key{e.cca2};
            _entries.insert_or_assign(std::move(key), std::move(e));
        }
    }

    void rebuild_indices_()
    {
        _cca2_order.clear();
        _cca2_order.reserve(_entries.size());

        for (const auto& key : _entries | std::views::keys)
        {
            _cca2_order.push_back(key);
        }
        std::ranges::sort(_cca2_order);

        if (!_cca2_order.empty())
        {
            _country_uniform =
                std::uniform_int_distribution<std::size_t>(
                    0, _cca2_order.size() - 1);
        }
    }
};

} // namespace dasmig

#endif // DASMIG_BIODATAGEN_HPP
