#!/usr/bin/env python3
"""Plot stacked bars for three events for a single network size (default 1225).

Produces a CSV and PNG with three stacked bars (recv, uav_depart, mission_complete),
where each bar's total height is the event's `*_conf_mean` and is split by
fromPeers (bottom) and fromUAV (top) according to counts from suspicious_* entries.
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
    m = re.search(r'(?:PerpectChannel_)?(\d{2,4})_true_', name)
    if m:
        return int(m.group(1))
    m2 = re.search(r'(\d{2,4})', name)
    return int(m2.group(1)) if m2 else None


def collect_for_size(results_dir, target_size):
    pattern = os.path.join(results_dir, '*.txt')
    files = glob.glob(pattern)
    files = [p for p in files if '_true_' in os.path.basename(p)]

    # accumulators
    accum = {
        'recv_sum': 0.0, 'recv_n': 0,
        'depart_sum': 0.0, 'depart_n': 0,
        'complete_sum': 0.0, 'complete_n': 0,
    }
    totals = {
        'recv_fromUAV': 0, 'recv_fromPeers': 0,
        'depart_fromUAV': 0, 'depart_fromPeers': 0,
        'complete_fromUAV': 0, 'complete_fromPeers': 0,
    }

    for p in files:
        size = extract_size_from_name(os.path.basename(p))
        if size != target_size:
            continue
        with open(p, 'r') as f:
            for line in f:
                line = line.strip()
                if 'recv_conf_mean=' in line:
                    try:
                        v = float(line.split('recv_conf_mean=', 1)[1].split()[0])
                        accum['recv_sum'] += v
                        accum['recv_n'] += 1
                    except Exception:
                        pass
                if 'uav_depart_conf_mean=' in line:
                    try:
                        v = float(line.split('uav_depart_conf_mean=', 1)[1].split()[0])
                        accum['depart_sum'] += v
                        accum['depart_n'] += 1
                    except Exception:
                        pass
                if 'mission_complete_conf_mean=' in line:
                    try:
                        v = float(line.split('mission_complete_conf_mean=', 1)[1].split()[0])
                        accum['complete_sum'] += v
                        accum['complete_n'] += 1
                    except Exception:
                        pass

                if line.startswith('suspicious_recv='):
                    d = parse_dict(line.split('=', 1)[1].strip())
                    if isinstance(d, dict):
                        totals['recv_fromUAV'] += int(d.get('fromUAV', 0))
                        totals['recv_fromPeers'] += int(d.get('fromPeers', 0))
                if line.startswith('suspicious_uav_depart='):
                    d = parse_dict(line.split('=', 1)[1].strip())
                    if isinstance(d, dict):
                        totals['depart_fromUAV'] += int(d.get('fromUAV', 0))
                        totals['depart_fromPeers'] += int(d.get('fromPeers', 0))
                if line.startswith('suspicious_mission_complete='):
                    d = parse_dict(line.split('=', 1)[1].strip())
                    if isinstance(d, dict):
                        totals['complete_fromUAV'] += int(d.get('fromUAV', 0))
                        totals['complete_fromPeers'] += int(d.get('fromPeers', 0))

    # compute means
    recv_mean = (accum['recv_sum'] / accum['recv_n']) if accum['recv_n'] > 0 else 0.0
    depart_mean = (accum['depart_sum'] / accum['depart_n']) if accum['depart_n'] > 0 else 0.0
    complete_mean = (accum['complete_sum'] / accum['complete_n']) if accum['complete_n'] > 0 else 0.0

    return {
        'recv_conf_mean': recv_mean,
        'depart_conf_mean': depart_mean,
        'complete_conf_mean': complete_mean,
        **totals,
    }


def save_csv_row(stats, size, out_csv):
    os.makedirs(os.path.dirname(out_csv), exist_ok=True)
    with open(out_csv, 'w') as f:
        f.write('size,event,conf_mean,fromUAV,fromPeers,peer_prop,peer_height,uav_height\n')
        for ev, mean_key, u_key, p_key in (
            ('recv', 'recv_conf_mean', 'recv_fromUAV', 'recv_fromPeers'),
            ('depart', 'depart_conf_mean', 'depart_fromUAV', 'depart_fromPeers'),
            ('complete', 'complete_conf_mean', 'complete_fromUAV', 'complete_fromPeers'),
        ):
            mean = stats[mean_key]
            u = stats[u_key]
            p = stats[p_key]
            tot = u + p
            if tot == 0:
                p_prop = 0.0
                u_prop = 0.0
            else:
                p_prop = p / tot
                u_prop = u / tot
            peer_h = mean * p_prop
            uav_h = mean * u_prop
            f.write(f"{size},{ev},{mean},{u},{p},{p_prop},{peer_h},{uav_h}\n")


def plot_for_size(stats, size, out_png):
    events = ['just received UAV fragment', 'after UAV departed', 'upon mission completion']
    peer_h = []
    uav_h = []
    means = []
    for ev, mean_key, u_key, p_key in (
        ('just received UAV fragment', 'recv_conf_mean', 'recv_fromUAV', 'recv_fromPeers'),
        ('after UAV departed', 'depart_conf_mean', 'depart_fromUAV', 'depart_fromPeers'),
        ('upon mission completion', 'complete_conf_mean', 'complete_fromUAV', 'complete_fromPeers'),
    ):
        mean = stats[mean_key]
        u = stats[u_key]
        p = stats[p_key]
        tot = u + p
        if tot == 0:
            p_prop = 0.0
            u_prop = 0.0
        else:
            p_prop = p / tot
            u_prop = u / tot
        peer_h.append(mean * p_prop)
        uav_h.append(mean * u_prop)
        means.append(mean)

    x = np.arange(len(events))
    width = 0.6

    plt.style.use('ggplot')
    # use a square / taller figure and swap peer/UAV colors (peer darker, UAV lighter)
    fig, ax = plt.subplots(figsize=(6, 6))
    ax.bar(x, peer_h, width, label='fromPeers', color='#31a354')
    ax.bar(x, uav_h, width, bottom=peer_h, label='fromUAV', color='#a1d99b')

    ax.set_xticks(x)
    ax.set_xticklabels(['just received UAV fragment', 'after UAV departed', 'upon mission completion'])
    ax.set_xlabel('Event')
    ax.set_ylabel('confidence mean (stacked)')
    ax.set_title(f'Confidence means (N={size})')
    ax.legend()
    plt.tight_layout()
    os.makedirs(os.path.dirname(out_png), exist_ok=True)
    fig.savefig(out_png)
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--size', type=int, default=1225)
    parser.add_argument('--results-dir', default='src/wsn/examples/visualize/results/batch/scenario4')
    parser.add_argument('--out-png', default='src/wsn/examples/visualize/results/plots/conf_events_N1225.png')
    parser.add_argument('--out-csv', default='src/wsn/examples/visualize/results/plots/conf_events_N1225.csv')
    args = parser.parse_args()

    stats = collect_for_size(args.results_dir, args.size)
    # quick check: ensure we have at least one measurement
    if stats['recv_conf_mean'] == 0 and stats['depart_conf_mean'] == 0 and stats['complete_conf_mean'] == 0:
        print(f'No measurements found for size {args.size} in {args.results_dir}')
        return
    save_csv_row(stats, args.size, args.out_csv)
    plot_for_size(stats, args.size, args.out_png)
    print('Saved CSV to', args.out_csv)
    print('Saved PNG to', args.out_png)


if __name__ == '__main__':
    main()
