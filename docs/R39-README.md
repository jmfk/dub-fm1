# DUB R39 — manual flash only

Defaults are4active drum voices and8global oscillators. OFF and INF settings remain available; benchmark sessions restore the caller's original limits. Drum-track DRIVE starts at0; saved patches keep their chosen drive. Four active full-level samples now get12dB summing headroom before monitor drive (was6dB). Independent drum taps and the drum-mix stem preserve their levels. Ordinary voice eviction protects the kick when a non-kick victim is available.

## Both cores
Core0 prepares PCM blocks, then renders synth tracks while core1 mixes the drums into its own five-bus accumulator. Core0 joins before delays/reverb/limiter/output. PCM positions advance once;16gain values are checkpointed so a failed/partial worker can be held, rolled back and rerun once without corrupting the mix. This offloads drum mixing, not flash reads or all PCM preparation. If track4 is melodic, this release renders its synth on core0 too.

SLOOP2.8.5/X0X hardened core HAL: checked cache-line-separated mailbox, separate user/supervisor stacks, boot self-test, guarded hold,8ms fault/timeout fallback and warm-reset bar. An idle core is paced. SLOOP's M-VAVE-stock supply initialization (VDC14/SYSVDD) is applied before core start; this is not an overclock. Core is held before boot memory clearing, resets and updater handoff. Source attribution is retained in hal/.

The display footer says2C only when core1's startup/self-test passed;1C means fallback/not available. Runtime telemetry additionally reports actual core jobs, worker/wait ticks, peak worker ticks and timeouts. No hardware proof that core1 started exists until owner flash and MIDI readback.

## Existing behavior
R38's32sample transition smoothing and SLOOP master limiter remain. Isolated outputs stay before the limiter. DLY A/B retain their page2 controls. Delay storage remains directPCM16; no companding/packing. Reverb defaults OFF. No BLE/USB playback added.

## Verification
Fresh cross-build, call-free SRAM kernels, linker/PC/copy/address bounds; pthread-vs-serial and forced-partial-timeout output/17tap parity, Address/UndefinedBehaviorSanitizer and ThreadSanitizer pass. Clean4x30000PCM mix/headroom/default caps tested. Runtime/USB/sample/limiter/UI checks pass. Actual R39 CPU and audible performance require owner manual flash and measurement.
