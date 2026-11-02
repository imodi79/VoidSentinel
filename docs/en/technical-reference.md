# Technical reference

[Guide home](README.md) · [🇭🇺 Magyar](../hu/technical-reference.md)

This page provides accuracy details and developer commands. For everyday operation, start with [Quick start](quick-start.md).

## How monitoring works

VoidSentinel uses Event Tracing for Windows (ETW) to observe file, thread, and process events. The ETW consumer runs on a dedicated trace thread.

A separate session worker handles capture start/stop, result snapshots, summaries, matching rules, sorting, result export, and watch-list writes. Settings loading and rule-file import/export also use background tasks. The interface reads immutable snapshots through a virtual Windows list control; large superseded views are reclaimed on the worker.

Rule patterns are expanded once per view build. Rename edges are indexed by application and path. Captured requests are aggregated in memory, so repeated activity on an existing entry increases its counts rather than storing every request as a new row.

## Interpret requests carefully

- **Success** means the observed Windows request returned successfully. Cached or memory-mapped writes may reach storage later, potentially through another process.
- **Deletion requested** records an accepted deletion request. Open handles can delay actual removal.
- **Transient candidate** means creation followed by an accepted deletion request was observed. It does not confirm the path is currently absent.
- Process chains describe observed parent/child relationships, not proof of a user's intent. Work handed to an independent service is attributed to that service when it is explicitly monitored.
- Only activity after Start is available. Earlier operations cannot be reconstructed.
- Rename summaries use successful destinations matched through kernel file events and request identity. Ambiguous overlapping lifetimes stay separate. Incomplete matches can be inspected through technical/unconfirmed views.
- Folder renames, hard links, path aliases, and overlapping lifetimes can make path-based summaries incomplete.
- The meaningful view can show successful requests from an entry that also contains failures internally. Category switches retain access to the other outcomes.

## Resource use and completeness

Memory usage grows with distinct captured entries. The pending-request table is limited to 50,000 entries and the filename cache is bounded. Overflow and ETW loss are reported.

Windows file events are collected system-wide and filtered by process during consumption. Resource use therefore also depends on overall disk activity. A visibility filter does not narrow the Windows event source.

One instance can capture at a time. Normal shutdown closes its ETW session; a later Start can recover its own abandoned session after a forced exit.

## Settings and export formats

Settings are stored in `%LOCALAPPDATA%\VoidSentinel\targets.txt` and `rules.tsv`. CSV results use UTF-8 with a BOM, comma separators, quoted text fields, and CRLF record endings. Export and rule saves use temporary files followed by replacement of the destination.

Rules use a versioned UTF-8 TSV format. Its first line is `VoidSentinel-Rules`, a tab, then `1`. Each later record has six tab-separated fields: enabled (`0` or `1`), action, match mode, path/pattern, optional application pattern, and description. Empty fields still need their separating tabs.

Actions are `include`, `exclude`, and `background`. Match modes are `folder`, `exact`, and `wildcard`. Limits are 4 MiB and 10,000 rules. Invalid imports leave the existing list unchanged. For normal editing, use the rule editor.

## Run tests

After [building](build.md), run from the project folder:

```powershell
ctest --test-dir out -C Release --output-on-failure
python tests\verify_csv.py out\results-test.csv
python tests\verify_icons.py dist\VoidSentinel.exe
```

Python is needed for the verification scripts. The C++ test suites cover aggregation, summaries, rules, persistence, invalid imports, background exports, concurrent snapshot reads, and distinct rename destinations.

For real ETW integration tests, stop recording in other instances and choose a fresh test folder. The test probe creates, writes, renames, and deletes test files inside that folder and explicitly exports results:

```powershell
$p = Start-Process .\dist\VoidSentinel.exe -Verb RunAs -ArgumentList '--capture-test "C:\VoidSentinelTest"' -PassThru -Wait
$p.ExitCode
python tests\verify_capture.py C:\VoidSentinelTest
```

Integration coverage includes Unicode paths, parent/child activity, unrelated-process exclusion, 10,000 repeated writes, atomic-save renames, hidden-record recovery, and the background session worker.

## Isolated interface tests

This development target uses normal user privileges and its own settings folder, allowing interface tests separately from elevated capture tests:

```powershell
cmake --build out --config Release --target VoidSentinelUiTest
.\out\ui-test\Release\VoidSentinel.exe
```

Use `dist\VoidSentinel.exe` for ordinary monitoring.

## Icon assets

The embedded icon contains transparent images at 16, 20, 24, 32, 40, 48, 64, 96, 128, and 256 pixels. Assets are in `assets/icons`. After changing the PNG sources, run `python assets/icons/build_ico.py` to repackage the ICO without resampling.

## Windows documentation

- [File I/O event classes](https://learn.microsoft.com/en-us/windows/win32/etw/fileio)
- [ETW session creation](https://learn.microsoft.com/en-us/windows/win32/api/evntrace/nf-evntrace-starttracew)
- [Event tracing and loss statistics](https://learn.microsoft.com/en-us/windows/win32/etw/about-event-tracing)
