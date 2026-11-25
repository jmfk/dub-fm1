---
id: ADR-0113
title: DUB private core1 drum mix and rollback
status: accepted
date: 2026-11-25
related_prds: [PRD-0099]
supersedes: []
superseded_by: []
---

# ADR-0113: Dual-core drum mixer

Use SLOOP2.8.5/X0X hardened HAL at bac9c05ea7627449405aa10e48da21d75b6c8c56: checked mailbox, dedicated stacks, boot self-test, guarded core hold and timeout fallback. Copy HAL supply initialization to M-VAVE stock levels, PC guards reopened only for start, core held before resets/update/boot clearing. Idle core paced.

Core0 prepares immutable PCM frames, then core1 mixes the drum block into a private5-bus accumulator while core0 renders melodic tracks. Join before FX/capture. Mixer coefficient data is frozen until join; core1 owns drum gains/peak/raw mix during that interval. Separate sound flag avoids shared byte OR. Checkpoint16gain integers before handing off; after timeout the HAL holds core1, restore gain state and re-run mixer exactly once on core0 with prepared PCM. No duplicate sample-position advance.

Use only unused SRAM0x01C06000..0x01C07F00 for dedicated .c1_bss stacks/scratch, with explicit linker bounds and boot clear; preserve all existing pool/data capacities. Worker loop remains in its own .c1_text copied before start; audio RAM kernels remain call-free.

Normalize monitored drum sum by4 instead of2 and default drum DRIVE=0. Raw individual taps and drum-mix stem retain prior levels. Explicit drive remains a distortion effect. Protect kick from ordinary older-voice eviction when other pads are available. Display/report actual core state/jobs/worker and wait timings/timeouts; never label prepared firmware as hardware proven.
