# Board identity and I/O

Store customer bindings as board UUID plus channel and operation. Discovery builds a fresh map of UUIDs to temporary routes. Moving a switch panel from Core port 1 to a chain on port 6 changes only the route after rediscovery. A new board UUID requires an explicit rebind. Duplicate UUIDs cause all access for that UUID to fail. Unplugged boards disappear when the transport publishes the next complete inventory.

The transport owns per-port enumeration, downstream routing, address collision handling, timing, route invalidation, and hardware-specific I/O and EEPROM access. It must reject stale routes and verify board identity before transactions, serialize discovery with I/O, and cause controlled outputs to enter the board-specific safe state on communication loss. Add a driver table keyed by product and hardware revision before enabling outputs. An EEPROM descriptor is configuration metadata, not proof of authenticity or electrical compatibility. The current build has no physical transport driver.

## Development EEPROM descriptor

Provision one persistent random UUID per board. Preserve it through EEPROM updates and never derive it from a port, bus address, or chain index. All integer fields are little-endian.

| Bytes | Field |
| --- | --- |
| 0–3 | ASCII TNBD |
| 4 | Schema version 1 |
| 5–20 | 16 UUID bytes in canonical display order |
| 21–22 | Product ID, nonzero |
| 23–24 | Hardware revision |
| 25 | Input channel count |
| 26 | Output channel count |
| 27–28 | EEPROM capacity in bytes |
| 29–30 | CRC-16/CCITT-FALSE over bytes 0–28 |

This layout is a software proposal. Verify EEPROM capacity, address selection, immutable serial availability, programming and write protection against each hardware revision before manufacturing. Channel types, units, ranges and safe states belong in revision-specific drivers. The registry interface currently provides one readable identity EEPROM per board.

## Configuration and updates

Expose supported adapter options and channel bindings in validated ESPHome YAML and keep transport interfaces callable from external components or standalone C++. Start local app support with authenticated provisioning over BLE or Wi-Fi and a versioned configuration schema shared by both app and portal. Prefer delivering signed configuration independently when behavior can change without rebuilding. For firmware updates, have the portal assign an authenticated device an eligible signed manifest and have the device verify product, revision, version, digest and signature before staging an update. Keep a last-known-good configuration and use boot health checks and rollback for firmware. Customer C++ belongs in an explicit customer build and signing path with isolated builds and a separate trust policy. No app, portal, or remote update service is active in this development target.
