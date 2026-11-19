"""Public-safe JSON, local documentation link and change-impact checks."""
import argparse
import json
from pathlib import Path
import re
import subprocess
from urllib.parse import unquote, urlsplit

AREAS = ("specifications", "product-firmware", "lab-firmware", "system-overview",
         "component-guides", "qa", "cost", "shared-designs", "release")


def inspect_paths(root, paths):
    root = Path(root).resolve()
    failures, records = [], []
    for name in paths:
        path = root / name
        if not path.is_file():
            continue
        if path.suffix == ".json":
            try:
                value = json.loads(path.read_text(encoding="utf-8-sig"))
            except (ValueError, UnicodeError):
                failures.append({"file": name, "issue": "Invalid JSON"}); continue
            if name.startswith(".change-reviews/"):
                records.append(name)
                if not value.get("summary") or not value.get("verification"):
                    failures.append({"file": name, "issue": "Missing summary/verification"})
                for area in AREAS:
                    item = value.get("impacts", {}).get(area, {})
                    if item.get("status") not in ("no-change", "updated", "pending") or not item.get("reason"):
                        failures.append({"file": name, "issue": f"Missing impact disposition: {area}"})
                    elif item["status"] == "updated" and not item.get("evidence"):
                        failures.append({"file": name, "issue": f"Missing impact evidence: {area}"})
        if path.suffix.lower() == ".md":
            text = re.sub(r"```.*?```", "", path.read_text(encoding="utf-8-sig"), flags=re.S)
            for raw in re.findall(r"\]\(([^)]+)\)", text):
                target = raw.split(' "', 1)[0].strip("<>")
                if not target or target.startswith("#") or urlsplit(target).scheme:
                    continue
                target = unquote(target.split("#", 1)[0].split("?", 1)[0])
                destination = root / target.lstrip("/") if target.startswith("/") else path.parent / target
                if not destination.exists():
                    failures.append({"file": name, "issue": "Missing local Markdown file target", "target": target})
    if not records:
        failures.append({"issue": "Change a .change-reviews/*.json record with explicit cross-project/QA dispositions"})
    return {"changedFileCount": len(paths), "impactRecords": records, "failures": failures,
            "limitations": ["File targets only; remote URLs/heading anchors and review truth are not validated",
                            "Does not replace firmware builds, hardware verification or release approval"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--base", required=True)
    args = parser.parse_args()
    changes = subprocess.check_output(["git", "-C", str(args.repo), "diff", "--name-only", "--diff-filter=ACMR", "-z", args.base, "HEAD"])
    names = [s for s in changes.decode().split("\0") if s]
    result = inspect_paths(args.repo, names)
    print(json.dumps(result, indent=2))
    return int(bool(result["failures"]))


if __name__ == "__main__":
    raise SystemExit(main())
