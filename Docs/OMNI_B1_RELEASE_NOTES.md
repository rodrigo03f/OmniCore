# OMNI B1 Release Notes

Status: HARD CLOSE
Data: 2026-02-28

## B1 entregou

- ActionGate asset-first (sem defaults hardcoded no caminho normal).
- Validacoes com mensagens acionaveis para erro de configuracao.
- Lifecycle explicito no ActionGate: start, end, denied.
- Movement observando lifecycle events (sem mudanca de comportamento).
- Headless compare com hash SHA256 externo (PowerShell).
- Execucao headless com `-ExecCmds="...;quit"` e fallback timeout + kill por PID.
- StatusSystem 100% data-driven via Manifest -> Profile -> Library.

## Commits principais

- `1c5676a` OmniForge: scaffold private context and phase stubs
- `11e8b85` OmniForge: split validate and resolve stages
- `e71a05d` OmniForge: make RunInternal a pure stage orchestrator
- `ab89a97` OmniForge: add fail-fast validate hardening
- `1d01377` OmniForge: harden validate for self-dependency and cycles
- `00a2a54` OmniForge: replace fragile SHA path and add headless compare script
- `59f5805` B1.1: enforce asset-first ActionGate definitions
- `fb2e869` B1.2: harden ActionGate validation and actionable errors
- `e84e5b5` B1.3: add explicit ActionGate lifecycle events
- `7a09534` B1.4: hook Movement to ActionGate lifecycle events
- `9ab14eb` B1.Status: migrate Status to asset-driven settings path
- `c66b318` Scripts: append quit to Forge ExecCmds in headless compare

## Como validar (sanity checks)

### Build
- `powershell -ExecutionPolicy Bypass -File Scripts/build_incremental.ps1`
- Resultado: PASS

### Smoke (Sprint + Stamina + Exhausted)
- Execucao: `UnrealEditor-Cmd.exe ... -game -ExecCmds="omni.sprint auto 3; omni.sprint status; quit"`
- Resultado: PASS por evidencia de inicializacao + autosprint.
- Evidencias:
  - `LogOmniStatusSystem: Status system initialized. Manifest=OmniOfficialManifest_0`
  - `LogOmniActionGateSystem: ActionGate system initialized. Definitions=1 Manifest=OmniOfficialManifest_0`
  - `LogOmniMovementSystem: Movement system initialized. Manifest=OmniOfficialManifest_0`
  - `LogTemp: [Omni] AutoSprint iniciada por 3.0s. Systems afetados: 1`
- Observacao operacional: `UnrealEditor-Cmd` ainda nao encerrou sozinho em todos os cenarios; fallback por timeout + kill por PID continua ativo.

### Conformance gate
- `powershell -ExecutionPolicy Bypass -File Scripts/omni_conformance_gate.ps1`
- Resultado: PASS

### Headless hash compare (determinismo)
- `powershell -ExecutionPolicy Bypass -File Scripts/compare_omni_forge_headless.ps1`
- Resultado: PASS
- Hash #1: `CD6D18789C5CEDDC09C99CC9E0ACA99488C3858862310B0B2C896BE76440A323`
- Hash #2: `CD6D18789C5CEDDC09C99CC9E0ACA99488C3858862310B0B2C896BE76440A323`

## Tag / mark

- Nao existe padrao de tags no repositorio (`git tag --list` vazio).
- Mark de fechamento registrado em documentacao:
  - `B1 HARD CLOSE (2026-02-28)`
  - Base tecnica validada em `9ab14eb034e49f33601100b7d0f64bed5a56bc49`.
