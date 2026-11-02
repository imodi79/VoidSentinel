# Filter rules

[Guide home](README.md) · [🇭🇺 Magyar](../hu/filters.md)

Rules let you hide repeated background work while keeping useful activity visible. They change the view of the current recording; hidden records remain available if you change your mind.

## Add a folder exclusion

1. Select **Filter rules...**.
2. Set **Action** to **Exclude**.
3. Set **Match** to **Folder and descendants**.
4. Enter the folder in **Path / pattern**, or use **Browse...**.
5. Leave **Application** empty to apply the rule to every watched application. You may enter an EXE name to limit its scope.
6. Optionally enter a **Description** to help you recognize the rule later.
7. Keep **Enabled** checked and select **Add**.
8. Select **Save rules** to apply the list and remember it for later runs.

For example, `%LOCALAPPDATA%\ExampleApp\Cache` targets one application's cache. You can keep that rule separate from the application's settings folder.

A faster way: right-click a result and choose **Exclude this folder...** or **Always include this folder...**. Review the suggested rule in the editor, then save the list.

## Change, disable, or remove a rule

Select the rule in the list, edit its fields, and choose **Update selected**. Changing a field alone does not update the listed rule.

To temporarily disable it, clear **Enabled**, choose **Update selected**, then **Save rules**. To remove it, choose **Remove selected**, then save.

**Cancel** closes the editor and keeps the previous active list. **Defaults** replaces the editor's list with the initial background rules; choose **Save rules** to apply that replacement.

## Choose an action

| Action | Effect |
|---|---|
| Include | Keeps a matching path visible despite Exclude or Background rules. It also retains matching transient and metadata entries. The Errors and Unconfirmed switches still control those outcomes. |
| Exclude | Hides matching paths in both profiles. |
| Background | Hides matching paths while the Background switch is off. |

**Include wins regardless of rule order.** For instance, you can exclude an application's work folder and add an Include rule for a useful subfolder inside it.

## Choose a matching method

- **Folder and descendants:** matches the named folder and everything below it. `C:\ExampleCache` matches `C:\ExampleCache\item.dat`, while `C:\ExampleCacheOther` stays separate.
- **Exact path:** matches one specific file or folder path.
- **Wildcard:** uses `*` for any sequence of characters and `?` for one character. A `*` can span folder separators, so review broad patterns carefully.

Matching ignores letter case and accepts either slash style. Use full paths or environment-variable paths.

The optional **Application** field accepts an EXE name such as `ExampleApp.exe`, a full EXE path, or a wildcard such as `Example*.exe`. It can match either the watched application or the helper performing the operation.

## Make rules useful on another computer

Usernames and drive letters can differ between computers. Windows environment variables let a rule refer to the current user's folders:

| Variable | Refers to |
|---|---|
| `%LOCALAPPDATA%` | The current user's local application-data folder. |
| `%APPDATA%` | The current user's roaming application-data folder. |
| `%PROGRAMDATA%` | Application data shared by users on the computer. |
| `%USERPROFILE%` | The current user's profile folder. |

For example, `%LOCALAPPDATA%\ExampleApp\Cache` is expanded on the computer where you import it. A literal path such as `D:\ExampleCache` still depends on that drive and folder being relevant there. Application patterns containing full paths may also need adjustment.

The folder picker and result context menu use these variables for recognized user-data folders where possible.

In summaries, exclusions follow the visible final path. Excluding a temporary source folder therefore keeps a document saved by rename into another folder visible.

## Initial background rules

The initial list marks specific NVIDIA caches and application timestamps, Windows caches, spelling dictionaries, and Notepad session-recovery state as background work. You can edit or disable these rules like any others. Broad application-data areas remain available for investigation.

## Export or import rules

**Export...** saves the list currently in the editor, including changes you have added or updated there. On another computer, use **Import (replace)...**, review the imported list, then choose **Save rules**.

Import replaces the editor list rather than combining lists. **Cancel** keeps the previously active rules. An invalid import leaves the editor list intact. See [Saving and sharing](saving-and-sharing.md) for the full transfer procedure.

**Next:** [Saving and sharing →](saving-and-sharing.md)
