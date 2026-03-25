#!/usr/bin/env python3
"""Plot grouped stacked bars per network size for three events.

For each network size (from filenames with '_true_'), plot three bars:
- recv_conf_mean
- uav_depart_conf_mean
- mission_complete_conf_mean

Each bar is stacked: bottom = peer contribution, top = UAV contribution,
with heights proportional to the counts from `suspicious_*` entries.

Saves CSV and PNG to the plots folder.
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


def process_file(path, accum_conf, totals):
    basename = os.path.basename(path)
    size = extract_size_from_name(basename)
    if size is None:
        return
    if size not in accum_conf:
        accum_conf[size] = {
            'recv_sum': 0.0, 'recv_n': 0,
            'depart_sum': 0.0, 'depart_n': 0,
            'complete_sum': 0.0, 'complete_n': 0,
        }
    with open(path, 'r') as f:
        for line in f:
            line = line.strip()
            if 'recv_conf_mean=' in line:
                try:
                    val = float(line.split('recv_conf_mean=', 1)[1].split()[0].strip())
                    accum_conf[size]['recv_sum'] += val
                    accum_conf[size]['recv_n'] += 1
                except Exception:
                    pass
            if 'uav_depart_conf_mean=' in line:
                try:
                    val = float(line.split('uav_depart_conf_mean=', 1)[1].split()[0].strip())
                    accum_conf[size]['depart_sum'] += val
                    accum_conf[size]['depart_n'] += 1
                except Exception:
                    pass
            if 'mission_complete_conf_mean=' in line:
                try:
                    val = float(line.split('mission_complete_conf_mean=', 1)[1].split()[0].strip())
                    accum_conf[size]['complete_sum'] += val
                    accum_conf[size]['complete_n'] += 1
                except Exception:
                    pass

            if line.startswith('suspicious_recv='):
                d = parse_dict(line.split('=', 1)[1].strip())
                if isinstance(d, dict):
                    totals[size]['recv_fromUAV'] += int(d.get('fromUAV', 0))
                    totals[size]['recv_fromPeers'] += int(d.get('fromPeers', 0))
            if line.startswith('suspicious_uav_depart='):
                d = parse_dict(line.split('=', 1)[1].strip())
                if isinstance(d, dict):
                    totals[size]['depart_fromUAV'] += int(d.get('fromUAV', 0))
                    totals[size]['depart_fromPeers'] += int(d.get('fromPeers', 0))
            if line.startswith('suspicious_mission_complete='):
                d = parse_dict(line.split('=', 1)[1].strip())
                if isinstance(d, dict):
                    totals[size]['complete_fromUAV'] += int(d.get('fromUAV', 0))
                    totals[size]['complete_fromPeers'] += int(d.get('fromPeers', 0))


def collect(results_dir):
    pattern = os.path.join(results_dir, '*.txt')
    files = glob.glob(pattern)
    true_files = [p for p in files if '_true_' in os.path.basename(p)]
    accum_conf = {}
    totals = defaultdict(lambda: {
        'recv_fromUAV': 0, 'recv_fromPeers': 0,
        'depart_fromUAV': 0, 'depart_fromPeers': 0,
        'complete_fromUAV': 0, 'complete_fromPeers': 0,
    })
    for p in true_files:
        process_file(p, accum_conf, totals)

    stats = {}
    for size, c in accum_conf.items():
        recv_mean = (c['recv_sum'] / c['recv_n']) if c['recv_n'] > 0 else 0.0
        depart_mean = (c['depart_sum'] / c['depart_n']) if c['depart_n'] > 0 else 0.0
        complete_mean = (c['complete_sum'] / c['complete_n']) if c['complete_n'] > 0 else 0.0
        stats[size] = {
            'recv_conf_mean': recv_mean,
            'depart_conf_mean': depart_mean,
            'complete_conf_mean': complete_mean,
            'recv_fromUAV': totals[size]['recv_fromUAV'],
            'recv_fromPeers': totals[size]['recv_fromPeers'],
            'depart_fromUAV': totals[size]['depart_fromUAV'],
            'depart_fromPeers': totals[size]['depart_fromPeers'],
            'complete_fromUAV': totals[size]['complete_fromUAV'],
            'complete_fromPeers': totals[size]['complete_fromPeers'],
        }
    return stats


def save_csv(stats, out_csv):
    os.makedirs(os.path.dirname(out_csv), exist_ok=True)
    with open(out_csv, 'w') as f:
        f.write('size,event,conf_mean,fromUAV,fromPeers,peer_prop,peer_height,uav_height\n')
        for size in sorted(stats.keys()):
            s = stats[size]
            for ev, mean_key, u_key, p_key in (
                ('recv', 'recv_conf_mean', 'recv_fromUAV', 'recv_fromPeers'),
                ('depart', 'depart_conf_mean', 'depart_fromUAV', 'depart_fromPeers'),
                ('complete', 'complete_conf_mean', 'complete_fromUAV', 'complete_fromPeers'),
            ):
                mean = s[mean_key]
                u = s[u_key]
                p = s[p_key]
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


def plot_stats(stats, out_png):
    sizes = sorted(stats.keys())
    n_sizes = len(sizes)
    labels = [str(s) for s in sizes]

    # For each size, get three means and corresponding stacks
    recv_peer = []
    recv_uav = []
    depart_peer = []
    depart_uav = []
    comp_peer = []
    comp_uav = []
    for s in sizes:
        st = stats[s]
        for key_mean, u_key, p_key, peer_list, uav_list in (
            ('recv_conf_mean', 'recv_fromUAV', 'recv_fromPeers', recv_peer, recv_uav),
            ('depart_conf_mean', 'depart_fromUAV', 'depart_fromPeers', depart_peer, depart_uav),
            ('complete_conf_mean', 'complete_fromUAV', 'complete_fromPeers', comp_peer, comp_uav),
        ):
            mean = st[key_mean]
            u = st[u_key]
            p = st[p_key]
            tot = u + p
            if tot == 0:
                p_prop = 0.0
                u_prop = 0.0
            else:
                p_prop = p / tot
                u_prop = u / tot
            peer_list.append(mean * p_prop)
            uav_list.append(mean * u_prop)

    x = np.arange(n_sizes)
    width = 0.22

    plt.style.use('ggplot')
    fig, ax = plt.subplots(figsize=(10, 6))

    ax.bar(x - width, recv_peer, width, label='recv fromPeers', color='#c7e9c0')
    ax.bar(x - width, recv_uav, width, bottom=recv_peer, label='recv fromUAV', color='#238b45')

    ax.bar(x, depart_peer, width, label='depart fromPeers', color='#fdd0a2')
    ax.bar(x, depart_uav, width, bottom=depart_peer, label='depart fromUAV', color='#e34a33')

    ax.bar(x + width, comp_peer, width, label='complete fromPeers', color='#9ecae1')
    ax.bar(x + width, comp_uav, width, bottom=comp_peer, label='complete fromUAV', color='#08519c')

    ax.set_xticks(x)
    ax.set_xticklabels(labels)
    ax.set_xlabel('Network size (N)')
    ax.set_ylabel('confidence mean (stacked)')
    ax.set_title('Confidence means (recv / depart / complete) stacked by source')
    ax.legend(ncol=2, bbox_to_anchor=(1.02, 1), loc='upper left')
    plt.tight_layout()
    os.makedirs(os.path.dirname(out_png), exist_ok=True)
    fig.savefig(out_png)
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--results-dir', default='src/wsn/examples/visualize/results/batch/scenario4')
    parser.add_argument('--out-png', default='src/wsn/examples/visualize/results/plots/conf_events_stacked_by_size.png')
    parser.add_argument('--out-csv', default='src/wsn/examples/visualize/results/plots/conf_events_stacked_by_size.csv')
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
