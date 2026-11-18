# Project Credits & Attribution — PHY104 Optical Assistant

## Lead author

**Movdy2** — created the PHY104 Optical Assistant for the PHY104 course at Zewail University of Science and Technology.

## Contribution

**Ahmed Samir** contributed to the project. His work is acknowledged in the Arabic project dedication ("أعدّه Movdy2 بمساهمة أحمد سمير") and in the README project-credits section.

## Scientific supervision & review

**Dr. Ashraf**, the course instructor, provided scientific supervision and review.

## Interface design

The interface design was prepared with **Figma AI**. AI tools also helped speed up development and troubleshoot issues; the app itself runs locally and does not depend on an AI service.

---

## Michelson interferometer — mirror convention (OpenStax / LibreTexts)

The PHY104 Michelson experiment follows the **conventional teaching-lab setup**:

- **M1 is fixed** (reference mirror).
- **M2 is movable** (on a translation / piezo stage).

Moving M2 by a distance Δx changes the round-trip optical path difference by **2nΔx**, so one fringe corresponds to a mirror travel of **λ₀/(2n)**. This convention is preserved in the code and in all mirror labels throughout the app.

### Source reference conflict

The fetched OpenStax/LibreTexts reference (*The Michelson Interferometer*) is internally inconsistent about which mirror moves:

- Its introduction and figure describe **M1 as the moving mirror** and M2 as stationary.
- Its worked example's opening sentence says to **keep M1 fixed and move M2**.

This conflict was **not** silently resolved or obscured. The app deliberately adopts the conventional, clearly-labeled setup (fixed M1, movable M2) and documents the discrepancy here so instructors and students are aware of it. If your course uses the opposite convention, swap the M1/M2 labels and the sign of Δx accordingly; the physics (ΔOPD = 2nΔx, one fringe per λ₀/(2n) of mirror travel) is unchanged.

## Bundled fonts & icons

Font and icon notices are in `assets/fonts/LICENSES.txt` and alongside the bundled Noto Sans Arabic font. The Casio-style LCD face uses the bundled `CASIO-Calculator-Font` with `DSEG14 Classic` and the system sans stack as per-glyph fallbacks (including Arabic glyphs).
