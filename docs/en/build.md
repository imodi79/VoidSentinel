# Build from source

[Guide home](README.md) · [🇭🇺 Magyar](../hu/build.md)

Building turns the project's source files into the application you can run. If you already have a built EXE, go directly to [Quick start](quick-start.md).

## Install the tools

Use Windows 11 x64. Install **Visual Studio Community** and select **Desktop development with C++** in its installer. Include:

- MSVC C++ build tools for x64/x86.
- Windows 11 SDK.
- C++ CMake tools for Windows.

Git is useful for downloading and updating a checkout. You can also work from a downloaded source archive.

## Build the application

1. Open the project folder containing `CMakeLists.txt` and `build.ps1`.
2. Open PowerShell in that folder. In File Explorer, **Open in Terminal** may be available in the folder's context menu.
3. Run:

```powershell
.\build.ps1
```

Wait for the build to finish successfully. The script locates the installed Visual Studio tools, builds the application, and copies the portable EXE to:

```text
dist\VoidSentinel.exe
```

Compiler output is also available at `out\Release\VoidSentinel.exe`. Use the `dist` copy as the clear starting point for running or copying the application.

The EXE contains its icons and C/C++ runtime. You can copy it to another compatible Windows 11 x64 computer; that computer does not need the development tools to run it.

## Build a debug version

Developers investigating a problem can run:

```powershell
.\build.ps1 -Configuration Debug
```

This also updates the `dist` copy, this time from the Debug build. Rebuild with the default command when you want the Release configuration again. “Release configuration” here is a compiler setting; publishing a GitHub release is a separate step.

## If the build fails

Read the last error in the terminal. Missing-tool messages usually mean a component above needs to be added through Visual Studio Installer. If the destination EXE is running, close that copy before rebuilding so the build can replace it.

Use a PowerShell session permitted to run the project's local build script. On managed computers, follow your administrator's script-execution policy.

For developer verification commands, see [Technical reference](technical-reference.md).

**Next:** [Quick start →](quick-start.md)
