# Core N16R8 memory and installation limits

The selected module remains ESP32-S3-WROOM-1U-N16R8. The development target `targets/core/s3-n16r8-ecc.yaml` enables octal PSRAM with ECC, retains 16 MB flash, and reserves 1/16 of the 8 MiB PSRAM for ECC (approximately 7.5 MiB before allocator overhead and other reservations). The existing generic ESP32 development profile is a different target and does not establish this memory configuration.

ESPHome's `psram.enable_ecc: true` must produce `CONFIG_SPIRAM_ECC_ENABLE=y` in ESP-IDF. The included compile-time guard rejects the wrong MCU, quad PSRAM, missing ECC or the wrong flash size. PSRAM is required at boot; initialization failure must not silently select a no-PSRAM configuration. Future output drivers must remain inhibited until revision-specific boot and memory health checks pass.

Espressif permits this module's ambient limit to increase from +65 °C to +85 °C when PSRAM ECC is enabled. This is a module allowance, not a whole-board or enclosure rating. ECC is not thermal shutdown and does not protect other components from overheating. Verify free heap, cold/warm boot, sustained traffic and firmware-update memory demand on the assembled board before release.

Use a protected, dry interior mounting location qualified for the exact hardware, firmware and enclosure revision. Behind the lower dash away from heater ducts, a dry under-seat compartment or an interior equipment bay are candidates, not automatically approved locations. Exclude direct sun, engine/exhaust/radiator heat, condensation, splash and locations that obstruct vehicle controls or safety equipment. A sealed enclosure can still overheat.

No unrestricted underhood, +85 °C whole-board, +105 °C or +125 °C operating claim follows from this profile. Product installation limits must come from the lowest-rated installed component, accessories and enclosure thermal rise, with margin. Physical thermal testing and a released Core installation temperature range remain pending. This profile has no expansion transport, production output pin mapping or customer release eligibility.

The configuration does not change the device UUID/EEPROM format, registry interface or update signing policy. The reduced PSRAM budget must be included in future memory and acceptance profiles. Current registry-only generic lab targets do not depend on this profile; a lab profile that later boots an N16R8 Core must use and verify the same ECC configuration.

## Sources

- [Espressif module datasheet](https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf), module comparison and temperature footnote.
- [ESPHome PSRAM configuration](https://esphome.io/components/psram/), ECC and required-PSRAM behavior.
- [ESP-IDF PSRAM configuration](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/kconfig.html), CONFIG_SPIRAM_ECC_ENABLE.

For every hardware or firmware design change, update affected installation, wiring, compatibility, update, service and system guides in the same change set. Record unaffected guides and the reason. Rebuild and review web/PDF editions together before release.
