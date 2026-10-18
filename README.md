# BiglyBTC-plugin-SpeedScheduler

Native C++23 policy and shared module for original **Speed Scheduler**, runtime ID
**SpeedScheduler**. Original plugin names/settings remain unchanged. This is not a
Java-JAR ABI loader.

The native source is published at
[d00rsfan/BiglyBTC-plugin-SpeedScheduler](https://github.com/d00rsfan/BiglyBTC-plugin-SpeedScheduler),
first source commit `91c417ca5aa5494f8c5791d8f29cf96350a0071d`.
The published [SOURCE_PROVENANCE.json](SOURCE_PROVENANCE.json) records the immutable
native review commit, pinned public SDK commit and exported file hashes.
See [NOTICE.md](NOTICE.md) for original-source attribution and license provenance.
No original Java source tree or user configuration is included.

The pure policy reducer accepts an explicit local day/minute and owned torrent
values. It returns commands and rate changes; it performs no networking or file
writes. It preserves original overlap, reversed/equal interval, category, forced
start, default-rate restoration and pause-ownership behavior. XML import retains
the entire original input, including unknown content and UTF16 encoding.

Original-source differential observations cover 151200 time values and 6336
torrent states using the original thread and pause-file helpers with explicit
host-value boundaries. Frozen observations replay without a runtime JVM. The same
6336 command/rate/ownership states also replay through the shared-module C ABI;
that is a separate execution path, not additional original cases.

The ownership codec reads existing PausedTorrents.conf bytes and returns bounded
replacement bytes after pause/resume acknowledgement. It preserves unknown raw
lines, mixed line endings and original first-only duplicate removal. Ten original
private-helper observations are frozen for replay; a foreign unterminated last
line is explicitly rejected on append because Java merges and loses the new
ownership.

Build separately with CMake and Ninja, using the pinned GCC16 or Clang24,
Qt6.12.0 Core, and an installed BiglyBTCPluginSDK0.1.0 package. Pass the SDK install
prefix and Qt prefix through `CMAKE_PREFIX_PATH`; the SDK commit for this first
export is `eefbd205ca0940ad4868ee07989b2d42be1a99dc`. Use a fresh build directory
outside the checkout; it need not be under `/tmp`. Replay scripts create their own
isolated `/tmp/biglybtc-*` fixture/module copies for the existing driver fences.
`BIGLYBTC_BUILD_PLUGIN=ON` (default) builds the module and all six CTest suites:

- scheduler policy
- independently loaded scheduler module fixtures
- original XML/time fixtures
- scheduler tick fixtures
- pause-ownership codec
- original pause-ownership fixtures

Fresh standalone local GCC and leak-enabled ASan+UBSan runs pass all six suites,
with the SDK's two C ABI/module-lifecycle suites tested separately. Hosted runs
are recorded in the repository's [Actions](https://github.com/d00rsfan/BiglyBTC-plugin-SpeedScheduler/actions).
Use `BIGLYBTC_BUILD_PLUGIN=OFF` to build only the static policy pilot and its five
suites.

The module returns rate and torrent commands through the C ABI in caller-owned
snapshots/output buffers; it does not apply them to the engine. Actual
ownership-file persistence, native host enablement, engine/AutoSpeed/tag
integration, timers and the editor remain unfinished. This bounded source port
is not a complete operational plugin or binary release.
