# OMNI — ESTADO ATUAL PARA CONTEXTO (Snapshot)

Projeto: Omni (Plugin UE5)
Objetivo: MVP como Framework (não jogo completo)

---

# Governança

- Documento canônico B0–B2 aprovado.
- Sem gate verde, sem merge.
- Gate local + CI ativo.
- Encapsulamento de TMap aplicado.
- B0 HARD CLOSE: DONE.

---

# Estado Técnico Atual

✔ Systems desacoplados via Registry.
✔ Comunicação via Command / Query / Event.
✔ Fail-fast ativo.
✔ ActionGate funcional.
✔ Status funcional.
✔ Movement funcional.
✔ Smoke loop básico existente.

---

# Próxima Fase Escolhida

FASE COMPLETA: B1 (Data-Driven Core)

Objetivo:
Migrar ActionGate, Status e Movement para arquitetura 100% baseada em Manifest → Profile → Library.

---

# Após B1

- B3 — OmniClock
- Forge Pipeline mínimo
- Hardening de Tags (B2)

---

# Meta do MVP

O MVP estará pronto quando:

- Systems 100% data-driven.
- OmniClock ativo.
- Gate verde.
- Clean rebuild verde.
- Smoke loop configurável via dados.
- Zero fallback silencioso.
