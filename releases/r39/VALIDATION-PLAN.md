# R39 validation

Local: exact serial/pthread/partial-timeout PCM and17taps over12288frames with varying1/7/32blocks, pad pans/sends, steals and independent synth states. ASan/UBSan and TSan pass. Clean4voice high-level headroom/defaults/kick protection; SLOOP limiter reference/voice fades; native simultaneous track participation, extended137byte runtime/lease, resumable hosts, sample power-cut/USB isolation, physical delay UI and WASM checked. Kernel SRAM call/ROM/MMIO guards and core1 stack/relocation bounds checked.

After manual flash: verify revision39, footer2C/state3, increasing completed jobs and zero timeouts. Then new canary/profile workload; inspect worker time vs core0 wait and sample/OSC totals, pre-limiter clips/gain and deadline misses. Listen to clean drum mix and busy arrangement. Record any1C fallback explicitly. Do not infer two-core hardware success from host pthread tests.
