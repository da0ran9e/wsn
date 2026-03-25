#!/usr/bin/env python3
"""Plot stacked bars of recv_conf_mean per network size.

For each results file containing '_true_' in its name, this script:
- extracts the numeric network size from the filename (e.g., 100, 400, ...)
- parses `recv_conf_mean=` values and averages them across files of the same size
- sums `fromUAV` and `fromPeers` from all `suspicious_recv=` entries per size
- builds stacked bars where total bar height = recv_conf_mean_avg,
  bottom = peer share, top = UAV share (according to counts)

Saves a CSV and PNG into the plots folder.
"""
import argparse
import ast
import glob
import os
import re
from collections import defaultdict

import matplotlib.pyplot as plt
import numpy as np


def parse_dict(s):
    try:
        return ast.literal_eval(s)
    except Exception:
        try:
            return eval(s, {})
        except Exception:
            return None


def extract_size_from_name(name):
    # matches either 'PerpectChannel_100_true...' or '100_true_...'
    m = re.search(r'(?:PerpectChannel_)?(\d{2,4})_true_', name)
    if m:
        return int(m.group(1))
    # fallback: look for first leading number
    m2 = re.search(r'(\d{2,4})', name)
    return int(m2.group(1)) if m2 else None


def process_file(path, accum_conf, counts, totals):
    basename = os.path.basename(path)
    size = extract_size_from_name(basename)
    if size is None:
        return
    if size not in accum_conf:
        accum_conf[size] = {'sum': 0.0, 'n': 0}
    with open(path, 'r') as f:
        for line in f:
            line = line.strip()
            if 'recv_conf_mean=' in line:
                try:
                    val = float(line.split('recv_conf_mean=', 1)[1].split()[0].strip())
                    accum_conf[size]['sum'] += val
                    accum_conf[size]['n'] += 1
                except Exception:
                    pass
            if line.startswith('suspicious_recv='):
                payload = line.split('=', 1)[1].strip()
                d = parse_dict(payload)
                if isinstance(d, dict):
                    u = int(d.get('fromUAV', 0))
                    p = int(d.get('fromPeers', 0))
                    totals[size]['fromUAV'] += u
                    totals[size]['fromPeers'] += p


def collect(results_dir):
    pattern = os.path.join(results_dir, '*.txt')
    files = glob.glob(pattern)
    true_files = [p for p in files if '_true_' in os.path.basename(p)]
    accum_conf = {}
    totals = defaultdict(lambda: {'fromUAV': 0, 'fromPeers': 0})
    for p in true_files:
        process_file(p, accum_conf, None, totals)
    # compute means
    stats = {}
    for size, c in accum_conf.items():
        mean_conf = (c['sum'] / c['n']) if c['n'] > 0 else 0.0
        stats[size] = {
            'recv_conf_mean': mean_conf,
            'fromUAV': totals[size]['fromUAV'],
            'fromPeers': totals[size]['fromPeers'],
        }
    return stats


def save_csv(stats, out_csv):
    os.makedirs(os.path.dirname(out_csv), exist_ok=True)
    with open(out_csv, 'w') as f:
        f.write('size,recv_conf_mean,fromUAV,fromPeers,uav_prop,peer_prop,peer_height,uav_height\n')
        for size in sorted(stats.keys()):
            s = stats[size]
            u = s['fromUAV']
            p = s['fromPeers']
            total = u + p
            if total == 0:
                u_prop = 0.0
                p_prop = 0.0
            else:
                u_prop = u / total
                p_prop = p / total
            peer_h = s['recv_conf_mean'] * p_prop
            uav_h = s['recv_conf_mean'] * u_prop
            f.write(f"{size},{s['recv_conf_mean']},{u},{p},{u_prop},{p_prop},{peer_h},{uav_h}\n")


def plot_stats(stats, out_png):
    sizes = sorted(stats.keys())
    recv_means = [stats[s]['recv_conf_mean'] for s in sizes]
    peer_heights = []
    uav_heights = []
    for s in sizes:
        u = stats[s]['fromUAV']
        p = stats[s]['fromPeers']
        tot = u + p
        if tot == 0:
            p_prop = 0.0
            u_prop = 0.0
        else:
            p_prop = p / tot
            u_prop = u / tot
        peer_heights.append(stats[s]['recv_conf_mean'] * p_prop)
        uav_heights.append(stats[s]['recv_conf_mean'] * u_prop)

    x = np.arange(len(sizes))
    width = 0.6

    plt.style.use('ggplot')
    fig, ax = plt.subplots(figsize=(8, 5))
    ax.bar(x, peer_heights, width, label='fromPeers', color='#7fbf7f')
    ax.bar(x, uav_heights, width, bottom=peer_heights, label='fromUAV', color='#4c72b0')

    ax.set_xticks(x)
    ax.set_xticklabels([str(s) for s in sizes])
    ax.set_xlabel('Network size (N)')
    ax.set_ylabel('recv_conf_mean (stacked)')
    ax.set_title('recv_conf_mean stacked by source (per network size)')
    ax.legend()
    plt.tight_layout()
    os.makedirs(os.path.dirname(out_png), exist_ok=True)
    fig.savefig(out_png)
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--results-dir', default='src/wsn/examples/visualize/results/batch/scenario4')
    parser.add_argument('--out-png', default='src/wsn/examples/visualize/results/plots/recv_conf_stacked_by_size.png')
    parser.add_argument('--out-csv', default='src/wsn/examples/visualize/results/plots/recv_conf_stacked_by_size.csv')
    args = parser.parse_args()

    stats = collect(args.results_dir)
    if not stats:
        print('No data found in', args.results_dir)
        return
    save_csv(stats, args.out_csv)
    plot_stats(stats, args.out_png)
    print('Saved CSV to', args.out_csv)
    print('Saved PNG to', args.out_png)


if __name__ == '__main__':
    main()
