# Changelog

## [0.1.2] - 2026-02-28

- CI: add Forge determinism gate (no runtime changes).
- Added required CI stage for headless Forge execution plus deterministic artifact compare.
- Added CI failure diagnostics summary and Forge artifacts upload for debugging.
- Burn-in marker PR3: docs-only validation trigger.

## [0.1.0] - 2026-02-28

- Promoted public version from `0.1.0-dev` policy target to official `0.1.0`.
- Marked B1 as officially closed (`B1 HARD CLOSE`).
- Consolidated architecture, milestone, and release note documentation.
- Confirmed deterministic runtime and Forge validation baseline:
  - Deterministic SystemRegistry
  - ActionGate asset-first
  - Status data-driven
  - Fail-fast validation
  - Headless SHA256 determinism compare
  - Smoke validation for Sprint + Stamina + Exhausted
