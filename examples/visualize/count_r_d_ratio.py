#!/usr/bin/env python3
"""Count occurrences of '-R-' and '-D-' in result files and compute D/(R+D).

Usage:
  python3 count_r_d_ratio.py path/to/result.txt
  python3 count_r_d_ratio.py -d src/wsn/examples/visualize/results

If a directory is given (with -d), the script computes per-file ratios and
prints the mean ratio across files that contain at least one R or D.
"""
from __future__ import annotations
import argparse
import os
import re
import sys
from typing import Optional, Tuple, List


R_RE = re.compile(r"-R-")
D_RE = re.compile(r"-D-")


def count_rd_in_text(text: str) -> Tuple[int, int]:
    r = len(R_RE.findall(text))
    d = len(D_RE.findall(text))
    return r, d


def process_file(path: str) -> Tuple[int, int, Optional[float]]:
    try:
        with open(path, "r", encoding="utf-8", errors="ignore") as f:
            text = f.read()
    except Exception as e:
        raise
    r, d = count_rd_in_text(text)
    total = r + d
    ratio = (d / total) if total > 0 else None
    return r, d, ratio


def main() -> int:
    p = argparse.ArgumentParser(description="Count -R- and -D- and compute D/(R+D)")
    p.add_argument("path", help="File path to results file (or directory with -d)")
    p.add_argument("-d", "--dir", action="store_true", help="Treat path as directory and process all files inside")
    args = p.parse_args()

    if args.dir:
        root = args.path
        if not os.path.isdir(root):
            print(f"Not a directory: {root}", file=sys.stderr)
            return 2
        ratios: List[float] = []
        for name in sorted(os.listdir(root)):
            fp = os.path.join(root, name)
            if not os.path.isfile(fp):
                continue
            try:
                r, d, ratio = process_file(fp)
            except Exception:
                print(f"Error reading {fp}", file=sys.stderr)
                continue
            total = r + d
            print(f"{name}: R={r}, D={d}, ratio={(ratio if ratio is not None else 'N/A')}")
            if ratio is not None:
                ratios.append(ratio)
        if ratios:
            mean_ratio = sum(ratios) / len(ratios)
            print(f"\nFiles with R or D: {len(ratios)}, mean D/(R+D) = {mean_ratio:.6f}")
        else:
            print("\nNo R or D tokens found in any files.")
        return 0

    # single file
    fp = args.path
    if not os.path.isfile(fp):
        print(f"Not a file: {fp}", file=sys.stderr)
        return 2
    try:
        r, d, ratio = process_file(fp)
    except Exception as e:
        print(f"Error reading {fp}: {e}", file=sys.stderr)
        return 3
    print(f"{os.path.basename(fp)}: R={r}, D={d}")
    if ratio is None:
        print("No R or D tokens found; ratio undefined.")
    else:
        print(f"D/(R+D) = {ratio:.6f}")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
