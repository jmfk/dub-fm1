# Initial public import

Imported DUB R39 on2026-11-25. Earlier local revisions are excluded; this commit does not reconstruct their history. Owner device logs/status, private backups, SDK/toolchain checkouts and build environments are excluded.

Validation: standalone tools/build_r39.py --check passed. Full standalone offline Docker build against the existing tested JieLi toolchain/SDK passed all native guards and reproduced the original R39.fwsc byte-for-byte. Existing R39 sanitizer/concurrency/UI evidence is retained, not relabelled as a new hardware test. No installer/device access during publication.

A staged-file secret scan passed before commit. Frozen r39/release files retain their original line endings and legacy whitespace so their recorded hashes stay valid. Hardware CPU/listening verification is still pending; publishing the repository does not certify device performance.
