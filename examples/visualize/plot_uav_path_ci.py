#!/usr/bin/env python3
"""Plot UAV flight path length vs network size with shaded 95% CI.

Usage:
  python3 plot_uav_path_ci.py --results-dir ./src/wsn/examples/visualize/results/batch/scenario4 \
      --uav 1 --out plots/uav_path_ci.png

The script parses autorun report files in the results folder, excludes files with
"PerpectChannel" in the name, groups by network size and cooperation flag (true/false),
and plots mean +/- 95% percentile interval for the chosen UAV's path length.
"""
import argparse
import glob
import os
import re
from collections import defaultdict
import numpy as np
import matplotlib.pyplot as plt


def parse_aggregate(path):
    """Parse the [UAV_PATH_AGGREGATE] block and return a dict of values.

    Returns dict like {'uav1_path_mean':..., 'uav1_path_min':..., 'uav1_path_max':..., 'uav2_path_mean':...}
    If block not found, returns empty dict.
    """
    agg = {}
    in_block = False
    with open(path, 'r') as f:
        for line in f:
            line = line.strip()
            if line == '[UAV_PATH_AGGREGATE]':
                in_block = True
                continue
            if in_block:
                if line == '' or line.startswith('['):
                    break
                if '=' in line:
                    k, v = line.split('=', 1)
                    try:
                        agg[k.strip()] = float(v.strip())
                    except Exception:
                        agg[k.strip()] = v.strip()
    return agg


def collect_data(results_dir, uav_idx=1):
    pattern = os.path.join(results_dir, '*.txt')
    files = [p for p in glob.glob(pattern) if 'PerpectChannel' not in os.path.basename(p)]
    # map (size, flag) -> aggregate dict
    agg_map = {}
    sizes_set = set()
    flags_set = set()
    for p in files:
        bn = os.path.basename(p)
        m = re.search(r'^(?:PerpectChannel_)?(\d+)_(true|false)_', bn)
        if not m:
            m = re.search(r'(\d+)_(true|false)', bn)
        if not m:
            continue
        size = int(m.group(1))
        flag = m.group(2)
        sizes_set.add(size)
        flags_set.add(flag)
        agg = parse_aggregate(p)
        if agg:
            agg_map[(size, flag)] = agg
    sizes = sorted(sizes_set)
    flags = sorted(flags_set)
    return agg_map, sizes, flags


def collect_series(agg_map, sizes):
    """Build three series:
    - NN: from (size, 'true') -> uav1_path_mean/min/max
    - GMC_with_coop: from (size, 'true') -> uav2_path_mean/min/max (if min/max present)
    - GMC_wo_coop: from (size, 'false') -> uav2_path_mean/min/max
    Returns dict of series each mapping to arrays (sizes, mean, low, high)
    """
    nn_sizes = []
    nn_mean = []
    nn_low = []
    nn_high = []

    w_sizes = []
    w_mean = []
    w_low = []
    w_high = []

    wo_sizes = []
    wo_mean = []
    wo_low = []
    wo_high = []

    for s in sizes:
        # NN from true -> uav1
        agg_t = agg_map.get((s, 'true'))
        if agg_t and 'uav1_path_mean' in agg_t:
            nn_sizes.append(s)
            nn_mean.append(agg_t.get('uav1_path_mean'))
            nn_low.append(agg_t.get('uav1_path_min', agg_t.get('uav1_path_mean')))
            nn_high.append(agg_t.get('uav1_path_max', agg_t.get('uav1_path_mean')))
        # GMC with coop from true -> uav2
        if agg_t and 'uav2_path_mean' in agg_t:
            w_sizes.append(s)
            w_mean.append(agg_t.get('uav2_path_mean'))
            # if min/max for uav2 are absent, use mean as bounds
            w_low.append(agg_t.get('uav2_path_min', agg_t.get('uav2_path_mean')))
            w_high.append(agg_t.get('uav2_path_max', agg_t.get('uav2_path_mean')))
        # GMC without coop from false -> uav2
        agg_f = agg_map.get((s, 'false'))
        if agg_f and 'uav2_path_mean' in agg_f:
            wo_sizes.append(s)
            wo_mean.append(agg_f.get('uav2_path_mean'))
            wo_low.append(agg_f.get('uav2_path_min', agg_f.get('uav2_path_mean')))
            wo_high.append(agg_f.get('uav2_path_max', agg_f.get('uav2_path_mean')))

    return {
        'NN': (np.array(nn_sizes), np.array(nn_mean), np.array(nn_low), np.array(nn_high)),
        'GMC_with_coop': (np.array(w_sizes), np.array(w_mean), np.array(w_low), np.array(w_high)),
        'GMC_wo_coop': (np.array(wo_sizes), np.array(wo_mean), np.array(wo_low), np.array(wo_high)),
    }


def plot(grouping, sizes, flags, uav_idx, out_path, show=False):
    # prefer seaborn-like appearance but fall back to built-in style
    try:
        plt.style.use('seaborn-darkgrid')
    except Exception:
        plt.style.use('ggplot')
    fig, ax = plt.subplots(figsize=(7,4))
    series = collect_series(grouping, sizes)
    colors = {'NN': 'tab:gray', 'GMC_with_coop': 'tab:blue', 'GMC_wo_coop': 'tab:orange'}
    labels = {'NN': 'GreedyNearestNeighbor', 'GMC_with_coop': 'GMC w/ coop', 'GMC_wo_coop': 'GMC w/o coop'}
    for key in ['NN', 'GMC_with_coop', 'GMC_wo_coop']:
        xs, means, lows, highs = series[key]
        if xs.size == 0:
            continue
        ax.plot(xs, means, marker='o', label=labels[key], color=colors[key])
        ax.fill_between(xs, lows, highs, color=colors[key], alpha=0.2)
    ax.set_xlabel('Network size (nodes)')
    ax.set_ylabel(f'Length (m)')
    ax.set_title(f'UAV path length')
    # set x ticks to union of sizes present in any series
    xticks = sorted(set().union(*[series[k][0].tolist() for k in series]))
    ax.set_xticks(xticks)
    ax.legend()
    fig.tight_layout()
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    fig.savefig(out_path, dpi=300)
    if show:
        plt.show()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--results-dir', default='src/wsn/examples/visualize/results/batch/scenario4')
    parser.add_argument('--uav', type=int, default=1, choices=[1,2])
    parser.add_argument('--out', default='src/wsn/examples/visualize/results/plots/uav_path_ci.png')
    parser.add_argument('--show', action='store_true')
    args = parser.parse_args()

    grouping, sizes, flags = collect_data(args.results_dir, uav_idx=args.uav)
    if not grouping:
        print('No data found. Check --results-dir and file naming convention.')
        return
    plot(grouping, sizes, flags, args.uav, args.out, show=args.show)
    print(f'Plot saved to {args.out}')


if __name__ == '__main__':
    main()
