---
id: PRD-0099
title: DUB R39 dual-core drum mix and clean defaults
status: in-progress
locked: true
created: 2026-11-25
related_adrs: [ADR-0113]
---

# PRD-0099: DUB R39

## Requirements
- Default global budgets:4PCM voices and8OSC.
- Prioritize simultaneous two-core render of synths and drums using hardened FM-1 HAL.
- Correct avoidable pre-master drum saturation and preserve isolated USB levels.
- Keep R38 limiter/transitions, R37 delay pages, PCM16 storage and benchmark participation checks.
- Prepare R39 manual-flash package; owner alone starts flash. Hardware CPU/listening verification remains pending.

## Local evidence
R39 sealed at manual-flash/dub-synth-r39. Fresh native build and guards pass; image258500B, RAM97724/98304B, pool343724/344064B; core1 private stacks/scratch3968B in unused SRAM and168B resident loop. Real pthread serial/parallel/forced-partial-timeout parity and TSan/ASan/UBSan pass; headroom/defaults/kick/limiter/runtime/USB/WASM checks pass. Package --check does not open MIDI; launchers parse. Hardware dual-core startup, CPU and sound acceptance pending owner manual flash.
