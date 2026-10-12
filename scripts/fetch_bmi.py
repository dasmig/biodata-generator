#!/usr/bin/env python3
"""Find OWID BMI indicator IDs and download BMI data."""

import urllib.request
import json
import re
import os

RAW = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'raw_data')
UA = {'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64)'}

# Try various OWID BMI chart URLs
chart_slugs = [
    'mean-body-mass-index-bmi-in-adult-males',
    'mean-bmi-male-vs-female',
    'body-mass-index-bmi-in-adult-males-vs-adult-females',
    'mean-bmi-in-adult-males',
    'average-bmi-men',
    'mean-bmi-adults',
    'mean-body-mass-index',
    'BMI-by-country',
    'share-of-adults-who-are-overweight',
    'mean-bmi-in-adult-males-vs-mean-bmi-in-adult-females',
]

for slug in chart_slugs:
    url = f'https://ourworldindata.org/grapher/{slug}'
    print(f'Trying: {slug}...')
    try:
        req = urllib.request.Request(url, headers=UA)
        resp = urllib.request.urlopen(req, timeout=10)
        html = resp.read().decode('utf-8')
        ids = re.findall(r'/v1/indicators/(\d+)\.data\.json', html)
        if ids:
            title_match = re.search(r'<title>([^<]+)</title>', html)
            title = title_match.group(1)[:80] if title_match else '?'
            print(f'  OK! IDs: {ids}, Title: {title}')

            # Fetch each indicator
            for iid in ids:
                meta_url = f'https://api.ourworldindata.org/v1/indicators/{iid}.metadata.json'
                meta_req = urllib.request.Request(meta_url, headers=UA)
                meta_resp = urllib.request.urlopen(meta_req, timeout=10)
                meta = json.loads(meta_resp.read())
                name = meta.get('name', meta.get('display', {}).get('name', '?'))
                print(f'    Indicator {iid}: {name}')

            # If we found BMI indicators, download them
            for iid in ids:
                iid_int = int(iid)
                data_url = f'https://api.ourworldindata.org/v1/indicators/{iid_int}.data.json'
                meta_url = f'https://api.ourworldindata.org/v1/indicators/{iid_int}.metadata.json'

                data_req = urllib.request.Request(data_url, headers=UA)
                data_resp = urllib.request.urlopen(data_req, timeout=15)
                data = json.loads(data_resp.read())

                meta_req = urllib.request.Request(meta_url, headers=UA)
                meta_resp = urllib.request.urlopen(meta_req, timeout=15)
                meta = json.loads(meta_resp.read())

                # Save
                with open(os.path.join(RAW, f'bmi_indicator_{iid}_data.json'), 'w') as f:
                    json.dump(data, f)
                with open(os.path.join(RAW, f'bmi_indicator_{iid}_metadata.json'), 'w') as f:
                    json.dump(meta, f)

                name = meta.get('name', '?')
                n_vals = len(data.get('values', []))
                print(f'    Saved indicator {iid} ({name}): {n_vals} values')

            break
    except urllib.error.HTTPError as e:
        if e.code == 404:
            print(f'  404')
        else:
            print(f'  HTTP {e.code}')
    except Exception as e:
        print(f'  Error: {e}')

print('\nDone!')
