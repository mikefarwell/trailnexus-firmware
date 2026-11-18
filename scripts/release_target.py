import json
import os
from pathlib import Path
import re

target = os.environ["TARGET"]
version = os.environ["VERSION"]
if not re.fullmatch(r"[a-z0-9]+(?:-[a-z0-9]+)+", target):
    raise SystemExit("Use a product-and-revision target ID.")
if not re.fullmatch(r"(?:0|[1-9][0-9]*)(?:\.(?:0|[1-9][0-9]*)){2}", version):
    raise SystemExit("Use a MAJOR.MINOR.PATCH version.")

approved = json.loads(Path("release-targets.json").read_text(encoding="utf-8"))
if target not in approved:
    raise SystemExit(f"Target is not approved for release: {target}")

config = Path(approved[target]).resolve()
if not config.is_relative_to(Path("targets").resolve()) or not config.is_file():
    raise SystemExit("Approved ESPHome configuration must exist under targets/.")

with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as output:
    print(f"config={config.relative_to(Path.cwd()).as_posix()}", file=output)
    print(f"tag={target}-v{version}", file=output)
