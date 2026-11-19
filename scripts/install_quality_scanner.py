"""Install a pinned official Gitleaks release after SHA256 verification."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import platform
import tarfile
import urllib.request
import zipfile

parser = argparse.ArgumentParser()
parser.add_argument("destination", type=Path)
args = parser.parse_args()
config = json.loads((Path(__file__).parent / "quality-security-tools.json").read_text())["gitleaks"]
system = platform.system().lower()
if system not in ("windows", "linux") or platform.machine().lower() not in ("amd64", "x86_64"):
    raise SystemExit("Only Windows/Linux x64 releases are pinned")
asset = config[system]
url = f"https://github.com/gitleaks/gitleaks/releases/download/v{config['version']}/{asset['asset']}"
with urllib.request.urlopen(url, timeout=60) as response:
    data = response.read()
if hashlib.sha256(data).hexdigest() != asset["sha256"]:
    raise SystemExit("Gitleaks release checksum mismatch")
filename = "gitleaks.exe" if system == "windows" else "gitleaks"
if system == "windows":
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        binary = archive.read(filename)
else:
    with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as archive:
        binary = archive.extractfile(filename).read()
args.destination.mkdir(parents=True, exist_ok=True)
target = args.destination / filename
target.write_bytes(binary)
target.chmod(0o755)
print(f"Verified Gitleaks {config['version']} installed at {target}")
