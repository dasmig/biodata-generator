#!/usr/bin/env python3
"""Validate biodata TSV resource files."""

import csv
import os

for tier in ['lite', 'full']:
    print(f'=== {tier} tier ===')
    path = os.path.join('resources', tier, 'biodata.tsv')
    with open(path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f, delimiter='\t')
        rows = list(reader)
    print(f'  Countries: {len(rows)}, Columns: {len(reader.fieldnames)}')
    
    # Show sample entries
    samples = ['US', 'BR', 'JP', 'DE', 'NG', 'IN', 'AU']
    for s in samples:
        row = next((r for r in rows if r['country_code'] == s), None)
        if row:
            name = row['country_name'][:20]
            mh = row['male_height_cm']
            fh = row['female_height_cm']
            mb = row['male_bmi_mean']
            fb = row['female_bmi_mean']
            eb = row['eye_blue']
            ei = row['eye_intermediate']
            ebr = row['eye_brown']
            lh = row['left_handed_pct']
            print(f'  {s} {name:20s}: H={mh}/{fh}cm BMI={mb}/{fb} '
                  f'Eye={eb}/{ei}/{ebr} Left={lh}%')
    print()
    
    # Sanity checks
    issues = 0
    for row in rows:
        cc = row['country_code']
        
        # Eye color sums
        eye_sum = float(row['eye_blue']) + float(row['eye_intermediate']) + float(row['eye_brown'])
        if abs(eye_sum - 100) > 1:
            print(f'  WARN: {cc} eye sum = {eye_sum}')
            issues += 1
        
        # Hair color sums
        hair_sum = (float(row['hair_black']) + float(row['hair_brown']) + 
                    float(row['hair_blond']) + float(row['hair_red']))
        if abs(hair_sum - 100) > 1:
            print(f'  WARN: {cc} hair sum = {hair_sum}')
            issues += 1
        
        # Skin tone sums
        skin_sum = sum(int(row[f'skin_{t}']) for t in ['I', 'II', 'III', 'IV', 'V', 'VI'])
        if abs(skin_sum - 100) > 1:
            print(f'  WARN: {cc} skin sum = {skin_sum}')
            issues += 1
        
        # Blood type sums
        bt_keys = ['blood_O_pos', 'blood_A_pos', 'blood_B_pos', 'blood_AB_pos',
                    'blood_O_neg', 'blood_A_neg', 'blood_B_neg', 'blood_AB_neg']
        bt_sum = sum(float(row[k]) for k in bt_keys)
        if abs(bt_sum - 100) > 5:
            print(f'  WARN: {cc} blood sum = {bt_sum}')
            issues += 1
        
        # Height sanity
        mh = float(row['male_height_cm'])
        fh = float(row['female_height_cm'])
        if mh < 150 or mh > 195 or fh < 140 or fh > 180:
            print(f'  WARN: {cc} unusual height M={mh} F={fh}')
            issues += 1
    
    if issues == 0:
        print('  All sanity checks passed!')
    else:
        print(f'  {issues} issues found')
    print()
