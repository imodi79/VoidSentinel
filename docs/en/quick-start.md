# Quick start

[Guide home](README.md) · [🇭🇺 Magyar](../hu/quick-start.md)

## 1. Open VoidSentinel

Open `VoidSentinel.exe` and approve the Windows administrator prompt. After building from source, the ready-to-run copy is `dist\VoidSentinel.exe`.

## 2. Choose an application

Select **+ Browse...** and choose the application's `.exe` file. An EXE is the file Windows runs to start a program. Repeat this step to watch more applications.

If you are unsure which EXE to choose, check the application's shortcut properties or installation folder. Select an entry and use **Remove** to take it out of the watch list.

Keep **Include child processes** checked. This follows helper programs started by the watched application, such as an editor or an installer helper. You can turn it off before starting a session if you want to follow only the selected EXEs.

Adding an EXE to the list selects it for monitoring. Open or use that application yourself when you are ready.

## 3. Start with the simple view

Leave the profile set to **Meaningful changes**. This combines related activity and hides common background work.

Select **Start**, then carry out one small task in the watched application—for example, save a new document to a folder you can recognize. Applications that are already running are monitored too.

Begin recording before the task you want to investigate. Activity from before **Start** is outside the recording.

## 4. Stop and review

Select **Stop** and wait for the status to show **Stopped**. Review the result list. Folders appear before files within each application group, and paths are sorted alphabetically.

Double-click a result for more information, including the programs involved and the counts of successful, failed, or unconfirmed requests.

If you see too little or too much detail, change profiles or switches. The current recording is kept, so you can adjust its view after stopping.

## 5. Save what you need

Select **Save result...**, choose a location and filename, and save the CSV file. See [Saving and sharing](saving-and-sharing.md) for spreadsheet instructions.

Save before starting another session or closing the application. Each **Start** replaces the previous recording with a new one.

**Next:** [Reading results →](results.md)
