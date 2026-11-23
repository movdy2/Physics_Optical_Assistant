# Changelog — PHY104 Optical Assistant

## [v3.0.9] — 2026-11-13

- Version bumped to 3.0.9 (CMakeLists.txt, version.json).
- Michelson Interferometry improvements: fringe count register (full + fractional), coherence-length decay V_eff = V·exp(−|ΔOPD|/Lc), mirror tilt control (0–10 mrad), glass plate OPD ΔOPD_plate = 2(n_plate−1)t, detector screen fringe visualization, phase difference readout (rad + degrees).
- Calculator refresh: compact 52-key scientific keypad, advanced functions and optics units, selectable calculation history, and an Ans memory workflow.
- Regression tests added for fringe count register, V<1 contrast, coherence decay, plate OPD, NaN/Infinity guard, and the compact calculator interactions.

## [v3.0.0] — 2026-11-13

### Michelson interferometer
- **Movable M2 mirror** restored per the conventional PHY104 setup (M1 fixed, M2 on a piezo stage). Clear mirror labels preserved; the OpenStax/LibreTexts convention conflict is documented in CREDITS.md.
- **Live fringe counter:** detector readout shows ΔN = 2nΔx/λ₀ next to intensity as M2 moves.
- **Refractive-index slider** (1.000–2.500, 0.001 steps) for the medium in one arm, bidirectionally synced with the displacement field via QSignalBlocker (no feedback loops).
- **Wavelength source presets:** He-Ne 632.8 nm, sodium 589 nm, laser diode 650/780 nm, with two-way combo↔field sync.
- **Beam path tracing:** color-coded round-trip arm paths from splitter → mirrors → detector on the canvas.
- **Mirror displacement validation:** ±2 µm stage range guard.

### Formula solver
- New formulas in the interference chapter: Michelson OPD (ΔOPD = 2ny), Michelson fringe count (ΔN = ΔOPD/λ₀), Fringe visibility (V = (Imax−Imin)/(Imax+Imin)), each with `noSolutionHint`.
- New variables: ΔOPD (m), ΔN (fringe count), I_min (W/m²).
- Copy-to-clipboard of the last answer (LaTeX) with "Copied!" confirmation on all platforms.
- Recent-formula panel capped at 8 entries; empty-state message ("No formulas match…") when search returns no results.

### Dashboard
- **RUN COMPARISON panel:** per-run checkboxes, experiment filter dropdown, LineChart overlay of saved runs.
- Activity log sidebar and experiment cards (item #6).

### Arabic localization (RTL)
- All four Arabic JSON files fully translated (ar-ui 360, ar-core 936, ar-content 296, ar-lessons 128 entries), including Michelson formulas/variables (ΔOPD, ΔN, fringe visibility).
- RTL layout fix: `BasePage::addThreeColumns()` swaps sidebar/notebook column positions in right-to-left mode so the Arabic dashboard reads correctly.
- PDF report page breaks verified for long Arabic content: `@page` A4 rule, RTL body direction, `page-break-inside: avoid` on summaries/figures/LCD registers, `page-break-before: always` on sections.

### Packaging / release
- GitHub Actions `package.yml` builds Linux x86_64 AppImage (linuxdeploy), Windows x64 NSIS installer and portable ZIP, smoke-tests on Arch Linux, and publishes a GitHub Release on every `v*` tag.
- Casio LCD font fallback chain (CASIO-Calculator-Font → DSEG14 → sans) verified on systems without the bundled font.

### Under the hood
- Version bumped to 3.0.0 (CMakeLists.txt).
- Build error fixes: unused `Theme` variable in DashboardPage.cpp; `QTRY_VERIFY_WITH_TIMEOUT` macro argument handling.

---

Earlier releases are documented in `docs/CHANGES.md` (v1.0 through v2.9) and summarized below.

## [v2.9] — One page template, integrated Study Hub, dashboard browser
- Consistent Save + Export (PDF/Excel/CSV) menu on every main page.
- Study Hub tabs: Chapters, Notes, Optical Setups, Constants.
- Dashboard "Browse all 11" menu; calculator cleanup.
- See `docs/CHANGES.md` for the full v2.9 entry.

## [v2.8] — Experiment extras: thick lens, film colours, slit envelope, grating resolution
- Thick Lens & Aberration page; Thin-Film Colours (CIE 1931); Single-Slit Envelope; Grating Resolution (R = mN).
- Qt-free ThickLens and Spectrum modules.
- See `docs/CHANGES.md` for the full v2.8 entry.

## [v2.7] — Study Hub, plain-text calculator keys, calmer theme
- Study Hub with ten chapters in course order; plain-text scientific keyboard; brighter dark-theme small text; warmer light theme.

## [v2.6] — Casio LCD digits, cleaner sidebar, printing, notes keyboard
- DSEG7 Classic LCD readouts with per-glyph fallback; two-section sidebar; Print… on Export screen; scientific keyboard on Notes.

## [v2.5] — polish
- Export menu hidden on the export screens themselves; window icon follows theme; remembered theme choice via QSettings.

## [v2.4] — keyboard shortcuts
- Ctrl+S save, Ctrl+Enter calculate/solve, Ctrl+P PDF report, Ctrl+1…9 pages, Ctrl+Tab next page, F11 full screen.

## [v2.3] — full-width pages, one export icon, leaner screens
- Full-width page layout; single `⤓ Export` icon per header; Notes page is only the Markdown workspace; Draw Space removed.

## [v2.2] — second tabs, Excel/CSV export, PDF report, Draw Space
- Real second tabs on all six model screens; unified Export page; academic PDF lab report; Draw Space whiteboard.

## [v2.1] — Markdown study workspace, TIR and mirror screens
- Markdown notes workspace; invalid-input pattern; calculator keypad; `App` class shared by main and tests.

## [v2.0] — Figma design
- New shell and screens from the Figma file; ExperimentReports, SessionLog, DataExport/XlsxWriter; optics_presets.json; `.phy` session format.

## [v1.2] — UI, search and calculator update
- Gruvbox theme + Casio LCD styling; formula search/pagination; FormulaManager; calculator with DEG/RAD; LaTeX renderer; SVG diagrams.

## [v1.0] — first unified release
- Unified Export page and academic PDF report; ExpressionEvaluator, EquationSolver, SolverEngine; Qt UI; five experiments; notes; theme.

[v3.0.9]: https://github.com/movdy2/phy104-sim/releases/tag/v3.0.9
[v3.0.0]: https://github.com/movdy2/phy104-sim/releases/tag/v3.0.0
[v2.9]: https://github.com/movdy2/phy104-sim/releases/tag/v2.9.0
