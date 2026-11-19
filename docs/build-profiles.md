# Software build profiles and dependency caches

The named `core-development` profile in `.ci/build-profiles.json` preserves the existing development target, Python version and registry behavior test. It is an operational software check, not a hardware-qualified or release-approved configuration. `release-targets.json` and release authorization are unchanged.

The Build job always reports `build (targets/core/development.yaml)`. Every run tests the build policy first. Pull requests changing only the explicit metadata allowlist may omit compilation. Source, requirements, target, workflow, routing-policy and unknown paths require compilation. Renames include the deleted path. Missing or empty diffs fail closed to a full build. Main pushes and manual runs always build fully.

The pip and PlatformIO caches contain dependency downloads/packages only. Exact keys include dependency-defining source hashes, runner/Python identity and policy/workflow hashes; the PlatformIO key also includes resolved Python distribution versions. There are no prefix restore keys, compiled firmware caches or reused passing build results. Firmware source changes can reuse dependency downloads, but still run the registry tests and compiler. Dependency and policy changes invalidate the relevant keys. GitHub scopes pull-request caches to their merge ref; these caches are not release evidence or a security boundary.

Run `python -m unittest discover -s tests -p 'test_*.py' -v` locally. Inspect a decision with `python scripts/build_policy.py route --base BASE --target targets/core/development.yaml`; use `--force-full` for a fresh full-build decision. Manual Build workflow runs also force full verification. Maintain the metadata allowlist and dependency input manifest when introducing new build inputs. The conservative unknown-path rule prevents an unlisted source from silently skipping compilation.

Software checks do not validate USB, automotive transients, current capacity, temperature, fixture operation or prototypes. No production cost changes are introduced; Actions/storage usage remains subject to the account's existing allocation.
