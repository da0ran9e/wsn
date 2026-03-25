#!/usr/bin/env python3
"""Plot uav2MeanCompletionTime vs network size including PerfectChannel files.

Usage:
  python3 plot_uav2_completion_with_perfect.py --results-dir ./src/wsn/examples/visualize/results/batch/scenario4 \
      --out ./src/wsn/examples/visualize/results/plots/uav2_completion.png

This script reads aggregate fields from batch report files (including filenames
that start with 'PerpectChannel_') and plots `uav2MeanCompletionTime` for three
variants: 'true' (with coop), 'false' (without coop), and 'perfect' (PerpectChannel).
If multiple files exist per (size,variant) the script uses their values to compute
mean and 2.5/97.5 percentiles for shaded CI.
"""
import argparse
import glob
import os
import re
from collections import defaultdict
import numpy as np
import matplotlib.pyplot as plt


def parse_aggregate(path):
    agg = {}
    in_block = False
    with open(path, 'r') as f:
        for line in f:
            line = line.strip()
            if line == '[CONCLUSION]':
                in_block = True
                continue
            if in_block:
                if line == '' or line.startswith('['):
                    break
                if '=' in line:
                    k, v = line.split('=', 1)
                    agg[k.strip()] = v.strip()
    return agg


def collect(results_dir):
    pattern = os.path.join(results_dir, '*.txt')
    files = glob.glob(pattern)
    # store per-file mean/min/max values when available
    data = defaultdict(lambda: {'mean': [], 'min': [], 'max': []})  # (size, variant) -> dict of lists
    sizes_set = set()
    for p in files:
        bn = os.path.basename(p)
        # detect perfect channel files
        if bn.startswith('PerpectChannel'):
            m = re.search(r'PerpectChannel_(\d+)_(true|false)_', bn)
            variant = 'perfect'
        else:
            m = re.search(r'(\d+)_(true|false)_', bn)
            variant = None
        if not m:
            continue
        size = int(m.group(1))
        if variant is None:
            variant = m.group(2)
        sizes_set.add(size)
        agg = parse_aggregate(p)
        # record uav2 mean completion time if present
        if 'uav2MeanCompletionTime' in agg:
            try:
                val = float(agg['uav2MeanCompletionTime'])
                data[(size, variant)]['mean'].append(val)
            except Exception:
                pass
        # prefer explicit min/max fields if present
        if 'uav2MinCompletionTime' in agg:
            try:
                vmin = float(agg['uav2MinCompletionTime'])
                data[(size, variant)]['min'].append(vmin)
            except Exception:
                pass
        if 'uav2MaxCompletionTime' in agg:
            try:
                vmax = float(agg['uav2MaxCompletionTime'])
                data[(size, variant)]['max'].append(vmax)
            except Exception:
                pass
        # also capture uav1 mean completion time under the 'nn' key for true files
        if variant == 'true' and 'uav1MeanCompletionTime' in agg:
            try:
                val1 = float(agg['uav1MeanCompletionTime'])
                data[(size, 'nn')]['mean'].append(val1)
            except Exception:
                pass
        # if uav1 min/max are provided in the file, record them for NN shading/lines
        if variant == 'true' and 'uav1MinCompletionTime' in agg:
            try:
                vmin1 = float(agg['uav1MinCompletionTime'])
                data[(size, 'nn')]['min'].append(vmin1)
            except Exception:
                pass
        if variant == 'true' and 'uav1MaxCompletionTime' in agg:
            try:
                vmax1 = float(agg['uav1MaxCompletionTime'])
                data[(size, 'nn')]['max'].append(vmax1)
            except Exception:
                pass
    sizes = sorted(sizes_set)
    return data, sizes


def summarize_series(data, sizes, variant):
    xs = []
    means = []
    lows = []
    highs = []
    for s in sizes:
        entry = data.get((s, variant), None)
        if not entry:
            continue
        mean_list = entry.get('mean', [])
        if not mean_list:
            continue
        xs.append(s)
        arr_mean = np.array(mean_list)
        means.append(arr_mean.mean())
        # if explicit min/max lists exist, use their envelope; otherwise fall back to mean-list min/max
        min_list = entry.get('min', [])
        max_list = entry.get('max', [])
        if min_list:
            lows.append(min(min_list))
        else:
            lows.append(arr_mean.min())
        if max_list:
            highs.append(max(max_list))
        else:
            highs.append(arr_mean.max())
    return np.array(xs), np.array(means), np.array(lows), np.array(highs)


def plot(data, sizes, out_path, show=False):
    plt.style.use('ggplot')
    # Create a broken y-axis: bottom linear 0-100 (expanded), top log-scale from 100 upward
    fig = plt.figure(figsize=(8, 8))
    # make bottom and top equal height so 0-10^2 and a decade on the top look similar
    gs = fig.add_gridspec(2, 1, height_ratios=[1, 1], hspace=0.05)
    ax_top = fig.add_subplot(gs[0, 0])
    ax_bot = fig.add_subplot(gs[1, 0], sharex=ax_top)

    variants = ['nn', 'true', 'false', 'perfect']
    labels = {'nn': 'NN (uav1 from true files)', 'true': 'GMC w/ coop (uav2)', 'false': 'GMC w/o coop (uav2)', 'perfect': 'PerfectChannel (uav2)'}
    colors = {'nn': 'tab:gray', 'true': 'tab:blue', 'false': 'tab:orange', 'perfect': 'tab:green'}

    all_highs = []
    series_store = []
    for v in variants:
        xs, means, lows, highs = summarize_series(data, sizes, v)
        if xs.size == 0:
            continue
        series_store.append((v, xs, means, lows, highs))
        if highs.size:
            all_highs.append(highs.max())

    if all_highs:
        ymax = max(all_highs)
    else:
        ymax = 600

    # plot on both axes (top: compressed upper region; bottom: zoomed 0-500)
    break_val = 100
    for v, xs, means, lows, highs in series_store:
        ax_top.plot(xs, means, marker='o', label=labels[v], color=colors[v])
        ax_top.plot(xs, lows, linestyle='--', color=colors[v], alpha=0.8)
        ax_top.plot(xs, highs, linestyle='--', color=colors[v], alpha=0.8)

        ax_bot.plot(xs, means, marker='o', label=labels[v], color=colors[v])
        ax_bot.plot(xs, lows, linestyle='--', color=colors[v], alpha=0.8)
        ax_bot.plot(xs, highs, linestyle='--', color=colors[v], alpha=0.8)

        # find crossing points where low < break_val < high and mark them
        if lows.size and highs.size:
            for xi, low, high in zip(xs, lows, highs):
                if low < break_val < high:
                    # plot upward triangle at the high value on top axis
                    ax_top.plot(xi, high, marker='^', color=colors[v], markersize=8)
                    # plot downward triangle at the low value on bottom axis
                    ax_bot.plot(xi, low, marker='v', color=colors[v], markersize=8)

    # set limits: bottom 0-100 (expanded linear), top 100..ymax (log scale)
    ax_bot.set_ylim(0, 100)
    ax_top.set_yscale('log')
    top_lower = 100
    top_upper = max(1000, ymax)
    if top_lower >= top_upper:
        top_lower = max(200, ymax) * 0.9
        top_upper = max(200, ymax) * 1.1
    ax_top.set_ylim(top_lower, top_upper)

    # hide spines between plots
    ax_top.spines['bottom'].set_visible(False)
    ax_bot.spines['top'].set_visible(False)
    ax_top.xaxis.tick_top()
    ax_top.tick_params(labeltop=False)  # don't put x labels on top

    # add diagonal break indicators
    d = .015  # size of diagonal lines in axes coordinates
    kwargs = dict(transform=ax_top.transAxes, color='k', clip_on=False)
    ax_top.plot((-d, +d), (-d, +d), **kwargs)
    ax_top.plot((1 - d, 1 + d), (-d, +d), **kwargs)
    kwargs = dict(transform=ax_bot.transAxes, color='k', clip_on=False)
    ax_bot.plot((-d, +d), (1 - d, 1 + d), **kwargs)
    ax_bot.plot((1 - d, 1 + d), (1 - d, 1 + d), **kwargs)

    ax_bot.set_xlabel('Network size (nodes)')
    ax_bot.set_ylabel('Completion time (s)')

    # legend on top axis: show scenario lines (colored) and separate markers for Mean/Max/Min
    from matplotlib.lines import Line2D
    handles, labels_ = ax_top.get_legend_handles_labels()
    # proxy markers for mean/max/min (marker-only, no connecting line)
    proxy_mean = Line2D([0], [0], color='k', marker='o', linestyle='None', label='Mean')
    proxy_max = Line2D([0], [0], color='k', marker='^', linestyle='None', label='Max')
    proxy_min = Line2D([0], [0], color='k', marker='v', linestyle='None', label='Min')
    handles.extend([proxy_mean, proxy_max, proxy_min])
    labels_.extend(['Mean', 'Max', 'Min'])
    # place a single legend below the plot spanning both axes
    fig.subplots_adjust(bottom=0.16)
    fig.legend(handles, labels_, loc='lower center', ncol=4, bbox_to_anchor=(0.5, -0.02))

    ax_bot.set_xticks(sorted(sizes))
    # save with tight bounding box (avoid tight_layout incompatibility warnings)
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    fig.savefig(out_path, dpi=300, bbox_inches='tight')
    if show:
        plt.show()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--results-dir', default='src/wsn/examples/visualize/results/batch/scenario4')
    parser.add_argument('--out', default='src/wsn/examples/visualize/results/plots/uav2_completion.png')
    parser.add_argument('--show', action='store_true')
    args = parser.parse_args()

    data, sizes = collect(args.results_dir)
    if not data:
        print('No uav2MeanCompletionTime values found in', args.results_dir)
        return
    plot(data, sizes, args.out, show=args.show)
    print('Plot saved to', args.out)


if __name__ == '__main__':
    main()
