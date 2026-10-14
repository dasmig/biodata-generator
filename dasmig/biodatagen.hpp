#ifndef DASMIG_BIODATAGEN_HPP
#define DASMIG_BIODATAGEN_HPP

#include "core.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <map>
#include <optional>
#include <ostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#ifdef DASMIG_BIODATA_EMBED
#include "biodatagen_data.hpp"
#endif

/// @file biodatagen.hpp
/// @brief Biodata generator library — procedural human physical
///        characteristics generation for C++23.
/// @author Diego Dasso Migotto (diegomigotto at hotmail dot com)
/// @see See doc/usage.md for the narrative tutorial.

DASMIG_CORE_REQUIRE(1, 0);

namespace dasmig
{

#ifndef DASMIG_SEX_DEFINED
#define DASMIG_SEX_DEFINED
/// @brief Biological sex.
enum class sex : std::uint8_t
{
    male,  ///< Male.
    female ///< Female.
};
#endif

/// @brief Eye colour (three-category model from Katsara & Nothnagel 2019).
enum class eye_color : std::uint8_t
{
    blue,         ///< Blue.
    intermediate, ///< Green, hazel, amber.
    brown         ///< Brown.
};

/// @brief Natural hair colour (four-category model).
enum class hair_color : std::uint8_t
{
    black, ///< Black.
    brown, ///< Brown.
    blond, ///< Blond.
    red,   ///< Red.
    grey   ///< Grey or white; only generated when an age is given.
};

/// @brief Fitzpatrick skin phototype (I–VI).
enum class skin_type : std::uint8_t
{
    I = 1, ///< Type I.
    II,    ///< Type II.
    III,   ///< Type III.
    IV,    ///< Type IV.
    V,     ///< Type V.
    VI     ///< Type VI.
};

/// @brief ABO/Rh blood type.
enum class blood_type : std::uint8_t
{
    O_pos,  ///< O+.
    A_pos,  ///< A+.
    B_pos,  ///< B+.
    AB_pos, ///< AB+.
    O_neg,  ///< O-.
    A_neg,  ///< A-.
    B_neg,  ///< B-.
    AB_neg  ///< AB-.
};

/// @brief Handedness.
enum class handedness : std::uint8_t
{
    right, ///< Right-handed.
    left   ///< Left-handed.
};

/// @brief Return type for biodata generation, holding all physical
///        characteristics.
///
/// Supports implicit conversion to std::string (summary line),
/// streaming via operator<< and JSON output via to_json().
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
    std::optional<std::uint8_t> age;        ///< Age in years, when one was requested.

    /// @brief Retrieve the random seed used to generate this biodata.
    /// @return The call seed; pass it to a seeded get_biodata() overload
    ///         to generate the same biodata again.
    [[nodiscard]] std::uint64_t seed() const { return _signature.seed; }

    /// @brief The seed and the data this biodata was generated from.
    /// @return The replay signature; `data` is bdg::data_hash() at the
    ///         time of generation.
    [[nodiscard]] const core::seed_signature& signature() const
    {
        return _signature;
    }

    /// @brief Human-readable summary string.
    /// @return Summary line.
    [[nodiscard]] std::string to_string() const
    {
        auto s = std::format("{} {} {:.1f}cm {:.1f}kg BMI={:.1f} "
                             "eyes={} hair={} skin={} blood={} {}",
                             country_code, sex_str(bio_sex),
                             height_cm, weight_kg, bmi,
                             eye_color_str(eyes),
                             hair_color_str(hair),
                             skin_type_str(skin),
                             blood_type_str(blood),
                             handedness_str(hand));
        if (age) s += std::format(" age={}", static_cast<int>(*age));
        return s;
    }

    /// @brief Write this biodata as a JSON object.
    /// @param w Writer positioned where a value is allowed.
    void to_json(core::json_writer& w) const
    {
        w.begin_object()
            .field("country", country_code)
            .field("sex", sex_str(bio_sex))
            .field("height_cm", height_cm)
            .field("weight_kg", weight_kg)
            .field("bmi", bmi)
            .field("eyes", eye_color_str(eyes))
            .field("hair", hair_color_str(hair))
            .field("skin", skin_type_str(skin))
            .field("blood", blood_type_str(blood))
            .field("hand", handedness_str(hand));
        if (age) w.field("age", static_cast<int>(*age));
        w.key("signature");
        _signature.to_json(w);
        w.end_object();
    }

    /// @brief This biodata as a JSON document.
    /// @return Compact JSON, for example
    ///   `{"country":"US","sex":"male","height_cm":176.2,...,"signature":{...}}`.
    [[nodiscard]] std::string to_json() const
    {
        core::json_writer w;
        to_json(w);
        return w.str();
    }

    /// @brief Implicit conversion to std::string.
    /// @return Summary string.
    operator std::string() const // NOLINT(hicpp-explicit-conversions)
    {
        return to_string();
    }

    /// @brief Stream the summary to an output stream.
    /// @param os Output stream.
    /// @param b Biodata to stream.
    /// @return The stream.
    friend std::ostream& operator<<(std::ostream& os, const biodata& b)
    {
        os << b.to_string();
        return os;
    }

    // -- Enum to string helpers -------------------------------------------

    /// @brief Eye colour label.
    /// @param c Colour.
    /// @return Label.
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
    /// @param c Colour.
    /// @return Label.
    [[nodiscard]] static std::string_view hair_color_str(hair_color c)
    {
        switch (c)
        {
        case hair_color::black: return "black";
        case hair_color::brown: return "brown";
        case hair_color::blond: return "blond";
        case hair_color::red:   return "red";
        case hair_color::grey:  return "grey";
        }
        return "unknown";
    }

    /// @brief Skin type label.
    /// @param t Type.
    /// @return Label.
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
    /// @param t Type.
    /// @return Label.
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
    /// @param h Handedness.
    /// @return Label.
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
    /// @param s Sex.
    /// @return Label.
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
    core::seed_signature _signature;
    friend class bdg;
};

/// @brief What to fix in a generation; anything left unset is drawn.
///
/// ```cpp
/// auto b = bdg::instance().get_biodata({.country = "BR", .age = 67});
/// ```
struct biodata_options
{
    std::string_view country{};          ///< ISO 3166-1 alpha-2 code; empty picks a random country.
    std::optional<sex> bio_sex{};        ///< Biological sex; unset draws 50/50.
    std::optional<unsigned> age{};        ///< Age in years, 0 to 120; unset applies no age effects.
};

/// @brief Biodata generator that produces demographically plausible
///        human physical characteristics using country-level
///        population statistics.
///
/// The generation pipeline, each step drawing from its own stream split
/// from the call seed (so forcing the sex leaves the other steps' draws
/// unchanged):
/// 1. Select biological sex (50/50 unless forced).
/// 2. Sample height from a Gaussian (country/sex mean and SD), clamped
///    to 4 SD.
/// 3. Sample BMI from a log-normal distribution with the country/sex mean
///    and SD, clamped to [14, 55].
/// 4. Derive weight from height and BMI.
/// 5. Sample eye colour, hair colour, skin type from categorical
///    distributions.
/// 6. Sample blood type from ABO/Rh distribution.
/// 7. Sample handedness from country-level left-handedness rate.
///
/// Output is identical on every compiler and platform (dasmig-core).
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

    bdg(const bdg&) = delete;            ///< Not copyable.
    bdg& operator=(const bdg&) = delete; ///< Not copyable.
    bdg(bdg&&) noexcept = default;       ///< Movable.
    bdg& operator=(bdg&&) noexcept = default; ///< Movable.
    ~bdg() = default;                    ///< Destructor.

    /// @brief Access the global singleton instance.
    ///
    /// Loads the `lite` tier (or `full` if `lite` is missing) on first
    /// access, from compiled-in data or an auto-probed `resources/`.
    /// @return The singleton.
    static bdg& instance()
    {
        static bdg inst{auto_probe_tag{}};
        return inst;
    }

    // -- Generation -------------------------------------------------------

    /// @brief Generate random biodata for a specific country.
    /// @param cca2 ISO 3166-1 alpha-2 country code (e.g. "US", "BR").
    /// @return The biodata.
    /// @throws std::runtime_error If no data has been loaded.
    /// @throws std::invalid_argument If the country code is unknown.
    [[nodiscard]] biodata get_biodata(std::string_view cca2)
    {
        return generate_(lookup_entry_(cca2), draw_seed_(), std::nullopt);
    }

    /// @brief Generate deterministic biodata for a specific country.
    /// @param cca2 ISO 3166-1 alpha-2 country code.
    /// @param call_seed Seed; the same seed and data give the same biodata.
    /// @return The biodata.
    /// @throws std::runtime_error If no data has been loaded.
    /// @throws std::invalid_argument If the country code is unknown.
    [[nodiscard]] biodata get_biodata(std::string_view cca2,
                                      std::uint64_t call_seed) const
    {
        return generate_(lookup_entry_(cca2), call_seed, std::nullopt);
    }

    /// @brief Generate random biodata from a random country.
    /// @return The biodata.
    /// @throws std::runtime_error If no data has been loaded.
    [[nodiscard]] biodata get_biodata()
    {
        return get_biodata(draw_seed_());
    }

    /// @brief Generate deterministic biodata from a random country.
    /// @param call_seed Seed.
    /// @return The biodata.
    /// @throws std::runtime_error If no data has been loaded.
    [[nodiscard]] biodata get_biodata(std::uint64_t call_seed) const
    {
        return generate_(random_entry_(call_seed), call_seed, std::nullopt);
    }

    // -- Sex-specific generation ------------------------------------------

    /// @brief Generate random biodata with a predetermined sex.
    /// @param cca2 ISO 3166-1 alpha-2 country code.
    /// @param bio_sex The sex.
    /// @return The biodata.
    [[nodiscard]] biodata get_biodata(std::string_view cca2, sex bio_sex)
    {
        return generate_(lookup_entry_(cca2), draw_seed_(), bio_sex);
    }

    /// @brief Generate deterministic biodata with a predetermined sex.
    /// @param cca2 ISO 3166-1 alpha-2 country code.
    /// @param bio_sex The sex.
    /// @param call_seed Seed.
    /// @return The biodata.
    [[nodiscard]] biodata get_biodata(std::string_view cca2, sex bio_sex,
                                      std::uint64_t call_seed) const
    {
        return generate_(lookup_entry_(cca2), call_seed, bio_sex);
    }

    /// @brief Generate random biodata from a random country with a
    ///        predetermined sex.
    /// @param bio_sex The sex.
    /// @return The biodata.
    [[nodiscard]] biodata get_biodata(sex bio_sex)
    {
        return get_biodata(bio_sex, draw_seed_());
    }

    /// @brief Generate deterministic biodata from a random country with
    ///        a predetermined sex.
    /// @param bio_sex The sex.
    /// @param call_seed Seed.
    /// @return The biodata.
    [[nodiscard]] biodata get_biodata(sex bio_sex,
                                      std::uint64_t call_seed) const
    {
        return generate_(random_entry_(call_seed), call_seed, bio_sex);
    }

    // -- Generation with options ------------------------------------------

    /// @brief Generate random biodata with the given fixed values.
    ///
    /// Giving an age leaves the base draws unchanged and then adjusts them:
    /// children are shorter and lighter (growth curve and child BMI),
    /// adults over 50 lose height, BMI follows a life-course curve, and hair
    /// turns grey with a probability that rises from the mid-30s. Without an
    /// age the result equals the other overloads' for the same seed.
    ///
    /// @param options Country, sex and age to fix.
    /// @return The biodata.
    /// @throws std::runtime_error If no data has been loaded.
    /// @throws std::invalid_argument If the country code is unknown or the
    ///         age is above 120.
    [[nodiscard]] biodata get_biodata(const biodata_options& options)
    {
        return get_biodata(options, draw_seed_());
    }

    /// @brief Generate deterministic biodata with the given fixed values.
    /// @param options Country, sex and age to fix.
    /// @param call_seed Seed.
    /// @return The biodata.
    /// @throws std::runtime_error If no data has been loaded.
    /// @throws std::invalid_argument If the country code is unknown or the
    ///         age is above 120.
    [[nodiscard]] biodata get_biodata(const biodata_options& options,
                                      std::uint64_t call_seed) const
    {
        const auto& e = options.country.empty() ? random_entry_(call_seed)
                                                : lookup_entry_(options.country);
        if (options.age && *options.age > max_age_)
        {
            throw std::invalid_argument(std::format("Age out of range (0-{}): {}", max_age_,
                                                    *options.age));
        }
        std::optional<std::uint8_t> age;
        if (options.age) age = static_cast<std::uint8_t>(*options.age);
        return generate_(e, call_seed, options.bio_sex, age);
    }

    // -- Seeding ----------------------------------------------------------

    /// @brief Seed the internal random engine for deterministic
    ///        sequences.
    /// @param seed_value Seed.
    /// @return `*this`.
    bdg& seed(std::uint64_t seed_value)
    {
        _engine.seed(seed_value);
        return *this;
    }

    /// @brief Reseed the engine with a non-deterministic source.
    /// @return `*this`.
    bdg& unseed()
    {
        _engine.unseed();
        return *this;
    }

    // -- Data management --------------------------------------------------

    /// @brief Check whether any data has been loaded.
    /// @return True when at least one country is loaded.
    [[nodiscard]] bool has_data() const { return !_entries.empty(); }

    /// @brief Return the number of loaded countries.
    /// @return The count.
    [[nodiscard]] std::size_t country_count() const
    {
        return _entries.size();
    }

    /// @brief Load biodata from a resource directory, replacing all data
    ///        and layers loaded before.
    ///
    /// Expects the directory to contain `biodata.tsv`. A path that is not
    /// a directory, or a directory without the file, is ignored.
    ///
    /// @param dir Directory such as `resources/lite`.
    /// @throws core::tsv_error If the file is malformed.
    void load(const std::filesystem::path& dir)
    {
        std::error_code ec;
        const auto file = dir / data_file_;
        if (!std::filesystem::is_directory(dir, ec) ||
            !std::filesystem::is_regular_file(file, ec))
        {
            return;
        }
        (void)extend(core::tsv_document::load(file), core::layer_mode::replace);
    }

    /// @brief Load a dataset tier, replacing all data and layers loaded
    ///        before.
    ///
    /// Uses the compiled-in copy when the library is built with
    /// `DASMIG_BIODATA_EMBED`, otherwise searches `resources/`,
    /// `../resources`, `biodata-generator/resources` and
    /// `../biodata-generator/resources`.
    ///
    /// @param tier The tier.
    /// @return False when the tier was not found.
    /// @throws core::tsv_error If the file is malformed.
    [[nodiscard]] bool load(dataset tier)
    {
        const std::string path = std::string{core::tier_name(tier)} + "/" + std::string{data_file_};
        auto doc = core::load_resource(path, embedded_files_(),
                                       core::find_resources(library_name_, path));
        if (!doc) return false;
        (void)extend(std::move(*doc), core::layer_mode::replace);
        return true;
    }

    /// @brief Add a data layer from a TSV file with the `biodata.tsv`
    ///        columns.
    ///
    /// Rows are keyed by `country_code`. In the default override mode a
    /// row replaces the loaded row for its country and new countries are
    /// added; a row with `disabled` set to 1 removes its country.
    ///
    /// @param file The layer file.
    /// @param mode How the layer combines with the data loaded before.
    /// @return What the layer did (rows added, replaced, removed, warnings).
    /// @throws core::tsv_error If the file cannot be read or a row is
    ///         invalid; the data is then unchanged.
    const core::layer_info& extend(const std::filesystem::path& file,
                                   core::layer_mode mode = core::layer_mode::override)
    {
        return extend(core::tsv_document::load(file), mode);
    }

    /// @brief Add a data layer from TSV text (for example from a
    ///        WebAssembly host).
    /// @param text TSV with the `biodata.tsv` columns.
    /// @param source Name used in errors.
    /// @param mode How the layer combines with the data loaded before.
    /// @return What the layer did.
    /// @throws core::tsv_error If a row is invalid; the data is unchanged.
    const core::layer_info& extend_from_string(std::string_view text, std::string source,
                                               core::layer_mode mode = core::layer_mode::override)
    {
        return extend(core::tsv_document::parse(text, std::move(source)), mode);
    }

    /// @brief Add a data layer.
    /// @param doc TSV with the `biodata.tsv` columns.
    /// @param mode How the layer combines with the data loaded before.
    /// @return What the layer did.
    /// @throws core::tsv_error If a row is invalid; the data is unchanged.
    const core::layer_info& extend(core::tsv_document doc,
                                   core::layer_mode mode = core::layer_mode::override)
    {
        const auto& info = _table.extend(std::move(doc), mode, [](const core::tsv_row& row) {
            (void)entry::parse(row);
        });
        rebuild_();
        return info;
    }

    /// @brief The data layers loaded, in order.
    /// @return The layer records.
    [[nodiscard]] const std::vector<core::layer_info>& layers() const
    {
        return _table.layers();
    }

    /// @brief Hash of the loaded data, recorded in each result's
    ///        signature().
    /// @return The hash; 0 when nothing is loaded.
    [[nodiscard]] std::uint64_t data_hash() const { return _table.data_hash(); }

  private:
    // -- Internal data structures -----------------------------------------

    static constexpr std::string_view library_name_ = "biodata-generator";
    static constexpr std::string_view data_file_ = "biodata.tsv";

    static constexpr std::array<std::string_view, 3> eye_columns_{
        "eye_blue", "eye_intermediate", "eye_brown"};
    static constexpr std::array<std::string_view, 4> hair_columns_{
        "hair_black", "hair_brown", "hair_blond", "hair_red"};
    static constexpr std::array<std::string_view, 6> skin_columns_{
        "skin_I", "skin_II", "skin_III", "skin_IV", "skin_V", "skin_VI"};
    static constexpr std::array<std::string_view, 8> blood_columns_{
        "blood_O_pos", "blood_A_pos", "blood_B_pos", "blood_AB_pos",
        "blood_O_neg", "blood_A_neg", "blood_B_neg", "blood_AB_neg"};

    static core::table_schema schema_()
    {
        core::table_schema s{.key = {"country_code"},
                             .required = {"male_height_cm", "male_height_sd",
                                          "female_height_cm", "female_height_sd",
                                          "male_bmi_mean", "female_bmi_mean", "bmi_sd",
                                          "left_handed_pct"},
                             .optional = {"country_name"}};
        for (const auto group : {std::span<const std::string_view>{eye_columns_},
                                 std::span<const std::string_view>{hair_columns_},
                                 std::span<const std::string_view>{skin_columns_},
                                 std::span<const std::string_view>{blood_columns_}})
        {
            for (const auto column : group) s.required.emplace_back(column);
        }
        return s;
    }

    struct entry
    {
        std::string cca2;
        std::string name;
        double male_height_mean{0.0};
        double male_height_sd{0.0};
        double female_height_mean{0.0};
        double female_height_sd{0.0};
        double male_bmi_mean{0.0};
        double female_bmi_mean{0.0};
        double bmi_sd{0.0};
        double left_handed{0.0}; // probability, from the percentage
        core::discrete eyes{1.0};
        core::discrete hair{1.0};
        core::discrete skin{1.0};
        core::discrete blood{1.0};

        // Parse and check a row; throws core::tsv_error located at the row.
        static entry parse(const core::tsv_row& row)
        {
            entry e;
            e.cca2 = row.get<std::string>("country_code");
            e.name = row.get_or<std::string>("country_name", "");
            e.male_height_mean = positive(row, "male_height_cm");
            e.male_height_sd = positive(row, "male_height_sd");
            e.female_height_mean = positive(row, "female_height_cm");
            e.female_height_sd = positive(row, "female_height_sd");
            e.male_bmi_mean = positive(row, "male_bmi_mean");
            e.female_bmi_mean = positive(row, "female_bmi_mean");
            e.bmi_sd = positive(row, "bmi_sd");
            const auto pct = row.get<double>("left_handed_pct");
            if (pct < 0.0 || pct > 100.0)
            {
                throw row.error("left_handed_pct", "must be between 0 and 100");
            }
            e.left_handed = pct / 100.0;
            e.eyes = weights(row, eye_columns_);
            e.hair = weights(row, hair_columns_);
            e.skin = weights(row, skin_columns_);
            e.blood = weights(row, blood_columns_);
            return e;
        }

      private:
        static double positive(const core::tsv_row& row, std::string_view column)
        {
            const auto v = row.get<double>(column);
            if (!(v > 0.0)) throw row.error(column, "must be positive");
            return v;
        }

        static core::discrete weights(const core::tsv_row& row,
                                      std::span<const std::string_view> columns)
        {
            std::vector<double> w;
            w.reserve(columns.size());
            for (const auto column : columns)
            {
                const auto v = row.get<double>(column);
                if (v < 0.0) throw row.error(column, "weight must not be negative");
                w.push_back(v);
            }
            try
            {
                return core::discrete{w};
            }
            catch (const std::invalid_argument&)
            {
                throw row.error(columns.front(), "the weights in this group are all zero");
            }
        }
    };

    core::layered_table _table{schema_()};
    std::vector<entry> _entries; // in table row order
    std::map<std::string, std::size_t, std::less<>> _index;

    core::rng _engine{core::rng::from_entropy()};

    struct auto_probe_tag {};

    explicit bdg(auto_probe_tag /*tag*/)
    {
        if (!load(dataset::lite)) (void)load(dataset::full);
    }

    // -- Helpers ----------------------------------------------------------

    static std::span<const core::embedded_file> embedded_files_()
    {
#ifdef DASMIG_BIODATA_EMBED
        return embedded::biodatagen::files;
#else
        return {};
#endif
    }

    std::uint64_t draw_seed_() { return _engine(); }

    void rebuild_()
    {
        std::vector<entry> entries;
        std::map<std::string, std::size_t, std::less<>> index;
        entries.reserve(_table.size());
        for (const auto& row : _table.rows())
        {
            entries.push_back(entry::parse(row));
            index.emplace(entries.back().cca2, entries.size() - 1);
        }
        _entries = std::move(entries);
        _index = std::move(index);
    }

    void require_data_() const
    {
        if (_entries.empty())
        {
            throw std::runtime_error("No biodata loaded. Call load() first.");
        }
    }

    [[nodiscard]] const entry& lookup_entry_(std::string_view cca2) const
    {
        require_data_();
        const auto it = _index.find(cca2);
        if (it == _index.end())
        {
            throw std::invalid_argument(std::format("Unknown country code: {}", cca2));
        }
        return _entries[it->second];
    }

    // A country chosen uniformly from the call seed's "country" stream.
    [[nodiscard]] const entry& random_entry_(std::uint64_t call_seed) const
    {
        require_data_();
        auto r = core::rng{call_seed}.split("country");
        return _entries[static_cast<std::size_t>(core::bounded(r, _entries.size()))];
    }

    // -- Age effects (simplified population-level curves) ------------------

    static constexpr unsigned max_age_ = 120;

    // Fraction of adult height reached at each age 0..18 (about 50 cm at
    // birth, girls finishing growing two to three years before boys).
    static constexpr std::array<double, 19> male_growth_{
        0.29, 0.42, 0.50, 0.56, 0.61, 0.65, 0.69, 0.72, 0.75, 0.78,
        0.81, 0.84, 0.87, 0.90, 0.94, 0.97, 0.985, 0.995, 1.0};
    static constexpr std::array<double, 19> female_growth_{
        0.30, 0.43, 0.51, 0.57, 0.62, 0.66, 0.70, 0.73, 0.77, 0.80,
        0.84, 0.88, 0.93, 0.96, 0.98, 0.995, 1.0, 1.0, 1.0};

    // Factor on the adult mean BMI: child BMI dips around age 5 and rebounds,
    // adults rise through mid-life and decline in old age.
    struct knot
    {
        double age;
        double factor;
    };
    static constexpr std::array<knot, 10> bmi_curve_{{{0, 0.68},  {2, 0.64},  {5, 0.58},  {8, 0.62},
                                                      {12, 0.72}, {18, 0.92}, {30, 0.98}, {50, 1.03},
                                                      {70, 1.02}, {100, 0.90}}};

    static double height_factor_(unsigned age, bool male)
    {
        if (age < 18) return (male ? male_growth_ : female_growth_)[age];
        if (age <= 50) return 1.0;
        // About 0.7% of height lost per decade after 50.
        return 1.0 - core::math::fenced(0.0007 * static_cast<double>(age - 50));
    }

    static double bmi_factor_(unsigned age)
    {
        const double a = static_cast<double>(age);
        for (std::size_t i = 1; i < bmi_curve_.size(); ++i)
        {
            if (a <= bmi_curve_[i].age)
            {
                const auto& lo = bmi_curve_[i - 1];
                const auto& hi = bmi_curve_[i];
                const double t = (a - lo.age) / (hi.age - lo.age);
                return lo.factor + core::math::fenced((hi.factor - lo.factor) * t);
            }
        }
        return bmi_curve_.back().factor;
    }

    // Chance that hair has turned grey: none before 25, 50% at 55.
    static double grey_probability_(unsigned age)
    {
        if (age < 25) return 0.0;
        return 1.0 / (1.0 + core::math::exp(-(static_cast<double>(age) - 55.0) / 9.0));
    }

    [[nodiscard]] biodata generate_(const entry& e, std::uint64_t call_seed,
                                    std::optional<sex> forced_sex,
                                    std::optional<std::uint8_t> age = std::nullopt) const
    {
        const core::rng call{call_seed};
        biodata b;
        b._signature = {.seed = call_seed, .data = data_hash()};
        b.country_code = e.cca2;

        // 1. Sex: fixed or 50/50.
        if (forced_sex)
        {
            b.bio_sex = *forced_sex;
        }
        else
        {
            auto r = call.split("sex");
            b.bio_sex = core::bernoulli(r, 0.5) ? sex::female : sex::male;
        }
        const bool male = b.bio_sex == sex::male;

        // 2. Height: Gaussian, clamped to 4 SD.
        {
            auto r = call.split("height");
            const double mean = male ? e.male_height_mean : e.female_height_mean;
            const double sd = male ? e.male_height_sd : e.female_height_sd;
            const double spread = core::math::fenced(4.0 * sd);
            b.height_cm = std::clamp(core::normal(r, mean, sd), mean - spread, mean + spread);
            if (age) b.height_cm *= height_factor_(*age, male);
        }

        // 3. BMI: log-normal with the given mean and SD (right-skewed).
        {
            auto r = call.split("bmi");
            const double mean = male ? e.male_bmi_mean : e.female_bmi_mean;
            const double ratio = (e.bmi_sd * e.bmi_sd) / (mean * mean);
            const double ln_sigma2 = core::math::log(1.0 + ratio);
            const double ln_mu = core::math::log(mean) - core::math::fenced(0.5 * ln_sigma2);
            b.bmi = core::lognormal(r, ln_mu, std::sqrt(ln_sigma2));
            if (age) b.bmi *= bmi_factor_(*age);
            b.bmi = std::clamp(b.bmi, 14.0, 55.0);
        }

        // 4. Weight = BMI × (height in m)².
        const double height_m = b.height_cm / 100.0;
        b.weight_kg = b.bmi * height_m * height_m;

        // 5-8. Categorical traits.
        {
            auto r = call.split("eyes");
            b.eyes = static_cast<eye_color>(e.eyes(r));
        }
        {
            auto r = call.split("hair");
            b.hair = static_cast<hair_color>(e.hair(r));
        }
        if (age)
        {
            auto r = call.split("grey");
            if (core::bernoulli(r, grey_probability_(*age))) b.hair = hair_color::grey;
        }
        {
            auto r = call.split("skin");
            b.skin = static_cast<skin_type>(e.skin(r) + 1); // enum starts at 1
        }
        {
            auto r = call.split("blood");
            b.blood = static_cast<blood_type>(e.blood(r));
        }

        // 9. Handedness.
        {
            auto r = call.split("hand");
            b.hand = core::bernoulli(r, e.left_handed) ? handedness::left : handedness::right;
        }
        b.age = age;
        return b;
    }
};

} // namespace dasmig

#endif // DASMIG_BIODATAGEN_HPP
