# Biodata Generator for C++

> **Requires C++23** (e.g., `-std=c++23` for GCC/Clang, `/std:c++latest` for MSVC).

[![Biodata Generator for C++](https://raw.githubusercontent.com/dasmig/biodata-generator/master/doc/biodata-generator.png)](https://github.com/dasmig/biodata-generator/releases)

[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/dasmig/biodata-generator/master/LICENSE.MIT)
[![CI](https://github.com/dasmig/biodata-generator/actions/workflows/ci.yml/badge.svg)](https://github.com/dasmig/biodata-generator/actions/workflows/ci.yml)
[![GitHub Releases](https://img.shields.io/github/release/dasmig/biodata-generator.svg)](https://github.com/dasmig/biodata-generator/releases)
[![C++23](https://img.shields.io/badge/standard-C%2B%2B23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Header-only](https://img.shields.io/badge/type-header--only-green.svg)](https://github.com/dasmig/biodata-generator#integration)
[![Documentation](https://img.shields.io/badge/docs-GitHub%20Pages-blue.svg)](https://dasmig.github.io/biodata-generator/)

**[API Reference](https://dasmig.github.io/biodata-generator/)** · **[Usage Guide](doc/usage.md)** · **[Data and Method](doc/data.md)** · **[Releases](https://github.com/dasmig/biodata-generator/releases)**

Demographically plausible physical characteristics (height, weight, eye and hair colour, skin type, blood type, handedness) by country, sex and age.

## Features

- **Demographically Plausible Characteristics**. Generates random human physical traits using a multi-stage pipeline: height from country/sex-specific Gaussian distributions, BMI from log-normal distributions, and categorical sampling for phenotypic traits.

- **Seven Physical Traits**. Every generated profile includes: height (cm), weight (kg), BMI, eye colour, hair colour, Fitzpatrick skin type, ABO/Rh blood type, and handedness.

- **Traits That Go Together**. Skin, hair and eyes are drawn jointly: each keeps the country's shares, and fair skin pairs with light hair and eyes, dark with dark, so a blond person from the US has blue eyes far more often than a black-haired one.

- **Country-Specific Distributions**. Height means, BMI values, eye/hair/skin distributions, blood type frequencies, and left-handedness rates all vary by country, reflecting real-world population statistics.

- **Data-Driven**. Height and BMI from [NCD-RisC](https://ncdrisc.org/) via [Our World in Data](https://ourworldindata.org/), blood types from published population studies, phenotypic traits from Katsara & Nothnagel 2019, VISAGE consortium, and Papadatou-Pastou 2020.

- **Request-First API**. All generation choices live in `biodata_request`: country, biological sex and optional age. Loading and extending data stay on the generator.

- **Typed Enumerations**. `eye_color`, `hair_color`, `skin_type`, `blood_type`, and `handedness` enums with string conversion helpers.

- **Deterministic Seeding**. Per-call `get_biodata(request, seed)` for reproducible results, and `biodata::seed()` for replaying a previous generation with the same request and data.

- **Batch Generation**. Generate thousands of results from one seed, on several threads with the same output for any thread count, optionally unique by a key, and stream them as JSON Lines or CSV (see the usage guide).

- **Identical Everywhere**. A seed gives the same biodata on every compiler, standard library and CPU (GCC, Clang, MSVC, Emscripten; x86-64 and ARM64), checked by known-answer tests.

- **Extensible Data**. Load your own TSV layers on top of the shipped data to add, change or remove countries (`extend()`), checked before they are applied.

- **JSON and Replay**. `to_json()` on every result, and a `signature()` recording the seed and the data it came from.

- **Compiled-in Data**. Define `DASMIG_BIODATA_EMBED` to build the `lite` tier into the program, for WebAssembly and other builds without files.

- **Multi-Instance Support**. Construct independent `bdg` instances with their own data and no mutable generation defaults.

## Integration

[`biodatagen.hpp`](https://github.com/dasmig/biodata-generator/blob/master/dasmig/biodatagen.hpp) is the library [released here](https://github.com/dasmig/biodata-generator/releases). Keep [`core.hpp`](https://github.com/dasmig/biodata-generator/blob/master/dasmig/core.hpp) and the [`core/`](https://github.com/dasmig/biodata-generator/tree/master/dasmig/core) directory ([dasmig-core](https://github.com/dasmig/dasmig-core)) in the same directory. Add

```cpp
#include <dasmig/biodatagen.hpp>
```

to the files you want to use it in, and supply the [`resources`](https://github.com/dasmig/biodata-generator/tree/master/resources) folder (or define `DASMIG_BIODATA_EMBED`).

## Usage

The public calls are `get_biodata(request)` and `get_biodata(request, seed)`, both `const`. A default `dasmig::biodata_request{}` selects a country uniformly, draws sex 50/50 and applies no age effects. The unseeded form takes a fresh entropy seed; the seeded form is independent of previous calls.

```cpp
#include <dasmig/biodatagen.hpp>
#include <iostream>

// For convenience.
using bdg = dasmig::bdg;

// Manually load a specific dataset tier if necessary.
bdg::instance().load(dasmig::dataset::lite);  // ~111 countries (best coverage)
// OR
bdg::instance().load(dasmig::dataset::full);  // ~197 countries (gap-filled)

// Generate random biodata (uniform country selection).
auto b = bdg::instance().get_biodata({});
std::cout << b << '\n';  // implicit string conversion

// Generate biodata for a specific country.
auto us = bdg::instance().get_biodata({.country = "US"});
std::cout << "Height: " << us.height_cm << " cm\n";
std::cout << "Weight: " << us.weight_kg << " kg\n";
std::cout << "BMI:    " << us.bmi << "\n";

// Request a specific sex.
auto m = bdg::instance().get_biodata({.country = "BR", .bio_sex = dasmig::sex::male});

// Include age effects.
auto older = bdg::instance().get_biodata({.country = "BR", .age = 67});

// Access typed enum fields.
std::cout << "Eyes:  " << dasmig::biodata::eye_color_str(b.eyes) << '\n';
std::cout << "Hair:  " << dasmig::biodata::hair_color_str(b.hair) << '\n';
std::cout << "Skin:  " << dasmig::biodata::skin_type_str(b.skin) << '\n';
std::cout << "Blood: " << dasmig::biodata::blood_type_str(b.blood) << '\n';
std::cout << "Hand:  " << dasmig::biodata::handedness_str(b.hand) << '\n';

// Deterministic generation — same seed always produces the same result.
auto seeded = bdg::instance().get_biodata({.country = "US"}, std::uint64_t{42});

// Replay a previous generation using its seed.
auto replay = bdg::instance().get_biodata({.country = "US"}, seeded.seed());

// Supply each sequence seed explicitly.
for (std::uint64_t seed = 100; seed < 103; ++seed)
    std::cout << bdg::instance().get_biodata({.country = "US"}, seed) << '\n';

// Independent instance — separate data.
bdg my_gen;
my_gen.load("path/to/resources/lite");
auto c = my_gen.get_biodata({.country = "JP"});
```

For the complete feature guide — fields, seeding, enums, and more — see the **[Usage Guide](doc/usage.md)**.

The positional overloads and generator-level `seed()` / `unseed()` have been removed. See [Migrating to the request API](doc/usage.md#migrating-to-the-request-api).

## Generation Pipeline

Each call to `get_biodata()` runs this pipeline:

1. **Sex** — 50/50 or fixed by `request.bio_sex`.
2. **Height** — Gaussian distribution using country/sex-specific mean and standard deviation from NCD-RisC anthropometric data.
3. **BMI** — Log-normal distribution from country/sex-specific mean, modelling the natural right-skew of BMI.
4. **Weight** — Derived: `BMI × height_m²`.
5. **Eye Colour** — Categorical sampling from country-specific blue/intermediate/brown distribution.
6. **Hair Colour** — Categorical sampling from country-specific black/brown/blond/red distribution.
7. **Skin Type** — Categorical sampling from Fitzpatrick I–VI distribution.
8. **Blood Type** — Categorical sampling from ABO/Rh frequencies (O+, A+, B+, AB+, O−, A−, B−, AB−).
9. **Handedness** — Bernoulli sampling from country-specific left-handedness rate.
10. **Age** (optional, `get_biodata({.age = 40})`) — adjusts height (growth curve, shrinkage after 50), BMI (life-course curve) and hair (greying) without changing the base draws.

## Data Sources

| Trait | Source | Coverage |
|-------|--------|----------|
| Height (mean, SD) | [NCD-RisC](https://ncdrisc.org/) via [OWID](https://ourworldindata.org/) | 202 countries |
| BMI (mean) | [WHO GHO](https://www.who.int/data/gho) via OWID | 197 countries |
| Blood type | Published population studies (Wikipedia compilation) | 124 countries |
| Eye colour | Katsara & Nothnagel 2019 + regional estimates | 70 countries |
| Hair colour | VISAGE consortium + regional estimates | 62 countries |
| Skin tone | WHO UV guidance + ethnic composition estimates | 81 countries |
| Handedness | Papadatou-Pastou et al. 2020 meta-analysis | 75 countries |

### Dataset Tiers

| Tier | Countries | Description |
|------|-----------|-------------|
| `lite` | ~111 | Countries with specific data for at least one phenotypic trait |
| `full` | ~197 | All countries with height data; phenotypic gaps filled with regional defaults |

## Building

```bash
make            # example
make test       # tests
make bench      # benchmarks, results in build/bench.tsv
make test-embed # tests with the lite tier compiled in
make coverage   # code coverage
make lint       # clang-tidy
make docs       # API docs
make embed      # regenerate dasmig/biodatagen_data.hpp from resources/lite
```

For targeted checks after `make test`: `./build/test_runner "[generation],[age],[seed],[kat]"`.

## Compiler Support

Tested with:
- Clang 18+ (`-std=c++23`)
- GCC 14+ (`-std=c++23`)
- MSVC 19.38+ (`/std:c++latest`)
- Emscripten 3.1.74

## Dependencies

| Dependency | Version | Bundled | Purpose |
|-----------|---------|---------|---------|
| [dasmig-core](https://github.com/dasmig/dasmig-core) | 1.3 | Yes (`core.hpp`, `core/`) | Platform-independent random engine and distributions, data layers, JSON |
| [Catch2](https://github.com/catchorg/Catch2) | 3.x | Yes (amalgamated) | Unit testing |

## Related Libraries

| Library | Description |
|---------|-------------|
| [address-generator](https://github.com/dasmig/address-generator) | Postal addresses with real streets by place |
| [birth-generator](https://github.com/dasmig/birth-generator) | Demographically plausible birthdays |
| [city-generator](https://github.com/dasmig/city-generator) | Weighted city selection by population |
| [country-generator](https://github.com/dasmig/country-generator) | Weighted country selection by population |
| [device-generator](https://github.com/dasmig/device-generator) | Devices, browsers, user agents and IP addresses that fit together |
| [email-generator](https://github.com/dasmig/email-generator) | Email addresses in each country's style |
| [entity-generator](https://github.com/dasmig/entity-generator) | ECS-based entity generation |
| [interest-generator](https://github.com/dasmig/interest-generator) | Religion, hobbies and favourite media by country, age and sex |
| [name-generator](https://github.com/dasmig/name-generator) | Culturally appropriate full names |
| [nickname-generator](https://github.com/dasmig/nickname-generator) | Gamer-style nicknames |
| [occupation-generator](https://github.com/dasmig/occupation-generator) | Employment status, education and occupation by country, age and sex |
| [persona-generator](https://github.com/dasmig/persona-generator) | Complete characters composed from the other generators |
| [phone-generator](https://github.com/dasmig/phone-generator) | Phone numbers valid in each country's numbering plan |
| [relationship-generator](https://github.com/dasmig/relationship-generator) | Families around a person: parents, siblings, partners and children |
| [trait-generator](https://github.com/dasmig/trait-generator) | Personality traits with conflicts, synergies and Big Five axes |
| [user-generator](https://github.com/dasmig/user-generator) | Complete user records: account, username, email, phone and address |

## License

This library is released under the [MIT License](https://opensource.org/licenses/MIT).

```
MIT License

Copyright (c) 2020-2026 Diego Dasso Migotto
```
