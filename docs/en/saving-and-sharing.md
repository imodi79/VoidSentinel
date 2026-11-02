# Saving and sharing

[Guide home](README.md) · [🇭🇺 Magyar](../hu/saving-and-sharing.md)

There are two different things you can save: **results** from a monitoring session and **rules** that control which activity you see.

## Save monitoring results

1. Stop monitoring if you want the completed session's results.
2. Choose the profile, switches, and rules you want reflected in the export.
3. Select **Save result...**.
4. Choose a filename and folder in the Windows save dialog.
5. Check the status bar for confirmation or a save error.

The CSV contains the current filtered view in the same order as the list, including full paths, counts, time ranges, contributors, rename destinations, and original paths.

You can save while monitoring. That saves a fixed snapshot of the view at the time you selected the button; later activity requires another export.

The save button becomes available when the current view has results. Hidden records are included only if you reveal them before exporting.

## Open a CSV in a spreadsheet

A CSV is a text file containing a table. You can open it in spreadsheet applications or a text editor.

If Excel puts everything into one column, use its text/CSV import feature and choose **comma** as the separator and **UTF-8** as the encoding. The file includes an encoding marker to help preserve accented characters.

An export contains full application and file paths. Review it before sharing if folder names or filenames contain personal information.

## Move rules to another computer

On the first computer:

1. Open **Filter rules...**.
2. Add or update the rules you want to transfer.
3. Choose **Export...** and save the `.tsv` file.
4. Transfer that file to the other computer.

On the second computer:

1. Open VoidSentinel and select **Filter rules...**.
2. If you want to keep the current list as a backup, export it first.
3. Select **Import (replace)...** and open the transferred file.
4. Review paths and optional application restrictions. Environment-variable paths adapt to the current user; literal drive paths may need editing.
5. Choose **Save rules** to apply and persist the imported list.

A rules file contains filtering instructions. It does not include captured results or the watch list of application EXEs.

## What is remembered

| Item | Lifetime |
|---|---|
| Current capture results | Until the next Start or application exit; save them as CSV to keep a copy. |
| Watch list | Saved when applications are added or removed. |
| Filter rules | Saved when you choose Save rules in the editor. |
| Profile and category switches | Choose these for the current run; rules are the reusable part of the filter setup. |

The watch list and rules live in `%LOCALAPPDATA%\VoidSentinel`. To open that folder, paste the path into File Explorer's address bar. Use the rule editor for ordinary changes.

**Next:** [Troubleshooting →](troubleshooting.md)
