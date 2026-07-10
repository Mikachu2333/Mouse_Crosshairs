# AGENTS.md

## Project

Windows desktop app: movable crosshair overlay rendered via Direct2D transparent layered windows. C++20, Win32 API only.

## Build

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 8
```

- **MSVC-only** — `CMakeLists.txt` enforces `FATAL_ERROR` for non-MSVC compilers.
- **Ninja generator** — always pass `-G Ninja` with cmake (no generator is hardcoded in CMakeLists.txt).
- Build output is written to the **project root** (`${PROJECT_SOURCE_DIR}`), not `build/`.
- Release links `/MT` (static runtime); Debug uses `/MDd` (dynamic). See `CMAKE_MSVC_RUNTIME_LIBRARY`.
- Architecture suffix is automatically appended: output is `mouse_crosshair_x64.exe` or `mouse_crosshair_x86.exe`.
- `CMAKE_EXPORT_COMPILE_COMMANDS` is ON; `.clangd` expects `compile_commands.json` in `build/`.

## Architecture

| File(s)                      | Role                                                                                                                                                                     |
| ---------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `main.cpp`                   | `WinMain` entry — mutex guard, config loading, hotkey registration, message loop                                                                                         |
| `src/crosshair.cpp/h`        | Four transparent `WS_EX_LAYERED` + `WS_EX_TRANSPARENT` topmost popup windows (L/R top/bottom). Direct2D rendering, low-level mouse hook (`WH_MOUSE_LL`), ~60fps throttle |
| `src/config.cpp/h`           | INI parsing via `GetPrivateProfileStringW`/`GetPrivateProfileIntW`. Custom `color` wrapper struct that clamps 0–255. Hotkey parsing with full vk map                     |
| `src/hotkey.cpp/h`           | Registers/unregisters two global hotkeys via `RegisterHotKey`                                                                                                            |
| `src/config_file_util.cpp/h` | Embedded `DEFAULT_INI` string; creates `crosshair.ini` next to EXE on first run                                                                                          |
| `res/resource.rc`            | App icon                                                                                                                                                                 |

## Configuration (crosshair.ini)

- Auto-generated next to the EXE on first launch.
- Config is **re-read on every toggle** (hide then show), so hot-reload works without restart.
- Sections: `[Crosshair]`, `[Horizontal]`, `[Vertical]`, `[Hide_Show_Hotkey]`, `[Exit_Hotkey]`.
- Width max is 200; gap max is 400 (exceeding resets to 0).
- Hotkey modifiers: `Ctrl`, `Alt`, `Win`, `Shift` (comma-separated in INI). Fn keys can be used without modifiers.

## Key runtime behaviors

- **Single-instance** enforced via named mutex `F5B6239126A64833BE094D6DC8DC1951`.
- **DPI-aware** set via `SetProcessDPIAware()` (not per-monitor v2).
- Exit hotkey (`Ctrl+Win+Alt+E` default) calls `PostQuitMessage(0)`.
- Toggle hotkey (`Ctrl+Win+Alt+H` default) toggles visibility; if showing, reloads config from disk and re-registers hotkeys.
- Windows span the **virtual screen** for multi-monitor support.
- Mouse hook dynamically installed/uninstalled with visibility toggles to reduce overhead when hidden.

## Code conventions

- Google style (`.clang-format`), includes NOT sorted (`SortIncludes: false`).
- `.clang-tidy` is CLion-generated; enabled checks are broad (bugprone, modernize, performance, readability, etc.).
- Includes use relative paths with `./` prefix in `.cpp` files.
- Comments and UI strings are in Chinese.
- `#pragma once` used for headers.
- Direct2D resources are raw COM pointers (manual `Release()`); no smart COM pointers.
- `VK_0`–`VK_9` and `VK_A`–`VK_Z` macros are **not defined** in Windows SDK 10.0.26100+ — `config.cpp` uses wide char literals (`L'0'`, `L'A'`) instead.
