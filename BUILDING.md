# Standalone R39 build

The compiler is JieLi's Linux pi32v2 toolchain; it runs in a Linuxamd64 Docker container. On macOS use OrbStack and the orbstack context. No USB/MIDI device is exposed to the build container, and its build command does not install firmware.

## Dependencies

1. Install Docker/OrbStack and Python3 on the host.
2. Download the JieLi toolchain using the vendored GPL helper, for example `sh platform/tools/get_toolchain.sh .build-env/jieli`. Tested archive:jieli-linux-toolchains-20260730.1. The containing directory has a toolchain/ symlink.
3. Obtain JieLi SDK AC79NN_SDK_V1.2.1_2023-12-13 (commit d179b4484759423312073f5fbb232501aa491047) from https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK. Only cpu/wl82/tools/uboot.boot, cfg_tool.bin and cfg/eq_cfg_hw.bin are read; the platform builder verifies their known checksums. Do not commit your SDK/toolchain checkout.
4. Build the container image:

```sh
docker --context orbstack build --platform linux/amd64 -f Dockerfile.build -t dub-fm1-build:local .
```

## Build

```sh
python3 tools/build_r39.py --check
python3 tools/build_r39.py --toolchain .build-env/jieli --sdk .build-env/ac79-sdk
```

Use `--docker-context default` on a host with that context, or `--image NAME` for a compatible image. The build is network-disabled. It stages the frozen platform, overlays canonical r39 application/HAL files, uses the R39 linker/entry/build recipe, and enforces native ROM/MMIO/call-free-SRAM/core1 region guards.

Output is under `.build-env/r39-build/build/`; the original `releases/r39` package is never overwritten. Code identity is checked against r39/verified-build.json. The project does not vendor the compiler or SDK binaries as build inputs.

## Narrow host checks

R39's existing verification record and logs are retained. Example independent native concurrency test (Apple/Linux Clang with pthread support):

```sh
mkdir -p .build-env
clang -O1 -g -fsanitize=address,undefined -pthread -I r39 r39/test_dual_core.c -o .build-env/test-dual-core
.build-env/test-dual-core
```

This checks serial/parallel/partial-worker-failure PCM and17tap equivalence. A host pthread test does not prove that the second FM-1 core starts or that its deadlines are met; those checks require owner manual flash and hardware measurements.
