# OMNI Forge v1 - Validacao de Determinismo (2026-02-23)

## Escopo

Validar determinismo byte-a-byte do pipeline `omni.forge.run` para os artefatos:

- `Saved/Omni/ResolvedManifest.json`
- `Saved/Omni/ForgeReport.md`

## Comando executado

```text
omni.forge.run manifestClass=/Script/OmniRuntime.OmniOfficialManifest requireContentAssets=1 root=/Game/Data
```

## Metodo

1. Executar `omni.forge.run` (run base).
2. Capturar hash SHA256 de ambos arquivos.
3. Executar `omni.forge.run` novamente (run comparativo).
4. Capturar hash SHA256 novamente e comparar.

## Evidencias

### Hashes base

- `ResolvedManifest.json`: `B88A6C00A4BA5A986B50EEA5399B864022C8F3ED21EB52D41F960509E46C65C2`
- `ForgeReport.md`: `73BC2A7BA399EF072D3009A95EF94480432EF51EA100516D173D668FD95A86A7`

### Hashes apos nova execucao

- `ResolvedManifest.json`: `B88A6C00A4BA5A986B50EEA5399B864022C8F3ED21EB52D41F960509E46C65C2`
- `ForgeReport.md`: `73BC2A7BA399EF072D3009A95EF94480432EF51EA100516D173D668FD95A86A7`

### Metadados de arquivo (ultima execucao)

- `ResolvedManifest.json`: 2163 bytes, `2026-02-23 19:32:15`
- `ForgeReport.md`: 432 bytes, `2026-02-23 19:32:15`

## Re-teste final (editor, 2x)

Execucao adicional no editor com o mesmo comando (2 vezes seguidas):

```text
omni.forge.run manifestClass=/Script/OmniRuntime.OmniOfficialManifest requireContentAssets=1 root=/Game/Data
```

Verificacoes:

- `ForgeReport.md` permaneceu `PASS` (`Errors=0`, `Warnings=0`).
- `ResolvedManifest.json` manteve:
  - `"generationRoot": "/Game/Data"`
  - `"actionId": "Movement.Sprint"`
- Hashes permaneceram identicos ao run base.

## Resultado

`PASS`

Determinismo confirmado para os dois artefatos no ambiente atual:

- Mesmo input -> mesmo output byte-a-byte (`SHA256` identico).
- Estrutura de output consistente com o scaffold v1.
