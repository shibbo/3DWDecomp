#!/usr/bin/env python3
"""Record or verify which source inputs produced the public progress snapshot.

tools/progress.py records this metadata after measuring progress. CI checks it
without the game binary or compiler, and refuses to publish stale reports.
"""

from __future__ import annotations

import hashlib
import json
import argparse
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
INPUTS = ["src", "include", "config", "configure.py", "tools/progress.py",
          "tools/elf_split.py", "tools/furyimg.py", "tools/report_snapshot.py"]


def input_digest(root: Path) -> str:
    paths = subprocess.check_output(
        ["git", "ls-files", "-z", "--cached", "--others", "--exclude-standard", "--", *INPUTS],
        cwd=root,
    ).decode("utf-8").split("\0")
    digest = hashlib.sha256()
    for name in sorted(set(paths) - {""}):
        path = root / name
        if not path.is_file():
            continue  # A locally deleted file is no longer a build input.
        # Git checkouts may use CRLF on Windows and LF on Linux.
        content = path.read_bytes().replace(b"\r\n", b"\n")
        digest.update(name.encode("utf-8") + b"\0")
        digest.update(hashlib.sha256(content).digest())
    return digest.hexdigest()


def snapshot(root: Path) -> dict:
    return {
        "version": 1,
        "inputs_sha256": input_digest(root),
        "report_sha256": hashlib.sha256((root / "report.json").read_bytes()).hexdigest(),
    }


def staged_snapshot(root: Path) -> dict:
    """Fingerprint exactly the indexed inputs, excluding unfinished worktree edits."""
    entries = subprocess.check_output(
        ["git", "ls-files", "--stage", "-z", "--", *INPUTS], cwd=root
    ).split(b"\0")
    objects = {}
    for entry in entries:
        if not entry:
            continue
        info, name = entry.split(b"\t", 1)
        mode, oid, stage = info.split()
        if stage != b"0":
            raise SystemExit("Resolve staged merge conflicts before recording progress.")
        objects[name] = oid
    names = sorted(objects)
    output = subprocess.check_output(
        ["git", "cat-file", "--batch"],
        input=b"".join(objects[name] + b"\n" for name in names), cwd=root,
    )
    digest = hashlib.sha256()
    cursor = 0
    for name in names:
        end = output.index(b"\n", cursor)
        oid, kind, size = output[cursor:end].split()
        if kind != b"blob":
            raise SystemExit("Progress inputs must be regular Git blobs.")
        start = end + 1
        cursor = start + int(size) + 1
        content = output[start:cursor - 1].replace(b"\r\n", b"\n")
        digest.update(name + b"\0")
        digest.update(hashlib.sha256(content).digest())
    report = subprocess.check_output(["git", "show", ":report.json"], cwd=root)
    return {
        "version": 1,
        "inputs_sha256": digest.hexdigest(),
        "report_sha256": hashlib.sha256(report).hexdigest(),
    }


def write_snapshot(root: Path = ROOT) -> None:
    (root / "report-metadata.json").write_text(
        json.dumps(snapshot(root), indent=2) + "\n", encoding="utf-8", newline="\n")


def check_snapshot(root: Path = ROOT, *, staged: bool = False) -> None:
    if staged:
        expected = json.loads(subprocess.check_output(
            ["git", "show", ":report-metadata.json"], cwd=root
        ))
        actual = staged_snapshot(root)
    else:
        expected = json.loads((root / "report-metadata.json").read_text(encoding="utf-8"))
        actual = snapshot(root)
    if expected != actual:
        raise SystemExit(
            "Stale progress snapshot: run python tools/progress.py and commit "
            "README.md, report.json, and report-metadata.json with the source changes.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--staged", action="store_true", help="check the exact pending commit")
    args = parser.parse_args()
    check_snapshot(staged=args.staged)
    print("Progress report matches the recorded source inputs and report digest.")
