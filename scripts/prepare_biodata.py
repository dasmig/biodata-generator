#!/usr/bin/env python3
"""Build biodata-generator TSV resource files from raw data.

Reads JSON files produced by fetch_all_data.py and parse_blood_types.py,
merges them by ISO alpha-2 country code, and outputs TSV resource files.

Usage:
    python prepare_biodata.py [--tier lite|full|both]

Output:
    resources/lite/biodata.tsv   (~75 countries with best data coverage)
    resources/full/biodata.tsv   (~200 countries, gaps filled with defaults)
"""

import argparse
import json
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
RAW = os.path.join(SCRIPT_DIR, '..', 'raw_data')
RES = os.path.join(SCRIPT_DIR, '..', 'resources')

# ── ISO 3166 alpha-3 → alpha-2 mapping ──────────────────────────────────────
# Only countries that appear in our height data (208 entities)

ISO3_TO_ISO2 = {
    'AFG': 'AF', 'ALB': 'AL', 'DZA': 'DZ', 'AND': 'AD', 'AGO': 'AO',
    'ATG': 'AG', 'ARG': 'AR', 'ARM': 'AM', 'AUS': 'AU', 'AUT': 'AT',
    'AZE': 'AZ', 'BHS': 'BS', 'BHR': 'BH', 'BGD': 'BD', 'BRB': 'BB',
    'BLR': 'BY', 'BEL': 'BE', 'BLZ': 'BZ', 'BEN': 'BJ', 'BTN': 'BT',
    'BOL': 'BO', 'BIH': 'BA', 'BWA': 'BW', 'BRA': 'BR', 'BRN': 'BN',
    'BGR': 'BG', 'BFA': 'BF', 'BDI': 'BI', 'KHM': 'KH', 'CMR': 'CM',
    'CAN': 'CA', 'CPV': 'CV', 'CAF': 'CF', 'TCD': 'TD', 'CHL': 'CL',
    'CHN': 'CN', 'COL': 'CO', 'COM': 'KM', 'COG': 'CG', 'COD': 'CD',
    'CRI': 'CR', 'CIV': 'CI', 'HRV': 'HR', 'CUB': 'CU', 'CYP': 'CY',
    'CZE': 'CZ', 'DNK': 'DK', 'DJI': 'DJ', 'DMA': 'DM', 'DOM': 'DO',
    'ECU': 'EC', 'EGY': 'EG', 'SLV': 'SV', 'GNQ': 'GQ', 'ERI': 'ER',
    'EST': 'EE', 'ETH': 'ET', 'FJI': 'FJ', 'FIN': 'FI', 'FRA': 'FR',
    'GAB': 'GA', 'GMB': 'GM', 'GEO': 'GE', 'DEU': 'DE', 'GHA': 'GH',
    'GRC': 'GR', 'GRD': 'GD', 'GTM': 'GT', 'GIN': 'GN', 'GNB': 'GW',
    'GUY': 'GY', 'HTI': 'HT', 'HND': 'HN', 'HUN': 'HU', 'ISL': 'IS',
    'IND': 'IN', 'IDN': 'ID', 'IRN': 'IR', 'IRQ': 'IQ', 'IRL': 'IE',
    'ISR': 'IL', 'ITA': 'IT', 'JAM': 'JM', 'JPN': 'JP', 'JOR': 'JO',
    'KAZ': 'KZ', 'KEN': 'KE', 'KIR': 'KI', 'PRK': 'KP', 'KOR': 'KR',
    'KWT': 'KW', 'KGZ': 'KG', 'LAO': 'LA', 'LVA': 'LV', 'LBN': 'LB',
    'LSO': 'LS', 'LBR': 'LR', 'LBY': 'LY', 'LTU': 'LT', 'LUX': 'LU',
    'MDG': 'MG', 'MWI': 'MW', 'MYS': 'MY', 'MDV': 'MV', 'MLI': 'ML',
    'MLT': 'MT', 'MHL': 'MH', 'MRT': 'MR', 'MUS': 'MU', 'MEX': 'MX',
    'FSM': 'FM', 'MDA': 'MD', 'MNG': 'MN', 'MNE': 'ME', 'MAR': 'MA',
    'MOZ': 'MZ', 'MMR': 'MM', 'NAM': 'NA', 'NRU': 'NR', 'NPL': 'NP',
    'NLD': 'NL', 'NZL': 'NZ', 'NIC': 'NI', 'NER': 'NE', 'NGA': 'NG',
    'MKD': 'MK', 'NOR': 'NO', 'OMN': 'OM', 'PAK': 'PK', 'PLW': 'PW',
    'PSE': 'PS', 'PAN': 'PA', 'PNG': 'PG', 'PRY': 'PY', 'PER': 'PE',
    'PHL': 'PH', 'POL': 'PL', 'PRT': 'PT', 'QAT': 'QA', 'ROU': 'RO',
    'RUS': 'RU', 'RWA': 'RW', 'KNA': 'KN', 'LCA': 'LC', 'VCT': 'VC',
    'WSM': 'WS', 'STP': 'ST', 'SAU': 'SA', 'SEN': 'SN', 'SRB': 'RS',
    'SYC': 'SC', 'SLE': 'SL', 'SGP': 'SG', 'SVK': 'SK', 'SVN': 'SI',
    'SLB': 'SB', 'SOM': 'SO', 'ZAF': 'ZA', 'SSD': 'SS', 'ESP': 'ES',
    'LKA': 'LK', 'SDN': 'SD', 'SUR': 'SR', 'SWZ': 'SZ', 'SWE': 'SE',
    'CHE': 'CH', 'SYR': 'SY', 'TWN': 'TW', 'TJK': 'TJ', 'TZA': 'TZ',
    'THA': 'TH', 'TLS': 'TL', 'TGO': 'TG', 'TON': 'TO', 'TTO': 'TT',
    'TUN': 'TN', 'TUR': 'TR', 'TKM': 'TM', 'TUV': 'TV', 'UGA': 'UG',
    'UKR': 'UA', 'ARE': 'AE', 'GBR': 'GB', 'USA': 'US', 'URY': 'UY',
    'UZB': 'UZ', 'VUT': 'VU', 'VEN': 'VE', 'VNM': 'VN', 'YEM': 'YE',
    'ZMB': 'ZM', 'ZWE': 'ZW',
    # Territories/others that appear in OWID
    'BMU': 'BM', 'ASM': 'AS', 'GUM': 'GU', 'PRI': 'PR', 'VIR': 'VI',
    'COK': 'CK', 'NIU': 'NU', 'TKL': 'TK',
}

# Country names for the output
COUNTRY_NAMES = {}  # populated from height data


def load_json(filename):
    """Load a JSON file from raw_data/."""
    path = os.path.join(RAW, filename)
    if not os.path.exists(path):
        return {}
    with open(path, 'r', encoding='utf-8') as f:
        return json.load(f)


def load_owid_indicator(indicator_id):
    """Load OWID indicator data and build ISO2-keyed dict of latest values."""
    data = load_json(f'bmi_indicator_{indicator_id}_data.json') or \
           load_json(f'height_indicator_{indicator_id}_data.json')
    meta = load_json(f'bmi_indicator_{indicator_id}_metadata.json') or \
           load_json(f'height_indicator_{indicator_id}_metadata.json')

    if not data or not meta:
        return {}

    # Build entity map
    emap = {}
    for info in meta.get('dimensions', {}).get('entities', {}).get('values', []):
        emap[info['id']] = (info.get('name', ''), info.get('code', ''))

    # Latest value per entity
    latest = {}
    values = data.get('values', [])
    years = data.get('years', [])
    entities = data.get('entities', [])

    for i in range(len(values)):
        eid = entities[i]
        yr = years[i]
        val = values[i]
        if eid not in latest or yr > latest[eid][0]:
            latest[eid] = (yr, val)

    # Convert to ISO2
    result = {}
    for eid, (yr, val) in latest.items():
        name, iso3 = emap.get(eid, ('', ''))
        iso2 = ISO3_TO_ISO2.get(iso3)
        if iso2:
            result[iso2] = val
            COUNTRY_NAMES[iso2] = name

    return result


# Wikipedia blood type uses country names, need name → ISO2 mapping
COUNTRY_NAME_TO_ISO2 = {
    'Afghanistan': 'AF', 'Albania': 'AL', 'Algeria': 'DZ', 'Argentina': 'AR',
    'Armenia': 'AM', 'Australia': 'AU', 'Austria': 'AT', 'Azerbaijan': 'AZ',
    'Bahrain': 'BH', 'Bangladesh': 'BD', 'Belarus': 'BY', 'Belgium': 'BE',
    'Bolivia': 'BO', 'Bosnia and Herzegovina': 'BA', 'Brazil': 'BR',
    'Bulgaria': 'BG', 'Cameroon': 'CM', 'Canada': 'CA', 'Chile': 'CL',
    'China': 'CN', 'Colombia': 'CO', 'Costa Rica': 'CR', 'Croatia': 'HR',
    'Cuba': 'CU', 'Cyprus': 'CY', 'Czech Republic': 'CZ', 'Czechia': 'CZ',
    'Denmark': 'DK', 'Dominican Republic': 'DO', 'Ecuador': 'EC',
    'Egypt': 'EG', 'El Salvador': 'SV', 'Estonia': 'EE', 'Ethiopia': 'ET',
    'Finland': 'FI', 'France': 'FR', 'Georgia': 'GE', 'Germany': 'DE',
    'Ghana': 'GH', 'Greece': 'GR', 'Guatemala': 'GT', 'Honduras': 'HN',
    'Hong Kong': 'HK', 'Hungary': 'HU', 'Iceland': 'IS', 'India': 'IN',
    'Indonesia': 'ID', 'Iran': 'IR', 'Iraq': 'IQ', 'Ireland': 'IE',
    'Israel': 'IL', 'Italy': 'IT', 'Jamaica': 'JM', 'Japan': 'JP',
    'Jordan': 'JO', 'Kazakhstan': 'KZ', 'Kenya': 'KE',
    'South Korea': 'KR', 'Korea, South': 'KR', 'North Korea': 'KP',
    'Kuwait': 'KW', 'Latvia': 'LV', 'Lebanon': 'LB', 'Libya': 'LY',
    'Lithuania': 'LT', 'Luxembourg': 'LU', 'Madagascar': 'MG',
    'Malaysia': 'MY', 'Mali': 'ML', 'Malta': 'MT', 'Mexico': 'MX',
    'Moldova': 'MD', 'Mongolia': 'MN', 'Montenegro': 'ME', 'Morocco': 'MA',
    'Mozambique': 'MZ', 'Myanmar': 'MM', 'Nepal': 'NP',
    'Netherlands': 'NL', 'New Zealand': 'NZ', 'Nicaragua': 'NI',
    'Niger': 'NE', 'Nigeria': 'NG', 'North Macedonia': 'MK', 'Norway': 'NO',
    'Oman': 'OM', 'Pakistan': 'PK', 'Palestine': 'PS', 'Panama': 'PA',
    'Paraguay': 'PY', 'Peru': 'PE', 'Philippines': 'PH', 'Poland': 'PL',
    'Portugal': 'PT', 'Qatar': 'QA', 'Romania': 'RO', 'Russia': 'RU',
    'Rwanda': 'RW', 'Saudi Arabia': 'SA', 'Senegal': 'SN', 'Serbia': 'RS',
    'Singapore': 'SG', 'Slovakia': 'SK', 'Slovenia': 'SI',
    'South Africa': 'ZA', 'Spain': 'ES', 'Sri Lanka': 'LK', 'Sudan': 'SD',
    'Sweden': 'SE', 'Switzerland': 'CH', 'Syria': 'SY', 'Taiwan': 'TW',
    'Tanzania': 'TZ', 'Thailand': 'TH', 'Trinidad and Tobago': 'TT',
    'Tunisia': 'TN', 'Turkey': 'TR', 'Türkiye': 'TR',
    'Uganda': 'UG', 'Ukraine': 'UA',
    'United Arab Emirates': 'AE', 'United Kingdom': 'GB',
    'United States': 'US', 'Uruguay': 'UY', 'Uzbekistan': 'UZ',
    'Venezuela': 'VE', 'Vietnam': 'VN', 'Yemen': 'YE',
    'Zambia': 'ZM', 'Zimbabwe': 'ZW',
    # Territories
    'Bermuda': 'BM', 'Puerto Rico': 'PR',
}


# ── Regional defaults for gap-filling ────────────────────────────────────────

# When a country lacks specific data for a trait, we fall back to regional
# defaults based on geographic/ethnic proximity.

REGION_DEFAULTS = {
    'northern_europe': {
        'eye': {'blue': 55.0, 'intermediate': 22.0, 'brown': 23.0},
        'hair': {'black': 5.0, 'brown': 45.0, 'blond': 44.0, 'red': 6.0},
        'skin': {'I': 16, 'II': 38, 'III': 28, 'IV': 12, 'V': 4, 'VI': 2},
        'left_pct': 11.0,
    },
    'western_europe': {
        'eye': {'blue': 30.0, 'intermediate': 30.0, 'brown': 40.0},
        'hair': {'black': 12.0, 'brown': 52.0, 'blond': 30.0, 'red': 6.0},
        'skin': {'I': 8, 'II': 28, 'III': 30, 'IV': 20, 'V': 9, 'VI': 5},
        'left_pct': 10.5,
    },
    'southern_europe': {
        'eye': {'blue': 14.0, 'intermediate': 28.0, 'brown': 58.0},
        'hair': {'black': 28.0, 'brown': 50.0, 'blond': 16.0, 'red': 6.0},
        'skin': {'I': 4, 'II': 16, 'III': 30, 'IV': 32, 'V': 14, 'VI': 4},
        'left_pct': 10.0,
    },
    'eastern_europe': {
        'eye': {'blue': 30.0, 'intermediate': 28.0, 'brown': 42.0},
        'hair': {'black': 12.0, 'brown': 50.0, 'blond': 32.0, 'red': 6.0},
        'skin': {'I': 8, 'II': 28, 'III': 30, 'IV': 20, 'V': 9, 'VI': 5},
        'left_pct': 9.5,
    },
    'east_asia': {
        'eye': {'blue': 0.0, 'intermediate': 1.5, 'brown': 98.5},
        'hair': {'black': 99.0, 'brown': 1.0, 'blond': 0.0, 'red': 0.0},
        'skin': {'I': 0, 'II': 3, 'III': 22, 'IV': 52, 'V': 20, 'VI': 3},
        'left_pct': 4.0,
    },
    'southeast_asia': {
        'eye': {'blue': 0.0, 'intermediate': 1.5, 'brown': 98.5},
        'hair': {'black': 98.0, 'brown': 2.0, 'blond': 0.0, 'red': 0.0},
        'skin': {'I': 0, 'II': 1, 'III': 10, 'IV': 35, 'V': 40, 'VI': 14},
        'left_pct': 5.0,
    },
    'south_asia': {
        'eye': {'blue': 0.0, 'intermediate': 3.0, 'brown': 97.0},
        'hair': {'black': 95.0, 'brown': 5.0, 'blond': 0.0, 'red': 0.0},
        'skin': {'I': 0, 'II': 1, 'III': 8, 'IV': 25, 'V': 44, 'VI': 22},
        'left_pct': 5.0,
    },
    'middle_east': {
        'eye': {'blue': 4.0, 'intermediate': 12.0, 'brown': 84.0},
        'hair': {'black': 70.0, 'brown': 24.0, 'blond': 4.0, 'red': 2.0},
        'skin': {'I': 1, 'II': 6, 'III': 18, 'IV': 36, 'V': 30, 'VI': 9},
        'left_pct': 6.0,
    },
    'north_africa': {
        'eye': {'blue': 2.0, 'intermediate': 10.0, 'brown': 88.0},
        'hair': {'black': 78.0, 'brown': 18.0, 'blond': 3.0, 'red': 1.0},
        'skin': {'I': 1, 'II': 4, 'III': 16, 'IV': 35, 'V': 32, 'VI': 12},
        'left_pct': 5.5,
    },
    'sub_saharan_africa': {
        'eye': {'blue': 0.0, 'intermediate': 1.0, 'brown': 99.0},
        'hair': {'black': 99.0, 'brown': 1.0, 'blond': 0.0, 'red': 0.0},
        'skin': {'I': 0, 'II': 0, 'III': 1, 'IV': 5, 'V': 28, 'VI': 66},
        'left_pct': 6.0,
    },
    'north_america': {
        'eye': {'blue': 28.0, 'intermediate': 18.0, 'brown': 54.0},
        'hair': {'black': 24.0, 'brown': 50.0, 'blond': 20.0, 'red': 6.0},
        'skin': {'I': 6, 'II': 18, 'III': 22, 'IV': 22, 'V': 16, 'VI': 16},
        'left_pct': 10.5,
    },
    'central_america': {
        'eye': {'blue': 2.0, 'intermediate': 8.0, 'brown': 90.0},
        'hair': {'black': 82.0, 'brown': 14.0, 'blond': 3.0, 'red': 1.0},
        'skin': {'I': 1, 'II': 4, 'III': 14, 'IV': 36, 'V': 34, 'VI': 11},
        'left_pct': 8.0,
    },
    'south_america': {
        'eye': {'blue': 6.0, 'intermediate': 14.0, 'brown': 80.0},
        'hair': {'black': 60.0, 'brown': 28.0, 'blond': 9.0, 'red': 3.0},
        'skin': {'I': 2, 'II': 8, 'III': 16, 'IV': 30, 'V': 28, 'VI': 16},
        'left_pct': 8.5,
    },
    'oceania': {
        'eye': {'blue': 28.0, 'intermediate': 20.0, 'brown': 52.0},
        'hair': {'black': 20.0, 'brown': 48.0, 'blond': 24.0, 'red': 8.0},
        'skin': {'I': 8, 'II': 20, 'III': 22, 'IV': 18, 'V': 16, 'VI': 16},
        'left_pct': 10.5,
    },
    'central_asia': {
        'eye': {'blue': 4.0, 'intermediate': 12.0, 'brown': 84.0},
        'hair': {'black': 80.0, 'brown': 16.0, 'blond': 3.0, 'red': 1.0},
        'skin': {'I': 1, 'II': 5, 'III': 18, 'IV': 42, 'V': 28, 'VI': 6},
        'left_pct': 6.0,
    },
    'caribbean': {
        'eye': {'blue': 4.0, 'intermediate': 10.0, 'brown': 86.0},
        'hair': {'black': 70.0, 'brown': 22.0, 'blond': 6.0, 'red': 2.0},
        'skin': {'I': 2, 'II': 8, 'III': 14, 'IV': 22, 'V': 28, 'VI': 26},
        'left_pct': 8.5,
    },
}

# ISO2 → region mapping
COUNTRY_REGION = {
    # Northern Europe
    'IS': 'northern_europe', 'NO': 'northern_europe', 'SE': 'northern_europe',
    'FI': 'northern_europe', 'DK': 'northern_europe', 'EE': 'northern_europe',
    'LV': 'northern_europe', 'LT': 'northern_europe', 'IE': 'northern_europe',
    'GB': 'northern_europe',
    # Western Europe
    'FR': 'western_europe', 'DE': 'western_europe', 'NL': 'western_europe',
    'BE': 'western_europe', 'LU': 'western_europe', 'AT': 'western_europe',
    'CH': 'western_europe',
    # Southern Europe
    'ES': 'southern_europe', 'IT': 'southern_europe', 'PT': 'southern_europe',
    'GR': 'southern_europe', 'MT': 'southern_europe', 'CY': 'southern_europe',
    'AD': 'southern_europe', 'HR': 'southern_europe', 'SI': 'southern_europe',
    'BA': 'southern_europe', 'RS': 'southern_europe', 'ME': 'southern_europe',
    'MK': 'southern_europe', 'AL': 'southern_europe',
    # Eastern Europe
    'PL': 'eastern_europe', 'CZ': 'eastern_europe', 'SK': 'eastern_europe',
    'HU': 'eastern_europe', 'RO': 'eastern_europe', 'BG': 'eastern_europe',
    'RU': 'eastern_europe', 'UA': 'eastern_europe', 'BY': 'eastern_europe',
    'MD': 'eastern_europe',
    # East Asia
    'CN': 'east_asia', 'JP': 'east_asia', 'KR': 'east_asia',
    'KP': 'east_asia', 'MN': 'east_asia', 'TW': 'east_asia',
    # Southeast Asia
    'ID': 'southeast_asia', 'PH': 'southeast_asia', 'TH': 'southeast_asia',
    'VN': 'southeast_asia', 'MM': 'southeast_asia', 'MY': 'southeast_asia',
    'SG': 'southeast_asia', 'KH': 'southeast_asia', 'LA': 'southeast_asia',
    'BN': 'southeast_asia', 'TL': 'southeast_asia',
    # South Asia
    'IN': 'south_asia', 'PK': 'south_asia', 'BD': 'south_asia',
    'LK': 'south_asia', 'NP': 'south_asia', 'BT': 'south_asia',
    'MV': 'south_asia', 'AF': 'south_asia',
    # Middle East
    'TR': 'middle_east', 'IR': 'middle_east', 'IQ': 'middle_east',
    'SA': 'middle_east', 'AE': 'middle_east', 'IL': 'middle_east',
    'JO': 'middle_east', 'LB': 'middle_east', 'SY': 'middle_east',
    'YE': 'middle_east', 'OM': 'middle_east', 'QA': 'middle_east',
    'KW': 'middle_east', 'BH': 'middle_east', 'PS': 'middle_east',
    'GE': 'middle_east', 'AM': 'middle_east', 'AZ': 'middle_east',
    # North Africa
    'EG': 'north_africa', 'MA': 'north_africa', 'TN': 'north_africa',
    'DZ': 'north_africa', 'LY': 'north_africa', 'SD': 'north_africa',
    # Sub-Saharan Africa
    'NG': 'sub_saharan_africa', 'ET': 'sub_saharan_africa',
    'CD': 'sub_saharan_africa', 'ZA': 'sub_saharan_africa',
    'KE': 'sub_saharan_africa', 'TZ': 'sub_saharan_africa',
    'UG': 'sub_saharan_africa', 'GH': 'sub_saharan_africa',
    'CM': 'sub_saharan_africa', 'CI': 'sub_saharan_africa',
    'SN': 'sub_saharan_africa', 'ML': 'sub_saharan_africa',
    'BF': 'sub_saharan_africa', 'NE': 'sub_saharan_africa',
    'MZ': 'sub_saharan_africa', 'MW': 'sub_saharan_africa',
    'ZM': 'sub_saharan_africa', 'ZW': 'sub_saharan_africa',
    'BW': 'sub_saharan_africa', 'NA': 'sub_saharan_africa',
    'RW': 'sub_saharan_africa', 'BI': 'sub_saharan_africa',
    'SS': 'sub_saharan_africa', 'SO': 'sub_saharan_africa',
    'ER': 'sub_saharan_africa', 'DJ': 'sub_saharan_africa',
    'CG': 'sub_saharan_africa', 'GA': 'sub_saharan_africa',
    'GQ': 'sub_saharan_africa', 'TD': 'sub_saharan_africa',
    'CF': 'sub_saharan_africa', 'AO': 'sub_saharan_africa',
    'MG': 'sub_saharan_africa', 'BJ': 'sub_saharan_africa',
    'TG': 'sub_saharan_africa', 'SL': 'sub_saharan_africa',
    'LR': 'sub_saharan_africa', 'GN': 'sub_saharan_africa',
    'GW': 'sub_saharan_africa', 'GM': 'sub_saharan_africa',
    'MR': 'sub_saharan_africa', 'CV': 'sub_saharan_africa',
    'ST': 'sub_saharan_africa', 'KM': 'sub_saharan_africa',
    'MU': 'sub_saharan_africa', 'SC': 'sub_saharan_africa',
    'LS': 'sub_saharan_africa', 'SZ': 'sub_saharan_africa',
    # North America
    'US': 'north_america', 'CA': 'north_america',
    # Central America
    'MX': 'central_america', 'GT': 'central_america', 'HN': 'central_america',
    'SV': 'central_america', 'NI': 'central_america', 'CR': 'central_america',
    'PA': 'central_america', 'BZ': 'central_america',
    # Caribbean
    'CU': 'caribbean', 'DO': 'caribbean', 'JM': 'caribbean',
    'TT': 'caribbean', 'BS': 'caribbean', 'BB': 'caribbean',
    'AG': 'caribbean', 'DM': 'caribbean', 'GD': 'caribbean',
    'KN': 'caribbean', 'LC': 'caribbean', 'VC': 'caribbean',
    'HT': 'caribbean',
    # South America
    'BR': 'south_america', 'AR': 'south_america', 'CO': 'south_america',
    'CL': 'south_america', 'PE': 'south_america', 'VE': 'south_america',
    'EC': 'south_america', 'BO': 'south_america', 'PY': 'south_america',
    'UY': 'south_america', 'GY': 'south_america', 'SR': 'south_america',
    # Oceania
    'AU': 'oceania', 'NZ': 'oceania', 'FJ': 'oceania', 'PG': 'oceania',
    'WS': 'oceania', 'TO': 'oceania', 'VU': 'oceania', 'SB': 'oceania',
    'KI': 'oceania', 'MH': 'oceania', 'FM': 'oceania', 'NR': 'oceania',
    'PW': 'oceania', 'TV': 'oceania',
    # Central Asia
    'KZ': 'central_asia', 'UZ': 'central_asia', 'TJ': 'central_asia',
    'KG': 'central_asia', 'TM': 'central_asia',
}


def get_regional_default(iso2, trait):
    """Get regional default for a trait."""
    region = COUNTRY_REGION.get(iso2, 'south_asia')  # default to south_asia
    defaults = REGION_DEFAULTS.get(region, REGION_DEFAULTS['south_asia'])
    return defaults.get(trait)


# ── TSV Column Definitions ───────────────────────────────────────────────────

TSV_COLUMNS = [
    'country_code',        # ISO 3166-1 alpha-2
    'country_name',        # Full English name
    'male_height_cm',      # Mean adult male height (cm)
    'male_height_sd',      # Male height standard deviation (cm)
    'female_height_cm',    # Mean adult female height (cm)
    'female_height_sd',    # Female height standard deviation (cm)
    'male_bmi_mean',       # Mean adult male BMI (kg/m²)
    'female_bmi_mean',     # Mean adult female BMI (kg/m²)
    'bmi_sd',              # BMI standard deviation (both sexes)
    'eye_blue',            # Eye color % blue
    'eye_intermediate',    # Eye color % intermediate (green/hazel)
    'eye_brown',           # Eye color % brown
    'hair_black',          # Hair color % black
    'hair_brown',          # Hair color % brown
    'hair_blond',          # Hair color % blond/fair
    'hair_red',            # Hair color % red/auburn
    'skin_I',              # Fitzpatrick scale % type I
    'skin_II',             # Fitzpatrick scale % type II
    'skin_III',            # Fitzpatrick scale % type III
    'skin_IV',             # Fitzpatrick scale % type IV
    'skin_V',              # Fitzpatrick scale % type V
    'skin_VI',             # Fitzpatrick scale % type VI
    'blood_O_pos',         # Blood type % O+
    'blood_A_pos',         # Blood type % A+
    'blood_B_pos',         # Blood type % B+
    'blood_AB_pos',        # Blood type % AB+
    'blood_O_neg',         # Blood type % O-
    'blood_A_neg',         # Blood type % A-
    'blood_B_neg',         # Blood type % B-
    'blood_AB_neg',        # Blood type % AB-
    'left_handed_pct',     # Left-handedness prevalence %
]


def build_dataset(full_mode=False):
    """Build the merged dataset.

    Args:
        full_mode: If True, include all countries with height data
                   (filling gaps with regional defaults).
                   If False, only include countries with good coverage.
    """
    # Load all raw data
    height = load_json('height_by_country.json')  # keyed by ISO3
    male_bmi = load_owid_indicator(1054174)  # male BMI, keyed by ISO2
    female_bmi = load_owid_indicator(1054175)  # female BMI, keyed by ISO2
    eye_color = load_json('eye_color_by_country.json')  # keyed by ISO3
    hair_color = load_json('hair_color_by_country.json')  # keyed by ISO3
    skin_tone = load_json('skin_tone_by_country.json')  # keyed by ISO3
    blood_type = load_json('blood_type_by_country.json')  # keyed by country name
    handedness = load_json('handedness_by_country.json')  # keyed by ISO3

    # Convert height ISO3 → ISO2
    height_iso2 = {}
    for iso3, entry in height.items():
        iso2 = ISO3_TO_ISO2.get(iso3)
        if iso2:
            height_iso2[iso2] = entry
            COUNTRY_NAMES[iso2] = entry.get('name', '')

    # Convert blood type country names → ISO2
    blood_iso2 = {}
    for name, dist in blood_type.items():
        iso2 = COUNTRY_NAME_TO_ISO2.get(name)
        if iso2:
            blood_iso2[iso2] = dist

    # Convert eye/hair/skin/handedness ISO3 → ISO2
    def convert_iso3_dict(d):
        out = {}
        for iso3, entry in d.items():
            iso2 = ISO3_TO_ISO2.get(iso3, iso3)  # Some may already be ISO2
            if len(iso2) == 2:
                out[iso2] = entry
        return out

    eye_iso2 = convert_iso3_dict(eye_color)
    hair_iso2 = convert_iso3_dict(hair_color)
    skin_iso2 = convert_iso3_dict(skin_tone)
    hand_iso2 = convert_iso3_dict(handedness)

    # Build rows
    rows = []
    all_countries = set(height_iso2.keys())
    skipped = []

    for iso2 in sorted(all_countries):
        h = height_iso2.get(iso2, {})
        mh = h.get('male_height_cm', 0)
        fh = h.get('female_height_cm', 0)

        if not mh or not fh:
            skipped.append(iso2)
            continue

        # Height SD
        mh_sd = h.get('male_height_sd', 7.0)
        fh_sd = h.get('female_height_sd', 6.5)

        # BMI
        mb = male_bmi.get(iso2, 24.5)
        fb = female_bmi.get(iso2, 25.0)
        bmi_sd = 4.5  # Standard BMI SD

        # Eye color
        eye = eye_iso2.get(iso2) or get_regional_default(iso2, 'eye') or \
              {'blue': 0, 'intermediate': 5, 'brown': 95}

        # Hair color
        hair = hair_iso2.get(iso2) or get_regional_default(iso2, 'hair') or \
               {'black': 90, 'brown': 8, 'blond': 1, 'red': 1}

        # Skin tone
        skin = skin_iso2.get(iso2) or get_regional_default(iso2, 'skin') or \
               {'I': 2, 'II': 8, 'III': 18, 'IV': 30, 'V': 28, 'VI': 14}

        # Blood type
        blood = blood_iso2.get(iso2)
        if not blood:
            # Use regional average or global default
            blood = {
                'O+': 37.0, 'A+': 28.0, 'B+': 20.0, 'AB+': 5.0,
                'O-': 4.0, 'A-': 3.0, 'B-': 2.0, 'AB-': 1.0,
            }
        # Normalize blood type percentages to sum to 100
        bt_total = sum(blood.values())
        if bt_total > 0 and abs(bt_total - 100) > 0.1:
            blood = {k: v * 100.0 / bt_total for k, v in blood.items()}

        # Handedness
        hand = hand_iso2.get(iso2)
        left_pct = hand.get('left_pct', 10.6) if hand else \
                   get_regional_default(iso2, 'left_pct') or 10.6

        # Check data coverage for lite tier
        has_specific_data = (
            iso2 in eye_iso2 or iso2 in hair_iso2 or
            iso2 in skin_iso2 or iso2 in blood_iso2
        )

        if not full_mode and not has_specific_data:
            continue

        name = COUNTRY_NAMES.get(iso2, iso2)

        row = {
            'country_code': iso2,
            'country_name': name,
            'male_height_cm': round(mh, 2),
            'male_height_sd': round(mh_sd, 2),
            'female_height_cm': round(fh, 2),
            'female_height_sd': round(fh_sd, 2),
            'male_bmi_mean': round(mb, 2),
            'female_bmi_mean': round(fb, 2),
            'bmi_sd': round(bmi_sd, 2),
            'eye_blue': round(eye.get('blue', 0), 1),
            'eye_intermediate': round(eye.get('intermediate', 0), 1),
            'eye_brown': round(eye.get('brown', 0), 1),
            'hair_black': round(hair.get('black', 0), 1),
            'hair_brown': round(hair.get('brown', 0), 1),
            'hair_blond': round(hair.get('blond', 0), 1),
            'hair_red': round(hair.get('red', 0), 1),
            'skin_I': skin.get('I', 0),
            'skin_II': skin.get('II', 0),
            'skin_III': skin.get('III', 0),
            'skin_IV': skin.get('IV', 0),
            'skin_V': skin.get('V', 0),
            'skin_VI': skin.get('VI', 0),
            'blood_O_pos': round(blood.get('O+', 0), 1),
            'blood_A_pos': round(blood.get('A+', 0), 1),
            'blood_B_pos': round(blood.get('B+', 0), 1),
            'blood_AB_pos': round(blood.get('AB+', 0), 1),
            'blood_O_neg': round(blood.get('O-', 0), 1),
            'blood_A_neg': round(blood.get('A-', 0), 1),
            'blood_B_neg': round(blood.get('B-', 0), 1),
            'blood_AB_neg': round(blood.get('AB-', 0), 1),
            'left_handed_pct': round(left_pct, 1),
        }
        rows.append(row)

    return rows, skipped


def write_tsv(rows, tier):
    """Write dataset to TSV file."""
    tier_dir = os.path.join(RES, tier)
    os.makedirs(tier_dir, exist_ok=True)
    outpath = os.path.join(tier_dir, 'biodata.tsv')

    with open(outpath, 'w', encoding='utf-8', newline='\n') as f:
        # Header
        f.write('\t'.join(TSV_COLUMNS) + '\n')
        # Data
        for row in rows:
            vals = [str(row.get(col, '')) for col in TSV_COLUMNS]
            f.write('\t'.join(vals) + '\n')

    print(f'  Wrote {outpath} ({len(rows)} countries, '
          f'{os.path.getsize(outpath):,} bytes)')


def main():
    parser = argparse.ArgumentParser(description='Build biodata TSV resources')
    parser.add_argument('--tier', choices=['lite', 'full', 'both'],
                        default='both', help='Which tier to generate')
    args = parser.parse_args()

    if args.tier in ('both', 'lite'):
        print('\n=== Building lite tier ===')
        rows, _ = build_dataset(full_mode=False)
        write_tsv(rows, 'lite')

    if args.tier in ('both', 'full'):
        print('\n=== Building full tier ===')
        rows, skipped = build_dataset(full_mode=True)
        write_tsv(rows, 'full')
        if skipped:
            print(f'  Skipped {len(skipped)} countries (missing height): '
                  f'{", ".join(skipped[:10])}...')

    # Summary
    print('\n=== Summary ===')
    for tier in ['lite', 'full']:
        path = os.path.join(RES, tier, 'biodata.tsv')
        if os.path.exists(path):
            with open(path, 'r', encoding='utf-8') as f:
                lines = f.readlines()
            print(f'  {tier}: {len(lines) - 1} countries, '
                  f'{len(TSV_COLUMNS)} columns')


if __name__ == '__main__':
    main()
