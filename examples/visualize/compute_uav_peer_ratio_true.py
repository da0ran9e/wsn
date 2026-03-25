#!/usr/bin/env python3
"""Compute total UAV / peer ratios for 'true' reports.

Scans all report files in the results directory and aggregates the 'fromUAV'
and 'fromPeers' counts from the keys:
- suspicious_recv
- suspicious_uav_depart
- suspicious_mission_complete

Only files containing '_true_' in their basename (including PerpectChannel_..._true_) are used.
Outputs a CSV summary and prints per-metric totals and ratios plus the combined ratio.
"""
import argparse
import ast
import glob
import os
from collections import defaultdict


def parse_round_dict(s):
    try:
        return ast.literal_eval(s)
    except Exception:
        # fallback: try to replace single quotes and evaluate
        try:
            return eval(s, {})
        except Exception:
            return None


def process_file(path, totals):
    with open(path, 'r') as f:
        for line in f:
            line = line.strip()
            for key in ('suspicious_recv=', 'suspicious_uav_depart=', 'suspicious_mission_complete='):
                if line.startswith(key):
                    payload = line.split('=', 1)[1].strip()
                    d = parse_round_dict(payload)
                    if isinstance(d, dict):
                        u = int(d.get('fromUAV', 0))
                        p = int(d.get('fromPeers', 0))
                        if key.startswith('suspicious_recv'):
                            totals['recv_uav'] += u
                            totals['recv_peer'] += p
                        elif key.startswith('suspicious_uav_depart'):
                            totals['depart_uav'] += u
                            totals['depart_peer'] += p
                        else:
                            totals['complete_uav'] += u
                            totals['complete_peer'] += p


def collect(results_dir):
    pattern = os.path.join(results_dir, '*.txt')
    files = glob.glob(pattern)
    # select files that include '_true_' in basename (also covers PerpectChannel_..._true_)
    true_files = [p for p in files if '_true_' in os.path.basename(p)]
    totals = defaultdict(int)
    for p in true_files:
        process_file(p, totals)
    return totals, true_files


def save_csv(totals, out_path):
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, 'w') as f:
        f.write('metric,total_fromUAV,total_fromPeers,ratio\n')
        for label, (u_key, p_key) in (
                ("just received UAV fragment", ('recv_uav', 'recv_peer')),
                ("after UAV departed", ('depart_uav', 'depart_peer')),
                ("upon mission completion", ('complete_uav', 'complete_peer')),
            ):
                u = totals.get(u_key, 0)
                p = totals.get(p_key, 0)
                ratio = (u / p) if p != 0 else float('inf')
                f.write(f"{label},{u},{p},{ratio}\n")
        # combined
        tu = totals.get('recv_uav', 0) + totals.get('depart_uav', 0) + totals.get('complete_uav', 0)
        tp = totals.get('recv_peer', 0) + totals.get('depart_peer', 0) + totals.get('complete_peer', 0)
        comb_ratio = (tu / tp) if tp != 0 else float('inf')
        f.write(f"combined,{tu},{tp},{comb_ratio}\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--results-dir', default='src/wsn/examples/visualize/results/batch/scenario4')
    parser.add_argument('--out', default='src/wsn/examples/visualize/results/plots/uav_peer_ratio_true.csv')
    args = parser.parse_args()

    totals, files = collect(args.results_dir)
    if not files:
        print('No _true_ report files found in', args.results_dir)
        return
    # print totals and ratios
    for label, (u_key, p_key) in (
        ("just received UAV fragment", ('recv_uav', 'recv_peer')),
        ("after UAV departed", ('depart_uav', 'depart_peer')),
        ("upon mission completion", ('complete_uav', 'complete_peer')),
    ):
        u = totals.get(u_key, 0)
        p = totals.get(p_key, 0)
        ratio = (u / p) if p != 0 else float('inf')
        print(f"{label}: total_fromUAV={u}, total_fromPeers={p}, ratio={ratio}")
    tu = totals.get('recv_uav', 0) + totals.get('depart_uav', 0) + totals.get('complete_uav', 0)
    tp = totals.get('recv_peer', 0) + totals.get('depart_peer', 0) + totals.get('complete_peer', 0)
    comb_ratio = (tu / tp) if tp != 0 else float('inf')
    print(f"combined: total_fromUAV={tu}, total_fromPeers={tp}, ratio={comb_ratio}")

    save_csv(totals, args.out)
    print('Saved CSV to', args.out)


if __name__ == '__main__':
    main()
