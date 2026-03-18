#!/usr/bin/env python3
"""
Scenario4 Autorun v1
- Run N rounds of example4 automatically
- Parse per-round summary appended to the result file
- Keep only one final TXT summary file
- Delete all intermediate result files after each run to save space
"""

from __future__ import annotations

import argparse
import shlex
import shutil
import statistics
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Optional


@dataclass
class RoundResult:
    round_index: int
    seed: int
    run_id: int
    status: str
    duration_sec: float
    uav1_completed_time: Optional[float]
    uav2_completed_time: Optional[float]
    suspicious_nodes: Optional[int]


def _parse_time(value: str) -> Optional[float]:
    text = value.strip()
    if text == "not-completed":
        return None
    try:
        return float(text)
    except ValueError:
        return None


def _parse_int(value: Optional[str]) -> Optional[int]:
    if value is None:
        return None
    try:
        return int(value.strip())
    except ValueError:
        return None


def parse_result_file(result_path: Path) -> dict:
    """
    Parse the SCENARIO4 SUMMARY block appended at the end of each result file.

    Format written by WriteScenario4Summary() in example4.cc:
        === SCENARIO4 SUMMARY ===
        SCENARIO scenario4
        RUN seed=<N> runId=<N>
        [CONFIG]
        key=value ...
        [PARAMS]
        key=value ...
        [NETWORK]
        key=value ...
        [MISSION]
        uav1CompletedTime=<float|not-completed>
        uav2CompletedTime=<float|not-completed>
    """
    parsed = {
        "scenario": None,
        "seed": None,
        "runId": None,
        "config": {},
        "params": {},
        "network": {},
        "mission": {},
    }

    in_summary = False
    current_section: Optional[str] = None

    for raw in result_path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()

        if not in_summary:
            if "=== SCENARIO4 SUMMARY ===" in line:
                in_summary = True
            continue

        if not line:
            continue

        if line.startswith("SCENARIO "):
            parts = line.split(maxsplit=1)
            parsed["scenario"] = parts[1] if len(parts) > 1 else None
            continue

        if line.startswith("RUN "):
            for token in line.split()[1:]:
                if "=" not in token:
                    continue
                k, v = token.split("=", 1)
                if k == "seed":
                    parsed["seed"] = int(v)
                elif k == "runId":
                    parsed["runId"] = int(v)
            continue

        if line.startswith("[") and line.endswith("]"):
            current_section = line[1:-1].strip().lower()
            continue

        if "=" in line and current_section in {"config", "params", "network", "mission"}:
            k, v = line.split("=", 1)
            parsed[current_section][k.strip()] = v.strip()

    return parsed


def _format_value(value: Optional[float]) -> str:
    if value is None:
        return "not-completed"
    return f"{value:.3f}"


def _safe_stat(values: list[float], op: str) -> Optional[float]:
    if not values:
        return None
    if op == "mean":
        return float(statistics.mean(values))
    if op == "median":
        return float(statistics.median(values))
    if op == "min":
        return float(min(values))
    if op == "max":
        return float(max(values))
    raise ValueError(op)


# ---------------------------------------------------------------------------
# Report writers
# ---------------------------------------------------------------------------

def _write_initial_txt_report(report_path: Path, args: argparse.Namespace) -> None:
    def _opt(v: object) -> str:
        return str(v) if v is not None else "default"

    lines = [
        "SCENARIO4 AUTORUN REPORT",
        "",
        "[CONFIGS]",
        f"rounds={args.rounds}",
        f"startSeed={args.start_seed}",
        f"startRunId={args.start_run_id}",
        f"simTime={_opt(args.sim_time)}",
        f"gridSize={_opt(args.grid_size)}",
        f"gridSpacing={_opt(args.grid_spacing)}",
        f"numFragments={_opt(args.num_fragments)}",
        f"numUavs={_opt(args.num_uavs)}",
        f"timeoutSec={args.timeout_sec}",
        f"buildFirst={str(args.build_first).lower()}",
        f"extraArgs={args.extra_args.strip() or '-'}",
        "",
        "[PARAMS]",
        "pending=will-be-filled-from-first-successful-summary",
        "",
        "[ROUNDS]",
    ]
    report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def _inject_params_into_report(report_path: Path, params: dict) -> None:
    if not params:
        return
    content = report_path.read_text(encoding="utf-8")
    placeholder = "[PARAMS]\npending=will-be-filled-from-first-successful-summary"
    if placeholder not in content:
        return
    param_lines = ["[PARAMS]"] + [f"{k}={v}" for k, v in params.items()]
    content = content.replace(placeholder, "\n".join(param_lines), 1)
    report_path.write_text(content, encoding="utf-8")


def _append_round_to_report(report_path: Path, row: RoundResult) -> None:
    lines = [
        f"round={row.round_index} seed={row.seed} runId={row.run_id} status={row.status}",
        f"  durationSec={row.duration_sec:.3f}",
        f"  suspiciousNodes={row.suspicious_nodes if row.suspicious_nodes is not None else 'None'}",
        f"  uav1CompletedTime={_format_value(row.uav1_completed_time)}",
        f"  uav2CompletedTime={_format_value(row.uav2_completed_time)}",
        "",
    ]
    with report_path.open("a", encoding="utf-8") as f:
        f.write("\n".join(lines))


def _append_conclusion_to_report(report_path: Path, agg: dict) -> None:
    uav2_rate: Optional[float] = None
    denom = agg["uav2_earlier_count"] + agg["uav1_earlier_count"]
    if denom > 0:
        uav2_rate = agg["uav2_earlier_count"] / denom

    uav1_comp_rate = (
        agg["uav1_completion_count"] / agg["ok_rounds"]
        if agg["ok_rounds"] > 0
        else None
    )
    uav2_comp_rate = (
        agg["uav2_completion_count"] / agg["ok_rounds"]
        if agg["ok_rounds"] > 0
        else None
    )
    both_comp_rate = (
        agg["both_completion_count"] / agg["ok_rounds"]
        if agg["ok_rounds"] > 0
        else None
    )

    def _fmt(v: Optional[float]) -> str:
        return f"{v:.3f}" if v is not None else "None"

    lines = [
        "[CONCLUSION]",
        f"totalRounds={agg['total_rounds']}",
        f"okRounds={agg['ok_rounds']}",
        f"failedRounds={agg['failed_rounds']}",
        f"uav1CompletionCount={agg['uav1_completion_count']}",
        f"uav2CompletionCount={agg['uav2_completion_count']}",
        f"bothCompletionCount={agg['both_completion_count']}",
        f"uav1CompletionRate={_fmt(uav1_comp_rate)}",
        f"uav2CompletionRate={_fmt(uav2_comp_rate)}",
        f"bothCompletionRate={_fmt(both_comp_rate)}",
        f"uav2EarlierCount={agg['uav2_earlier_count']}",
        f"uav1EarlierCount={agg['uav1_earlier_count']}",
        f"uav2EarlierRate={_fmt(uav2_rate)}",
        f"uav1MeanCompletionTime={_fmt(agg['uav1_time_mean'])}",
        f"uav2MeanCompletionTime={_fmt(agg['uav2_time_mean'])}",
        f"uav1MinCompletionTime={_fmt(agg['uav1_time_min'])}",
        f"uav2MinCompletionTime={_fmt(agg['uav2_time_min'])}",
        f"uav1MaxCompletionTime={_fmt(agg['uav1_time_max'])}",
        f"uav2MaxCompletionTime={_fmt(agg['uav2_time_max'])}",
        f"avgEarlierGapSec={_fmt(agg['uav2_advantage_mean'])}",
        "",
    ]
    with report_path.open("a", encoding="utf-8") as f:
        f.write("\n".join(lines))


# ---------------------------------------------------------------------------
# Command builder
# ---------------------------------------------------------------------------

def _build_command(args: argparse.Namespace, seed: int, run_id: int) -> list[str]:
    sim_args = [
        f"--seed={seed}",
        f"--runId={run_id}",
    ]
    if args.sim_time is not None:
        sim_args.append(f"--simTime={args.sim_time}")
    if args.grid_size is not None:
        sim_args.append(f"--gridSize={args.grid_size}")
    if args.grid_spacing is not None:
        sim_args.append(f"--gridSpacing={args.grid_spacing}")
    if args.num_fragments is not None:
        sim_args.append(f"--numFragments={args.num_fragments}")
    if args.num_uavs is not None:
        sim_args.append(f"--numUavs={args.num_uavs}")
    if args.extra_args.strip():
        sim_args.extend(shlex.split(args.extra_args.strip()))

    return ["./ns3", "run", f"example4 {' '.join(sim_args)}"]


# ---------------------------------------------------------------------------
# Main batch runner
# ---------------------------------------------------------------------------

def run_batch(args: argparse.Namespace) -> tuple[list[RoundResult], dict]:
    repo_root = Path(args.repo_root).resolve()
    results_root = repo_root / "src/wsn/examples/visualize/results"
    batch_root = results_root / "batch/scenario4"
    logs_dir = batch_root / "logs"

    logs_dir.mkdir(parents=True, exist_ok=True)

    report_path = batch_root / "scenario4_batch_summary.txt"
    _write_initial_txt_report(report_path, args)

    if args.build_first:
        print("[build] Running ./ns3 build ...")
        subprocess.run(["./ns3", "build"], cwd=repo_root, check=True)

    all_rows: list[RoundResult] = []
    params_written = False

    for i in range(args.rounds):
        round_index = i + 1
        seed = args.start_seed + i
        run_id = args.start_run_id + i

        cmd = _build_command(args, seed, run_id)
        result_file = results_root / f"scenario4_result_{seed}_{run_id}.txt"

        stdout_log = logs_dir / f"round_{round_index:03d}_seed{seed}_run{run_id}.stdout.log"
        stderr_log = logs_dir / f"round_{round_index:03d}_seed{seed}_run{run_id}.stderr.log"

        print(f"[run {round_index:03d}/{args.rounds}] seed={seed} runId={run_id}  ", end="", flush=True)

        t0 = time.time()
        status = "ok"

        try:
            completed = subprocess.run(
                cmd,
                cwd=repo_root,
                text=True,
                capture_output=True,
                timeout=args.timeout_sec,
            )
            stdout_log.write_text(completed.stdout, encoding="utf-8")
            stderr_log.write_text(completed.stderr, encoding="utf-8")

            if completed.returncode != 0:
                status = "failed-return"
        except subprocess.TimeoutExpired as ex:
            status = "timeout"
            stdout_log.write_text(
                (ex.stdout or "") if isinstance(ex.stdout, str) else "",
                encoding="utf-8",
            )
            stderr_log.write_text(
                (ex.stderr or "") if isinstance(ex.stderr, str) else "",
                encoding="utf-8",
            )

        duration_sec = time.time() - t0

        uav1_time: Optional[float] = None
        uav2_time: Optional[float] = None
        suspicious_nodes: Optional[int] = None

        if result_file.exists():
            parsed = parse_result_file(result_file)
            mission = parsed.get("mission", {})
            network = parsed.get("network", {})

            uav1_time = _parse_time(mission.get("uav1CompletedTime", "not-completed"))
            uav2_time = _parse_time(mission.get("uav2CompletedTime", "not-completed"))
            suspicious_nodes = _parse_int(network.get("suspiciousNodes"))

            if not params_written:
                _inject_params_into_report(report_path, parsed.get("params", {}))
                params_written = True

            if status == "ok" and parsed.get("scenario") != "scenario4":
                status = "invalid-summary"

            result_file.unlink(missing_ok=True)
        else:
            if status == "ok":
                status = "missing-summary"

        # Delete the node-init snapshot file written each run (fixed path)
        (results_root / "scenario4_nodes_init.txt").unlink(missing_ok=True)

        row = RoundResult(
            round_index=round_index,
            seed=seed,
            run_id=run_id,
            status=status,
            duration_sec=duration_sec,
            uav1_completed_time=uav1_time,
            uav2_completed_time=uav2_time,
            suspicious_nodes=suspicious_nodes,
        )
        all_rows.append(row)
        _append_round_to_report(report_path, row)

        # Summarise this round on a single line
        u1 = _format_value(uav1_time)
        u2 = _format_value(uav2_time)
        print(f"[{status}] {duration_sec:.1f}s  uav1={u1}  uav2={u2}")

        # Delete per-round stdout/stderr logs immediately to save space
        stdout_log.unlink(missing_ok=True)
        stderr_log.unlink(missing_ok=True)

    # -----------------------------------------------------------------------
    # Aggregate statistics
    # -----------------------------------------------------------------------
    uav1_values = [r.uav1_completed_time for r in all_rows if r.uav1_completed_time is not None]
    uav2_values = [r.uav2_completed_time for r in all_rows if r.uav2_completed_time is not None]

    both_completed_rows = [
        r for r in all_rows
        if r.uav1_completed_time is not None and r.uav2_completed_time is not None
    ]
    uav2_advantage_values = [
        r.uav1_completed_time - r.uav2_completed_time  # positive = uav2 finished earlier
        for r in both_completed_rows
        if r.uav1_completed_time is not None and r.uav2_completed_time is not None
    ]

    uav2_earlier_count = sum(
        1 for r in both_completed_rows
        if r.uav1_completed_time is not None
        and r.uav2_completed_time is not None
        and r.uav2_completed_time < r.uav1_completed_time
    )
    uav1_earlier_count = sum(
        1 for r in both_completed_rows
        if r.uav1_completed_time is not None
        and r.uav2_completed_time is not None
        and r.uav1_completed_time < r.uav2_completed_time
    )

    ok_rounds = sum(1 for r in all_rows if r.status == "ok")

    agg = {
        "total_rounds": len(all_rows),
        "ok_rounds": ok_rounds,
        "failed_rounds": len(all_rows) - ok_rounds,
        "uav1_completion_count": len(uav1_values),
        "uav2_completion_count": len(uav2_values),
        "both_completion_count": len(both_completed_rows),
        "uav1_time_mean": _safe_stat(uav1_values, "mean"),
        "uav2_time_mean": _safe_stat(uav2_values, "mean"),
        "uav1_time_min": _safe_stat(uav1_values, "min"),
        "uav2_time_min": _safe_stat(uav2_values, "min"),
        "uav1_time_max": _safe_stat(uav1_values, "max"),
        "uav2_time_max": _safe_stat(uav2_values, "max"),
        "uav2_earlier_count": uav2_earlier_count,
        "uav1_earlier_count": uav1_earlier_count,
        "uav2_advantage_mean": _safe_stat(uav2_advantage_values, "mean"),
    }
    _append_conclusion_to_report(report_path, agg)

    # Remove empty logs dir if no logs remain
    if logs_dir.exists():
        shutil.rmtree(logs_dir, ignore_errors=True)

    print("\n=== Batch done ===")
    print(f"Report : {report_path}")
    print(f"OK     : {agg['ok_rounds']}/{agg['total_rounds']}")
    print(f"UAV1 completed: {agg['uav1_completion_count']} rounds  "
          f"(mean={agg['uav1_time_mean']:.3f}s)" if agg["uav1_time_mean"] is not None
          else f"UAV1 completed: {agg['uav1_completion_count']} rounds")
    print(f"UAV2 completed: {agg['uav2_completion_count']} rounds  "
          f"(mean={agg['uav2_time_mean']:.3f}s)" if agg["uav2_time_mean"] is not None
          else f"UAV2 completed: {agg['uav2_completion_count']} rounds")

    return all_rows, agg


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Scenario4 autorun v1 — batch runner for example4",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("--repo-root", default=".", help="Path to ns-3 repo root")
    parser.add_argument("--rounds", type=int, default=100, help="Number of rounds to run")
    parser.add_argument("--start-seed", type=int, default=60, help="Starting random seed")
    parser.add_argument("--start-run-id", type=int, default=1, help="Starting runId")
    parser.add_argument(
        "--sim-time",
        type=float,
        default=None,
        help="simTime per round (s). Omit to use example4's built-in default (scales with gridSize).",
    )
    parser.add_argument("--grid-size", type=int, default=None, help="gridSize (N×N). Omit to use example4 default.")
    parser.add_argument("--grid-spacing", type=float, default=None, help="gridSpacing (m). Omit to use example4 default.")
    parser.add_argument("--num-fragments", type=int, default=None, help="numFragments. Omit to use example4 default.")
    parser.add_argument("--num-uavs", type=int, default=None, help="numUavs. Omit to use example4 default.")
    parser.add_argument(
        "--timeout-sec",
        type=int,
        default=300,
        help="Wall-clock timeout per round (seconds)",
    )
    parser.add_argument(
        "--build-first",
        action="store_true",
        help="Run ./ns3 build before starting batch",
    )
    parser.add_argument(
        "--extra-args",
        default="",
        help="Extra CLI args forwarded verbatim to example4 (e.g. '--alertThreshold=0.7')",
    )

    args = parser.parse_args()

    if args.rounds <= 0:
        raise SystemExit("--rounds must be > 0")

    run_batch(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
