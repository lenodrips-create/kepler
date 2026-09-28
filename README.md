# Kepler

**A web browser from another orbit.** Written in C++ with Qt 6 and Chromium (via Qt WebEngine).

## Features

- **Space-themed UI.** Dark interface with a violet-to-cyan accent and your planet logo throughout.
- **Animated new tab page** (`kepler://start`). Starfield with parallax, shooting stars, a floating logo, a search box, a clock and bookmark tiles.
- **Shields.** A built-in ad and tracker blocker (around 70 networks) that shows a per-site count and a session total.
- **Orbit mode.** Private windows with a magenta theme. History, cookies and cache are forgotten when the window closes.
- **Tabs.** You can drag to reorder, middle-click to close, and double-click empty space for a new tab. Right-click a tab to mute, duplicate or close others. `Ctrl+Shift+T` reopens a closed tab.
- **Smart address bar.** It searches or navigates depending on what you type, suggests from history and bookmarks, and shows a lock for secure sites.
- **Bookmarks bar.** `Ctrl+D` bookmarks the current page; right-click a bookmark to remove it.
- **History page** (`kepler://history`) with instant search.
- **Downloads panel** with live progress, speed, cancel and open.
- **Find in page.** A floating card with a match counter.
- **Zoom indicator.** Shows the zoom level; click it to reset.
- **Session restore.** Your tabs come back when you reopen Kepler.
- **Other tools:** save a page as PDF or MHTML, view source, DevTools (`F12`), fullscreen video, and site permission prompts.
- **Search engines:** Google, DuckDuckGo, Bing or Brave (pick one in the menu).

## Building

You need **Qt 6.4 or newer** with the **Qt WebEngine** and **Qt SVG** modules, **CMake 3.16+**, and a C++17 compiler.

### Windows

1. Install Qt from the [Qt Online Installer](https://www.qt.io/download-qt-installer). Pick Qt 6.x → **MSVC 2019/2022 64-bit** and tick **Qt WebEngine**. (WebEngine only works with MSVC, not MinGW.)
2. Install Visual Studio with the "Desktop development with C++" workload.
3. Open the **x64 Native Tools Command Prompt** and run:

```bat
cd kepler
cmake -B build -DCMAKE_PREFIX_PATH=C:\Qt\6.8.0\msvc2022_64
cmake --build build --config Release
C:\Qt\6.8.0\msvc2022_64\bin\windeployqt.exe build\Release\Kepler.exe
build\Release\Kepler.exe
```

`windeployqt` copies the Qt DLLs next to `Kepler.exe`, so you can zip that folder and run it on another PC.

### macOS

```bash
brew install qt cmake
cmake -B build -DCMAKE_PREFIX_PATH=$(brew --prefix qt)
cmake --build build
open build/Kepler.app
```

### Linux (Ubuntu/Debian)

```bash
sudo apt install cmake g++ qt6-base-dev qt6-webengine-dev qt6-svg-dev libgl1-mesa-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/Kepler
```

You can also pass addresses on the command line: `./build/Kepler github.com nasa.gov`.

## Keyboard shortcuts

| Keys | Action |
| --- | --- |
| `Ctrl+T` / `Ctrl+W` | New tab / close tab |
| `Ctrl+Shift+T` | Reopen closed tab |
| `Ctrl+Tab`, `Ctrl+1…9` | Switch tabs |
| `Ctrl+N` / `Ctrl+Shift+N` | New window / new Orbit (private) window |
| `Ctrl+L` | Focus address bar |
| `Ctrl+R` / `Ctrl+Shift+R` | Reload / hard reload |
| `Alt+←` / `Alt+→` | Back / forward |
| `Ctrl+D` | Bookmark page |
| `Ctrl+Shift+B` | Toggle bookmarks bar |
| `Ctrl+H` / `Ctrl+J` | History / downloads |
| `Ctrl+F`, `F3` | Find in page |
| `Ctrl +` / `Ctrl -` / `Ctrl+0` | Zoom |
| `Ctrl+S` / `Ctrl+P` | Save page / save as PDF |
| `Ctrl+U` | View source |
| `F11` / `F12` | Fullscreen / DevTools |

## Project layout

```
src/
  main.cpp            startup, session restore
  Browser.*           profiles, windows, downloads, address parsing
  BrowserWindow.*     the main window: tabs, toolbar, menus, shortcuts
  WebView.*           one tab's web page, popups, permissions, DevTools
  UrlBar.*            the address bar
  FindBar.*           find-in-page card
  DownloadsPanel.*    downloads popup
  Shields.*           ad & tracker blocker
  SchemeHandler.*     serves kepler://start, kepler://history
  Storage.*           history, bookmarks, settings (JSON + QSettings)
  Theme.*             colors & stylesheet
resources/
  web/start.html      new tab page
  web/history.html    history page
  icons/*.svg         toolbar icons
  logo-icon.png, kepler.ico
```
