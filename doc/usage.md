# Usage Guide

This guide covers every feature of the biodata-generator library in detail. For a quick overview, see the [README](../README.md). For the full API reference, run `doxygen Doxyfile` from the repository root and open `doc/api/html/index.html`.

[TOC]

<!-- GitHub-rendered TOC (Doxygen uses [TOC] above instead) -->
<ul>
<li><a href="#quick-start">Quick Start</a></li>
<li><a href="#installation">Installation</a></li>
<li><a href="#building-on-windows">Building on Windows</a></li>
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
<li><a href="#extending-data">Extending Data</a></li>
<li><a href="#replay-and-json">Replay and JSON</a></li>
<li><a href="#compiled-in-data">Compiled-in Data</a></li>
<li><a href="#migrating-to-the-request-api">Migrating to the Request API</a></li>
<li><a href="#migrating-from-v1">Migrating from v1</a></li>
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
    auto b = gen.get_biodata({});
    std::cout << b << "\n";

    // Country-specific biodata.
    auto jp = gen.get_biodata({.country = "JP"});
    std::cout << "Japan: " << jp.height_cm << "cm, "
              << jp.weight_kg << "kg\n";

    // Sex-specific biodata.
    auto m = gen.get_biodata({.country = "BR", .bio_sex = dasmig::sex::male});
    std::cout << "Male from Brazil: " << m << "\n";
}
```

## Installation

1. Copy `dasmig/biodatagen.hpp`, `dasmig/core.hpp` and the `dasmig/core/` directory into your include path, keeping them side by side.
2. Copy the `resources/` folder (containing `full/` and/or `lite/` subdirectories with `biodata.tsv`) so it is accessible at runtime.
3. Compile with C++23 enabled: `-std=c++23`.

### Building on Windows

With LLVM-MinGW (Clang and libc++) on `PATH`, run these PowerShell commands from the repository root:

```powershell
New-Item -ItemType Directory -Force build | Out-Null
clang++ -std=c++23 -O2 -Wall -Wextra -Wpedantic -Wno-c2y-extensions -I. -o build\example.exe examples\example.cpp
.\build\example.exe
clang++ -std=c++23 -O2 -Wall -Wextra -Wpedantic -Wno-c2y-extensions -I. -o build\test_runner.exe tests\tests.cpp tests\catch_amalgamated.cpp
# Targeted generation, age, seed and known-answer checks:
.\build\test_runner.exe "[generation],[age],[seed],[kat]"
# Full file-backed suite:
.\build\test_runner.exe
# Full compiled-in-data suite:
clang++ -std=c++23 -O2 -Wall -Wextra -Wpedantic -Wno-c2y-extensions -DDASMIG_BIODATA_EMBED -I. -o build\test_embed.exe tests\tests.cpp tests\catch_amalgamated.cpp
.\build\test_embed.exe
# Regenerate the API reference:
doxygen Doxyfile
```

In an MSVC developer command prompt, use `/std:c++latest /EHsc /O2 /W4 /permissive- /utf-8 /I.`, `/Fobuild\`, `/Fe:build\test_runner.exe` and `/DDASMIG_BIODATA_EMBED` in place of the corresponding Clang flags (see the CI workflow).

## Loading Resources

The library ships two dataset tiers:

| Tier | Enum | Countries | Description |
|------|------|-----------|-------------|
| lite | `dasmig::dataset::lite` | ~111 | Countries with best phenotypic data coverage |
| full | `dasmig::dataset::full` | ~197 | All countries; gaps filled with regional defaults |

### Auto-Probing (Singleton)

The singleton (`bdg::instance()`) loads the `lite` tier on first access (or `full` if `lite` is missing). It uses the compiled-in copy when built with `DASMIG_BIODATA_EMBED` (see Compiled-in Data), and otherwise looks for `lite/biodata.tsv` in these directories, relative to the working directory:

1. `resources`
2. `../resources`
3. `biodata-generator/resources`
4. `../biodata-generator/resources`

`load(dataset::lite)` and `load(dataset::full)` search the same way.

### Explicit Loading

```cpp
dasmig::bdg gen;

// By tier enum:
gen.load(dasmig::dataset::lite);
gen.load(dasmig::dataset::full);

// By explicit path (a directory containing biodata.tsv):
gen.load("path/to/resources/lite");
```

Loading replaces the data and any layers loaded before. A path that is not a directory, or has no `biodata.tsv`, is ignored; a malformed file throws `dasmig::core::tsv_error` with its location.

### Checking Data

```cpp
if (gen.has_data())
    std::cout << gen.country_count() << " countries loaded\n";
```

## Generating Biodata

There are exactly two public generation calls:

```cpp
biodata get_biodata(const biodata_request& request) const;
biodata get_biodata(const biodata_request& request, std::uint64_t seed) const;
```

All per-call choices are on the request; the generator holds only loaded data. Both calls leave the request and generator unchanged.

| Request field | Type | Default behavior |
|---------------|------|------------------|
| `country` | `std::string_view` | Empty selects uniformly from loaded countries |
| `bio_sex` | `std::optional<dasmig::sex>` | Unset draws male/female 50/50 |
| `age` | `std::optional<unsigned>` | Unset applies no age effects; accepted ages are 0–120 inclusive |

The country string must remain valid throughout the call. A request can be named and reused, or passed inline with designated initializers.

### Basic Generation

```cpp
auto& gen = dasmig::bdg::instance();

// Random country:
auto b = gen.get_biodata({});

// Specific country:
auto us = gen.get_biodata({.country = "US"});
```

### Sex-Specific Generation

```cpp
// Force biological sex:
auto m = gen.get_biodata({.country = "US", .bio_sex = dasmig::sex::male});
auto f = gen.get_biodata({.country = "US", .bio_sex = dasmig::sex::female});

// Random country with forced sex:
auto rm = gen.get_biodata({.bio_sex = dasmig::sex::male});
```

### Deterministic Generation

```cpp
// Per-call seed (const method):
auto b = gen.get_biodata({.country = "US"}, std::uint64_t{42});

// With sex and seed:
auto m = gen.get_biodata({.country = "US", .bio_sex = dasmig::sex::male}, std::uint64_t{42});

// Random country with seed:
auto r = gen.get_biodata({}, std::uint64_t{42});

// Random country with sex and seed:
auto rs = gen.get_biodata({.bio_sex = dasmig::sex::female}, std::uint64_t{42});
```

### Generation with Age

`biodata_request` fixes country, sex and age independently. An unset age skips age effects rather than drawing an age.

```cpp
// A 67-year-old from Brazil, sex drawn:
auto b = gen.get_biodata({.country = "BR", .age = 67});

// Deterministic (const) form:
auto d = gen.get_biodata({.country = "BR", .bio_sex = dasmig::sex::female, .age = 8}, std::uint64_t{42});
```

An empty `country` picks a random one. An age above 120 throws `std::invalid_argument`. Both `get_biodata({}, seed)` and `get_biodata(dasmig::biodata_request{}, seed)` select a random country and sex without age effects.

Age is applied after the base draws, so the same country, sex and seed give the same underlying person at every age. Omitting age preserves the existing adult-model seeded output. The curves are simplified population-level models:

| Trait | Effect of age |
|-------|---------------|
| Height | Growth curve to adult height by 16 (girls) or 18 (boys); about 0.7% lost per decade after 50 |
| BMI / weight | Factor on the adult mean: child dip around 5, rebound through the teens, rise to about 50, decline after 70 |
| Hair | Turns `grey` with a probability of 0 before 25, 50% at 55 and rising after |

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
| `hair` | `dasmig::hair_color` | Hair colour (black/brown/blond/red, grey when an age is given) |
| `skin` | `dasmig::skin_type` | Fitzpatrick skin type (I–VI) |
| `blood` | `dasmig::blood_type` | ABO/Rh blood type |
| `hand` | `dasmig::handedness` | Handedness (right/left) |
| `age` | `std::optional<std::uint8_t>` | Age in years, set only when requested (also in JSON) |

Additionally:
- `seed()` — the random seed used for this generation (for replay).
- `to_string()` — human-readable summary.
- Implicit `operator std::string()` and `operator<<`.

## Typed Enumerations

All phenotypic traits use typed enumerations. String conversion helpers are static methods on `biodata`:

```cpp
auto b = gen.get_biodata({.country = "US"});

std::cout << dasmig::biodata::eye_color_str(b.eyes) << "\n";
std::cout << dasmig::biodata::hair_color_str(b.hair) << "\n";
std::cout << dasmig::biodata::skin_type_str(b.skin) << "\n";
std::cout << dasmig::biodata::blood_type_str(b.blood) << "\n";
std::cout << dasmig::biodata::handedness_str(b.hand) << "\n";
std::cout << dasmig::biodata::sex_str(b.bio_sex) << "\n";
```

### sex

| Value | Label |
|-------|-------|
| `male` | "male" |
| `female` | "female" |

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
| `grey` | "grey" (only generated when an age is given) |

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
auto us = gen.get_biodata({.country = "US"});
auto jp = gen.get_biodata({.country = "JP"});
auto ng = gen.get_biodata({.country = "NG"});
```

Different countries produce different distributions. For example, the Netherlands produces taller individuals on average than Guatemala, and Japan produces predominantly brown-eyed individuals while Scandinavian countries produce more blue-eyed individuals.

## Seeding and Deterministic Generation

`get_biodata(request)` takes a fresh seed from `std::random_device` for each call. For deterministic output, pass the seed explicitly. Replay requires the same request, seed and loaded data.

### Per-Call Seed

```cpp
auto b = gen.get_biodata({.country = "US"}, std::uint64_t{42});
// Same request, seed and data → same result.
```

### Seed Replay

```cpp
auto b1 = gen.get_biodata({.country = "US"});
auto b2 = gen.get_biodata({.country = "US"}, b1.seed());
// b1 and b2 are identical.
```

### Deterministic Sequences

```cpp
const dasmig::biodata_request request{.country = "US"};
for (std::uint64_t seed = 100; seed < 103; ++seed)
    std::cout << gen.get_biodata(request, seed) << '\n';
```

Repeat the same per-call seeds to replay the sequence. Generator-level seeding is not part of this API; callers can also use their own `dasmig::core::rng` and pass each drawn seed explicitly.

## Multi-Instance Support

```cpp
dasmig::bdg gen1;
gen1.load(dasmig::dataset::lite);

dasmig::bdg gen2;
gen2.load(dasmig::dataset::full);

// gen1 and gen2 are independent.
auto a = gen1.get_biodata({.country = "US"});
auto b = gen2.get_biodata({.country = "US"});
```

## Generation Pipeline

Each call to `get_biodata()` runs this pipeline. Every step draws from its own stream, split from the call seed with a label (`"sex"`, `"height"`, ...), using the [dasmig-core](https://github.com/dasmig/dasmig-core) distributions, so the result is identical on every platform:

1. **Sex** — 50/50 Bernoulli, or fixed by `request.bio_sex`.
2. **Height** — normal with the country/sex mean and SD, clamped to ±4σ.
3. **BMI** — log-normal with the country/sex mean and the country SD, clamped to [14, 55].
4. **Weight** — derived: `BMI × (height_m)²`.
5. **Eye Colour** — weighted choice over 3 categories.
6. **Hair Colour** — weighted choice over 4 categories.
7. **Skin Type** — weighted choice over 6 Fitzpatrick types.
8. **Blood Type** — weighted choice over 8 ABO/Rh groups.
9. **Handedness** — Bernoulli with the country's left-handedness rate.

Because each step has its own stream, forcing the sex changes nothing else: `get_biodata({.country = "US", .bio_sex = sex::male}, seed)` equals `get_biodata({.country = "US"}, seed)` whenever the latter is male. A country chosen at random (`get_biodata({}, seed)`) is drawn uniformly from the `"country"` stream.

## Data Pipeline

The resource TSV files are built from raw data using Python scripts in `scripts/`. [Data and Method](data.md) describes every column's source and method, how to rebuild the files, and what the model leaves out.

## Extending Data

The shipped data is a base layer. Files with the same columns as `biodata.tsv` can be loaded on top of it to add countries, change them or remove them, without editing the shipped file. Rows are keyed by `country_code`.

```cpp
auto& gen = dasmig::bdg::instance();

// Override mode (the default): a row replaces the loaded row for its
// country; new countries are added.
gen.extend("mods/tall_people.tsv");

// From text, for example supplied by a WebAssembly host or a game scenario.
gen.extend_from_string(text, "scenario: drowned coast");

// Remove countries: a layer needs only the key and a disabled column.
gen.extend_from_string("country_code\tdisabled\nQQ\t1\n", "cleanup");

// Add mode refuses to touch countries that are already loaded.
gen.extend("mods/new_countries.tsv", dasmig::core::layer_mode::add);
```

- Every value in a layer is checked (numbers, positive heights and standard deviations, a percentage between 0 and 100, at least one non-zero weight per trait group) before anything changes. A bad layer throws `dasmig::core::tsv_error` with the file, line and column, and the data stays as it was.
- `layers()` lists what is loaded: source, mode, rows added, replaced and removed, and warnings such as unknown columns.
- `load(...)` replaces the base and drops every layer.

## Replay and JSON

Every result records the seed and the data it came from:

```cpp
auto b = gen.get_biodata({.country = "BR"});
b.seed();                    // the call seed
b.signature().data;          // gen.data_hash() when it was generated

// Same request, seed and data: the same biodata on every compiler and platform.
auto again = gen.get_biodata({.country = "BR"}, b.seed());

if (b.signature().data != gen.data_hash())
{
    // Different layers are loaded now; the replay would differ.
}
```

`to_json()` gives the result as JSON, the form the WebAssembly layer and the game consume:

```json
{"country":"US","sex":"male","height_cm":173.5172709253195,"weight_kg":91.22083570504105,
 "bmi":30.297628050803606,"eyes":"blue","hair":"blond","skin":"III","blood":"O+","hand":"right",
 "signature":{"seed":"0000000000000001","data":"076d859c658fef72"}}
```

Seeds are strings because JavaScript numbers cannot hold every 64-bit value. `to_json(writer)` writes into an existing `dasmig::core::json_writer`, for composing larger documents.

## Compiled-in Data

Define `DASMIG_BIODATA_EMBED` before including the header to compile the `lite` tier into the program (about 20 KB). `load(dataset::lite)` and `instance()` then use it and need no files, which is what a WebAssembly build wants. The compiled-in data has the same `data_hash()` as the file, so seeds replay the same either way.

```cpp
#define DASMIG_BIODATA_EMBED
#include <dasmig/biodatagen.hpp>
```

`dasmig/biodatagen_data.hpp` is generated from `resources/lite` by `make embed`; CI checks it is up to date.

## Migrating to the Request API

This is a breaking API standardization. Rename `biodata_options` to `biodata_request`; its fields and defaults are unchanged. Replace positional calls with requests:

| Previous call | Request-first call |
|---------------|--------------------|
| `get_biodata()` | `get_biodata({})` |
| `get_biodata("US")` | `get_biodata({.country = "US"})` |
| `get_biodata(sex::female)` | `get_biodata({.bio_sex = sex::female})` |
| `get_biodata("US", sex::male, seed)` | `get_biodata({.country = "US", .bio_sex = sex::male}, seed)` |
| `get_biodata(seed)` | `get_biodata({}, seed)` |

The old overloads, type name and `bdg::seed()` / `bdg::unseed()` are removed, not deprecated aliases. All generation calls are now `const` and use no mutable generator defaults or random engine. To preserve an old seeded sequence, own a `dasmig::core::rng` in the caller and pass its draws as per-call seeds. `biodata::seed()` remains the result replay accessor.

For the same request, per-call seed and data, output and JSON signatures remain unchanged, including age effects. Resource loading, tiers, extensions, layers and data hashes remain instance-level and unchanged.

## Migrating from v1

Version 2 moves the library onto [dasmig-core](https://github.com/dasmig/dasmig-core).

- **Every generated value changes.** The engine (xoshiro256\*\*) and the distributions are now the core's own, so a seed gives the same result on every compiler, standard library and CPU, which `std::` distributions could not promise. Seeds saved with v1 do not reproduce their v1 results. The distributions themselves are unchanged (checked against v1 over 20,000 draws per scenario).
- **Forcing the sex no longer shifts the other draws.** Each trait draws from its own stream split from the call seed, so `get_biodata({.country = "US", .bio_sex = sex::male}, s)` equals `get_biodata({.country = "US"}, s)` whenever the latter is male.
- **Files:** replace `random.hpp` with `core.hpp` and the `core/` directory (see Installation).
- **Errors:** a malformed `biodata.tsv` now throws `dasmig::core::tsv_error` instead of skipping rows silently. A missing directory is still ignored by `load(path)`.
- **New:** `extend()`, `extend_from_string()`, `layers()`, `data_hash()`, `biodata::signature()`, `biodata::to_json()`, `DASMIG_BIODATA_EMBED`.

## Thread Safety

Each `bdg` instance is independent. Both generation calls are `const` and may run concurrently on the **same** instance while its data is unchanged.

Loading or extending data requires external synchronisation with generation and other access. This also applies to the shared singleton (`bdg::instance()`).

## Batch Generation

Every seeded getter has batch forms: `get_biodata_batch` return a `std::vector`, and `for_each_biodata` streams the items to a sink in index order without holding the batch.

```cpp
auto& gen = dasmig::bdg::instance();
const auto request = dasmig::biodata_request{};

// 1,000 results from one batch seed, on four threads.
auto items = gen.get_biodata_batch(request, {.count = 1000, .threads = 4}, 42);

// Item 1 replays on its own: the batch seed and the index give its seed.
auto again = gen.get_biodata(request, dasmig::core::item_seed(42, 1));

// A million results to a file as JSON Lines, never all in memory.
std::ofstream out{"items.jsonl"};
dasmig::core::jsonl_writer lines{out};
gen.for_each_biodata(request, {.count = 1'000'000, .threads = 0}, 42, lines);
```

- **Item seeds.** Item `i` is exactly `get_biodata(request, core::item_seed(batch_seed, first + i))`. A range can be generated directly (`first = 1000` starts at item 1000), two batches that follow each other equal one, and the output is the same for any `threads` (0 means the hardware count). Without a `batch_seed` one is drawn like the single getter draws its own.
- **Options.** `core::batch_options` has `count`, `first`, `threads` and `unique`. The request describes one item; the options describe the batch.
- **Uniqueness.** `.unique = core::unique_rule::by<biodata>(key)` regenerates an item whose key already appeared in the batch, with a deterministic retry seed, and throws `std::runtime_error` naming the item when the attempts run out.
- **Sinks.** `core::jsonl_writer` writes one `to_json()` per line, `core::csv_writer` flattens each result's JSON into dotted columns. Any callable `(std::uint64_t index, biodata&)` works.
- Batch calls use the same const getters from several threads, which the thread-safety rules above allow; the CI runs the batch tests under ThreadSanitizer.

## Error Reference

| Exception | When |
|-----------|------|
| `std::runtime_error("No biodata loaded. Call load() first.")` | Any generation call before `load()` |
| `std::invalid_argument("Unknown country code: XX")` | Unknown ISO alpha-2 code |
| `std::invalid_argument("Age out of range (0-120): N")` | Request age exceeds 120 |
| `dasmig::core::tsv_error` | A malformed `biodata.tsv` or layer (message gives file, line and column); the data is unchanged |
