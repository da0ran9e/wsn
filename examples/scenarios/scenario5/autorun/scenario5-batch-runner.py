#!/usr/bin/env python3
"""
Scenario5 Autorun v1
- Run ~100 rounds of example5 automatically
- Parse per-round summary file
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


def parse_summary_file(summary_path: Path) -> dict:
    result = {
        "scenario": None,
        "seed": None,
        "runId": None,
        "config": {},
        "params": {},
        "network": {},
        "mission": {},
    }

    current_section = None
    for raw in summary_path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line:
            continue

        if line.startswith("SCENARIO "):
            parts = line.split(maxsplit=1)
            result["scenario"] = parts[1] if len(parts) > 1 else None
            continue

        if line.startswith("RUN "):
            # format: RUN seed=222 runId=1
            for token in line.split()[1:]:
                if "=" not in token:
                    continue
                k, v = token.split("=", 1)
                if k == "seed":
                    result["seed"] = int(v)
                elif k == "runId":
                    result["runId"] = int(v)
            continue

        if line.startswith("[") and line.endswith("]"):
            current_section = line[1:-1].strip().lower()
            continue

        if "=" in line and current_section in {"config", "params", "network", "mission"}:
            k, v = line.split("=", 1)
            result[current_section][k.strip()] = v.strip()

    return result


def _parse_int(value: Optional[str]) -> Optional[int]:
    if value is None:
        return None
    try:
        return int(value.strip())
    except ValueError:
        return None


def _format_value(value: Optional[float]) -> str:
    if value is None:
        return "not-completed"
    return f"{value:.3f}"


def _determine_faster_uav(uav1_time: Optional[float], uav2_time: Optional[float]) -> tuple[str, Optional[float]]:
    if uav1_time is not None and uav2_time is not None:
        gap = uav1_time - uav2_time
        if abs(gap) < 1e-9:
            return "tie", 0.0
        if uav2_time < uav1_time:
            return "uav2", uav1_time - uav2_time
        return "uav1", uav2_time - uav1_time
    if uav2_time is not None:
        return "uav2-only", None
    if uav1_time is not None:
        return "uav1-only", None
    return "none", None


def _write_initial_txt_report(report_txt_path: Path, args: argparse.Namespace) -> None:
    lines = [
        "SCENARIO5 AUTORUN REPORT",
        "",
        "[CONFIGS]",
        f"rounds={args.rounds}",
        f"startSeed={args.start_seed}",
        f"startRunId={args.start_run_id}",
        f"simTime={args.sim_time}",
        f"gridSize={args.grid_size}",
        f"gridSpacing={args.grid_spacing}",
        f"numFragments={args.num_fragments}",
        f"numUavs={args.num_uavs}",
        f"timeoutSec={args.timeout_sec}",
        f"buildFirst={str(args.build_first).lower()}",
        f"extraArgs={args.extra_args.strip() or '-'}",
        "",
        "[PARAMS]",
        "pending=will-be-filled-from-first-successful-summary",
        "",
        "[ROUNDS]",
    ]
    report_txt_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def _inject_params_into_txt_report(report_txt_path: Path, params: dict) -> None:
    if not params:
        return

    content = report_txt_path.read_text(encoding="utf-8")
    placeholder = "[PARAMS]\npending=will-be-filled-from-first-successful-summary"
    if placeholder not in content:
        return

    param_lines = ["[PARAMS]"] + [f"{key}={value}" for key, value in params.items()]
    content = content.replace(placeholder, "\n".join(param_lines), 1)
    report_txt_path.write_text(content, encoding="utf-8")


def _append_round_summary(report_txt_path: Path, row: RoundResult) -> None:
    lines = [
        f"round={row.round_index} seed={row.seed} runId={row.run_id}",
        f"  durationSec={row.duration_sec:.3f}",
        f"  suspiciousNodes={row.suspicious_nodes if row.suspicious_nodes is not None else 'None'}",
        f"  uav1CompletedTime={_format_value(row.uav1_completed_time)}",
        f"  uav2CompletedTime={_format_value(row.uav2_completed_time)}",
        "",
    ]
    with report_txt_path.open("a", encoding="utf-8") as f:
        f.write("\n".join(lines))


def _append_conclusion_txt(report_txt_path: Path, agg: dict) -> None:
    earlier_rate = None
    denominator = agg["uav2_earlier_count"] + agg["uav1_earlier_count"]
    if denominator > 0:
        earlier_rate = agg["uav2_earlier_count"] / denominator

    lines = [
        "[CONCLUSION]",
        f"totalRounds={agg['total_rounds']}",
        f"okRounds={agg['ok_rounds']}",
        f"failedRounds={agg['failed_rounds']}",
        f"uav1CompletionCount={agg['uav1_completion_count']}",
        f"uav2CompletionCount={agg['uav2_completion_count']}",
        f"uav2EarlierCount={agg['uav2_earlier_count']}",
        f"uav1EarlierCount={agg['uav1_earlier_count']}",
        f"earlierRate={f'{earlier_rate:.3f}' if earlier_rate is not None else 'None'}",
        f"uav1MeanCompletionTime={agg['uav1_time_mean']}",
        f"uav2MeanCompletionTime={agg['uav2_time_mean']}",
        f"averageEarlierTime={agg['uav2_advantage_mean_sec']}",
        "",
    ]
    with report_txt_path.open("a", encoding="utf-8") as f:
        f.write("\n".join(lines))


def _build_command(args: argparse.Namespace, seed: int, run_id: int) -> list[str]:
    sim_args = [
        f"--seed={seed}",
        f"--runId={run_id}",
        f"--simTime={args.sim_time}",
        f"--gridSize={args.grid_size}",
        f"--gridSpacing={args.grid_spacing}",
        f"--numFragments={args.num_fragments}",
        f"--numUavs={args.num_uavs}",
    ]

    if args.extra_args.strip():
        sim_args.extend(shlex.split(args.extra_args.strip()))

    return ["./ns3", "run", f"example5 {' '.join(sim_args)}"]


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


def run_batch(args: argparse.Namespace) -> tuple[list[RoundResult], dict]:
    repo_root = Path(args.repo_root).resolve()
    results_root = repo_root / "src/wsn/examples/visualize/results"
    batch_root = results_root / "batch/scenario5"
    logs_dir = batch_root / "logs"

    logs_dir.mkdir(parents=True, exist_ok=True)

    for legacy_path in [
        batch_root / "raw-summary",
        batch_root / "scenario5_rounds.csv",
        batch_root / "scenario5_rounds.json",
        batch_root / "scenario5_batch_report.md",
    ]:
        if legacy_path.is_dir():
            shutil.rmtree(legacy_path, ignore_errors=True)
        else:
            legacy_path.unlink(missing_ok=True)

    report_txt_path = batch_root / "scenario5_batch_summary.txt"
    _write_initial_txt_report(report_txt_path, args)

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
        start = time.time()
        status = "ok"

        stdout_file = logs_dir / f"round_{round_index:03d}_seed{seed}_run{run_id}.stdout.log"
        stderr_file = logs_dir / f"round_{round_index:03d}_seed{seed}_run{run_id}.stderr.log"

        summary_file = results_root / f"scenario5_result_{seed}_{run_id}.txt"

        print(f"[run {round_index:03d}/{args.rounds}] seed={seed} runId={run_id}")

        try:
            completed = subprocess.run(
                cmd,
                cwd=repo_root,
                text=True,
                capture_output=True,
                timeout=args.timeout_sec,
            )
            stdout_file.write_text(completed.stdout, encoding="utf-8")
            stderr_file.write_text(completed.stderr, encoding="utf-8")

            if completed.returncode != 0:
                status = "failed-return"
        except subprocess.TimeoutExpired as ex:
            status = "timeout"
            stdout_file.write_text((ex.stdout or "") if isinstance(ex.stdout, str) else "", encoding="utf-8")
            stderr_file.write_text((ex.stderr or "") if isinstance(ex.stderr, str) else "", encoding="utf-8")

        duration_sec = time.time() - start

        uav1_time: Optional[float] = None
        uav2_time: Optional[float] = None
        suspicious_nodes: Optional[int] = None

        if summary_file.exists():
            parsed = parse_summary_file(summary_file)
            mission = parsed.get("mission", {})
            network = parsed.get("network", {})
            uav1_time = _parse_time(mission.get("uav1CompletedTime", "not-completed"))
            uav2_time = _parse_time(mission.get("uav2CompletedTime", "not-completed"))

            suspicious_nodes = _parse_int(network.get("suspiciousNodes"))

            if not params_written:
                _inject_params_into_txt_report(report_txt_path, parsed.get("params", {}))
                params_written = True

            if status == "ok" and (parsed.get("scenario") != "scenario5"):
                status = "invalid-summary"

            summary_file.unlink(missing_ok=True)
        else:
            if status == "ok":
                status = "missing-summary"

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
        _append_round_summary(report_txt_path, row)

        stdout_file.unlink(missing_ok=True)
        stderr_file.unlink(missing_ok=True)

    uav1_values = [r.uav1_completed_time for r in all_rows if r.uav1_completed_time is not None]
    uav2_values = [r.uav2_completed_time for r in all_rows if r.uav2_completed_time is not None]
    both_completed = [r for r in all_rows if r.uav1_completed_time is not None and r.uav2_completed_time is not None]
    uav2_advantage_values = [
        r.uav1_completed_time - r.uav2_completed_time
        for r in both_completed
        if r.uav1_completed_time is not None and r.uav2_completed_time is not None
    ]

    uav2_earlier_count = sum(
        1
        for r in both_completed
        if r.uav1_completed_time is not None
        and r.uav2_completed_time is not None
        and r.uav2_completed_time < r.uav1_completed_time
    )
    uav1_earlier_count = sum(
        1
        for r in both_completed
        if r.uav1_completed_time is not None
        and r.uav2_completed_time is not None
        and r.uav1_completed_time < r.uav2_completed_time
    )

    agg = {
        "total_rounds": len(all_rows),
        "ok_rounds": sum(1 for r in all_rows if r.status == "ok"),
        "failed_rounds": sum(1 for r in all_rows if r.status != "ok"),
        "uav1_completion_count": len(uav1_values),
        "uav2_completion_count": len(uav2_values),
        "uav1_time_mean": _safe_stat(uav1_values, "mean"),
        "uav2_time_mean": _safe_stat(uav2_values, "mean"),
        "uav2_earlier_count": uav2_earlier_count,
        "uav1_earlier_count": uav1_earlier_count,
        "uav2_advantage_mean_sec": _safe_stat(uav2_advantage_values, "mean"),
    }
    _append_conclusion_txt(report_txt_path, agg)

    if logs_dir.exists():
        shutil.rmtree(logs_dir, ignore_errors=True)

    print("\n=== Batch done ===")
    print(f"TXT   : {report_txt_path}")

    return all_rows, agg


def main() -> int:
    parser = argparse.ArgumentParser(description="Scenario5 autorun v1")
    parser.add_argument("--repo-root", default=".", help="Path to ns-3 repo root")
    parser.add_argument("--rounds", type=int, default=100, help="Number of rounds")
    parser.add_argument("--start-seed", type=int, default=222, help="Start seed")
    parser.add_argument("--start-run-id", type=int, default=1, help="Start runId")
    parser.add_argument("--sim-time", type=float, default=10.0, help="simTime for example5")
    parser.add_argument("--grid-size", type=int, default=20, help="gridSize")
    parser.add_argument("--grid-spacing", type=float, default=20.0, help="gridSpacing")
    parser.add_argument("--num-fragments", type=int, default=10, help="numFragments")
    parser.add_argument("--num-uavs", type=int, default=2, help="numUavs")
    parser.add_argument("--timeout-sec", type=int, default=180, help="Timeout per round")
    parser.add_argument("--build-first", action="store_true", help="Run ./ns3 build first")
    parser.add_argument(
        "--extra-args",
        default="",
        help="Extra CLI args forwarded to example5 (raw string)",
    )

    args = parser.parse_args()

    if args.rounds <= 0:
        raise SystemExit("--rounds must be > 0")

    run_batch(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
