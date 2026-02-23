# OMNI Forge Scaffold v1 (P0)

## Objetivo

Implementar o pipeline minimo do Forge com saida auditavel:

- `Normalize -> Validate -> Resolve -> Generate -> Report`
- `Saved/Omni/ResolvedManifest.json`
- `Saved/Omni/ForgeReport.md`

Nesta fase:

- Nao gera codigo C++
- Nao gera Blueprint final
- Nao inclui UI avancada

## Interface

Comando:

```text
omni.forge.run
```

Args opcionais:

```text
omni.forge.run root=/Game/Data
omni.forge.run manifestAsset=/Game/.../DA_MeuManifest.DA_MeuManifest
omni.forge.run manifestClass=/Script/OmniRuntime.OmniOfficialManifest
omni.forge.run requireContentAssets=0
```

Precedencia:

- `manifestAsset` tem prioridade sobre `manifestClass`
- sem ambos: usa `UOmniOfficialManifest`
- `requireContentAssets=1` por padrao
- `root` afeta o contexto de geracao (inputs continuam vindo do Manifest)

## Estruturas

Implementado:

- `FOmniForgeInput`
- `FOmniForgeNormalized`
- `FOmniForgeResolved`
- `FOmniForgeReport`
- `FOmniForgeError`
- `FOmniForgeResult`
- `UOmniForgeRunner::Run(const FOmniForgeInput&) -> FOmniForgeResult`

## Validacoes (fail-fast)

Casos cobertos:

- `OMNI_FORGE_E001_DUPLICATE_SYSTEMID`
- `OMNI_FORGE_E002_DUPLICATE_ACTIONID`
- `OMNI_FORGE_E006_DEPENDENCY_CYCLE`
- `OMNI_FORGE_E010_MISSING_ASSET`
- `OMNI_FORGE_E012_NULL_LIBRARY`
- `OMNI_FORGE_E020_INVALID_ACTIONID_PREFIX`

Mensagens incluem contexto acionavel:

- system id
- action id (quando aplicavel)
- path/setting
- recomendacao de correcao

## Determinismo

Garantias aplicadas:

- toposort deterministico (fila ordenada)
- arrays ordenados antes da serializacao
- serializacao JSON por escrita sequencial de chaves (ordem fixa)
- sem campo temporal no JSON
- sem dependencia de iteracao de `TMap` no output

Resultado esperado:

- mesmo input gera `ResolvedManifest.json` byte-a-byte identico

## Outputs

- `Saved/Omni/ResolvedManifest.json`
  - systems resolvidos
  - ordem deterministica de inicializacao
  - profiles/libraries resolvidos
  - action definitions finais
- `Saved/Omni/ForgeReport.md`
  - PASS/FAIL
  - erros/warnings
  - lista de erros com codigo estavel

## Observacao

`/Game/Data` continua root temporario do sandbox. O runner aceita `root=` para permitir evolucao futura para `/Game/{UserPrefix}/Data` sem hardcode final.
