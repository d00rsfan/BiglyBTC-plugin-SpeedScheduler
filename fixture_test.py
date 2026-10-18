#!/usr/bin/env python3
"""Frozen original XML/model/time observations without a runtime JVM dependency."""

import hashlib
import json
import pathlib
import shutil
import subprocess
import sys
import tempfile

local = pathlib.Path(__file__).resolve().parent / "fixtures"
root = local if local.is_dir() else pathlib.Path(__file__).resolve().parents[3] / "migration/fixtures/speedscheduler"
manifest = json.loads((root / "manifest.json").read_text())
work = pathlib.Path(tempfile.mkdtemp(prefix="biglybtc-scheduler-fixtures-", dir="/tmp"))
for case in manifest["cases"]:
    source, expected = root / case["input"], root / case["expected"]
    for path, key in ((source, "input_sha256"), (expected, "expected_sha256")):
        if hashlib.sha256(path.read_bytes()).hexdigest() != case[key]:
            raise ValueError("Original fixture differs from its frozen observation: " + str(path))
    target = work / source.name
    shutil.copyfile(source, target)
    actual = subprocess.run([sys.argv[1], "--xml", str(target)], check=True, capture_output=True, timeout=45)
    if actual.stdout != expected.read_bytes() or target.read_bytes() != source.read_bytes():
        raise ValueError("Original XML/model/time fixture differs: " + source.name)
print("PASS frozen original scheduler XML/time fixtures, unchanged input bytes")
