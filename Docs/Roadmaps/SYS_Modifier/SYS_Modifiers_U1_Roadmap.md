# SYS_Modifiers_U1 — Roadmap (Pipeline genérico determinístico)

Status: DESIGN DRAFT  
Data: 2026-03-02  
System: SYS_Modifiers  
Update: U1

---

## 1) Identidade do Sistema

Responsabilidade única:
- Manter e avaliar modificadores determinísticos sobre valores alvo (ex: MovementSpeed, StaminaRegen, DamageTaken).

Não faz:
- Não decide ações (ActionGate)
- Não aplica atributos base diretamente (Attributes)
- Não chama Systems concretos diretamente
- Não depende de Editor/Forge

---

## 2) Conceitos

Target (alvo) por tag:
- Game.ModTarget.MovementSpeed
- Game.ModTarget.StaminaRegen

Modifier:
- ModifierTag (id)
- TargetTag
- Operation (Add | Mul)
- Magnitude
- SourceId
- Duration (opcional, via OmniClock no futuro)

Pipeline:
1) BaseValue
2) Sum(Adds) em ordem estável
3) Product(Muls) em ordem estável
4) Clamp (opcional)

---

## 3) Leis do Sistema

1) Ordenação determinística: por TargetTag, depois ModifierTag, depois SourceId.
2) Sem GUID, sem Random, sem WorldTime.
3) Snapshot obrigatório e ordenado.
4) Logs apenas em mudança (add/remove/expire).

---

## 4) Contratos Públicos (mínimos)

- AddModifier(Modifier)
- RemoveModifier(ModifierTag, SourceId)
- Evaluate(TargetTag, BaseValue) -> Result
- GetSnapshot()

---

## 5) Plano U1

- Implementar Add/Remove + armazenamento determinístico
- Implementar Evaluate com ordem estável
- Expor snapshot
- Integração mínima:
  - Movement consome multiplier de MovementSpeed (1 ponto)
  - ou Attributes consome multiplier de StaminaRegen (1 ponto)

Critério de aceite:
- Build/Gate/Smoke OK
- Cenário mínimo de validação com 1 modifier aplicado e refletido

