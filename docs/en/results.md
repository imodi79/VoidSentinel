# Reading results

[Guide home](README.md) · [🇭🇺 Magyar](../hu/results.md)

## Start with Meaningful changes

This profile answers: **Which files and folders did the watched application affect?** It combines related operations on the same path into one summary under the watched application.

For example, a fictional application might start a helper to save a report:

| Application | Path | Change | Process chain |
|---|---|---|---|
| ExampleApp.exe | `C:\ExampleOutput\report.txt` | Created; Written / resized | ExampleApp.exe → ExampleHelper.exe |

The process chain shows the observed relationship between the applications involved. A helper application is also called a *child process*. If you explicitly add that child's EXE to the watch list, it forms its own watched application group.

Repeated writes are counted instead of adding a new row each time. File writes and changes to the file's size are summarized as **Written / resized**. When a program saves through a temporary file and then renames it, the summary can show the final destination and retain the original path in the CSV.

## What the columns mean

| Column | Meaning |
|---|---|
| Application | The watched application in summaries, or the executing application in technical rows. |
| Type | Folder, File, or Unknown if Windows did not provide enough information. |
| Path | The full location affected. Summary rows may show the final rename destination. |
| Change / operation | The readable summary or the individual technical operation. |
| Count | Included underlying file-operation requests. Ten requests do not necessarily mean ten user actions. |
| First seen / Last seen | The recorded time range for the entry, shown in local time. |
| Result | Success, mixed results, or a request outcome. |
| Process chain | The watched application and its observed helper chain. Multiple contributors can be listed. |
| Rename destination | The destination when it could be matched to the rename request. |

Scroll horizontally to see additional columns. Double-click an entry for its details. The CSV also includes the full application paths and separate success, failure, and unconfirmed counts.

Within each application group, folders come first, then files, with each section sorted by full path. **Unknown** entries appear last.

## When to use Full application footprint

Choose this profile to investigate settings, caches, temporary files, or failed requests. It enables technical rows and all category switches.

Your explicit **Exclude** rules still apply. Disable or edit a rule if you want to inspect activity it hides.

## Category switches

| Switch | Shows |
|---|---|
| Technical rows | Separate application/path/operation entries instead of file summaries. |
| Errors | Requests that failed. |
| Unconfirmed | Pending or incomplete requests, and file objects whose path could not be resolved. |
| Background | Paths marked as background activity by your rules. |
| Transient | Candidates created and subsequently requested for deletion during the session. |
| Metadata | Attributes, timestamps, and other file-information changes. |

You can change these during or after recording. Hidden records remain available until the next **Start** or until you close the app.

## Read the status bar too

**Rows** is the visible result count; **Events** counts captured underlying requests. **Hidden** counters explain how requests were removed from the current view. They count requests, so their values can be larger than the number of visible rows.

If **Lost** or **Dropped** is above zero, the recording is incomplete. See [Troubleshooting](troubleshooting.md).

**Success** describes a successful Windows request. In particular, **Deletion requested** does not prove that a file has already disappeared: another program may still have it open. **Transient candidate** is a classification based on observed requests. See [Technical reference](technical-reference.md) for accuracy details.

**Next:** [Filter rules →](filters.md)
