# PHY104 Optical Assistant — Roadmap & Attribution Guide

This document tracks implemented and outstanding improvements, fixes, releases, and attribution guidance. Checkbox status refers to code/documentation changes; items that still need runtime verification are called out separately.

**Current branch update:** The Michelson laser-wavelength mode has been implemented with movable M1, λ/N/d solving, equation/readout updates, Gruvbox result cards, and a circular fringe view. Qt UI verification is still pending; see the open fixes below.

---

## 1. Improvements to Do

### Michelson Interferometer
- [x] Add interferogram fringe-count display (live fringe shift counter as legacy M2 moves)
- [x] Add refractive-index slider for the medium in one arm (air, water, glass, custom)
- [x] Add wavelength presets (He-Ne 632.8 nm, sodium 589 nm, laser diode 650/780 nm)
- [x] Add laser-wavelength mode based on the Virtual Labs Michelson wavelength model: move M1, solve λ = 2d/N, or solve N/d when λ is known
- [x] Display the BS/CP setup, virtual mirror/source separations, path difference, bright/dark conditions, λ/4-to-λ/2 relation, and live d/Δ/N/λ readouts
- [ ] Add "reset M2 to zero" button
- [ ] Add export of fringe-shift data to CSV
- [ ] Add animated fringe playback when dragging M2

### Visual SFX / Canvas
- [x] Add beam path tracing lines (show splitter → mirrors → detector)
- [x] Add sampled circular fringe visualization for Michelson wavelength mode
- [ ] Add phase difference readout (radians + degrees)
- [ ] Add dark/light theme toggle for the canvas

### Formula Solver
- [x] Add more physics formulas (Snell's law, lensmaker, thin-film interference, Brewster's angle — already existed)
- [x] Add Michelson OPD (ΔOPD = 2ny), Michelson fringe count (ΔN = ΔOPD/λ₀), Fringe visibility (V = (Imax−Imin)/(Imax+Imin))
- [ ] Add formula search with category filters (Optics, Waves, Thermodynamics)
- [ ] Add "solve for any variable" mode (currently solves for one target)
- [ ] Add calculation history with timestamps
- [ ] Add PDF export of solved formulas

### Dashboard
- [x] Add experiment comparison chart (overlay multiple runs)
- [x] Add run filtering by experiment type (comparison dropdown)
- [ ] Add date range filtering
- [ ] Add export of activity log to CSV/PDF
- [ ] Add "favorite runs" pinning

### Arabic Localization
- [x] Complete remaining untranslated UI strings (all four Arabic JSON files fully translated)
- [x] Add Arabic translations for new Michelson formulas and variables (ΔOPD, ΔN, fringe visibility)
- [ ] Add RTL-aware number formatting (Arabic-Indic digits option)
- [ ] Add Arabic lesson content for all experiments

### Packaging / Release
- [x] Build Windows `.exe` installer + portable ZIP (CI: .github/workflows/package.yml windows-installer job, NSIS; builds on v* tags)
- [x] Build Linux `AppImage` (CI: .github/workflows/package.yml linux-appimage job, linuxdeploy; smoke-tested on Arch; builds on v* tags)
- [x] Add checksums (SHA256) for all release assets (SHA256SUMS.txt generated and attached to each GitHub Release)
- [x] Add release notes per version (CHANGELOG.md with v3.0.9 entry and v1.0–v2.9 summary)

---

## 2. Fixes to Do

- [x] Verify mirror displacement range validation (±2 µm) in core physics
- [x] Test slider ↔ field bidirectional sync without feedback loops
- [x] Validate Casio LCD font fallback chain on systems without the font installed
- [x] Fix RTL layout issues in Arabic dashboard/log panels (addThreeColumns swaps sidebar/notebook positions in RTL mode)
- [x] Test PDF report page breaks on long Arabic content (@page rule, RTL body direction, page-break-inside: avoid on summaries/figures/LCD registers, page-break-before: always on sections)
- [x] Confirm formula copy-to-clipboard works on all platforms (QClipboard with "Copied!" confirmation, enabled/disabled state management)
- [x] Check recent-formula panel max-8 limit behavior (m_recent capped at 8 with removeLast(), empty state shows "(none yet)")
- [x] Validate empty-state message when search returns no results (m_emptyState shows "No formulas match..." / page label "No formulas found")
- [x] Remove copy buttons from experiment pages and remove the PHY104 text from the top logo
- [x] Remove chart gridlines and compact experiment-page layouts; use full-width primary action buttons
- [x] Normalize SVG rendering for Optical Setup diagrams and app icons, and align Optical Setup styling
- [x] Preserve legacy M2 dragging while adding wavelength-mode M1 dragging and slider synchronization
- [ ] Build the Qt application and run UI/interaction tests, including the new M1 drag and λ/N/d solve flows (Qt/CMake tooling unavailable in the current environment)
- [ ] Run the app at desktop/narrow sizes and in Arabic RTL; verify the canvas, chart, result cards, controls, and equations have no clipping or collisions
- [ ] Investigate and fix the currently failing non-Michelson core/physics assertions and the later `test_core` invalid-argument abort; re-run the complete suites

---

## 3. Editions / Edits to Do

### Documentation
- [x] Update `README.md` with v3.0.9 feature list (download links + install commands now v3.0.9)
- [x] Add `CHANGELOG.md` with per-version entries (v3.0.9 + v1.0–v2.9 summary)
- [x] Add `CONTRIBUTING.md` for new contributors
- [x] Add `CREDITS.md` with attribution (Movdy2, Ahmed Samir, Dr. Ashraf)
- [x] Document the OpenStax/LibreTexts mirror convention conflict (in CREDITS.md)

### Code Quality
- [x] Add unit tests for WaveExperiments physics calculations (legacy Michelson checks, fringe-shift checks, and wavelength-mode λ/N/d plus zero-gap-dark-center checks in test_core.cpp)
- [x] Add UI tests for dashboard log interactions (dashboardComparesAndDeletesSavedRuns in test_interaction.cpp)
- [x] Add UI interaction coverage for wavelength-mode M1 dragging and solving N/d (test_interaction.cpp; execution pending Qt availability)
- [x] Add test for fringe shift formula (ΔOPD = 2nΔx) (9 CHECK assertions in test_core.cpp: round-trip OPD, sign of Δx, n dependence, λ/(2n) per fringe, intensity min/max, ±2 µm guard)
- [x] Clean up unused includes in header files (removed <QMap> from DashboardPage.h, <QList> from Components.h)

### Version File
- [x] Add `version.json` for programmatic version checks (v3.0.9, features, attribution, Michelson convention)
- [x] Add build date/commit hash infrastructure for About dialog (version.h.in + CMake configure_file; PHY104_GIT_COMMIT, PHY104_BUILD_DATE available as C++ macros)

---

## 4. How to Improve the Version / Markdown Documentation

### Update `README.md`

Add or update these sections:

```markdown
## Version
**Current:** v3.0.9

## Features
- Michelson interferometer with movable M2 mirror
- Next release (after verification): M1 laser-wavelength mode (solve λ, N, or d)
- Visual SFX: drag glow, zero-crossing pulse, intensity-driven beam
- Arabic localization with RTL support
- Casio-style LCD font for numeric displays
- Dashboard activity log sidebar
- Formula solver with variable help, recent formulas, copy-to-clipboard
- PDF report generation with page breaks and RTL tables

## Installation
Download from Releases:
- Windows: `PHY104OpticalAssistant-3.0.9-Windows-x64-Setup.exe`
- Linux: `PHY104OpticalAssistant-3.0.9-Linux-x86_64.AppImage`
```

### Add `CHANGELOG.md`

```markdown
# Changelog

## Unreleased
- Michelson wavelength mode: M1 is movable; calculate λ, fringe count N, or displacement d
- Michelson: display virtual-image geometry, path difference, bright/dark equations, and a circular fringe view
- Keep legacy M2 interactions, session persistence, and formulas readable LTR in Arabic RTL mode
- Improve chart legend spacing and use Gruvbox Dark result cards

## v3.0.9 (2026-11-13)
- Michelson: M1 fixed / M2 movable convention restored
- M2 travel slider synced bidirectionally with displacement field
- Visual SFX: drag glow, zero-crossing pulse, intensity-driven beam
- Arabic: M2 specified as movable mirror in all explanations
- Casio LCD font: CASIO-Calculator-Font as primary LCD face
- Dashboard: three-column layout with activity log sidebar
- Formula Solver: resizable panels, variable help, recent formulas
- PDF report: page breaks, RTL support, CSS tables, image scaling

## v2.9.0 (previous)
- Windows and Linux release packaging
- AppImage support for Arch Linux
- Arabic language switching and RTL UI
```

### Version Bump Process

1. Edit `CMakeLists.txt`: `project(PHY104OpticalAssistant VERSION X.Y.Z ...)`
2. Commit: `git commit -m "Bump version to X.Y.Z"`
3. Tag: `git tag -a vX.Y.Z -m "Release notes..."`
4. Push: `git push origin main vX.Y.Z`
5. Create GitHub Release from the tag and attach `.exe` / `.AppImage` binaries

---

## 5. How to Show It's All Done by You (Attribution)

### Git Commit Attribution

Make sure your git user is set correctly before committing:

```bash
git config user.name "Your Name"
git config user.email "your.email@example.com"
```

All commits will then show you as the author on GitHub.

### GitHub Release Attribution

When creating a release on GitHub:
- The release is automatically attributed to the user who creates it
- In the release description, write: "Developed by [Your Name]" or "By [Your Name]"

### Add a CREDITS / ACKNOWLEDGMENTS File

Create `CREDITS.md`:

```markdown
# Credits

## Lead Developer
[Your Name] — All v3.0.9 features, fixes, and releases

## Contributors
- Ahmed Samir — Original project contribution (preserved per project notes)

## References
- OpenStax University Physics — Michelson Interferometer
- LibreTexts — The Michelson Interferometer
```

### Add an About Dialog in the App

In the Qt UI, add an "About" page showing:

```
PHY104 Optical Assistant v3.0.9

Developed by: [Your Name]
Contributions: Ahmed Samir (original project)
References: OpenStax, LibreTexts

License: [Your License]
```

### Preserve Ahmed Samir's Contribution

Per project constraints, Ahmed Samir's contribution must be preserved in project credits. Do not remove his name from:
- `CREDITS.md` / `ACKNOWLEDGMENTS.md`
- `README.md` contributors section
- `data/notes.json` (if it lists contributors)
- Any `About` dialog in the app

### GitHub Profile Attribution

- Pin the `movdy2/phy104-sim` (or `movdy2/Physics_Optical_Assistant`) repository to your GitHub profile
- Add the repository to your profile README with a description of your contributions

---

## 6. Suggested Order of Work

| Priority | Task | Why |
|----------|------|-----|
| 1 | Build the Qt app and run the core, physics, UI, and interaction test suites | Confirm the Michelson update compiles and regression tests pass |
| 2 | Investigate the failing non-Michelson core/physics assertions and `test_core` abort | Restore a trustworthy green test baseline |
| 3 | Visually verify the experiment at desktop/narrow sizes and Arabic RTL | Catch clipping, equation-direction, and layout issues not covered without Qt runtime |
| 4 | Update the actual `README.md` / `CHANGELOG.md` for the next release after verification | Document only validated behavior |
| 5 | Continue remaining Michelson improvements (CSV export, reset-to-zero, optional fringe animation) | Complete the outstanding experiment roadmap |
| 6 | Resume packaging/release tasks when a version bump is approved | Avoid publishing unverified UI changes |

---

**Status note:** Code changes are present on the active working branch, including the Michelson wavelength mode and related UI/test updates. The pure-C++ Michelson model compiles with strict warnings and the Michelson-specific core assertions pass, but broader tests still fail outside Michelson; Qt UI/interaction execution and visual verification remain open. Do not mark the release-ready items complete until those checks pass.
