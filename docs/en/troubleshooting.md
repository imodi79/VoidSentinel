# Troubleshooting

[Guide home](README.md) · [🇭🇺 Magyar](../hu/troubleshooting.md)

## The list is empty

Check these in order:

1. Confirm that you added the correct application EXE and selected **Start**.
2. Perform a task that changes a file, such as saving a document. Reading a file alone is outside the displayed change operations.
3. Enable **Include child processes** before starting if the application uses helpers.
4. Choose **Full application footprint** to reveal more categories.
5. Check **Filter rules...** for an Exclude rule affecting the location.

Some shortcuts start a launcher or ask an already running, separate application to do the work. If the actual writer is outside the watched process tree, add that writer's EXE as another target. Starting capture cannot recover activity that happened earlier.

## There is too much background activity

Use **Meaningful changes** first. Leave Background, Metadata, Errors, Unconfirmed, and Transient off unless you need them.

For a repeated noisy location, right-click its result and create a folder rule. Choose **Background** if you want a convenient on/off switch, or **Exclude** to hide it in both profiles. Keep rules focused on relevant subfolders so other application settings remain visible.

## A file is still missing from the simple view

Turn on the category switches or choose the full profile. A path may be classified as background, transient, or unresolved. Include rules can override path exclusions; failed or unconfirmed outcomes still need their corresponding switches.

## Monitoring cannot start

- Approve the administrator prompt when opening the production EXE.
- If another VoidSentinel instance is already monitoring, select **Stop** there first. One instance can capture at a time.
- Read the status-bar error message. If Windows prevents monitoring on a managed computer, contact its administrator.

After a forced application exit, a later Start can recover VoidSentinel's abandoned monitoring session.

## Lost or Dropped is greater than zero

Some events were missed, so treat the recording as incomplete. Save it if useful, then repeat a smaller task in a shorter session, with less unrelated disk activity where practical.

Changing visible filters affects the view; it does not reduce the underlying Windows event stream. Missing events cannot be restored by changing the view after capture.

## Unknown types or unresolved paths

**Unknown** means the file/folder type could not be determined. An entry such as `[unresolved file object ...]` means its path could not be matched. Enable **Unconfirmed** or use the full profile to inspect these records.

These labels describe gaps in the available event information. They are separate from a Windows request failing.

## Saving or importing fails

- Choose a writable location for exports.
- Close a destination file if another application has locked it, then try saving again.
- Import the `.tsv` rules file produced by **Export...**, rather than a result CSV.
- Check environment variables and full paths after transferring rules.

Invalid imports keep the current list intact. A failed replacement save is designed to keep the previous destination file. The rule editor shows its error message; result-export errors appear in the status bar.

## The icon or version looks old

Use the latest `dist\VoidSentinel.exe` after building. Refresh the folder view in File Explorer or Total Commander. Older build directories can contain copies from earlier development versions.

## Information to include in a bug report

Describe the Windows version, application being watched, selected profile and switches, relevant rules, steps to reproduce, and any Lost/Dropped counts. Add a small CSV or screenshot if useful, after reviewing it for personal paths and filenames.

**Next:** [Build from source →](build.md)
