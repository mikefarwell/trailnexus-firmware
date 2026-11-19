"""Conservative build routing and exact source/tool/policy dependency-cache keys."""
import argparse
import fnmatch
import hashlib
import importlib.metadata
import json
import os
from pathlib import Path
import platform
import subprocess
import sys


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def load(root, profile_name="core-development", target=None):
    root = Path(root).resolve()
    files = [root / ".ci" / name for name in ("build-profiles.json", "build-policy.json", "cache-policy.json")]
    profiles, routing, caching = [json.loads(path.read_text(encoding="utf-8-sig")) for path in files]
    if any(value.get("schemaVersion") != 1 for value in (profiles, routing, caching)):
        raise ValueError("Unsupported build policy schema")
    profile = profiles["profiles"].get(profile_name)
    if not profile or profile.get("status") != "operational" or profile.get("qualificationEligible") is not False:
        raise ValueError("Unknown/unreviewed software profile")
    if target is not None and target != profile["target"]:
        raise ValueError("Workflow target does not match its declared profile")
    for field in ("target", "requirements", "registryTest"):
        path = (root / profile[field]).resolve()
        if not path.is_relative_to(root) or not path.is_file():
            raise ValueError(f"Missing/unsafe profile input: {field}")
    exact_metadata = {"README.md", "AGENTS.md", "LICENSE", ".github/workflows/change-quality.yml", "scripts/change_quality_gate.py",
                      "scripts/install_quality_scanner.py", "scripts/quality-security-tools.json", "tests/test_change_quality.py"}
    for pattern in routing["metadataOnlyPatterns"]:
        if pattern in exact_metadata:
            continue
        if ".." in pattern or not ((pattern.startswith("docs/") and pattern.endswith(".md")) or
                                   (pattern.startswith(".change-reviews/") and pattern.endswith(".json"))):
            raise ValueError("Unsafe metadata-only pattern; review source/input coverage")
    if not all(routing.get(flag) is True for flag in ("pushAndManualAlwaysFull", "unknownPathsAlwaysFull", "emptyOrUnavailableDiffAlwaysFull", "requiredCheckAlwaysReports")):
        raise ValueError("Conservative build-routing safeguards cannot be disabled")
    if caching.get("exactKeysOnly") is not True or caching.get("cacheCompiledOutputs") is not False or caching.get("cacheQualificationResults") is not False:
        raise ValueError("Cache policy must remain exact-key downloads only")
    return profile, routing, caching


def classify(paths, policy, event="pull_request", force_full=False):
    if force_full or event != "pull_request":
        return {"fullBuild": True, "reason": "Forced, default-branch push or manual verification"}
    if not paths:
        return {"fullBuild": True, "reason": "Empty/unavailable diff; conservative full build"}
    unknown = [path for path in paths if not any(fnmatch.fnmatchcase(path, pattern) for pattern in policy["metadataOnlyPatterns"])]
    return {"fullBuild": bool(unknown), "reason": "Source/tool/policy or unknown input changed" if unknown else "Metadata-only PR; full compilation not required", "nonMetadataPaths": unknown}


def changed_paths(root, base):
    if not base:
        return None
    try:
        commit = subprocess.check_output(["git", "-C", str(root), "rev-parse", "--verify", "--end-of-options", base + "^{commit}"], stderr=subprocess.DEVNULL, text=True).strip()
        raw = subprocess.check_output(["git", "-C", str(root), "diff", "--name-only", "--no-renames", "-z", commit, "HEAD"])
        return [name for name in raw.decode().split("\0") if name]
    except subprocess.CalledProcessError:
        return None


def cache_keys(root, profile_name="core-development", resolved=False):
    root = Path(root).resolve(); profile, _, policy = load(root, profile_name)
    raw = subprocess.check_output(["git", "-C", str(root), "ls-files", "--cached", "--others", "--exclude-standard", "-z"])
    names = sorted(set(name for name in raw.decode().split("\0") if name and
                       any(fnmatch.fnmatchcase(name, pattern) for pattern in policy["dependencySourceInputs"])))
    if not names:
        raise ValueError("Dependency source inputs are empty")
    inputs = []
    for name in names:
        path = (root / name).resolve()
        if not path.is_relative_to(root) or not path.is_file():
            raise ValueError("Missing/unsafe dependency input")
        inputs.append({"file": name, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    policy_files = [root / ".ci" / name for name in ("build-profiles.json", "build-policy.json", "cache-policy.json")]
    policy_files += [root / "scripts/build_policy.py", root / ".github/workflows/build.yml"]
    policy_hash = digest([{ "file": p.relative_to(root).as_posix(), "sha256": hashlib.sha256(p.read_bytes()).hexdigest()} for p in policy_files])
    tool = {"python": sys.version, "pip": importlib.metadata.version("pip"), "implementation": platform.python_implementation(), "system": platform.system(), "machine": platform.machine(),
            "imageOS": os.environ.get("ImageOS", "local"), "imageVersion": os.environ.get("ImageVersion", "local")}
    if resolved:
        tool["distributions"] = sorted((distribution.metadata["Name"].lower(), distribution.version) for distribution in importlib.metadata.distributions())
        tool["esphome"] = importlib.metadata.version("esphome")
    source_hash, tool_hash = digest(inputs), digest(tool)
    key = "tn-downloads-v1-" + digest({"source": source_hash, "tool": tool_hash, "policy": policy_hash, "profile": profile_name, "resolved": resolved})
    return {"key": key, "sourceHash": source_hash, "toolHash": tool_hash, "policyHash": policy_hash, "resolvedToolEnvironment": resolved,
            "qualificationEligible": False, "cachedContent": "downloads/packages only; compilation still required"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("route", "cache"))
    parser.add_argument("--repo", type=Path, default=Path.cwd())
    parser.add_argument("--profile", default="core-development")
    parser.add_argument("--target")
    parser.add_argument("--base")
    parser.add_argument("--event", default="pull_request")
    parser.add_argument("--force-full", action="store_true")
    parser.add_argument("--resolved-tools", action="store_true")
    parser.add_argument("--github-output", type=Path)
    args = parser.parse_args()
    profile, policy, _ = load(args.repo, args.profile, args.target)
    if args.action == "route":
        paths = changed_paths(args.repo, args.base)
        result = {**classify(paths, policy, args.event, args.force_full), "changedFiles": paths, "profile": args.profile,
                  "target": profile["target"], "qualificationEligible": False}
        output = {"full_build": str(result["fullBuild"]).lower()}
    else:
        result = cache_keys(args.repo, args.profile, args.resolved_tools)
        output = {"key": result["key"]}
    print(json.dumps(result, indent=2))
    if args.github_output:
        with args.github_output.open("a", encoding="utf-8") as stream:
            for key, value in output.items(): stream.write(f"{key}={value}\n")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, KeyError) as error:
        raise SystemExit(str(error))
