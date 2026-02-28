# OMNI - ESTADO ARQUITETURAL CONSOLIDADO

Data: 2026-02-28
Status oficial: B1 HARD CLOSE (2026-02-28)
Release publica: 0.1.0
Objetivo: consolidar o estado real do projeto e evitar deriva estrategica.

## 1) Estado atual

### Fechado

#### Governanca
- B0 HARD CLOSE
- Gate local + CI ativos
- Politica: sem gate verde, sem merge
- Encapsulamento estrutural aplicado

#### Forge / Infra
- Pipeline estruturado
- Validate hardening aplicado
- Validacoes anti erro humano aplicadas
- Determinismo validado (hash externo headless)
- Fix SHA headless aplicado
- Auto-exit UnrealEditor-Cmd com fallback timeout + kill por PID

#### Runtime Core
- SystemRegistry deterministico
- Systems desacoplados via Command / Query / Event
- ActionGate 100% asset-first
- Remocao de fallback silencioso
- Fail-fast configuravel
- Lifecycle events explicitos:
  - OnActionStarted
  - OnActionEnded
  - OnActionDenied
- Movement conectado aos eventos para observabilidade (sem mudanca funcional)
- Smoke test funcional (Sprint + Stamina + Exhausted)

#### Status Data-Driven Final
- Parametros de stamina/exhausted vindos de Profile/Library
- Remocao de path hardcoded no caminho normal
- Fail-fast explicito quando Profile/Library/dado faltam
- Sem numero magico no caminho normal do StatusSystem

### Pendente (P0 roadmap)
- Forge pipeline minimo oficial (Normalize -> Validate -> Resolve -> Generate -> Report)
- Build fail-early consolidado no main com carimbo recorrente
- Documento canonico unico consolidado para B0-B2

## 2) Arquitetura travada (inviolavel)

1. Runtime nao depende de Editor ou Forge.
2. SystemRegistry mantem ordem deterministica.
3. Manifest e a autoridade de definicao.
4. Systems nao se chamam diretamente.
5. Comunicacao apenas via Command / Query / Event.
6. Nenhum fallback silencioso no caminho normal.
7. Nenhum estado implicito.
8. Omni.* e namespace reservado do framework.
9. Nenhuma decisao critica sem log claro.
10. Nenhum acesso direto ao estado interno de outro system.

## 3) Criterio de fase fechada

- Build limpo ou incremental (quando clean nao for viavel) em PASS.
- CI e conformance gate em PASS.
- Smoke test em PASS.
- Validacao headless de determinismo em PASS.
- Documentacao de estado e release notes atualizadas.

## 4) Missao atual

Proxima missao: definir B2 / iniciar Forge P0 (sem abrir escopo novo neste documento).
