# RM_StatusAndModifiers_P2 — Execution Order Oficial

Status: CONGELADO  
Data: 2026-03-02

---

## 🎯 Objetivo

Definir a ordem oficial de execução do RM_StatusAndModifiers_P2,
mantendo disciplina (um sistema por vez), determinismo e governança Omni.

Regra central:
Executar U1 completo de cada sistema antes de iniciar o próximo.

---

# 🧭 Ordem Oficial de Execução

## 1️⃣ SYS_Status — U1

Responsabilidade:
- Buff/Debuff real: duração, stacks e tick determinísticos via OmniClock.

Critério para avançar:

- Apply/Remove funcionando por Tag
- Duração + expiração determinística (OmniClock-only)
- Stacks determinísticos (max + refresh policy mínimo)
- Tick determinístico (intervalo estável)
- Snapshot ordenado (Tag, stacks, remaining)
- Logs por evento (sem spam)
- Build OK + Gate PASS + Smoke OK

---

## 2️⃣ SYS_Modifiers — U1

Responsabilidade:
- Pipeline genérico determinístico de modificadores (Add/Mul) por TargetTag.

Critério para avançar:

- Add/Remove funcionando com ordenação determinística (TargetTag, ModifierTag, SourceId)
- Evaluate determinístico (base + adds + muls)
- Snapshot ordenado por TargetTag
- Logs por evento (sem spam)
- Integração mínima com 1 consumidor (MovementSpeed ou StaminaRegen) sem quebrar U1 existentes
- Build OK + Gate PASS + Smoke OK

---

## 3️⃣ Integração mínima (U0.5)

Responsabilidade:
- Provar que Status pode acionar Modifiers de forma limpa (sem acoplamento direto).

Critério para fechar o RM:

- 1 status aplica/remova 1 modifier (ex: Haste -> MovementSpeed mul)
- Determinismo preservado
- Build OK + Gate PASS + Smoke OK
- Docs atualizados com evidências

---

# 🧠 Regra Arquitetural

Ordem conceitual:

Definir efeitos → Calcular impacto → Integrar sem acoplamento

Traduzido:

SYS_Status → SYS_Modifiers → Integração mínima

Nenhum sistema deve avançar para U2
antes que U1 esteja fechado e validado.

---

# 🚀 Próximo Marco

Início recomendado da execução:
2026-03-02

