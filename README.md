# DUB for M-VAVE FM-1

Experimental dub-techno instrument firmware. This repository starts with **R39**; earlier local revisions are not imported as invented Git history.

DUB has four tracks, up to eight oscillators per synth track, an11-part sample drum bank on track4, a mixer, modulation routing, two delays, reverb, MIDI and a sequencer. The browser simulator uses the same fixed-point sound engine and physical-control model.

## R39

- Default global limits: **4drum voices and8oscillators**.
- Drum mixing on the second core while the first renders synth tracks; hardened mailbox, boot self-test and timeout rollback/fallback.
- More drum summing headroom, drum DRIVE0 at startup, kick protection during voice eviction.
- Short voice-transition fades and the SLOOP master look-ahead limiter.
- DirectPCM16 delay storage; DLY A/B repeat presses switch normal controls/page2 engine settings.
- Benchmarks verify actual per-track participation and report core jobs/wait/timeouts.

**Local build, sanitizer and simulator checks passed. R39 hardware performance and listening verification are pending.** A2C footer means the core startup/self-test passed; MIDI job counters establish that work actually ran there. This is an experimental developer image, not a production release.

## Try the simulator

Open [r39/simulator.html](r39/simulator.html) locally in a browser. Enable audio with its Audio button. The screen is a display; use the surrounding physical controls or the shown computer shortcuts.

## Firmware and manual installation

The unchanged prepared package is in [releases/r39](releases/r39). Its manifest records the handed-off files. No installer runs as part of a build/check.

```sh
python3 -m venv .build-env/midi-venv
.build-env/midi-venv/bin/pip install -r requirements.txt
.build-env/midi-venv/bin/python releases/r39/manual_install.py --check
```

Only the owner starts flashing, manually:

```sh
sh "releases/r39/Installera DUB R39.command"
```

Do not run an updater concurrently with another installer or a benchmark. The scripts use the FM-1 USB-MIDI interface and retain explicit manual confirmation.

## Build from source

Use the standalone build entry point below; it does not depend on the surrounding original workspace. [BUILDING.md](BUILDING.md) documents the toolchain/SDK setup.

```sh
python3 tools/build_r39.py --check
python3 tools/build_r39.py --toolchain /path/to/jieli --sdk /path/to/ac79-sdk
```

- [r39/](r39): R39 application source, tests, simulator and original local-development helpers.
- [platform/](platform): frozen platform/build support; the build overlays r39/ as the canonical application/HAL source.
- [releases/r39/](releases/r39): original prepared firmware, source archive, installer and verification records.
- [docs/](docs): R39 design/verification notes.

Some historical r39 build/test helpers retain paths from the original multi-project workspace. Use tools/build_r39.py for a standalone firmware build. Native tests such as test_dual_core.c can also compile directly against r39/.

## Licence

Code: **GPL-3.0-only**. See [LICENSE](LICENSE) and [ATTRIBUTION.md](ATTRIBUTION.md) for upstream, font, SDK and owner-supplied audio provenance. M-VAVE and FM-1 are their owners' trademarks; no endorsement is implied.
