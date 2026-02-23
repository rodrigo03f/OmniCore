# OMNI — FASE COMPLETA B1 (Data-Driven Core)

Status atual:
- B0 HARD CLOSE: DONE
- Gate ativo (local + CI)
- Encapsulamento aplicado
- Sem acesso direto a TMap

Objetivo da Fase B1:
Transformar ActionGate, Status e Movement em sistemas 100% data-driven via Manifest → Profile → Library.

---

# Escopo da Fase B1 (Completa)

## 1) ActionGate — Asset-First

- Criar assets reais de:
  - UOmniActionLibrary
  - UOmniActionProfile
- Migrar de ClassPath para AssetPath no Manifest.
- Caminho normal deve usar dados vindos do asset.
- Hardcode permitido apenas como DEV_FALLBACK explícito e logado.

Critério de aceite:
- Nenhuma regra hardcoded no caminho normal.
- Smoke loop funcionando via dados.

---

## 2) Status — Data-Driven

- Migrar parâmetros (stamina, regen, thresholds) para Profile/Library.
- Remover valores fixos no C++.
- Garantir leitura exclusiva via dados.

Critério de aceite:
- Alterar asset muda comportamento sem recompilar.

---

## 3) Movement — Data-Driven

- Parametrizar sprint/dash via dados.
- Remover dependência rígida interna.
- Comunicação apenas via mensagens e dados.

Critério de aceite:
- Movement responde somente a dados + eventos.

---

# Estratégia Técnica

- Agrupar mudanças de header em uma única batelada.
- Iterar lógica em .cpp usando incremental.
- 1 clean rebuild final (carimbo da fase).

---

# B1 DONE quando:

- ActionGate, Status e Movement seguem padrão Manifest → Profile → Library.
- Sem hardcode no caminho normal.
- Gate verde.
- Clean rebuild verde.
- Smoke loop jogável e configurável por asset.
