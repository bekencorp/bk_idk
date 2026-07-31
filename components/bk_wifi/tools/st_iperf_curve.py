#!/usr/bin/env python3
"""Parse ST UART mirror log and render deterministic iperf time-throughput curves."""

from __future__ import annotations

import argparse
import os
import re
import sys
from dataclasses import dataclass
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd

# Match per-interval and final average bandwidth lines from device iperf output.
BANDWIDTH_RE = re.compile(
    r"\[(\d+)-(\d+)\]\s+sec bandwidth:\s+([\d.]+)\s+(Kbits/sec|Mbits/sec)",
    re.IGNORECASE,
)
CMD_MARKER_RE = re.compile(
    r"^--- cmd(?:\s+\[[^\]]+\])?:\s*(.+?)\s*---\s*$",
    re.MULTILINE,
)


@dataclass(frozen=True)
class BandwidthSample:
    t_start: int
    t_end: int
    throughput_mbps: float


def _to_mbps(value: float, unit: str) -> float:
    unit_norm = unit.lower()
    if unit_norm.startswith("k"):
        return value / 1000.0
    return value


def _extract_client_section(log_text: str, client_prefix: str) -> str:
    """Return UART capture text for the last iperf client command section."""
    sections: list[tuple[str, str]] = []
    matches = list(CMD_MARKER_RE.finditer(log_text))
    for idx, match in enumerate(matches):
        cmd = match.group(1).strip()
        start = match.end()
        end = matches[idx + 1].start() if idx + 1 < len(matches) else len(log_text)
        sections.append((cmd, log_text[start:end]))
    for cmd, body in reversed(sections):
        if cmd.startswith(client_prefix):
            return body
    return ""


def parse_bandwidth_samples(section_text: str) -> list[BandwidthSample]:
    samples: list[BandwidthSample] = []
    for match in BANDWIDTH_RE.finditer(section_text):
        t_start = int(match.group(1))
        t_end = int(match.group(2))
        value = float(match.group(3))
        unit = match.group(4)
        samples.append(
            BandwidthSample(
                t_start=t_start,
                t_end=t_end,
                throughput_mbps=_to_mbps(value, unit),
            )
        )
    return samples


def _dedupe_interval_samples(samples: list[BandwidthSample]) -> list[BandwidthSample]:
    """Keep one sample per 1-second interval; drop final cumulative average."""
    by_interval: dict[tuple[int, int], BandwidthSample] = {}
    for sample in samples:
        if sample.t_end - sample.t_start != 1:
            continue
        key = (sample.t_start, sample.t_end)
        by_interval[key] = sample
    return [by_interval[k] for k in sorted(by_interval)]


def build_dataframe(samples: list[BandwidthSample]) -> pd.DataFrame:
    rows = []
    for sample in samples:
        rows.append(
            {
                "time_sec": sample.t_end,
                "interval_start_sec": sample.t_start,
                "interval_end_sec": sample.t_end,
                "throughput_mbps": round(sample.throughput_mbps, 3),
            }
        )
    return pd.DataFrame(rows)


def render_curve(df: pd.DataFrame, png_path: Path, title: str) -> None:
    plt.style.use("seaborn-v0_8-darkgrid")
    fig, ax = plt.subplots(figsize=(10, 5.5), dpi=120)
    ax.plot(
        df["time_sec"],
        df["throughput_mbps"],
        marker="o",
        linewidth=2.2,
        markersize=7,
        color="#2563eb",
    )
    ax.fill_between(df["time_sec"], df["throughput_mbps"], alpha=0.12, color="#2563eb")
    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Throughput (Mbps)")
    ax.set_title(title)
    ax.set_xlim(left=0)
    ax.set_ylim(bottom=0)
    ax.grid(True, linestyle="--", alpha=0.35)
    fig.tight_layout()
    fig.savefig(png_path, bbox_inches="tight")
    plt.close(fig)


def write_markdown(md_path: Path, png_name: str, df: pd.DataFrame, meta: dict[str, str]) -> None:
    avg = df["throughput_mbps"].mean()
    peak = df["throughput_mbps"].max()
    table_lines = [
        "| time_sec | interval_start_sec | interval_end_sec | throughput_mbps |",
        "| --- | --- | --- | --- |",
    ]
    for row in df.itertuples(index=False):
        table_lines.append(
            "| {} | {} | {} | {} |".format(
                row.time_sec,
                row.interval_start_sec,
                row.interval_end_sec,
                row.throughput_mbps,
            )
        )
    lines = [
        "# iperf ST Throughput Curve",
        "",
        "## Summary",
        "",
        f"- Case: `{meta.get('case', '')}`",
        f"- Duration: {meta.get('duration', '')} s",
        f"- Samples: {len(df)}",
        f"- Average: {avg:.2f} Mbps",
        f"- Peak: {peak:.2f} Mbps",
        "",
        "## Curve",
        "",
        f"![iperf throughput curve]({png_name})",
        "",
        "## Raw samples",
        "",
        *table_lines,
        "",
    ]
    md_path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate iperf ST throughput curve artifacts.")
    parser.add_argument(
        "--log",
        default="",
        help="UART mirror log path (default: $BK_ST_LOG_DIR/<component>.log)",
    )
    parser.add_argument("--duration", type=int, default=10, help="Expected iperf -t duration in seconds")
    parser.add_argument(
        "--client-cmd",
        default="iperf -c",
        help="Command prefix used to locate the client log section",
    )
    parser.add_argument(
        "--min-samples",
        type=int,
        default=0,
        help="Minimum interval samples required (default: duration - 1)",
    )
    args = parser.parse_args()

    log_dir = Path(os.environ.get("BK_ST_LOG_DIR", "."))
    component = os.environ.get("BK_ST_COMPONENT", "bk_wifi")
    case_name = os.environ.get("BK_ST_CASE", "wifi_iperf_tcp_throughput")
    log_path = Path(args.log) if args.log else log_dir / f"{component}.log"
    min_samples = args.min_samples if args.min_samples > 0 else max(args.duration - 1, 1)

    if not log_path.is_file():
        print(f"ERROR: log not found: {log_path}")
        return 1

    log_text = log_path.read_text(encoding="utf-8", errors="replace")
    section = _extract_client_section(log_text, args.client_cmd)
    if not section:
        print(f"ERROR: no iperf client section found in {log_path}")
        return 1

    samples = _dedupe_interval_samples(parse_bandwidth_samples(section))
    samples = [s for s in samples if s.t_end <= args.duration]
    if len(samples) < min_samples:
        print(
            "ERROR: insufficient samples {} < {} in {}".format(
                len(samples), min_samples, log_path
            )
        )
        return 1

    df = build_dataframe(samples)
    prefix = log_path.stem
    out_dir = log_path.parent
    csv_path = out_dir / f"{prefix}_iperf_curve.csv"
    xlsx_path = out_dir / f"{prefix}_iperf_curve.xlsx"
    png_path = out_dir / f"{prefix}_iperf_curve.png"
    md_path = out_dir / f"{prefix}_iperf_curve.md"

    df.to_csv(csv_path, index=False)
    df.to_excel(xlsx_path, index=False, sheet_name="iperf")
    render_curve(
        df,
        png_path,
        title="{} iperf TCP throughput ({}s)".format(case_name, args.duration),
    )
    write_markdown(
        md_path,
        png_path.name,
        df,
        {"case": case_name, "duration": str(args.duration)},
    )

    print("samples={}".format(len(df)))
    print("avg_mbps={:.2f}".format(df["throughput_mbps"].mean()))
    print("peak_mbps={:.2f}".format(df["throughput_mbps"].max()))
    print("csv={}".format(csv_path))
    print("xlsx={}".format(xlsx_path))
    print("png={}".format(png_path))
    print("md={}".format(md_path))
    print("CURVE_OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
