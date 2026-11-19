# TrailNexus firmware engineering coordination

For every firmware change, assess the impact on the canonical private `mikefarwell/trailnexus-engineering-specs` repository and the other firmware repository. The public product firmware lives in `mikefarwell/trailnexus-firmware`. Staff-only fixture firmware has its own private repository.

Update affected engineering requirements, interface contracts, compatibility records, design decisions, or verification evidence in a companion engineering pull request as part of the same change. Changes to hardware behavior, discovery, I/O, EEPROM formats, configuration, release or update behavior, supported revisions, and meaningful verification results warrant this review. For changes that need no engineering edits, state why in the firmware pull request.

Check whether the corresponding product or lab target, fixture procedure, or pinned dependency needs updating. Update affected consumers together; if sequencing is necessary, record the dependency and compatibility limits in the private engineering review. Record firmware commit SHAs, relevant requirement IDs, and evidence in the engineering review. Keep private engineering details and staff fixture data out of public pull requests, issues, artifacts, and releases.

Always update every impacted product/system, installation, wiring, compatibility, update and service guide as the design changes. Record unaffected guides with a reason. The N16R8 Core profile requires octal PSRAM ECC, 16 MB flash and a required-PSRAM boot configuration. Keep module temperature allowances separate from qualified board/enclosure limits; never infer automotive or underhood suitability from ECC.
