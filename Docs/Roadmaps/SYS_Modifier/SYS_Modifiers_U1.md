# SYS_Modifiers_U1 — Execução e Evidências

Status: CONCLUÍDO  
Data: 2026-03-03  
System: SYS_Modifiers  
Update: U1 (pipeline determinístico Add/Mul)

---

## 1) Escopo U1

- `AddModifier` / `RemoveModifier`
- `Evaluate(TargetTag, BaseValue)` determinístico
- Pipeline estável: `base + adds + muls`
- Snapshot ordenado por `TargetTag`, `ModifierTag`, `SourceId`
- Logs de evento em add/remove
- Integração mínima com consumidor de `MovementSpeed`

---

## 2) Checklist

- [x] Criar `UOmniModifiersSystem`
- [x] Implementar contratos command/query (`AddModifier`, `RemoveModifier`, `EvaluateModifier`)
- [x] Ordenação determinística + snapshot
- [x] Integrar consumo mínimo no `UOmniMovementSystem`
- [x] Smoke: add/eval/remove com logs

---

## 3) Evidências

- [x] Build OK (`omni_build_dev.ps1`)
  - Resultado: `Result: Succeeded` em 2026-03-03.
- [x] Gate PASS (`omni_conformance_gate.ps1`)
  - Resultado: `Conformance gate PASSED. No B1/B0 conformance violations found.` em 2026-03-03.
- [x] Smoke OK (headless)
  - Comando:
    - `-ExecCmds="omni.mod add Game.Modifier.HasteSpeed Game.ModTarget.MovementSpeed Mul 1.20 Console,omni.mod eval Game.ModTarget.MovementSpeed 1.00,omni.mod status,omni.mod remove Game.Modifier.HasteSpeed Console,omni.mod status,omni.sprint status,quit"`
  - Evidência de logs:
    - `[Omni][Modifiers][Event] Add | mode=Insert modifier=Game.Modifier.HasteSpeed target=Game.ModTarget.MovementSpeed op=Mul magnitude=1.2000 source=Console`
    - `[Omni][Runtime][Console] omni.mod eval | target=Game.ModTarget.MovementSpeed base=1.000 result=1.200`
    - `[Omni][Modifiers][Event] Remove | modifier=Game.Modifier.HasteSpeed target=Game.ModTarget.MovementSpeed source=Console`
  - Integração mínima: `UOmniMovementSystem::UpdateEffectiveSpeed()` consome `EvaluateModifier` do target `Game.ModTarget.MovementSpeed`.

---

## 4) Commit

`feat(SYS_Modifiers_U1): deterministic modifier pipeline (add/mul)`
