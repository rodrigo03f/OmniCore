# OMNI — Sync Git Geral (2026-02-23)

## Commit principal
- Hash: `59866cb`
- Link: https://github.com/rodrigo03f/OmniCore/commit/59866cb

## O que entrou nesse push
- Sync completo do workspace solicitado (tracked + untracked pendentes).
- Inclusão do plugin `RiderLink` no projeto em `Plugins/Developer/RiderLink`.
- Scripts novos de build:
  - `Scripts/omni_build_dev.ps1`
  - `Scripts/omni_build_carimbo.ps1`
  - `Scripts/omni_build_nuclear.ps1`
- Atualizações em assets/data/scripts/docs que estavam na fila local.

## Contexto técnico relevante
- O Rider estava com falha de load/integration por modelo UE desatualizado + processo UBT preso.
- Foi feito refresh de project files e integração RiderLink; build voltou a compilar.

## Estado final após push
- `origin/main` atualizado com o snapshot completo pedido.
