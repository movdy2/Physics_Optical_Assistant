# PHY104 Optical Assistant — Roadmap & Attribution Guide

This document lists planned improvements, fixes, and editions for the app, plus guidance on version documentation and how to claim attribution for the work.

---

## 1. Improvements to Do

### Michelson Interferometer
- [x] Add interferogram fringe-count display (live fringe shift counter as M2 moves)
- [x] Add refractive-index slider for the medium in one arm (air, water, glass, custom)
- [x] Add wavelength presets (He-Ne 632.8 nm, sodium 589 nm, laser diode 650/780 nm)
- [ ] Add "reset M2 to zero" button
- [ ] Add export of fringe-shift data to CSV
- [ ] Add animated fringe playback when dragging M2

### Visual SFX / Canvas
- [x] Add beam path tracing lines (show splitter → mirrors → detector)
- [ ] Add interference fringe visualization on the detector screen
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
- [x] Add release notes per version (CHANGELOG.md with v3.0.0 entry and v1.0–v2.9 summary)

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

---

## 3. Editions / Edits to Do

### Documentation
- [x] Update `README.md` with v3.0.0 feature list (download links + install commands now v3.0.0)
- [x] Add `CHANGELOG.md` with per-version entries (v3.0.0 + v1.0–v2.9 summary)
- [x] Add `CONTRIBUTING.md` for new contributors
- [x] Add `CREDITS.md` with attribution (Movdy2, Ahmed Samir, Dr. Ashraf)
- [x] Document the OpenStax/LibreTexts mirror convention conflict (in CREDITS.md)

### Code Quality
- [x] Add unit tests for WaveExperiments physics calculations (existing Michelson tests + 9 new fringe-shift formula checks in test_core.cpp)
- [x] Add UI tests for dashboard log interactions (dashboardComparesAndDeletesSavedRuns in test_interaction.cpp)
- [x] Add test for fringe shift formula (ΔOPD = 2nΔx) (9 CHECK assertions in test_core.cpp: round-trip OPD, sign of Δx, n dependence, λ/(2n) per fringe, intensity min/max, ±2 µm guard)
- [x] Clean up unused includes in header files (removed <QMap> from DashboardPage.h, <QList> from Components.h)

### Version File
- [x] Add `version.json` for programmatic version checks (v3.0.0, features, attribution, Michelson convention)
- [x] Add build date/commit hash infrastructure for About dialog (version.h.in + CMake configure_file; PHY104_GIT_COMMIT, PHY104_BUILD_DATE available as C++ macros)

---

## 4. How to Improve the Version / Markdown Documentation

### Update `README.md`

Add or update these sections:

```markdown
## Version
**Current:** v3.0.0

## Features
- Michelson interferometer with movable M2 mirror
- Visual SFX: drag glow, zero-crossing pulse, intensity-driven beam
- Arabic localization with RTL support
- Casio-style LCD font for numeric displays
- Dashboard activity log sidebar
- Formula solver with variable help, recent formulas, copy-to-clipboard
- PDF report generation with page breaks and RTL tables

## Installation
Download from Releases:
- Windows: `PHY104OpticalAssistant-3.0.0-Windows-x64-Setup.exe`
- Linux: `PHY104OpticalAssistant-3.0.0-Linux-x86_64.AppImage`
```

### Add `CHANGELOG.md`

```markdown
# Changelog

## v3.0.0 (2026-11-13)
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
[Your Name] — All v3.0.0 features, fixes, and releases

## Contributors
- Ahmed Samir — Original project contribution (preserved per project notes)

## References
- OpenStax University Physics — Michelson Interferometer
- LibreTexts — The Michelson Interferometer
```

### Add an About Dialog in the App

In the Qt UI, add an "About" page showing:

```
PHY104 Optical Assistant v3.0.0

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
| 1 | Update `README.md` with v3.0.0 features | Immediate visibility |
| 2 | Add `CHANGELOG.md` | Track what changed per version |
| 3 | Add `CREDITS.md` with your name + Ahmed Samir | Attribution requirement |
| 4 | Build Windows `.exe` and Linux `AppImage` for v3.0.0 | Distributable release |
| 5 | Create GitHub Release from `v3.0.0` tag with binaries | Public release |
| 6 | Add Michelson fringe counter + wavelength presets | Core feature improvement |
| 7 | Add more physics formulas to Formula Solver | App scope expansion |

---

**Note:** No code changes or pull requests were made in this session. This is a planning document only. When you're ready to implement, start with the documentation updates (README, CHANGELOG, CREDITS) before building binaries.
