#!/usr/bin/env python3
"""Generate separate routing-algorithm comparison charts from EALR metrics CSV files."""

from __future__ import annotations

import argparse
import csv
import json
import math
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


PLOTS = (
    ("average_end_to_end_delay_ms", "latency-comparison", "Average End-to-End Delay", "Milliseconds"),
    ("throughput_mbps", "throughput-comparison", "Throughput", "Mbps"),
    ("average_battery_remaining_percent", "battery-comparison", "Average Battery Remaining", "Percent"),
    ("packet_delivery_ratio_percent", "packet-delivery-ratio-comparison", "Packet Delivery Ratio", "Percent"),
    ("network_lifetime_seconds", "network-lifetime-comparison", "Network Lifetime", "Seconds"),
    ("charging_events", "charging-events-comparison", "Charging Events", "Count"),
    ("packet_loss_percent", "packet-loss-comparison", "Packet Loss", "Percent"),
)

ALGORITHM_ORDER = {"dijkstra": 0, "greedy": 1, "ealr": 2}


@dataclass(frozen=True)
class Experiment:
    label: str
    metrics_path: Path
    metrics: dict[str, float]


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Create one comparison chart per EALR metric from timestamped experiment directories."
    )
    parser.add_argument(
        "--input",
        action="append",
        required=True,
        type=Path,
        help="Experiment directory containing metrics.csv, or a metrics.csv file. Repeat for each experiment.",
    )
    parser.add_argument(
        "--label",
        action="append",
        default=[],
        help="Optional label for the matching --input. Defaults to routing.algorithm in config.json.",
    )
    parser.add_argument("--output-dir", required=True, type=Path, help="Directory for generated chart files.")
    parser.add_argument("--format", choices=("png", "pdf"), default="png", help="Chart format (default: png).")
    parser.add_argument("--dpi", type=int, default=200, help="PNG resolution (default: 200).")
    return parser.parse_args()


def resolve_metrics_path(input_path: Path) -> Path:
    path = input_path.expanduser().resolve()
    if path.is_dir():
        path = path / "metrics.csv"
    if not path.is_file():
        raise ValueError(f"Metrics file was not found: {path}")
    return path


def read_metrics(metrics_path: Path) -> dict[str, float]:
    metrics: dict[str, float] = {}
    with metrics_path.open(newline="", encoding="utf-8") as csv_file:
        reader = csv.DictReader(csv_file)
        if reader.fieldnames != ["metric", "value"]:
            raise ValueError(f"Expected metric,value CSV columns in {metrics_path}")
        for line_number, row in enumerate(reader, start=2):
            metric = (row.get("metric") or "").strip()
            raw_value = (row.get("value") or "").strip()
            if not metric:
                raise ValueError(f"Missing metric name at {metrics_path}:{line_number}")
            if metric in metrics:
                raise ValueError(f"Duplicate metric '{metric}' in {metrics_path}")
            try:
                value = float(raw_value)
            except ValueError as error:
                raise ValueError(f"Invalid value for '{metric}' at {metrics_path}:{line_number}") from error
            if not math.isfinite(value):
                raise ValueError(f"Metric '{metric}' at {metrics_path}:{line_number} must be finite")
            metrics[metric] = value
    if not metrics:
        raise ValueError(f"No metric rows found in {metrics_path}")
    return metrics


def label_from_configuration(metrics_path: Path) -> str:
    config_path = metrics_path.parent / "config.json"
    if not config_path.is_file():
        return metrics_path.parent.name
    try:
        with config_path.open(encoding="utf-8") as config_file:
            configuration = json.load(config_file)
        algorithm = configuration["routing"]["algorithm"]
    except (json.JSONDecodeError, KeyError, TypeError):
        return metrics_path.parent.name
    return str(algorithm)


def load_experiments(inputs: Iterable[Path], labels: list[str]) -> list[Experiment]:
    input_paths = list(inputs)
    if labels and len(labels) != len(input_paths):
        raise ValueError("Provide either no --label values or exactly one label for every --input")

    experiments: list[Experiment] = []
    used_labels: set[str] = set()
    for index, input_path in enumerate(input_paths):
        metrics_path = resolve_metrics_path(input_path)
        base_label = labels[index] if labels else label_from_configuration(metrics_path)
        label = base_label
        suffix = 2
        while label in used_labels:
            label = f"{base_label} ({suffix})"
            suffix += 1
        used_labels.add(label)
        experiments.append(Experiment(label, metrics_path, read_metrics(metrics_path)))

    return sorted(experiments, key=lambda experiment: (ALGORITHM_ORDER.get(experiment.label.lower(), 99), experiment.label))


def plotting_backend():
    try:
        import matplotlib

        matplotlib.use("Agg")
        from matplotlib import pyplot
    except ModuleNotFoundError as error:
        raise RuntimeError(
            "matplotlib is required for plotting. Install dependencies with "
            "python3 -m pip install -r scripts/requirements.txt"
        ) from error
    return pyplot


def render_plots(experiments: list[Experiment], output_directory: Path, image_format: str, dpi: int) -> list[Path]:
    if dpi <= 0:
        raise ValueError("--dpi must be greater than zero")
    output_directory.mkdir(parents=True, exist_ok=True)
    pyplot = plotting_backend()
    generated: list[Path] = []

    for metric, filename, title, unit in PLOTS:
        available = [experiment for experiment in experiments if metric in experiment.metrics]
        if not available:
            print(f"Skipping {metric}: no input contains this metric", file=sys.stderr)
            continue

        labels = [experiment.label.upper() if experiment.label.lower() == "ealr" else experiment.label.title() for experiment in available]
        values = [experiment.metrics[metric] for experiment in available]
        figure, axis = pyplot.subplots(figsize=(7.2, 4.5), constrained_layout=True)
        bars = axis.bar(labels, values, color="#4C78A8")
        axis.set_title(title)
        axis.set_ylabel(unit)
        axis.grid(axis="y", alpha=0.25)
        axis.set_axisbelow(True)
        for bar, value in zip(bars, values, strict=True):
            axis.annotate(
                f"{value:.3g}",
                (bar.get_x() + (bar.get_width() / 2), bar.get_height()),
                xytext=(0, 4),
                textcoords="offset points",
                ha="center",
                va="bottom",
            )

        output_path = output_directory / f"{filename}.{image_format}"
        figure.savefig(output_path, dpi=dpi)
        pyplot.close(figure)
        generated.append(output_path)

    if not generated:
        raise ValueError("None of the supplied metrics files contains a supported plotting metric")
    return generated


def main() -> int:
    try:
        arguments = parse_arguments()
        experiments = load_experiments(arguments.input, arguments.label)
        generated = render_plots(experiments, arguments.output_dir.expanduser().resolve(), arguments.format, arguments.dpi)
    except (RuntimeError, ValueError, OSError) as error:
        print(f"Plot generation failed: {error}", file=sys.stderr)
        return 1

    for output_path in generated:
        print(f"Generated {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
