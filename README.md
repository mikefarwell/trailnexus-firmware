# TrailNexus firmware

Open-source ESPHome firmware and reusable C++ components for TrailNexus products. The current Core profile is an ESP32 development-board target with no expansion transport or production pin assignments. No customer firmware has been released.

| Target | ESPHome configuration | Status |
| --- | --- | --- |
| Core development | `targets/core/development.yaml` | Generic ESP32 development board |

Run `python -m pip install -r requirements.txt`, then `esphome compile targets/core/development.yaml`. The development image configures serial logging and exposes the board registry to ESPHome lambdas as `id(tn_core).boards()`. Use ESPHome external components or includes to add your own C++ code. The registry header also works in standalone C++17 firmware. No cloud account or network connection is required for the development build.

See [board identity and I/O](docs/architecture.md) and [release and storefront integration](docs/releases.md). Only TrailNexus maintainers have upstream write access. Customers can fork, modify, and build their own firmware under the MIT license.

## Release verification

For each change, document affected products and revisions, regression evidence, and impact on production acceptance, persistent device identity across upgrades, and service procedures. TrailNexus maintainers synchronize affected internal QA and engineering records before release and record why any procedure is unaffected.
