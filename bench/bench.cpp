// Benchmarks for biodata-generator; see bench.hpp for the options.

#include "../dasmig/biodatagen.hpp"
#include "bench.hpp"

int main(int argc, char** argv)
{
    dasmig::bench::runner run{"biodata-generator", argc, argv};

    run.load("load", [&] {
        dasmig::bdg gen;
        return gen.load(run.tier());
    });

    dasmig::bdg gen;
    if (!gen.load(run.tier())) return run.missing_tier();

    run.calls("get_biodata", [&](std::uint64_t seed) { return gen.get_biodata({}, seed); });
    run.calls("get_biodata BR female 30", [&](std::uint64_t seed) {
        return gen.get_biodata({.country = "BR", .bio_sex = dasmig::sex::female, .age = 30}, seed);
    });
    run.calls("get_biodata to_json", [&](std::uint64_t seed) {
        return gen.get_biodata({}, seed).to_json();
    });

    run.batches("get_biodata_batch", [&](std::size_t count, std::size_t threads, std::uint64_t seed) {
        return gen.get_biodata_batch({}, {.count = count, .threads = threads}, seed);
    });

    return run.finish();
}
