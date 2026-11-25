# R39: delay settings on page 2

In FX, press DLY A or DLY B again to toggle that delay between page 1 (time/feedback/sync/route) and page 2 (ENGINE/RATE/BITS/DEGRADE). Each delay remembers its page. Settings/CPU no longer provides a DELAY entry. This replaces the older SET + SHIFT/HOME instructions below. Physical controls only; screen is read-only. Audio DSP is unchanged from R36.

# R39 direct PCM16 delays

ALL delay audio buffers now store signed16-bit PCM directly. The logarithmic
encoder/decoder,4-bit lookup and nibble packing have been removed from target
source and symbols. No audio companding or compressed ring representation remains.
The four stereo ring arrays still occupy192KiB; each channel has24576 PCM cells.

SET > SHIFT+HOME opens independent Delay A/B pages (HOME/SAVE). K1 ENGINE,
K2 RATE, K3 BITS, K4 optional LOFI feedback DEGRADE. BITS16/12/8/4 is only a
linear sound effect; storage always remains16bit and capacity never changes.
DEGRADE OFF/1..127 adds sample-hold/quantization in each feedback circulation.

| Engine/rate | Maximum |
|---|---:|
| FILTERED22.05kHz | 1s |
| FILTERED11.025kHz | 2s |
| FILTERED5.5125kHz | 4s(default) |
| LOFI22.05kHz | 1s |
| LOFI11.025kHz | 2s |
| LOFI5.5125kHz | 4s |
| LOFI2.75625kHz | 8s |
| LOFI1.378125kHz | 16s |
| LOFI689.0625Hz | 32s |

All capacity figures are identical for every bit-effect choice. Long LOFI
intentionally loses bandwidth. Outer44.1/22.05 conversion/dry alignment remain;
FILTERED uses0/1/2 inner stages and LOFI box-average/sample-held return. The
normal default tick rate is the same5.5125kHz as R35, with raw PCM replacing its
codec; its maximum is4s instead of8s. Existing402/536ms default echo times fit.
Wet audio intentionally changes because compression is gone. Dry/stems/transport
are reference-checked unchanged; the original/new drum renderers match exactly.

Changing config invalidates that delay only without clearing its full ring;
B/reverb continue. Stored patch time survives capacity changes, while playback,
editing and sync/modulation honor the audible clamp. Existing signed16-bit time
fields and beat divisions remain compatible. Choices are global/session-only;
boot restores FILTERED5.5kHz,4s max,16bit effect OFF, feedback DEGRADE OFF.

Runtime fields12..19 remain A then B: ENGINE0/1, RATE0..5(exponent of22050Hz;
FILTERED clamps exponent to2), BITS0..3 ->16/12/8/4, DEGRADE0..127. Appended
snapshot49..56;57-byte body/68-byte response. Legacy2/3 replies unchanged.
Bundled R39 tools gate release36; R35 bit/rate semantics must not be assumed.
Leased benchmark sessions save/restore every setting and never write patches.

Actual CPU gains await owner manual flash and real profile comparison. A lower
maximum by itself is not proof of lower CPU; the codec removal and drum SRAM
work are measured separately by the retained stage profiler.
