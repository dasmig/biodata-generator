# Data and Method

Where the numbers in `resources/*/biodata.tsv` come from, how they are built, and what the model leaves out. For using the library, see the [usage guide](usage.md).

[TOC]

## Tiers

| Tier | Countries | Which |
| --- | --- | --- |
| `lite` | 111 | Countries with height data and a specific (non-default) value for at least one of eye colour, hair colour, skin type or blood type |
| `full` | 197 | Every country with height data; traits without country data use a regional default |

## Columns

| Columns | Source | Method |
| --- | --- | --- |
| `male_height_cm`, `female_height_cm` | NCD Risk Factor Collaboration (NCD-RisC) via Our World in Data, indicators 1206196 and 1206195 | Latest mean adult height per country |
| `male_height_sd`, `female_height_sd` | Derived, not measured | Men: `6.5 + 0.02 × (mean − 170)`, clamped to 5.5–8.0 cm. Women: `6.0 + 0.02 × (mean − 160)`, clamped to 5.0–7.5 cm |
| `male_bmi_mean`, `female_bmi_mean` | WHO Global Health Observatory via Our World in Data, indicators 1054174 and 1054175 | Latest mean BMI per country (age-standardised, adults 18+); 24.5 (men) and 25.0 (women) where missing |
| `bmi_sd` | Fixed | 4.5 for every country |
| `eye_*` (blue, intermediate, brown) | Katsara & Nothnagel (2019) and the VISAGE consortium for 7 countries; estimates for 63 more | Percentages, curated in `scripts/fetch_all_data.py` with the source of each entry; regional defaults elsewhere |
| `hair_*` (black, brown, blond, red) | VISAGE for 1 country; estimates for 61 more | As eye colour |
| `skin_I` … `skin_VI` | Estimates (81 countries) from UV exposure and population composition | Fitzpatrick type percentages; regional defaults elsewhere |
| `blood_*` (ABO and Rh) | Wikipedia's "Blood type distribution by country" (124 entries) | Parsed from the article's wikitext; normalised to 100%; a global default elsewhere |
| `left_handed_pct` | Papadatou-Pastou et al. (2020) meta-analysis (75 countries) | Percentage left-handed; 10.6% (the global estimate) where missing |

Regional defaults come from `REGION_DEFAULTS` and `COUNTRY_REGION` in `scripts/prepare_biodata.py`.

**How reliable each column is.** Height and BMI means are measured national statistics. Height and BMI spreads are rules of thumb. Most eye, hair and skin values are estimates, so treat them as plausible rather than measured.

## Generation Model

- **Height** is normal with the country and sex mean and SD, clamped to ±4 SD. Height is close to normal within a population because many genetic and environmental factors add up.
- **BMI** is log-normal, because population BMI is right-skewed: the upper tail has lengthened over the decades while the lower bound has not moved. **Weight** is BMI × height².
- **Skin, hair and eyes** are drawn together from one joint distribution (below), so they go together. **Blood type and handedness** are drawn independently from the country's percentages.
- **The joint distribution** starts from a table of how well each skin, hair and eye combination fits: the pairs' darkness is compared (fair skin with light hair and light eyes, dark with dark), and red hair is pulled towards the fairest skin. The table is then rescaled by iterative proportional fitting until its skin, hair and eye totals equal the country's percentages. Each trait therefore keeps the country's shares exactly; only how they pair up changes. In the shipped US data, 71% of blond people have blue eyes and 11% brown, where independent draws would give 27% and 55%; a trait a country has none of (blond hair in Japan) stays absent. The strengths of the associations are authored to match what the pigmentation literature reports (for example Katsara & Nothnagel 2019 for eye and hair colour), not fitted to a joint survey, so treat the pairings as plausible rather than measured.
- **Handedness** follows the expressed rate, which varies a lot between countries (for example much lower in East Asia, where left-handed children were often made to switch), rather than the roughly 10% biological rate.

The [usage guide](usage.md) ("Generation Pipeline") gives the exact steps.

## Rebuilding the Data

The scripts in `scripts/` rebuild both tiers. The raw downloads (`raw_data/`, about 2 MB) are not in the repository.

1. `fetch_all_data.py`: downloads height and BMI from Our World in Data and the blood-type article from Wikipedia, writes the curated eye, hair, skin and handedness tables, and adds the derived height SDs. Writes `raw_data/`.
2. `parse_blood_types.py`: parses the blood-type wikitext.
3. `fetch_bmi.py`: finds the BMI indicator IDs on Our World in Data (used once to locate them).
4. `prepare_biodata.py --tier both`: merges everything by ISO country code, fills gaps with regional defaults and writes `resources/lite` and `resources/full`. From the saved raw downloads it reproduces the shipped files byte for byte.
5. `validate_data.py`: sanity checks on the output.

`fetch_data.py` and `analyze_height.py` are earlier exploration scripts, kept for reference.

After changing the data, run `make embed` to regenerate the compiled-in copy, and update the known-answer tests (the data hash and results change).

## Known Limitations

These are what the model leaves out, with the sources a future version could use.

- **Height by age uses a curve, not cohort data.** An age shrinks height and shifts BMI along fixed curves. NCD-RisC publishes height by birth cohort, which would replace the curve with measured values.
- **The pigmentation pairings are authored, not measured.** Skin, hair and eyes are drawn together (see the generation model), but the strength of each association is a judgement matched to the literature, because no public survey gives their joint distribution by country. A joint survey would replace the authored strengths.
- **Skin is categorical.** Fitzpatrick types are coarse, and self-reported types often do not follow ethnicity. A continuous measure such as the Individual Typology Angle (from CIE L\*a\*b\* skin colour) would give smooth skin tones.
- **Handedness is expressed only.** An option for the biological rate (about 10% everywhere) would suit some uses.
- **No other body dimensions.** The US Army ANSUR II survey (public, 93 measurements) could derive proportions from height and weight; that belongs in `appearance-generator`.

Ideas for other libraries from the same research: personality norms by country (Big Five surveys) for `trait-generator`, and the World Bank Human Capital Index for education and occupation in `occupation-generator`.
