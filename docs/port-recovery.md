# DI2C port recovery

Core exposes `id(tn_core).port_recovery()` for a hardware-specific control adapter. A development build has no adapter or port power mapping. Older Core revisions can retain their existing transport and omit power recovery.

The owning coordinator requests recovery only after bridge-level recovery has failed or is unavailable. It calls `start()` with a physical port (1–8) and measured off, startup, and discovery timing and a positive attempt limit. `started` means the request was accepted. The adapter event reports the outcome, including synchronous failure at startup. Core advances the sequence from its ESPHome loop. Only one port can be cycled at a time. Discovery must confirm expected UUIDs before reporting recovery. A retry invalidates incomplete discovery and repeats the off interval. Hardware fault, lost main supply, uncertain control, or exhausted attempts lock the port in software until explicitly cleared. The adapter must implement independent hardware power interlocks and validate its actual power and fault feedback. A failed control path can prevent software from confirming shutdown.

The adapter owns transport command inhibition, route invalidation and asynchronous rediscovery. It may restore read access as part of discovery. Application output rearm remains an explicit product decision. Test on qualified hardware before enabling this feature.
