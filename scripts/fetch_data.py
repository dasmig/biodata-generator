#!/usr/bin/env python3
"""Fetch raw data sources for biodata-generator."""

import urllib.request
import json
import os
import csv
import sys
import time

RAW = os.path.join(os.path.dirname(__file__), '..', 'raw_data')
os.makedirs(RAW, exist_ok=True)

HEADERS = {'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64)',
           'Referer': 'https://ncdrisc.org/'}


def download(url, filename, headers=None):
    """Download a URL to raw_data/filename. Returns True on success."""
    outpath = os.path.join(RAW, filename)
    if os.path.exists(outpath) and os.path.getsize(outpath) > 100:
        print(f"  [SKIP] {filename} already exists ({os.path.getsize(outpath)} bytes)")
        return True
    print(f"  Downloading {filename}...")
    try:
        hdrs = headers or HEADERS
        req = urllib.request.Request(url, headers=hdrs)
        resp = urllib.request.urlopen(req, timeout=30)
        data = resp.read()
        # Check if we got HTML (error page) instead of CSV
        text = data.decode('utf-8-sig', errors='replace')
        if text.strip().startswith('<!') or text.strip().startswith('<html'):
            print(f"  [FAIL] {filename}: got HTML error page")
            return False
        with open(outpath, 'wb') as f:
            f.write(data)
        lines = text.strip().split('\n')
        print(f"  [OK] {filename}: {len(data)} bytes, {len(lines)} rows")
        print(f"       Header: {lines[0][:120]}")
        return True
    except Exception as e:
        print(f"  [FAIL] {filename}: {e}")
        return False


def preview(filename, n=3):
    """Preview first n lines of a raw_data file."""
    path = os.path.join(RAW, filename)
    if not os.path.exists(path):
        return
    with open(path, 'r', encoding='utf-8-sig') as f:
        for i, line in enumerate(f):
            if i >= n:
                break
            print(f"  {line.rstrip()[:150]}")


def fetch_ncdrisc():
    """Fetch NCD-RisC height and BMI data."""
    print("\n=== NCD-RisC Height & BMI ===")

    # Global child/adolescent height (ages 5-19) - this one works
    download(
        'https://ncdrisc.org/downloads/bmi-height-2020/height/global/'
        'NCD_RisC_Lancet_2020_height_child_adolescent_global.csv',
        'ncdrisc_height_child_global.csv'
    )

    # Try country-level patterns
    country_patterns = [
        'https://ncdrisc.org/downloads/bmi-height-2020/height/countries/'
        'NCD_RisC_Lancet_2020_height_child_adolescent_countries.csv',
        'https://ncdrisc.org/downloads/bmi-height-2020/height/country_all/'
        'NCD_RisC_Lancet_2020_height_child_adolescent_country_all.csv',
    ]
    for url in country_patterns:
        fn = url.split('/')[-1]
        if download(url, fn):
            break

    # Adult height (eLife 2016 - age 18 attained height by birth cohort)
    adult_patterns = [
        'https://ncdrisc.org/downloads/height/NCD_RisC_eLife_2016_height_age18_global.csv',
        'https://ncdrisc.org/downloads/height/NCD_RisC_eLife_2016_height_age_standardised_global.csv',
        'https://ncdrisc.org/downloads/height/NCD_RisC_eLife_2016_height_mean_global.csv',
    ]
    for url in adult_patterns:
        fn = url.split('/')[-1]
        download(url, fn)

    # BMI adult (Lancet 2016/2017)
    bmi_patterns = [
        'https://ncdrisc.org/downloads/bmi/NCD_RisC_Lancet_2017_BMI_age_standardised_global.csv',
        'https://ncdrisc.org/downloads/bmi/NCD_RisC_Nature_2019_BMI_age_standardised_global.csv',
    ]
    for url in bmi_patterns:
        fn = url.split('/')[-1]
        download(url, fn)


def fetch_owid_height():
    """Try OWID GitHub catalog for NCD-RisC height data."""
    print("\n=== OWID Height/BMI Data ===")

    # OWID catalog
    owid_urls = [
        ('owid_mean_height_men.csv',
         'https://raw.githubusercontent.com/owid/owid-datasets/master/datasets/'
         'Mean%20male%20height%20-%20Various%20sources/Mean%20male%20height%20-%20Various%20sources.csv'),
        ('owid_human_height.csv',
         'https://raw.githubusercontent.com/owid/owid-datasets/master/datasets/'
         'Human%20Height%20-%20Clio%20Infra%20(2014)/Human%20Height%20-%20Clio%20Infra%20(2014).csv'),
        ('owid_share_adult_overweight.csv',
         'https://raw.githubusercontent.com/owid/owid-datasets/master/datasets/'
         'Share%20of%20adults%20that%20are%20overweight%20-%20WHO%20(2017)/Share%20of%20adults%20that%20are%20overweight%20-%20WHO%20(2017).csv'),
    ]
    for fn, url in owid_urls:
        download(url, fn, {'User-Agent': 'Mozilla/5.0'})


def fetch_blood_type():
    """Fetch blood type distribution data."""
    print("\n=== Blood Type Data ===")

    # Try known GitHub repos with blood type data
    urls = [
        ('blood_type_beduka.csv',
         'https://raw.githubusercontent.com/Beduka/blood-type-world/main/data/blood_type_by_country.csv'),
        ('blood_type_freq.csv',
         'https://raw.githubusercontent.com/datasets/blood-type-distribution/main/data/blood-type-distribution.csv'),
    ]
    for fn, url in urls:
        download(url, fn, {'User-Agent': 'Mozilla/5.0'})


def fetch_eye_color():
    """Fetch eye color distribution data by country."""
    print("\n=== Eye Color Data ===")
    # Katsara & Nothnagel (2019) data from various curated sources
    urls = [
        ('eye_color_world.csv',
         'https://raw.githubusercontent.com/datasets/eye-color-distribution/main/data/eye-color-distribution.csv'),
    ]
    for fn, url in urls:
        download(url, fn, {'User-Agent': 'Mozilla/5.0'})


def github_search(query, per_page=5):
    """Search GitHub repositories."""
    url = f'https://api.github.com/search/repositories?q={urllib.parse.quote(query)}&sort=stars&per_page={per_page}'
    req = urllib.request.Request(url, headers={
        'User-Agent': 'Mozilla/5.0',
        'Accept': 'application/vnd.github.v3+json'
    })
    try:
        resp = urllib.request.urlopen(req, timeout=15)
        return json.loads(resp.read())
    except Exception as e:
        print(f"  Search error: {e}")
        return {'items': [], 'total_count': 0}


def search_datasets():
    """Search GitHub for relevant datasets."""
    print("\n=== GitHub Dataset Search ===")
    import urllib.parse

    queries = [
        'blood type distribution country csv',
        'human height country mean csv data',
        'eye color distribution country',
        'hair color distribution country',
        'handedness left prevalence country',
    ]
    for q in queries:
        print(f"\nQuery: '{q}'")
        result = github_search(q)
        print(f"  Found: {result.get('total_count', 0)} repos")
        for item in result.get('items', [])[:3]:
            print(f"  - {item['full_name']} ({item['stargazers_count']}*)")
            desc = (item.get('description') or '')[:100]
            if desc:
                print(f"    {desc}")
        time.sleep(2)  # avoid rate limit


def check_wikipedia_blood_types():
    """Fetch blood type data from Wikipedia tables (well-known source)."""
    print("\n=== Wikipedia Blood Type Table ===")
    url = 'https://en.wikipedia.org/w/api.php?action=parse&page=Blood_type_distribution_by_country&prop=wikitext&format=json'
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        resp = urllib.request.urlopen(req, timeout=15)
        data = json.loads(resp.read())
        wikitext = data.get('parse', {}).get('wikitext', {}).get('*', '')
        print(f"  Got wikitext: {len(wikitext)} chars")
        # Save for parsing
        with open(os.path.join(RAW, 'wikipedia_blood_type_wikitext.txt'), 'w', encoding='utf-8') as f:
            f.write(wikitext)
        # Show first table row
        lines = wikitext.split('\n')
        table_lines = [l for l in lines if l.startswith('|') or l.startswith('!')]
        print(f"  Table rows: {len(table_lines)}")
        for l in table_lines[:5]:
            print(f"  {l[:120]}")
    except Exception as e:
        print(f"  Error: {e}")


def list_downloaded():
    """List all downloaded files."""
    print("\n=== Downloaded Files ===")
    for fn in sorted(os.listdir(RAW)):
        path = os.path.join(RAW, fn)
        if os.path.isfile(path):
            size = os.path.getsize(path)
            print(f"  {fn}: {size:,} bytes")


if __name__ == '__main__':
    mode = sys.argv[1] if len(sys.argv) > 1 else 'all'

    if mode in ('all', 'ncdrisc'):
        fetch_ncdrisc()
    if mode in ('all', 'owid'):
        fetch_owid_height()
    if mode in ('all', 'blood'):
        fetch_blood_type()
        check_wikipedia_blood_types()
    if mode in ('all', 'eye'):
        fetch_eye_color()
    if mode in ('all', 'search'):
        search_datasets()

    list_downloaded()
