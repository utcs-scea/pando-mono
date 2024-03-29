# SPDX-License-Identifier: MIT
# Copyright (c) 2024 University of Washington

import pandas as pd
import argparse
import sys

parser = argparse.ArgumentParser(description='Merge stats and tags CSVs')
parser.add_argument('stats_csv', default='stats.csv', type=str, help='stats CSV')
parser.add_argument('tags_csv', default='tags.csv', type=str, help='tags CSV')
parser.add_argument('merged_csv', default='merged.csv', type=str, help='merged CSV')

args = parser.parse_args()

stats_csv = args.stats_csv
tags_csv = args.tags_csv
merged_csv = args.merged_csv

stats_df = pd.read_csv(stats_csv)
tags_df = pd.read_csv(tags_csv)
merged_df = pd.merge(stats_df, tags_df, on='SimTime')

merged_df.to_csv(merged_csv, index=False)
