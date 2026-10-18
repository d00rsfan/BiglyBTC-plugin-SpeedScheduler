#!/usr/bin/env python3
"""Replay unchanged-original ownership observations without a JVM."""

import hashlib
import json
import pathlib
import shutil
import subprocess
import sys
import tempfile

local = pathlib.Path(__file__).resolve().parent / "ownership-fixtures"
root = local if local.is_dir() else pathlib.Path(__file__).resolve().parents[3] / "migration/fixtures/scheduler-ownership"
manifest = json.loads((root / "manifest.json").read_text())
work = pathlib.Path(tempfile.mkdtemp(prefix="biglybtc-scheduler-ownership-fixtures-", dir="/tmp"))
for case in manifest["cases"]:
    target = work / case["case"]
    target.mkdir()
    for name, expected in case["sha256"].items():
        source = root / case["case"] / name
        if hashlib.sha256(source.read_bytes()).hexdigest() != expected:
            raise ValueError("Original ownership fixture changed: " + str(source))
        shutil.copyfile(source, target / name)
    result = subprocess.run([sys.argv[1], "--original-file", str(target / "input.conf")],
        check=True, capture_output=True, timeout=45)
    if result.stdout != (target / "native.expected").read_bytes():
        raise ValueError("Ownership behavior differs from qualified original observation: " + case["case"])
print("PASS", len(manifest["cases"]), "original ownership helper cases, including explicit unterminated-append rejection")
