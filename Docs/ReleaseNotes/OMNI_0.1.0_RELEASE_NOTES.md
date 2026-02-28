# OMNI 0.1.0 Release Notes

Date: 2026-02-28
Release: v0.1.0
Status: B1 HARD CLOSE

## Technical Delivery

- Deterministic SystemRegistry in runtime.
- ActionGate is asset-first (Manifest/Profile/Library authority).
- StatusSystem is data-driven (Manifest -> Profile -> Library).
- Fail-fast validation enforced in Forge and runtime load paths.
- Headless determinism validated by external SHA256 compare.
- Smoke test consolidated for Sprint + Stamina + Exhausted.
- CI conformance gate active on `main` and pull requests.

## Validation Summary

- Clean rebuild: PASS (`Scripts/clean_rebuild_normal.ps1`)
- Incremental build: PASS (`Scripts/build_incremental.ps1`)
- Smoke: PASS by log evidence (`omni.sprint auto 3`, systems initialized, autosprint affected systems = 1)
- Conformance gate: PASS (`Scripts/omni_conformance_gate.ps1`)
- Headless hash compare: PASS (`Scripts/compare_omni_forge_headless.ps1`)
  - Hash #1: `CD6D18789C5CEDDC09C99CC9E0ACA99488C3858862310B0B2C896BE76440A323`
  - Hash #2: `CD6D18789C5CEDDC09C99CC9E0ACA99488C3858862310B0B2C896BE76440A323`

## Notes

- `UnrealEditor-Cmd` can still require timeout + kill-by-PID fallback in smoke execution.
