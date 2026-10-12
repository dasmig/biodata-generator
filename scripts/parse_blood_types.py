#!/usr/bin/env python3
"""Parse blood type data from Wikipedia wikitext."""

import re
import json
import os

RAW = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'raw_data')


def parse_blood_types():
    with open(os.path.join(RAW, 'wikipedia_blood_type_wikitext.txt'), 'r', encoding='utf-8') as f:
        text = f.read()

    lines = text.split('\n')
    results = {}
    country_name = None
    i = 0
    while i < len(lines):
        line = lines[i].strip()

        # Country name line: ! style="text-align:left" | {{flag|CountryName}}
        flag_match = re.search(r'\{\{flag\|([^}|]+)', line)
        if flag_match and line.startswith('!'):
            country_name = flag_match.group(1).strip()
            i += 1
            continue

        # Data line with percentages: | population || pct% || pct% ...
        if country_name and line.startswith('|') and '||' in line and '%' in line:
            # Extract all percentage values
            # Format: | value || style="..."|pct% || style="..."|pct% || ...
            # Or:     | value || pct% || pct% || ...

            # Split by ||
            parts = line.split('||')

            # Extract percentages from each cell
            pcts = []
            for part in parts:
                # Find percentage value: could be just "30.0%" or "style=...|30.0%"
                pct_match = re.search(r'([\d.]+)\s*%', part)
                if pct_match:
                    try:
                        pcts.append(float(pct_match.group(1)))
                    except ValueError:
                        pass

            # We need exactly 8 percentages: O+, A+, B+, AB+, O-, A-, B-, AB-
            if len(pcts) == 8:
                total = sum(pcts)
                if 90 < total < 110:  # sanity check
                    results[country_name] = {
                        'O+': pcts[0], 'A+': pcts[1], 'B+': pcts[2], 'AB+': pcts[3],
                        'O-': pcts[4], 'A-': pcts[5], 'B-': pcts[6], 'AB-': pcts[7],
                    }
            country_name = None  # reset for next country

        i += 1

    return results


if __name__ == '__main__':
    blood = parse_blood_types()
    print(f'Parsed {len(blood)} countries')

    # Show first 10
    for name, dist in list(blood.items())[:10]:
        total = sum(dist.values())
        print(f'  {name:30s} O+={dist["O+"]:5.1f}  A+={dist["A+"]:5.1f}  '
              f'B+={dist["B+"]:5.1f}  AB+={dist["AB+"]:5.1f}  '
              f'O-={dist["O-"]:5.1f}  A-={dist["A-"]:5.1f}  '
              f'B-={dist["B-"]:5.1f}  AB-={dist["AB-"]:5.1f}  '
              f'(sum={total:.1f})')

    # Save
    out = os.path.join(RAW, 'blood_type_by_country.json')
    with open(out, 'w', encoding='utf-8') as f:
        json.dump(blood, f, indent=2, ensure_ascii=False)
    print(f'\nSaved to {out} ({os.path.getsize(out):,} bytes)')
