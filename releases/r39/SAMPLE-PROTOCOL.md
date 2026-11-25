# SMP1 — DUB sample management over USB-MIDI

Implementation/build only. Runtime, power-loss and hardware validation pending.
This protocol manages sample DATA; it never invokes a firmware updater. DUB R39
is the first development implementation. R27/R28 do not provide SMP1.

## Transport

One outstanding request, stop-and-wait. Full MIDI SysEx:

```
F0 7D 53 4D 50 01 command requestID pack7(body || crc32LE) F7
```

`7D` is the experimental/non-commercial manufacturer namespace, `SMP` the
application namespace, `01` the protocol version. command1..8; requestID0..127.
CRC32 is standard reflected IEEE polynomial0xEDB88320, initial0xFFFFFFFF/final
complement, over `[command,requestID] || body` before7bit packing.

pack7: groups of at most7 raw bytes. Prefix each group with a byte whose bits0..6
are those bytes' high bits; then output each byte masked with0x7F. Unused high
bits in a short group's prefix must be0. No zero-length trailing group.

Replies use command|0x40, the same requestID, body `[status] || result`, and CRC
computed with the REPLY command. Read/validate identity, size and CRC before
using a reply. Ignore MIDI realtime bytes while collecting SysEx. Requests and
replies are <=160wire bytes; PCM chunks are at most96raw bytes. Little-endian
uint32 everywhere below; pad indices0..10 (CLI/UI display1..11).

Retry an identical command/ID/body after timeout, up to3times; do not assign a
new ID to a retry. Suggested timeout3sec, BEGIN15sec, COMMIT5sec. Immediate
duplicates reuse the cached reply. DATA can also acknowledge an already-written
matching range without programming again. A silent/disconnected client times
out after37seconds on the device. Until BEGIN completes, no other commands are
accepted (including duplicate BEGIN, which waits for its original result).

## Commands

| ID | Name | Request body | Successful result |
|---|---|---|---|
|1|INFO|empty|9uint32 values described below|
|2|LIST PAD|pad:u8|pad:u8, custom:u8,40byte sample record|
|3|READ BANK|generation:u32,offset:u32,count:u8|offset:u32, count bytes|
|4|BEGIN|expectedGeneration:u32,blobLength:u32,blobCRC32:u32|token:u32,nextOffset:u32(0)|
|5|DATA|token:u32,offset:u32,1..96bytes|nextOffset:u32|
|6|COMMIT|token:u32|newGeneration:u32,blobLength:u32|
|7|ABORT|token:u32|empty|
|8|READ PAD|pad:u8,generation:u32,byteOffset:u32,count:u8|pad:u8,byteOffset:u32,count bytes|

READ count1..96 and entire requested range must be in bounds. READ BANK reads an
FSB1 blob, not the private physical commit header. READ PAD reads prepared raw
signed16 mono PCM for either factory or custom sound. LIST PAD always describes
the sound currently used by that pad; the custom flag distinguishes fallback.
The offset field of a factory LIST PAD record is0 and not a bank offset.

INFO result, in order:

1. protocol version1
2. sample rate22050
3. pad count11
4. maximum blob bytes81888
5. usable custom PCM bytes81376
6. current custom PCM bytes used (factory kit does not count)
7. active generation
8. current blob bytes (512 for an empty custom catalog)
9. flags: bit0 writable flash available; bit1 transfer active

Free PCM = field5-field6. Two physical80KiB banks are reserved; the second is
rollback space, not additional advertised capacity. A new generation becomes
active only after COMMIT. Expected generation guards BEGIN and all reads against
stale concurrent clients. Refetch after STALE; never blindly overwrite another
client's changes. An empty factory-fallback catalog is virtually readable at
generation0 even if no custom bank has ever been committed.

## Transfer flow

Stop transport, release notes and stop USB audio recording. INFO -> READ BANK
using the reported generation -> edit/encode the catalog -> BEGIN -> DATA at
strict sequential offsets -> COMMIT -> INFO/LIST refresh. Delete/reset one pad
by replacing its record with zeroes and compacting remaining PCM in a new full
bank. Export READ BANK as a `.dubkit`; import via the same atomic flow. To change
only mode/name, preserve PCM and replace that record in a new bank.

BEGIN fails BUSY for playing/held sources, active preview/saving or USB audio
flow. At an audio boundary DUB clears residual FX tails and pauses audio. It
then erases only the needed sectors in the INACTIVE bank. BEGIN acknowledgement
is delayed until preparation completes. While receiving, musical commands are
discarded. Device shows progress; REC cancels. ABORT/timeout/failed transfer
resume audio without activating unfinished data. COMMIT validates blob CRC and
catalog, writes the private flash header last, publishes views/cache while still
muted, then resumes audio. Playback must be started again by the user.

Factory PCM and all presets, patterns and mixer data are unchanged. Do not
transfer samples during USB audio recording. Native flash erase still disables
interrupts; this is a stopped-instrument transfer protocol, not background
streaming. USB-MIDI only in this version; no TRS/BLE sample transfer claimed.

## Status values

0 OK;1 BUSY;2 BAD_DATA;3 BAD_CRC;4 OFFSET;5 FLASH;6 TOKEN;7 UNSUPPORTED;
8 STALE. Errors may have an empty result; DATA includes current nextOffset when
its token/shape was otherwise valid. Wrong namespace/replies/oversized malformed
frames receive no acknowledgement. An invalid owned namespace never reaches the
updater parser. After uncertain COMMIT success, refresh INFO rather than assuming
no write occurred. Lost power can activate either the last old valid generation
or a fully committed new generation, never an unvalidated partial blob; this
property is designed into the layout and still needs power-cut validation.

## FSB1 blob format

512byte catalog followed by contiguous signed16 little-endian PCM22050Hz mono.
Header16bytes: ASCII `FSB1`,version:u32(1),count:u32(11),totalBlobBytes:u32.
Then11records of40bytes:

| Offset | Bytes | Field |
|---|---:|---|
|0|24|Nonempty printable ASCII name, max23characters, NUL and zero padding|
|24|4|PCM byte offset relative to byte512|
|28|4|PCM frame count, minimum2|
|32|4|Rate22050|
|36|4|Mode0 one-shot or1 loop|

Absent custom records are40zero bytes and use that pad's builtin sound. Data
for present records is concatenated in pad order, no gaps/overlap; each offset
must equal the sum of preceding present lengths. Bytes456..511 are0. Exact
blob size512+sum(frames*2), maximum81888. PCM capacity81376bytes (~1.845sec total
custom audio). No embedded runtime pitch/trim/EQ/gain processing: bake all edits
on the host. One-shot live hits finish naturally; sequenced notes retain their
existing gate fade. Loop cycles over the entire prepared sample while held,
with note-off/source ownership and a short fade. No independent loop subregion.
WAV export from DUB Librarian embeds a full-range `smpl` loop when selected.

## Physical storage (FM-1 only)

A0xB7000..0xCB000; B0xCB000..0xDF000,80KiB each. Preserve8existing16KiB song
slots0x97000..0xB7000, updater0xE0000+, preset blobs0xF4000+, settings0xFC000+.
The extra sector0xDF000..0xE0000 remains unused. Narrow app write whitelist is
extended only for these sample banks; underlying flash guard remains active.
Private32byte PFS1 header: magic:u32(0x31534650),version1,generation,blobLength,
blobCRC,CRC32(first20bytes),reserved0,~magic. Header is written last. Boot checks
headers/blob/catalog and chooses newest valid generation with wrap-safe ordering.

## Reference client

`sample_tool.py` uses standard Python plus mido/python-rtmidi for actual MIDI.
It does not import MIDI until an explicit command opens the named ports. From
project root (examples are instructions, not commands run by Codex):

```sh
.build-env/midi-venv/bin/python dub-synth/r39/sample_tool.py --input Felucca --output Felucca info
.build-env/midi-venv/bin/python dub-synth/r39/sample_tool.py --input Felucca --output Felucca list
.build-env/midi-venv/bin/python dub-synth/r39/sample_tool.py --input Felucca --output Felucca export-bank backup.dubkit
.build-env/midi-venv/bin/python dub-synth/r39/sample_tool.py --input Felucca --output Felucca export-pad 1 kick.wav
.build-env/midi-venv/bin/python dub-synth/r39/sample_tool.py --input Felucca --output Felucca set-pad 1 prepared.wav --name KICK --loop
.build-env/midi-venv/bin/python dub-synth/r39/sample_tool.py --input Felucca --output Felucca reset-pad 1
```

set-pad requires already-converted mono22050Hz16bit PCM WAV. DUB Librarian does
conversion. Export never silently overwrites an existing CLI output. Import-bank
explicitly replaces the full custom bank after validation/generation check.
