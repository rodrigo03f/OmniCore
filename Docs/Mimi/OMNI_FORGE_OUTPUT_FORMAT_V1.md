# OMNI Forge Output Format v1

## Objetivo

Formalizar o contrato de saida do Forge v1 para:

- estabilidade de formato
- versionamento explicito
- compatibilidade futura
- determinismo byte-a-byte

## Arquivos de saida

- `Saved/Omni/ResolvedManifest.json`
- `Saved/Omni/ForgeReport.md`

## Estrutura oficial do JSON

Campos de metadata no topo:

```json
{
  "forgeVersion": 1,
  "inputHash": "<sha256-do-manifest-normalizado>",
  "systemsCount": 3,
  "actionsCount": 1,
  "generatedAt": null,
  "schema": "omni.forge.resolved_manifest.v1",
  "...": "demais campos do resolved manifest"
}
```

Campos principais:

- `forgeVersion`
  - versao do contrato de saida.
  - alteracao de estrutura implica incremento de versao.
- `inputHash`
  - SHA256 do manifest de entrada normalizado.
  - permite comparar input efetivo entre execucoes/branches.
- `systemsCount`
  - total de systems resolvidos.
- `actionsCount`
  - total de action definitions finais apos resolve.
- `generatedAt`
  - sempre `null` para preservar determinismo.
- `schema`
  - identificador textual do formato atual do resolved manifest.

## Regras de determinismo

- ordem fixa de chaves no JSON (serializer sequencial).
- arrays em ordem estavel:
  - systems
  - actions
  - dependencies
- sem dependencia de ordem de `TMap`.
- sem timestamp no output.

Resultado esperado:

- mesmo input normalizado -> mesmo `ResolvedManifest.json` byte-a-byte.

## ForgeReport.md (metadata)

Secao obrigatoria:

```md
## Forge Metadata

- forgeVersion: 1
- inputHash: <hash>
- systemsCount: X
- actionsCount: Y
```

Regras:

- sem timestamp
- sem caminho absoluto
- sem tempo de execucao

## Politica de versionamento

- `forgeVersion` inicia em `1`.
- aumento de versao somente quando houver mudanca de contrato/estrutura de output.
- mudancas internas sem impacto de formato nao alteram `forgeVersion`.
