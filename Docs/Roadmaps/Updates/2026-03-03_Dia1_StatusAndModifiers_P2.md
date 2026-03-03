# Update do Dia — 2026-03-03

Roadmap: `RM_StatusAndModifiers_P2`  
Status do dia: `CONCLUÍDO`

---

## Entregas fechadas

- `SYS_AvatarBridge_U1`
- `SYS_AnimBridge_U1`
- `SYS_Status_U1`
- `SYS_Modifiers_U1`
- Integração mínima `Status -> Modifier` (prova funcional)

---

## Commits do dia

1. `feat(SYS_AvatarBridge_U1): add avatar integration bridge (CMC backend)`
2. `feat(SYS_AnimBridge_U1): add anim instance base + bridge component`
3. `feat(SYS_Status_U1): deterministic status effects (duration/stack/tick)`
4. `feat(SYS_Modifiers_U1): deterministic modifier pipeline (add/mul)`
5. `chore(RM_StatusAndModifiers_P2): integrate status->modifier minimal proof`

---

## Evidência principal da integração mínima

- `Game.Status.Haste` aplicado via `omni.statusfx apply` gera:
  - Add do modifier `Game.Modifier.HasteSpeed` em `Game.ModTarget.MovementSpeed`
  - `omni.mod eval` retorna `1.200` para base `1.000`
- Na expiração do status:
  - Remove automático do modifier por contrato no Registry

---

## Validação

- Build (`omni_build_dev.ps1`): PASS
- Conformance (`omni_conformance_gate.ps1`): PASS
- Smoke headless: PASS
