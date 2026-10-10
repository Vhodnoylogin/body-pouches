# Changelog

## 0.2.0 — 2026-10-10

First release prepared from the owner-accepted 0.1.21 gameplay implementation.
No new gameplay changes relative to that build.

- Default exclusive potion pouch on VRIK stomach slot 13.
- Draw and return by hand; drinking through HIGGS.
- Inventory-based selection by potion effects; next-potion body display.
- Independent assignments in each save through paired SKSE co-saves.
- Physical-hand and coherent post-VRIK/HIGGS integration and diagnostics.
- English/Russian installation guide and explicit release limitations.

Owner manual acceptance covers baseline placement, both hands, drinking, distinct
healing/magicka saves and full restarts. Asymmetric fresh-entry ergonomics and
legacy-after-new save transition remain separate findings, not reasons to defer
the accepted baseline release. Advanced configurations have narrower acceptance.

## Development versions

0.1.21 introduced per-save assignment serialization; 0.1.20 made hand measurements
coherent with the completed VRIK/HIGGS callback; 0.1.19 corrected physical-hand
node selection and current VRIK offset use. Earlier implementation/testing history
is preserved in `docs/development-history.en.md` and `.ru.md`.
