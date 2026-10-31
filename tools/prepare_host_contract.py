#!/usr/bin/env python3
"""Prepare one private local ABI fixture. Never inspect keys or copy host services."""
import argparse
import hashlib
from pathlib import Path

EXPECTED = "b105002ee0833cb2ed10314ee12cc5ace698a410a5638d86b30b9c81e5c5b75f"
parser = argparse.ArgumentParser()
parser.add_argument("--host-root", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()
source = args.host_root / "components/app_manager/include/app_base.h"
data = source.read_bytes()
if hashlib.sha256(data).hexdigest() != EXPECTED:
    raise SystemExit("app_base.h hash mismatch; review a changed ABI before preparing")
args.output.mkdir(parents=True, exist_ok=True)
target = args.output / "app_base.h"
if target.exists() and target.read_bytes() != data:
    raise SystemExit("Refusing to replace a different local contract")
target.write_bytes(data)
print("Prepared only app_base.h; private local fixture, not a licence to redistribute host code.")
