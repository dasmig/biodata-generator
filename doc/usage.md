# Usage Guide

This guide covers every feature of the biodata-generator library in detail. For a quick overview, see the [README](../README.md). For the full API reference, run `doxygen Doxyfile` from the repository root and open `doc/api/html/index.html`.

[TOC]

<!-- GitHub-rendered TOC (Doxygen uses [TOC] above instead) -->
<ul>
<li><a href="#quick-start">Quick Start</a></li>
<li><a href="#installation">Installation</a></li>
<li><a href="#loading-resources">Loading Resources</a></li>
<li><a href="#generating-biodata">Generating Biodata</a>
  <ul>
  <li><a href="#sex-specific-generation">Sex-Specific Generation</a></li>
  </ul>
</li>
<li><a href="#biodata-fields">Biodata Fields</a></li>
<li><a href="#typed-enumerations">Typed Enumerations</a></li>
<li><a href="#country-specific-generation">Country-Specific Generation</a></li>
<li><a href="#seeding-and-deterministic-generation">Seeding and Deterministic Generation</a></li>
<li><a href="#multi-instance-support">Multi-Instance Support</a></li>
<li><a href="#generation-pipeline">Generation Pipeline</a></li>
<li><a href="#data-pipeline">Data Pipeline</a></li>
<li><a href="#thread-safety">Thread Safety</a></li>
<li><a href="#error-reference">Error Reference</a></li>
</ul>

## Quick Start

```cpp
#include <dasmig/biodatagen.hpp>
#include <iostream>

int main()
{
    auto& gen = dasmig::bdg::instance();

    // Random biodata (uniform country selection).
    auto b = gen.get_biodata();
    std::cout << b << "\n";

    // Country-specific biodata.
    auto jp = gen.get_biodata("JP");
    std::cout << "Japan: " << jp.height_cm << "cm, "
              << jp.weight_kg << "kg\n";

    // Sex-specific biodata.
    auto m = gen.get_biodata("BR", dasmig::sex::male);
    std::cout << "Male from Brazil: " << m << "\n";
}
```

## Installation

1. Copy `dasmig/biodatagen.hpp` and `dasmig/random.hpp` into your include path.
2. Copy the `resources/` folder (containing `full/` and/or `lite/` subdirectories with `biodata.tsv`) so it is accessible at runtime.
3. Compile with C++23 enabled: `-std=c++23`.

## Loading Resources

The library ships two dataset tiers:

| Tier | Enum | Countries | Description |
|------|------|-----------|-------------|
| lite | `dasmig::dataset::lite` | ~111 | Countries with best phenotypic data coverage |
| full | `dasmig::dataset::full` | ~197 | All countries; gaps filled with regional defaults |

### Auto-Probing (Singleton)

The singleton (`bdg::instance()`) auto-probes these paths on first access:

1. `resources/lite/biodata.tsv`
2. `resources/full/biodata.tsv`
3. `../resources/lite/biodata.tsv`
4. `biodata-generator/resources/lite/biodata.tsv`

### Explicit Loading

```cpp
dasmig::bdg gen;

// By tier enum:
gen.load(dasmig::dataset::lite);
gen.load(dasmig::dataset::full);

// By explicit path:
gen.load("path/to/resources/lite");
```

### Checking Data

```cpp
if (gen.has_data())
    std::cout << gen.country_count() << " countries loaded\n";
```

## Generating Biodata

### Basic Generation

```cpp
auto& gen = dasmig::bdg::instance();

// Random country:
auto b = gen.get_biodata();

// Specific country:
auto us = gen.get_biodata("US");
```

### Sex-Specific Generation

```cpp
// Force biological sex:
auto m = gen.get_biodata("US", dasmig::sex::male);
auto f = gen.get_biodata("US", dasmig::sex::female);

// Random country with forced sex:
auto rm = gen.get_biodata(dasmig::sex::male);
```

### Deterministic Generation

```cpp
// Per-call seed (const method):
auto b = gen.get_biodata("US", std::uint64_t{42});

// With sex and seed:
auto m = gen.get_biodata("US", dasmig::sex::male, std::uint64_t{42});

// Random country with seed:
auto r = gen.get_biodata(std::uint64_t{42});

// Random country with sex and seed:
auto rs = gen.get_biodata(dasmig::sex::female, std::uint64_t{42});
```

## Biodata Fields

Every `biodata` object contains:

| Field | Type | Description |
|-------|------|-------------|
| `country_code` | `std::string` | ISO 3166-1 alpha-2 code |
| `bio_sex` | `dasmig::sex` | Biological sex (male/female) |
| `height_cm` | `double` | Height in centimetres |
| `weight_kg` | `double` | Weight in kilograms |
| `bmi` | `double` | Body mass index (kg/m²) |
| `eyes` | `dasmig::eye_color` | Eye colour (blue/intermediate/brown) |
| `hair` | `dasmig::hair_color` | Hair colour (black/brown/blond/red) |
| `skin` | `dasmig::skin_type` | Fitzpatrick skin type (I–VI) |
| `blood` | `dasmig::blood_type` | ABO/Rh blood type |
| `hand` | `dasmig::handedness` | Handedness (right/left) |

Additionally:
- `seed()` — the random seed used for this generation (for replay).
- `to_string()` — human-readable summary.
- Implicit `operator std::string()` and `operator<<`.

## Typed Enumerations

All phenotypic traits use typed enumerations. String conversion helpers are static methods on `biodata`:

```cpp
auto b = gen.get_biodata("US");

std::cout << dasmig::biodata::eye_color_str(b.eyes) << "\n";
std::cout << dasmig::biodata::hair_color_str(b.hair) << "\n";
std::cout << dasmig::biodata::skin_type_str(b.skin) << "\n";
std::cout << dasmig::biodata::blood_type_str(b.blood) << "\n";
std::cout << dasmig::biodata::handedness_str(b.hand) << "\n";
```

### eye_color

| Value | Label | Description |
|-------|-------|-------------|
| `blue` | "blue" | Blue eyes |
| `intermediate` | "intermediate" | Green, hazel, amber |
| `brown` | "brown" | Brown eyes |

### hair_color

| Value | Label |
|-------|-------|
| `black` | "black" |
| `brown` | "brown" |
| `blond` | "blond" |
| `red` | "red" |

### skin_type

Fitzpatrick scale types I through VI, based on UV sensitivity and melanin content.

### blood_type

| Value | Label |
|-------|-------|
| `O_pos` | "O+" |
| `A_pos` | "A+" |
| `B_pos` | "B+" |
| `AB_pos` | "AB+" |
| `O_neg` | "O-" |
| `A_neg` | "A-" |
| `B_neg` | "B-" |
| `AB_neg` | "AB-" |

### handedness

| Value | Label |
|-------|-------|
| `right` | "right" |
| `left` | "left" |

## Country-Specific Generation

Pass an ISO 3166-1 alpha-2 country code:

```cpp
auto us = gen.get_biodata("US");
auto jp = gen.get_biodata("JP");
auto ng = gen.get_biodata("NG");
```

Different countries produce different distributions. For example, the Netherlands produces taller individuals on average than Guatemala, and Japan produces predominantly brown-eyed individuals while Scandinavian countries produce more blue-eyed individuals.

## Seeding and Deterministic Generation

Three levels of determinism:

### Per-Call Seed

```cpp
auto b = gen.get_biodata("US", std::uint64_t{42});
// Same seed → same result (const method).
```

### Seed Replay

```cpp
auto b1 = gen.get_biodata("US");
auto b2 = gen.get_biodata("US", b1.seed());
// b1 and b2 are identical.
```

### Engine Seed

```cpp
gen.seed(100);
auto a = gen.get_biodata("US");
auto b = gen.get_biodata("US");

gen.seed(100);
auto a2 = gen.get_biodata("US");  // == a
auto b2 = gen.get_biodata("US");  // == b

gen.unseed();  // restore non-deterministic state
```

Both `seed()` and `unseed()` return `*this` for chaining:

```cpp
gen.seed(42).get_biodata("US");
```

## Multi-Instance Support

```cpp
dasmig::bdg gen1;
gen1.load(dasmig::dataset::lite);

dasmig::bdg gen2;
gen2.load(dasmig::dataset::full);

// gen1 and gen2 are independent.
auto a = gen1.get_biodata("US");
auto b = gen2.get_biodata("US");
```

## Generation Pipeline

Each call to `get_biodata()` runs this pipeline:

1. **Sex** — 50/50 Bernoulli or forced via parameter.
2. **Height** — `std::normal_distribution` with country/sex-specific mean and SD. Clamped to ±4σ.
3. **BMI** — `std::lognormal_distribution` parameterised from country/sex mean and global SD (4.5). Clamped to [14, 55].
4. **Weight** — Derived: `BMI × (height_m)²`.
5. **Eye Colour** — `std::discrete_distribution` over 3 categories.
6. **Hair Colour** — `std::discrete_distribution` over 4 categories.
7. **Skin Type** — `std::discrete_distribution` over 6 Fitzpatrick types.
8. **Blood Type** — `std::discrete_distribution` over 8 ABO/Rh groups.
9. **Handedness** — `std::bernoulli_distribution` from country-specific left-handedness rate.

## Data Pipeline

The resource TSV files are built from raw data using Python scripts in `scripts/`:

1. `fetch_all_data.py` — Downloads height, eye, hair, skin, handedness data.
2. `parse_blood_types.py` — Parses blood type data from Wikipedia.
3. `fetch_bmi.py` — Discovers BMI indicator IDs from OWID.
4. `prepare_biodata.py` — Merges all sources, maps ISO codes, fills gaps with regional defaults, and outputs the final TSV files.
5. `validate_data.py` — Sanity checks on the output TSV files.

## Thread Safety

Each `bdg` instance is independent. Concurrent calls to `get_biodata()` on the **same** instance require external synchronisation (e.g. a `std::mutex`).

The singleton (`bdg::instance()`) returns a shared instance — wrap calls in a mutex if used from multiple threads.

## Error Reference

| Exception | When |
|-----------|------|
| `std::runtime_error("No biodata loaded. Call load() first.")` | Any generation call before `load()` |
| `std::invalid_argument("Unknown country code: XX")` | Unknown ISO alpha-2 code |
