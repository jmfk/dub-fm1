# R39 batch

Goal: two-core drum mix,4/8source defaults, avoid pre-master drum distortion. Modified engine/drums/mix/block renderer, added drum_parallel.h and hardened HAL files with dedicated .c1_text/.c1_bss relocation/build guard. .c1_bss3968B lives entirely in unused0x01C06000..0x01C07F00; main data/pool capacities preserved. Added native pthread/partial-worker rollback and clean-level tests. No MIDI/device access/install performed.

Source references: SLOOP bac9c05ea7627449405aa10e48da21d75b6c8c56; X0X6515e77d978c23414c80162558532f0b4312dd4f; Melodee165d177a425944f3d30b0b669255a6ed0050c3a5. SLOOP/X0X distribute source render units. Melodee uses an FX worker then immediate join in the examined mix_block; that is not proof of overlapped voice/FX throughput. R39 chooses independent source parallelism first, based on the measured drum cost. Stage drum time now covers preparation/wait/merge on core0, not all core1 execution; use dedicated worker/wait telemetry.

Core1 startup and hardware supply/guard behavior have only been compiled/checked locally, not run on this FM-1. No performance victory claimed before flash. Earlier sealed packages unchanged.
