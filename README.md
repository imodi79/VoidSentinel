<p align="center">
  <img src="assets/icons/VoidSentinel_256.png" width="256" height="256" alt="VoidSentinel app icon">
</p>
<h1 align="center">VoidSentinel</h1>

**See which files and folders an application changes on your Windows PC.**

Choose one or more applications, start monitoring, and use them as usual. VoidSentinel shows the files and folders they create, write, rename, or request to delete, including work done by their child processes. It helps you understand where an application saves documents, settings, caches, and other working files.

The default **Meaningful changes** view combines related operations into readable file summaries and hides common background activity. Switch to **Full application footprint** to inspect the technical details, customize reusable filter rules, and export the current results to CSV.

## Build

Requirements: Windows 11 x64 and Visual Studio Community with **Desktop development with C++**, the MSVC x64/x86 toolset, Windows 11 SDK, and C++ CMake tools for Windows.

From PowerShell in the project folder:

```powershell
.\build.ps1
```

Run **`dist\VoidSentinel.exe`**. The compiler output is also available at `out\Release\VoidSentinel.exe`. Administrator approval is required to start the production application and collect kernel file events. The portable EXE includes the application icons and C/C++ runtime.

For a debug build:

```powershell
.\build.ps1 -Configuration Debug
```

## Quick start

1. Use **+ Browse...** to add application EXEs.
2. Keep **Include child processes** enabled to follow helper applications and subprocesses.
3. Select **Start**, then use the watched applications. Existing and newly started instances are monitored.
4. Select **Stop** when finished. Results remain available until the next Start or until the application closes.
5. Use **Save result...** to export the current filtered view as CSV. While monitoring, this saves an immutable snapshot of the displayed view.

Each **Start** begins a fresh capture. Capture results stay in memory; export the results you want to keep. The application list and filter rules are saved automatically when changed.

## Views and summaries

**Meaningful changes** groups results under the watched root application. A file created by a parent and written by its child appears in one summary, with both contributors in **Process chain**.

For example:

| Root application | Path | Change | Contributors |
|---|---|---|---|
| TOTALCMD64.EXE | E:\d\ac | Created; Written / resized | TOTALCMD64.EXE → Notepad.exe |

Repeated writes are counted in one entry. Write, allocation-size, and end-of-file operations become **Written / resized**. Create, rename, deletion requests, and optional metadata changes are combined in the same file summary. **Count** is the number of underlying included I/O requests, rather than the number of user actions.

**Full application footprint** enables technical rows and all event categories. Explicit path exclusions still apply; edit or disable those rules to reveal their records.

The individual switches let you inspect:

- **Technical rows** — separate application/path/operation records.
- **Errors** — failed requests.
- **Unconfirmed** — incomplete/pending requests and unresolved file paths.
- **Background** — paths categorized as background activity.
- **Transient** — candidates created and subsequently requested for deletion during this capture.
- **Metadata** — attributes, timestamps, and other set-information operations.

Hidden records remain in the current capture. Change profiles, switches, or rules during or after monitoring to rebuild the view from those records. Status-bar counters explain how many underlying requests were hidden in each category. **Lost** and **Dropped** indicate capture completeness problems.

Within each application group, folders precede files; paths are alphabetically sorted. Entries whose type could not be determined are marked **Unknown** and appear last. Double-click an entry for contributors, rename destination, and success/failure/unconfirmed counts.

## Filter rules

Open **Filter rules...** to add, edit, enable/disable, remove, import, or export rules. Use **Add** for a new rule or **Update selected** to change the selected rule, then **Save rules** to persist the list and apply it. **Cancel** keeps the previous list. **Defaults** restores the initial background rules in the editor; save to apply them.

Right-click an existing result to start an **Exclude this folder...** or **Always include this folder...** rule.

### Actions

| Action | Effect |
|---|---|
| Include | Overrides matching Exclude and Background rules, and keeps matching transient/metadata entries. Error and unconfirmed switches still control those outcomes. |
| Exclude | Hides matching paths in both profiles. Captured records remain available if the rule is disabled. |
| Background | Hides matching paths unless Background is enabled. |

Include takes precedence regardless of rule order. An optional application pattern can match the watched root or the executing process, by EXE name or full path.

### Matching and portability

- **Folder and descendants** matches the exact folder and its descendants, with path-boundary checks: `C:\Cache` does not match `C:\CacheOther`.
- **Exact path** matches one path.
- **Wildcard** supports `*` and `?`. A star may span directory separators.
- Matching is case-insensitive and normalizes path separators, trailing separators, and common Windows path prefixes.
- Portable environment variables are expanded on the destination machine: `%LOCALAPPDATA%`, `%APPDATA%`, `%PROGRAMDATA%`, and `%USERPROFILE%`.
- The folder picker and result context menu use environment variables for recognized user-data folders where possible. Absolute drive paths remain machine-specific.
- In the summary view, exclusions apply to the visible final path. Excluding a temporary source directory therefore keeps a document saved by rename into another directory visible.

The initial rules categorize specific NVIDIA caches and application timestamps, Windows caches, spelling dictionaries, and Notepad session-recovery state as background activity. Application settings elsewhere remain visible.

### Save, export, and import

Settings are stored under:

```text
%LOCALAPPDATA%\VoidSentinel\targets.txt
%LOCALAPPDATA%\VoidSentinel\rules.tsv
```

**Export...** writes a portable UTF-8 rules file. On another PC, **Import (replace)...** loads that file into the editor; **Save rules** applies and persists it. Import replaces the editor list rather than merging it. Invalid imports leave the existing list intact, and saving uses a temporary file followed by an atomic replacement.

The versioned TSV format begins with `VoidSentinel-Rules<TAB>1`. Subsequent records have six tab-separated fields:

```text
enabled    action       match       path/pattern       application       description
1          exclude      folder      %LOCALAPPDATA%\Example\Cache
```

The example is visually spaced; actual files use tabs, including empty application/description fields. Supported actions are `include`, `exclude`, and `background`; match modes are `folder`, `exact`, and `wildcard`. Files are limited to 4 MiB and 10,000 rules.

## CSV export

CSV exports contain the current view, in its current order, with full application paths, first/last timestamps, counts, outcomes, root applications, process chains, rename destinations, and original paths. Files use UTF-8 with a BOM, comma separators, quoted text fields, and CRLF record endings. Excel users can select comma separation when importing the CSV.

## How capture and filtering work

The ETW consumer processes file, thread, and process events on a dedicated trace thread. A separate session worker handles capture start/stop, snapshots, semantic summaries, rule matching, sorting, CSV output, and application-list writes. Rule-file imports/exports and settings loading also use background tasks.

The GUI consumes immutable snapshots and updates a virtual Windows list control. Large superseded views are reclaimed on the session worker. Rule patterns are expanded once per view build, and rename edges are indexed by application/path. Capture records are aggregated in memory rather than accumulated as an unbounded raw-event stream.

Successful observed rename destinations are correlated through kernel file events and I/O request identity. Related temporary-file operations can be presented under the final destination. Ambiguous overlapping lifetimes are kept separate instead of guessed. Explicitly watched child EXEs form their own root groups.

## Interpretation and limitations

- **Success** means the observed I/O request returned successfully. Cached or memory-mapped writes may be persisted later by another process.
- **Deletion requested** records an accepted deletion request; open handles can delay actual removal. **Transient candidate** means creation followed by an accepted deletion request was observed. It is a classification, not proof that the path is currently absent.
- Process chains describe observed parent/child relationships. Work delegated to an independent service is attributed to that service when it is explicitly monitored.
- Capture starts at Start. Earlier activity and services outside the watched process tree cannot be reconstructed.
- Rename destinations are used when a matching path event is available. Incomplete correlations remain visible through technical/unconfirmed views.
- Failed and successful requests may be aggregated together internally. The meaningful view selects the successful subset while retaining the other counts for technical inspection.
- The memory footprint grows with the number of distinct captured records. The pending-I/O table is limited to 50,000 entries and the filename cache is bounded. Overflow or ETW loss is reported.
- One VoidSentinel instance can capture at a time. Normal shutdown stops its ETW session; a later start can recover its own abandoned session after a forced exit.
- Kernel file events are system-wide and are filtered by process during consumption, so overhead also depends on the PC's overall I/O activity.
- Folder renames, hard links, path aliases, and overlapping file lifetimes can make path-based summaries incomplete. Use the technical view to inspect the observed paths.

## Tests

Run unit and integration-model tests after building:

```powershell
ctest --test-dir out -C Release --output-on-failure
python tests\verify_csv.py out\results-test.csv
```

For real ETW integration tests, stop capture in other instances and use a fresh test directory:

```powershell
$p = Start-Process .\dist\VoidSentinel.exe -Verb RunAs -ArgumentList '--capture-test "C:\Tmp\VoidSentinel-test"' -PassThru -Wait
$p.ExitCode
python tests\verify_capture.py C:\Tmp\VoidSentinel-test
python tests\verify_icons.py dist\VoidSentinel.exe
```

The probe performs file operations inside the chosen test directory and explicitly exports its results. Python is a development verification tool.

Coverage includes repeated-write aggregation, parent/child grouping, child and unrelated-process exclusion, Unicode paths, atomic-save renames, transient candidates, successful subsets of mixed outcomes, rule precedence and path boundaries, portable rule round trips, invalid imports, background exports, concurrent frame reads, and 10,000 distinct rename destinations.

A development-only UI target uses normal user privileges and isolated settings:

```powershell
cmake --build out --config Release --target VoidSentinelUiTest
.\out\ui-test\Release\VoidSentinel.exe
```

The production EXE continues to request administrator rights for ETW capture.

## Icons

The embedded Windows icon includes 16, 20, 24, 32, 40, 48, 64, 96, 128, and 256 pixel images with transparency. Source assets are in `assets/icons`. After changing the PNGs, run `python assets/icons/build_ico.py` to repackage the ICO without resampling.

## Windows references

- [File I/O event classes](https://learn.microsoft.com/en-us/windows/win32/etw/fileio)
- [ETW session creation](https://learn.microsoft.com/en-us/windows/win32/api/evntrace/nf-evntrace-starttracew)
- [Event tracing and loss statistics](https://learn.microsoft.com/en-us/windows/win32/etw/about-event-tracing)
