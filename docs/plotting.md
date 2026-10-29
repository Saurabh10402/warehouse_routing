# Python comparison plots

`scripts/plot_results.py` creates one graph per metric so unrelated values are never combined. It accepts experiment directories containing `metrics.csv` and, when present, reads `routing.algorithm` from the adjacent `config.json` to label the comparison.

Install the plotting dependency, then pass one input for each algorithm:

```sh
python3 -m pip install -r scripts/requirements.txt
python3 scripts/plot_results.py \
  --input results/dijkstra-run \
  --input results/greedy-run \
  --input results/ealr-run \
  --output-dir results/comparison-graphs
```

The script generates separate latency, throughput, battery, packet-delivery-ratio, network-lifetime, charging-events, and packet-loss charts. Use `--label` for an explicit label if a configuration snapshot is unavailable.
