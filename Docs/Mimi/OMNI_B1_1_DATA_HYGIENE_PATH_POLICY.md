# OMNI - B1.1 Data Hygiene + Path Policy

## O que foi endurecido
- ActionId canonico do sprint: `Movement.Sprint`.
- Runtime agora rejeita `ActionId` com prefixo `Input.*` no `ActionGate` com fail-fast explicito.
- Gate expandido para impedir regressao de:
  - `Input.*` nos assets default de Action (`Library/Profile`),
  - mistura de roots no `OmniOfficialManifest.cpp`,
  - perfil sem referencia de library esperada nos defaults.

## Politica temporaria de path (ate Forge)
- Root temporario oficial nesta fase: `/Game/Data`.
- `OmniOfficialManifest.cpp` e mensagens de fail-fast usam `/Game/Data/...`.
- Root final sera definido pelo Forge (prefixo do usuario), fora do escopo desta fase.

## Regras de validacao (gate)
- Comando:
  - `powershell -ExecutionPolicy Bypass -File .\Scripts\omni_conformance_gate.ps1`
- Para validar conteudo de assets no ambiente local:
  - `powershell -ExecutionPolicy Bypass -File .\Scripts\omni_conformance_gate.ps1 -RequireContentAssets`
- O gate falha se:
  - detectar `Input.*` em assets default de Action,
  - detectar root misto em manifest (ex.: `/Game/Data` + `/Game/Omni/Data`),
  - faltar asset default canonico,
  - perfil default nao referenciar library esperada.
