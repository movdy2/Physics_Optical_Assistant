# PHY104 Optical Assistant

المشروع كان اهداء الى  🦤🧢 ولكنه انقرض في ظروف غامضه  نتمنه له كل خير

**A free, open-source, offline optics lab and study companion for PHY104.** Built with C++20 and Qt 6 for Windows and Linux.

## عن المشروع

<div dir="rtl" align="right">
<p>مشروع <strong> محاكه ماده فيزياء 104</strong> أعدّه <strong>Movdy2</strong> بمساهمة <strong>أحمد سمير</strong> لمقرر فيزياء 104 في جامعة زويل للعلوم والتكنولوجيا، بإشراف علمي ومراجعة من الدكتور أشرف، مدرّس المقرر.</p>

<p>يهدف البرنامج إلى مساعدة الطلاب والمدرّسين وأعضاء هيئة التدريس على تطبيق قوانين المقرر واستكشاف تجاربه. وهو بيئة تعليمية متكاملة، لا مجرد أداة واحدة، ويضم:</p>

<ul>
<li><strong>محلّل المعادلات (Formula Solver):</strong> يضم قوانين المقرر ويساعد على حل المسائل بإدخال المعطيات والتعويض بها.</li>
<li><strong>11 تجربة فيزيائية</strong> مستمدة من المقرر.</li>
<li>أدوات لتدوين الملاحظات والعمل على المسائل، وملخصات أُعدّت يدويًا للمراجعة.</li>
</ul>

</div>

[![Build](https://github.com/movdy2/phy104-sim/actions/workflows/build.yml/badge.svg)](https://github.com/movdy2/phy104-sim/actions/workflows/build.yml)
[![Latest release](https://img.shields.io/github/v/release/movdy2/phy104-sim?display_name=tag)](https://github.com/movdy2/phy104-sim/releases/latest)

![PHY104 Optical Assistant dashboard — a project by Movdy](docs/screenshots/readme-cover.png)

## Download v3.0.9

| Platform | Download |
|---|---|
| **Windows x64 installer** | [PHY104 Optical Assistant Setup](https://github.com/movdy2/phy104-sim/releases/download/v3.0.9/PHY104OpticalAssistant-3.0.9-Windows-x64-Setup.exe) |
| **Windows x64 portable** | [Portable ZIP](https://github.com/movdy2/phy104-sim/releases/download/v3.0.9/PHY104OpticalAssistant-3.0.9-Windows-x64-portable.zip) |
| **Linux x86_64** | [AppImage](https://github.com/movdy2/phy104-sim/releases/download/v3.0.9/PHY104OpticalAssistant-3.0.9-Linux-x86_64.AppImage) |

See the [v3.0.9 release page](https://github.com/movdy2/phy104-sim/releases/tag/v3.0.9) for all files and release notes.

### Installation

- **Windows installer:** download and run the Setup `.exe`.
- **Windows portable:** extract the ZIP, then run `PHY104OpticalAssistant.exe` from the extracted folder. The ZIP includes the Qt runtime and application data; the executable should not be separated from those files.
- **Linux:** download the AppImage, make it executable, and run it:

  ```sh
  chmod +x PHY104OpticalAssistant-3.0.9-Linux-x86_64.appimage
  ./PHY104OpticalAssistant-3.0.9-Linux-x86_64.appimage
  ```

  The AppImage is smoke-tested on Arch Linux. For normal AppImage mounting on Arch, install FUSE 2 and the runtime libraries if needed:

  ```sh
  sudo pacman -S --needed fuse2 gcc-libs harfbuzz libglvnd fontconfig freetype2 libxkbcommon
  ```

- **Nix:** build or run from the repository root using the flake in `packaging/nix/`:

  ```sh
  nix build ./packaging/nix
  ./result/bin/PHY104OpticalAssistant
  # Or build and run in one step:
  nix run ./packaging/nix
  ```

## Features

- **11 interactive optics experiments:** refraction and total internal reflection, lenses and mirrors, prisms, double-slit interference, polarization, single-slit diffraction, diffraction gratings, thin films, Michelson interferometry, Newton's rings, and thick lenses with aberration.
- **Study tools:** Markdown notes with live LaTeX preview and a scientific keyboard; Equation Lab; a chapter-based Study Hub; a 42-formula solver; and a scientific calculator.
- **Save and export:** calculations and notes save locally. Export measurements to CSV or Excel-compatible `.xls`, or create and print a PDF lab report.
- **English and Arabic:** switch languages in the app, with right-to-left Arabic layout and bundled Arabic-capable fonts. Equations and numeric entry remain left-to-right.
- **Dark and light themes:** explore interactive ray diagrams, plots, and resizable experiment pages in either theme.

## Screenshots

| Arabic dashboard · RTL | Refraction and total internal reflection |
|---|---|
| ![Arabic dashboard in a right-to-left layout](docs/screenshots/dashboard_arabic.png) | ![Refraction and total internal reflection experiment](docs/screenshots/refraction_tir.png) |

| Notes workspace | Scientific calculator |
|---|---|
| ![Markdown notes workspace with live preview](docs/screenshots/notes_workspace.png) | ![Scientific calculator keypad](docs/screenshots/calculator_keypad.png) |

More captures are in [`docs/screenshots/`](docs/screenshots/)

## Build from source

### Requirements

- CMake 3.21 or newer
- A C++20 compiler
- Qt 6 with **Widgets**, **Svg**, and **PrintSupport**
- Ninja (recommended)
- Lua 5.4 (optional; only needed to regenerate the checked-in SVG diagrams)

On Arch Linux, install the build dependencies with:

```sh
sudo pacman -S --needed base-devel cmake ninja qt6-base qt6-svg mesa libegl
```

### Build and test

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/src/PHY104OpticalAssistant
```

For a Windows build, configure CMake with a Qt 6 MSVC installation, for example `-DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64`. To build and test only the Qt-free core, configure with `-DPHY104_BUILD_GUI=OFF`.

To capture screenshots after building, run:

```sh
./build/src/PHY104OpticalAssistant --screenshots build/screenshots
```

## Editing the interface in Qt Creator

Open the repository's top-level `CMakeLists.txt` as a CMake project. Shared Designer forms are in `src/ui/` (`MainWindow.ui`, `BasePage.ui`, `Panel.ui`, `UnitField.ui`, `CalculatorKeypad.ui`, `ModeTabs.ui`, `RegisterView.ui`, `ResultCard.ui`, `NoticeCard.ui`, `NotesPage.ui`, `ConstantsPage.ui`, `DashboardActivity.ui`, `DashboardPage.ui`, `DashboardTools.ui`, and `DiagramsPage.ui`). CMake AUTOUIC generates their C++ wrappers during the build; edit the `.ui` files rather than generated `ui_*.h` files. Dashboard activity rows and the comparison chart remain data-driven C++ widgets; physics, session logic, and custom-painted diagrams also remain in C++.

Linux and Windows Qt builds and the core-only tests run in GitHub Actions. The standalone Lua generator does not add a Lua runtime dependency to the application; when Lua 5.4 is installed, regenerate the diagrams with:

```sh
cmake --build build --target generate_diagrams
# or write to another directory directly:
mkdir -p /tmp/diagrams
lua5.4 tools/make_diagrams.lua /tmp/diagrams
```

## Project layout

- `src/ui/` — Qt application, pages, diagrams, plots, themes, and language handling
- `src/core/` — physics, calculations, reports, and export logic
- `src/data/` — JSON repositories and session persistence
- `data/` — built-in study and experiment data
- `assets/` — translations, diagrams, bundled fonts, and icons
- `packaging/` — Windows installer and Nix flake/package definitions
- `tests/` — core, data, UI, and interaction tests

The built-in formulas, constants, chapters, keyboard definitions, and optics presets are in `data/`. Session files are stored locally in the application's `.phy` format. Font and icon notices are in [`assets/fonts/LICENSES.txt`](assets/fonts/LICENSES.txt) and alongside the bundled Noto Sans Arabic font.

## Project credits

Created for the PHY104 course at Zewail University of Science and Technology by [**Movdy2**](https://github.com/movdy2), with a contribution from **Ahmed Samir**. Scientific supervision and review were provided by **Dr. Ashraf**, the course instructor. Nix packaging is contributed by **Ag1O1**.
