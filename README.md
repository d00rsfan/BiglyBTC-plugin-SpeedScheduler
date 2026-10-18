# BiglyBTC-plugin-SpeedScheduler

Native C++23 internal service for original **Speed Scheduler**, runtime ID
**SpeedScheduler**. The project name follows BiglyBTC naming; original plugin
names/settings remain unchanged. This is not a Java-JAR ABI loader.

The pure policy reducer accepts an explicit local day/minute and owned torrent
values. It returns commands and rate changes; it performs no networking or file
writes. It preserves original overlap, reversed/equal interval, category, forced
start, default-rate restoration and pause-ownership behavior. XML import retains
the entire original input, including unknown content and UTF16 encoding.

GCC units/frozen XML fixtures pass. Original-source differential tests cover
151200 time values and6336 torrent states using the unchanged original thread and
pause-file helpers with explicit host-value boundaries. Frozen observations can
be replayed without a runtime JVM. Evidence and remaining gates are under
`migration/evidence/P04-05` and `migration/tasks/P04-05-speed-scheduler.md`.

Build separately with CMake, selecting the pinned GCC16 or Clang24 and Qt6.12.0
prefix `/opt/Qt/6.12.0/gcc_64`; use an isolated `/tmp/biglybtc-*` build directory.
Run CTest there. Native ownership-file persistence, actual engine/AutoSpeed/tag
integration, the editor and release licensing remain required before enabling
this feature in the client.

The ownership codec reads existing PausedTorrents.conf bytes and returns bounded
replacement bytes after pause/resume acknowledgement. It preserves unknown raw
lines, mixed line endings and original first-only duplicate removal. Ten original
private-helper observations are frozen for replay; a foreign unterminated last
line is explicitly rejected on append because Java merges and loses the new
ownership. GCC/Clang five-suite runs pass, including50000 indexed lookups. All five suites also pass with address, undefined-behavior and leak checks.
Actual file persistence and native host enablement remain required.

The development build now also creates an independently loadable module using
BiglyBTCPluginSDK0.1.0 (`find_package`, installed SDK prefix required). The module
returns rate and torrent commands through the C ABI, with caller-owned snapshots
and output buffers; it does not apply them to the engine. A sixth suite replays
the original6336 command/rate/ownership states through that module boundary.
Use `BIGLYBTC_BUILD_PLUGIN=OFF` to build only the original static policy pilot.
Independent source publication remains pending the upstream redistribution grant.
