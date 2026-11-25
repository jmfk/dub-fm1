# R39 benchmark

The14-case stage benchmark checks actual oscillator counts and nonzero contributions from all intended tracks; it never reports stable for silently stolen tracks. Global source defaults4PCM/8OSC are restored after leased tests. Stress cases intentionally use their own limits; reporting actual counts remains essential. Runtime GET4/SET5 now returns137bytes (126payload fields); R38 clients expecting111bytes must use the R39 scripts. Legacy CPU commands2/3 remain14bytes. Profile6/7 is591bytes with revision39.

Core state0=host/not built,1=not started,2=failed/fallback,3=ready. Jobs/wait/worker/peak/timeouts prove actual hardware activity. Profile stage wall times include only core0 preparation/wait/merge; remote drum work overlaps synths and is reported separately. Do not add remote worker ticks to core0 wall time or call that a single CPU load. A ready footer alone is not proof of jobs completed.

No real R39 results before owner flash. Run Profilera DUB R39.command for distinct2-case canary then14-case run. General Benchmark DUB R39.command runs16cases. Persistent results under dub-synth/hardware-reports/benchmarks, exact-command resume, fingerprint mismatch rejection and REC/15s lease restoration retained. USB audio must not stream during these measurements.
