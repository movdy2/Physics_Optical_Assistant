# Contributing to PHY104 Optical Assistant

Thank you for contributing to PHY104 Optical Assistant — an offline optics lab and study companion for PHY104, built with C++20 and Qt 6.

## How to contribute

1. **Fork the repository** on GitHub (`movdy2/phy104-sim`, now at `movdy2/Physics_Optical_Assistant`).
2. **Create a branch** from `main` for your change: `git checkout -b feature/your-change`.
3. **Make your change** following the coding standards below.
4. **Test** the core layer with plain `g++` (no Qt needed): `g++ -std=c++20 -Isrc/core tests/test_core.cpp src/core/*.cpp -o test_core && ./test_core`.
5. **Commit** with a clear message and push to your branch.
6. **Open a pull request** against `main`.

## Coding standards

- **C++20** throughout. Use `-Wall -Wextra` and fix all warnings.
- Pure C++ business layer in `src/core/` (no Qt) — easy to unit test.
- Qt UI layer in `src/ui/` — keep UI code thin; put physics in `src/core/`.
- Data files (JSON) live in `data/`; translations live in `assets/i18n/`.
- Match the existing style: snake_case for functions/variables, PascalCase for types/classes.
- Document new public functions with `///` comments.

## Physics accuracy

This is a physics education app. Any change to calculations must:

- Preserve the Michelson convention: **M1 fixed, M2 movable**; `ΔOPD = 2nΔx`, one fringe per `λ₀/(2n)` of mirror travel.
- Keep clear mirror labels and state assumptions, sign conventions, and units.
- Not invent constants or data — cite sources (OpenStax, LibreTexts) where relevant.

## Attribution

- Lead developer: **Movdy2**
- Contribution: **Ahmed Samir** — preserve his name in all credits (CREDITS.md, README.md, About dialog).
- Scientific supervision: **Dr. Ashraf**, course instructor.

## Arabic localization

- All Arabic strings live in `assets/i18n/ar-*.json` (FusHa Arabic, schoolbook physics terms).
- RTL layout fixes go in `BasePage::addThreeColumns()` (column swap for right-to-left).
- When adding user-facing English strings, add Arabic translations to all four `ar-*.json` files.

## Releases

- Version is in `CMakeLists.txt` (`project(PHY104OpticalAssistant VERSION X.Y.Z)`).
- Tag releases with `git tag -a vX.Y.Z` and push: `git push origin main vX.Y.Z`.
- GitHub Actions builds Windows `.exe` + portable ZIP and Linux `AppImage` on every `v*` tag, then publishes a GitHub Release with SHA256 checksums.

## Questions

Open an issue on GitHub or contact the repository maintainer.
