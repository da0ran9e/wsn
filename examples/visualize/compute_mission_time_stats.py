#!/usr/bin/env python3
"""Compute mean/min/max of uav completion times from batch reports.

Prints a table usable to paste into LaTeX: for each strategy and size,
outputs mean (min--max).
"""
import glob
import os
import re
import numpy as np


def parse_agg(path):
    agg = {}
    in_block = False
    with open(path) as f:
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


def main():
    d = 'src/wsn/examples/visualize/results/batch/scenario4'
    files = glob.glob(os.path.join(d, '*.txt'))
    sizes = sorted({int(re.search(r'(?:PerpectChannel_)?(\d{2,4})_', os.path.basename(p)).group(1)) for p in files if re.search(r'(?:PerpectChannel_)?(\d{2,4})_', os.path.basename(p))})
    strategies = ['Proposed', 'NN', 'GMC-no-coop', 'Perfect']
    stats = {s: {size: [] for size in sizes} for s in strategies}

    for p in files:
        bn = os.path.basename(p)
        m_perfect = re.match(r'PerpectChannel_(\d+)_(true|false)_', bn)
        m = re.match(r'(\d+)_(true|false)_', bn)
        agg = parse_agg(p)
        if m_perfect:
            size = int(m_perfect.group(1))
            # perfect uses uav2MeanCompletionTime
            if 'uav2MeanCompletionTime' in agg:
                try:
                    stats['Perfect'][size].append(float(agg['uav2MeanCompletionTime']))
                except Exception:
                    pass
        elif m:
            size = int(m.group(1))
            variant = m.group(2)
            # true -> Proposed (uav2), and also NN uses uav1 from true files
            if variant == 'true':
                if 'uav2MeanCompletionTime' in agg:
                    try:
                        stats['Proposed'][size].append(float(agg['uav2MeanCompletionTime']))
                    except Exception:
                        pass
                if 'uav1MeanCompletionTime' in agg:
                    try:
                        stats['NN'][size].append(float(agg['uav1MeanCompletionTime']))
                    except Exception:
                        pass
            else:
                # false -> GMC-no-coop (uav2)
                if 'uav2MeanCompletionTime' in agg:
                    try:
                        stats['GMC-no-coop'][size].append(float(agg['uav2MeanCompletionTime']))
                    except Exception:
                        pass

    # print latex-ready table rows
    for strat in strategies:
        row = [strat.ljust(20)]
        for size in sizes:
            vals = stats[strat][size]
            if not vals:
                row.append('N/A')
            else:
                arr = np.array(vals)
                mean = arr.mean()
                lo = arr.min()
                hi = arr.max()
                row.append(f"{mean:.3f} ({lo:.3f}--{hi:.3f})")
        print(' & '.join(row) + ' \\\\')


if __name__ == '__main__':
    main()
