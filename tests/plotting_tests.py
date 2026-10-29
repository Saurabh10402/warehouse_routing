#!/usr/bin/env python3
"""Tests for plot input discovery that do not require matplotlib."""

import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path


SCRIPT_PATH = Path(__file__).parents[1] / "scripts" / "plot_results.py"
SPECIFICATION = importlib.util.spec_from_file_location("plot_results", SCRIPT_PATH)
plot_results = importlib.util.module_from_spec(SPECIFICATION)
assert SPECIFICATION.loader is not None
sys.modules[SPECIFICATION.name] = plot_results
SPECIFICATION.loader.exec_module(plot_results)


class PlotResultsTests(unittest.TestCase):
    def create_experiment(self, directory: Path, algorithm: str, throughput: float) -> Path:
        directory.mkdir()
        (directory / "metrics.csv").write_text(
            "metric,value\nthroughput_mbps," + str(throughput) + "\npacket_delivery_ratio_percent,99\n",
            encoding="utf-8",
        )
        (directory / "config.json").write_text(json.dumps({"routing": {"algorithm": algorithm}}), encoding="utf-8")
        return directory

    def test_loads_and_orders_algorithm_experiments(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            ealr = self.create_experiment(root / "ealr", "ealr", 3.0)
            dijkstra = self.create_experiment(root / "dijkstra", "dijkstra", 1.0)
            experiments = plot_results.load_experiments([ealr, dijkstra], [])
            self.assertEqual([experiment.label for experiment in experiments], ["dijkstra", "ealr"])
            self.assertEqual(experiments[1].metrics["throughput_mbps"], 3.0)

    def test_rejects_malformed_csv(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            metrics_path = Path(temporary_directory) / "metrics.csv"
            metrics_path.write_text("name,value\nthroughput,1\n", encoding="utf-8")
            with self.assertRaises(ValueError):
                plot_results.read_metrics(metrics_path)


if __name__ == "__main__":
    unittest.main()
