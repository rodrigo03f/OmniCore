# OMNI — CHECKLIST REALISTA DO MVP (Framework)

Data: 2026-02-23  
Objetivo: Comparar roadmap do MVP com estado atual do main.

---

# 0) Governança e Gates

## Governança Canônica
Status: ✅ FEITO  
- Documento B0–B2 existe no repo.
- Regras R1–R7 definidas.
- Política “Sem gate verde, sem merge” alinhada.

## Gate Local + CI
Status: 🔴 NÃO CONFIRMADO NO MAIN  
- Script e workflow citados, mas ainda não visíveis no main.
- Bloqueia carimbo oficial de “B0 DONE”.

---

# 1) M1 — B0 HARD CLOSE (Bloqueador)

## Encapsulamento de TMap público
Status: 🔴 FALTA  
- FOmniSystemManifestEntry expõe TMap Settings publicamente.
- Viola R1 da governança.

## Acesso direto (.Add/.Find/[])
Status: 🔴 FALTA  
- Uso direto de Settings.Add no OfficialManifest.
- Ainda permite bypass estrutural.

Conclusão M1: NÃO FECHADO.

---

# 2) M2 — B1 Asset-First (ActionGate)

## Library/Profile existem
Status: 🟡 PARCIAL  
- UOmniActionLibrary e UOmniActionProfile implementados.

## Asset real no Content
Status: 🔴 FALTA  
- Uso atual ainda via ClassPath em Settings.
- Precisa migrar para AssetPath real.

## Hardcode no caminho normal
Status: 🟡 PARCIAL  
- Ainda existe definição default inline C++.
- Precisa virar apenas DEV_FALLBACK explícito.

Conclusão M2: Meio caminho andado.

---

# 3) M3 — Status + Movement Data-Driven

## Status
Status: 🟡 PARCIAL  
- Lógica de stamina/exhausted existe.
- Parâmetros ainda definidos no próprio system (não via Manifest/Profile).

## Movement
Status: 🟡 PARCIAL  
- Integração com ActionGate funciona.
- Ainda depende de WorldTime.

Conclusão M3: Funciona como gameplay, mas não está 100% data-driven.

---

# 4) M4 — OmniClock

Status: 🔴 FALTA  
- Systems ainda usam GetWorld()->TimeSeconds.
- OmniClock não implementado no main.

---

# 5) M5 — Forge Pipeline Mínimo

Status: 🔴 NÃO IMPLEMENTADO  
- Módulo OmniForge existe.
- Pipeline Normalize/Validate/Resolve/Generate/Report ainda não implementado.

---

# 6) M6 — B2 Tags & Namespace

Status: 🔴 NÃO APLICADO  
- Convenção Omni.* vs Game.* ainda não reforçada no código.
- Tags atuais não seguem namespace formal.

---

# FOTO GERAL DO MVP

Base sólida já existente:
- Registry funcional.
- Systems desacoplados.
- Loop sprint/stamina funcional.
- Contratos iniciais de Profile/Library.

Bloqueadores reais:
1) B0 HARD CLOSE
2) Asset-first no ActionGate
3) OmniClock
4) Forge mínimo
5) Hardening de tags

---

# Ordem recomendada para chegar no MVP

1) Fechar M1 (B0 + Gate no main)
2) M2 (ActionGate asset-first)
3) M4 (OmniClock)
4) M3 (Status/Movement 100% data-driven)
5) M5 (Forge mínimo)
6) M6 (Tags hardening)

---

# MVP estará oficialmente pronto quando:

- Gate verde no CI
- Clean rebuild carimbo OK
- ActionGate + Status + Movement data-driven
- OmniClock ativo
- Smoke loop jogável
- Sem fallback silencioso
- Sem acesso direto a map
