# checkpoint_016 - viewer entry-point repair

User screenshot showed scripts/warehouse_viewer.html opened directly. Its unfilled data marker caused a JavaScript syntax error and empty controls. Replaced marker with a valid null fallback, hid empty controls, and added a visible link to the populated 500-device viewer. Exporter now replaces exactly one complete marker. Added OPEN_WAREHOUSE.html in the repository root and included it in checkpoint/recovery source enumeration. Updated run instructions.

Read checkpoint 015/current state/latest session and verified manifest before edits. No simulation data or native source changed. Three exporter tests and 21/21 core verification tests PASS. Browser verified template explanation and clicked through to the populated 8-zone/500-device/506-link/120-delivery viewer via localhost. Direct file protocol remains outside browser-tool authority; user screenshot established local file loading of the original template.

Next: continue actual spine-failure/recovery demonstration. Source manifest: manifests/checkpoint_016.json. Existing results and prior manifests preserved.
