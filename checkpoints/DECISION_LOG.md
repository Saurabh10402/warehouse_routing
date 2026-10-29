# Decisions

1. Preserve legacy robot algorithms and examples; primary workflow will be a stationary warehouse model.
2. Keep Boost.JSON. Acquire a portable compiler instead of replacing the parser.
3. Build an independently tested C++ discrete-event abstraction, because ns-3 and NR are absent.
   Explicit link serialization, propagation, queueing, packet paths and failures are required.
   Wi-Fi/5G access labels will describe abstract links, not radio PHY/MAC behavior.
4. Version the explicit graph configuration alongside legacy schemas. Use the existing Boost.JSON
   dependency and reuse the temperature safety controller; do not silently reinterpret legacy plans.
