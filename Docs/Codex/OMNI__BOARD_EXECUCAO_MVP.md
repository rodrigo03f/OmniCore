# OMNI — BOARD DE EXECUÇÃO MVP (Framework)

Data: 2026-02-23  
Objetivo: Transformar o checklist do MVP em board de execução por recortes controlados.

---

# 🔴 FASE 0 — B0 HARD CLOSE (Bloqueador)

## B0.1 — Encapsulamento do ManifestEntry
Objetivo:
- Remover TMap público de FOmniSystemManifestEntry.
- Tornar storage private.
- Criar helpers controlados (Set/TryGet/Has).

Aceite:
- Não existe TMap público em contratos runtime.
- Código que usa .Settings.Add/.Find não compila mais.

Compile esperado:
- 1 batelada de headers.
- 1 clean rebuild carimbo no final.

---

## B0.2 — Remover acessos diretos
Objetivo:
- Substituir todos os .Add/.Find/[] por helpers.
- Ajustar OfficialManifest e callsites.

Aceite:
- Gate detecta zero padrões proibidos.
- Incremental build verde.

---

## B0.3 — Gate oficial no main
Objetivo:
- Commitar Scripts/omni_conformance_gate.ps1.
- Commitar workflow CI.
- Validar execução local + CI.

Aceite:
- CI falha ao inserir padrão proibido.
- Sem gate verde, sem merge funcionando.

Status B0 DONE quando:
- Encapsulado.
- Zero bypass.
- Gate ativo em CI.
- Clean rebuild verde.

---

# 🟡 FASE 1 — B1 Asset-First (ActionGate)

## B1.1 — Criar Assets reais
Objetivo:
- Criar UOmniActionLibrary e UOmniActionProfile como assets no Content.
- Remover dependência exclusiva de ClassPath.

Aceite:
- Manifest aponta para AssetPath real.
- ActionGate resolve definitions via asset.

Compile esperado:
- Principalmente incremental.

---

## B1.2 — Remover hardcode do caminho normal
Objetivo:
- Transformar BuildDefaultDefinitionsIfNeeded em DEV_FALLBACK explícito.
- Garantir log DEV_FALLBACK.

Aceite:
- Caminho normal 100% data-driven.
- Hardcode só executa se manifest falhar.

---

Status B1 ActionGate DONE quando:
- Assets reais ativos.
- Sem regra hardcoded no caminho normal.
- Smoke loop funcionando.

---

# 🟡 FASE 2 — B1 Status + Movement

## B1.3 — Status data-driven
Objetivo:
- Migrar parâmetros (stamina, regen, thresholds) para Profile/Library.
- Remover valores fixos no system.

Aceite:
- Status configurável via Manifest/Profile.
- Sem valores fixos obrigatórios no C++.

---

## B1.4 — Movement data-driven
Objetivo:
- Parametrizar sprint/dash via dados.
- Remover dependência rígida interna.

Aceite:
- Movement responde apenas a dados + mensagens.

---

Status B1 COMPLETO quando:
- ActionGate, Status e Movement seguem padrão Manifest → Profile → Library.

---

# 🟢 FASE 3 — B3 OmniClock

## B3.1 — Implementar OmniClock
Objetivo:
- Criar fonte central de tempo.
- Substituir GetWorld()->TimeSeconds nos systems.

Aceite:
- Systems do MVP não usam world time direto.

Compile esperado:
- 1 batelada de header.
- 1 clean rebuild carimbo.

---

# 🔵 FASE 4 — Forge Pipeline Mínimo

## F4.1 — Normalize
## F4.2 — Validate
## F4.3 — Resolve
## F4.4 — Generate
## F4.5 — Report

Objetivo:
- Implementar fluxo mínimo de geração e validação.

Aceite:
- Configure → Generate → Play funciona.
- Erros estruturais falham cedo.

---

# 🟣 FASE 5 — B2 Hardening Tags

## B2.1 — Namespace enforcement
Objetivo:
- Garantir Omni.* reservado.
- Separar Game.*

Aceite:
- Nenhuma tag fora do padrão entra no main.

---

# 🎯 MVP OFICIALMENTE PRONTO QUANDO:

- B0 DONE.
- B1 COMPLETO.
- OmniClock ativo.
- Forge mínimo funcional.
- Gate verde.
- Clean rebuild carimbo verde.
- Smoke loop jogável.
- Zero fallback silencioso.
