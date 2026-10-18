#!/usr/bin/env python3
"""Replay frozen original scheduler commands without starting a JVM."""

import hashlib
import json
import pathlib
import shutil
import subprocess
import sys
import tempfile

local = pathlib.Path(__file__).resolve().parent / "tick-fixtures"
root = local if local.is_dir() else pathlib.Path(__file__).resolve().parents[3] / "migration/fixtures/scheduler-ticks"
manifest = json.loads((root / "manifest.json").read_text())
work = pathlib.Path(tempfile.mkdtemp(prefix="biglybtc-scheduler-ticks-fixtures-", dir="/tmp"))
for case in manifest["cases"]:
    target = work / case["case"]
    target.mkdir()
    for name, expected in case["sha256"].items():
        source = root / case["case"] / name
        if hashlib.sha256(source.read_bytes()).hexdigest() != expected:
            raise ValueError("Original scheduler tick fixture changed: " + str(source))
        shutil.copyfile(source, target / name)
    actual = subprocess.run([sys.argv[1], "--tick", str(target / "SavedSchedules.xml"), str(target / "values.properties")],
        check=True, capture_output=True, timeout=45)
    if actual.stdout != (target / "original.expected").read_bytes():
        raise ValueError("Native policy differs from original tick: " + case["case"])
print("PASS frozen original scheduler commands/rates/selection/ownership for", manifest["total_torrent_states"], "states")
