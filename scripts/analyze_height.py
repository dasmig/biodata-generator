#!/usr/bin/env python3
"""Analyze OWID height data structure."""

import json
import os

RAW = os.path.join(os.path.dirname(__file__), '..', 'raw_data')

# Load male height data
with open(os.path.join(RAW, 'height_indicator_1206196_data.json')) as f:
    male = json.load(f)

with open(os.path.join(RAW, 'height_indicator_1206195_data.json')) as f:
    female = json.load(f)

with open(os.path.join(RAW, 'height_indicator_1206196_metadata.json')) as f:
    meta_m = json.load(f)

with open(os.path.join(RAW, 'height_indicator_1206195_metadata.json')) as f:
    meta_f = json.load(f)

print('=== Data structure ===')
print('Keys:', list(male.keys()))
print('Male values:', len(male['values']))
print('Female values:', len(female['values']))

# Entity mapping from metadata
entities_meta = meta_m.get('dimensions', {}).get('entities', {}).get('values', [])
print(f'Entity metadata: {len(entities_meta)} entities')

emap = {}
for info in entities_meta:
    emap[info['id']] = (info.get('name', ''), info.get('code', ''))

# Show some entities
print('\n=== Entity samples ===')
for eid, (name, code) in sorted(emap.items())[:10]:
    print(f'  {code:5s} {name}')

# Build latest-year male/female heights per entity
def latest_per_entity(data):
    out = {}
    for i in range(len(data['values'])):
        eid = data['entities'][i]
        yr = data['years'][i]
        val = data['values'][i]
        if eid not in out or yr > out[eid][0]:
            out[eid] = (yr, val)
    return out

latest_m = latest_per_entity(male)
latest_f = latest_per_entity(female)

print(f'\n=== Latest heights ({len(latest_m)} M, {len(latest_f)} F) ===')
print(f'{"Code":5s} {"Country":30s} {"YrM":4s} {"Male":7s} {"YrF":4s} {"Female":7s}')
print('-' * 60)

# Show all entities sorted by code
for eid in sorted(latest_m.keys()):
    name, code = emap.get(eid, ('?', '?'))
    if not code or code == '?':
        continue
    yr_m, val_m = latest_m.get(eid, (0, 0))
    yr_f, val_f = latest_f.get(eid, (0, 0))
    print(f'  {code:5s} {name:30s} {yr_m} {val_m:7.2f}  {yr_f} {val_f:7.2f}')

# Check year range
all_years_m = sorted(set(male['years']))
all_years_f = sorted(set(female['years']))
print(f'\nMale year range: {min(all_years_m)}-{max(all_years_m)}')
print(f'Female year range: {min(all_years_f)}-{max(all_years_f)}')

# Count entries per year for one entity to see granularity
first_eid = list(latest_m.keys())[0]
years_for_first = [(male['years'][i], male['values'][i]) for i in range(len(male['values'])) if male['entities'][i] == first_eid]
years_for_first.sort()
name0, code0 = emap.get(first_eid, ('?', '?'))
print(f'\nAll years for {code0} ({name0}): {len(years_for_first)} entries')
for yr, val in years_for_first[:5]:
    print(f'  {yr}: {val:.2f} cm')
print('  ...')
for yr, val in years_for_first[-3:]:
    print(f'  {yr}: {val:.2f} cm')
