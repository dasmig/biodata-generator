# Biodata Generator for C++

[![Biodata Generator for C++](https://raw.githubusercontent.com/dasmig/biodata-generator/master/doc/biodata-generator.png)](https://github.com/dasmig/biodata-generator/releases)

> **Requires C++23** (e.g., `-std=c++23` for GCC/Clang, `/std:c++latest` for MSVC).

[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/dasmig/biodata-generator/master/LICENSE.MIT)
[![CI](https://github.com/dasmig/biodata-generator/actions/workflows/ci.yml/badge.svg)](https://github.com/dasmig/biodata-generator/actions/workflows/ci.yml)
[![GitHub Releases](https://img.shields.io/github/release/dasmig/biodata-generator.svg)](https://github.com/dasmig/biodata-generator/releases)
[![GitHub Issues](https://img.shields.io/github/issues/dasmig/biodata-generator.svg)](https://github.com/dasmig/biodata-generator/issues)
[![C++23](https://img.shields.io/badge/standard-C%2B%2B23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Header-only](https://img.shields.io/badge/type-header--only-green.svg)](https://github.com/dasmig/biodata-generator#integration)
[![Platform](https://img.shields.io/badge/platform-linux%20|%20windows%20|%20macos-lightgrey.svg)](https://github.com/dasmig/biodata-generator)
[![Documentation](https://img.shields.io/badge/docs-GitHub%20Pages-blue.svg)](https://dasmig.github.io/biodata-generator/)

**[API Reference](https://dasmig.github.io/biodata-generator/)** · **[Usage Guide](doc/usage.md)** · **[Releases](https://github.com/dasmig/biodata-generator/releases)**

## Features

- **Demographically Plausible Characteristics**. Generates random human physical traits using a multi-stage pipeline: height from country/sex-specific Gaussian distributions, BMI from log-normal distributions, and categorical sampling for phenotypic traits.

- **Seven Physical Traits**. Every generated profile includes: height (cm), weight (kg), BMI, eye colour, hair colour, Fitzpatrick skin type, ABO/Rh blood type, and handedness.

- **Country-Specific Distributions**. Height means, BMI values, eye/hair/skin distributions, blood type frequencies, and left-handedness rates all vary by country, reflecting real-world population statistics.

- **Data-Driven**. Height and BMI from [NCD-RisC](https://ncdrisc.org/) via [Our World in Data](https://ourworldindata.org/), blood types from published population studies, phenotypic traits from Katsara & Nothnagel 2019, VISAGE consortium, and Papadatou-Pastou 2020.

- **Deterministic Seeding**. Per-call `get_biodata(seed)` for reproducible results, generator-level `seed()` / `unseed()` for deterministic sequences, and `biodata::seed()` for replaying a previous generation.

- **Identical Everywhere**. A seed gives the same biodata on every compiler, standard library and CPU (GCC, Clang, MSVC, Emscripten; x86-64 and ARM64), checked by known-answer tests.

- **Extensible Data**. Load your own TSV layers on top of the shipped data to add, change or remove countries (`extend()`), checked before they are applied.

- **JSON and Replay**. `to_json()` on every result, and a `signature()` recording the seed and the data it came from.

- **Compiled-in Data**. Define `DASMIG_BIODATA_EMBED` to build the `lite` tier into the program, for WebAssembly and other builds without files.

- **Multi-Instance Support**. Construct independent `bdg` instances with their own data and random engine.

- **Typed Enumerations**. `eye_color`, `hair_color`, `skin_type`, `blood_type`, and `handedness` enums with string conversion helpers.

## Integration

[`biodatagen.hpp`](https://github.com/dasmig/biodata-generator/blob/master/dasmig/biodatagen.hpp) is the library [released here](https://github.com/dasmig/biodata-generator/releases). Keep [`core.hpp`](https://github.com/dasmig/biodata-generator/blob/master/dasmig/core.hpp) and the [`core/`](https://github.com/dasmig/biodata-generator/tree/master/dasmig/core) directory ([dasmig-core](https://github.com/dasmig/dasmig-core)) in the same directory. Add

```cpp
#include <dasmig/biodatagen.hpp>

// For convenience.
using bdg = dasmig::bdg;
```

to the files you want to generate biodata and set the necessary switches to enable C++23 (e.g., `-std=c++23` for GCC and Clang).

Additionally you must supply the biodata generator with the [`resources`](https://github.com/dasmig/biodata-generator/tree/master/resources) folder containing `full/` and/or `lite/` subdirectories with the TSV data file, also available in the [release](https://github.com/dasmig/biodata-generator/releases).

## Usage

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
auto b = bdg::instance().get_biodata();
std::cout << b << '\n';  // implicit string conversion

// Generate biodata for a specific country.
auto us = bdg::instance().get_biodata("US");
std::cout << "Height: " << us.height_cm << " cm\n";
std::cout << "Weight: " << us.weight_kg << " kg\n";
std::cout << "BMI:    " << us.bmi << "\n";

// Request a specific sex.
auto m = bdg::instance().get_biodata("BR", dasmig::sex::male);

// Access typed enum fields.
std::cout << "Eyes:  " << dasmig::biodata::eye_color_str(b.eyes) << '\n';
std::cout << "Hair:  " << dasmig::biodata::hair_color_str(b.hair) << '\n';
std::cout << "Skin:  " << dasmig::biodata::skin_type_str(b.skin) << '\n';
std::cout << "Blood: " << dasmig::biodata::blood_type_str(b.blood) << '\n';
std::cout << "Hand:  " << dasmig::biodata::handedness_str(b.hand) << '\n';

// Deterministic generation — same seed always produces the same result.
auto seeded = bdg::instance().get_biodata("US", std::uint64_t{42});

// Replay a previous generation using its seed.
auto replay = bdg::instance().get_biodata("US", seeded.seed());

// Seed the engine for a deterministic sequence.
bdg::instance().seed(100);
// ... generate biodata ...
bdg::instance().unseed();  // restore non-deterministic state

// Independent instance — separate data and random engine.
bdg my_gen;
my_gen.load("path/to/resources/lite");
auto c = my_gen.get_biodata("JP");
```

For the complete feature guide — fields, seeding, enums, and more — see the **[Usage Guide](doc/usage.md)**.

## Generation Pipeline

Each call to `get_biodata()` runs this pipeline:

1. **Sex** — 50/50 or forced via `sex` parameter.
2. **Height** — Gaussian distribution using country/sex-specific mean and standard deviation from NCD-RisC anthropometric data.
3. **BMI** — Log-normal distribution from country/sex-specific mean, modelling the natural right-skew of BMI.
4. **Weight** — Derived: `BMI × height_m²`.
5. **Eye Colour** — Categorical sampling from country-specific blue/intermediate/brown distribution.
6. **Hair Colour** — Categorical sampling from country-specific black/brown/blond/red distribution.
7. **Skin Type** — Categorical sampling from Fitzpatrick I–VI distribution.
8. **Blood Type** — Categorical sampling from ABO/Rh frequencies (O+, A+, B+, AB+, O−, A−, B−, AB−).
9. **Handedness** — Bernoulli sampling from country-specific left-handedness rate.

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
# Example
make

# Tests
make test

# Code coverage
make coverage

# API docs
make docs
```

## Compiler Support

Tested with:
- Clang 18+ (`-std=c++23`)
- GCC 14+ (`-std=c++23`)
- MSVC 19.38+ (`/std:c++latest`)

## Dependencies

| Dependency | Version | Bundled | Purpose |
|-----------|---------|---------|---------|
| [dasmig-core](https://github.com/dasmig/dasmig-core) | 0.3 | Yes (`core.hpp`, `core/`) | Platform-independent random engine and distributions, data layers, JSON |
| [Catch2](https://github.com/catchorg/Catch2) | 3.x | Yes (amalgamated) | Unit testing |

## Related Libraries

| Library | Description |
|---------|-------------|
| [name-generator](https://github.com/dasmig/name-generator) | Culturally appropriate full names |
| [nickname-generator](https://github.com/dasmig/nickname-generator) | Gamer-style nicknames |
| [birth-generator](https://github.com/dasmig/birth-generator) | Demographically plausible birthdays |
| [city-generator](https://github.com/dasmig/city-generator) | Weighted city selection by population |
| [country-generator](https://github.com/dasmig/country-generator) | Weighted country selection by population |
| [entity-generator](https://github.com/dasmig/entity-generator) | ECS-based entity generation |

## License

<img align="right" src="https://opensource.org/trademarks/opensource/OSI-Approved-License-100x137.png">

This library is released under the [MIT License](https://opensource.org/licenses/MIT).

```
MIT License

Copyright (c) 2020-2026 Diego Dasso Migotto
```
