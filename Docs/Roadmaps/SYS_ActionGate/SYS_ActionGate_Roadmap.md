# SYS_ActionGate_Roadmap

Status: ATIVO

## Identidade
Policy Engine + Locks determinísticos.

## Leis do Sistema

- Nunca retornar bool cru.
- Nunca chamar outro System diretamente.
- Determinismo obrigatório.
- Locks devem possuir SourceId estável.
- Trace apenas em modo debug.
- ReasonTag sempre presente.

## Plano de Evolução

U1 — Base Policy Engine + Locks
U2 — Decision Trace determinístico
U3 — Rule Composition (ALL / ANY / NOT)
U4 — Cooldowns via OmniClock
