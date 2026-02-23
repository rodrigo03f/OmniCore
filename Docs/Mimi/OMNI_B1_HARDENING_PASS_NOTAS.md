# OMNI B1 - Hardening Pass (Seguranca + Evolucao)

## O que foi endurecido

1. Fail-fast real no bootstrap
- `UOmniRuntimeSystem` agora expõe status de inicializacao (`IsInitializationSuccessful`).
- `UOmniSystemRegistrySubsystem` aborta o startup quando qualquer system falha init.
- Resultado: com asset/profile/library quebrado e DEV defaults OFF, o runtime para de subir "como se estivesse ok".

2. Manifest asset-first com validacao obrigatoria (ActionGate/Status/Movement)
- Cada system valida:
  - `ProfileAssetPath` obrigatorio no Manifest.
  - asset de profile precisa existir e ser da classe correta.
  - library referenciada no profile precisa existir e ser da classe correta.
- Erros incluem `SystemId`, path esperado e recomendacao de correcao.

3. DEV fallback virou opt-in
- Novo controle canonicamente em runtime:
  - CVar: `omni.devdefaults` (`0/1`)
  - Config: `bAllowDevDefaults` em `OmniSystemRegistrySubsystem`
- Padrão: OFF.
- ON: fallback permitido com warning explicito e metrica de overlay.

4. Overlay/diagnostico operacional
- Metricas adicionadas/reforcadas:
  - `Omni.ManifestLoaded` = `True/False`
  - `Omni.Profile.Action` = `Pending/Loaded/Failed`
  - `Omni.Profile.Status` = `Pending/Loaded/Failed`
  - `Omni.Profile.Movement` = `Pending/Loaded/Failed`
  - `Omni.DevDefaults` = `ON/OFF`

5. Gate expandido para B1/B0
- `Scripts/omni_conformance_gate.ps1` agora valida:
  - regras B0 (encapsulamento de maps)
  - paths canonicos no `OmniOfficialManifest.cpp`
  - existencia dos 6 assets canonicos em `Content/Data/**` (local por padrao)
  - heuristica de fallback nao-guardado por `omni.devdefaults`
- Em CI, a checagem de `.uasset` eh pulada por padrao.

## Como ligar/desligar DEV defaults

1. Via CVar (runtime):
```text
omni.devdefaults 1
```
Para voltar ao modo estrito:
```text
omni.devdefaults 0
```

2. Via config:
```ini
[/Script/OmniRuntime.OmniSystemRegistrySubsystem]
bAllowDevDefaults=False
```

## Como o gate funciona

Comando padrao:
```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\omni_conformance_gate.ps1
```

Forcar validacao de assets no ambiente atual:
```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\omni_conformance_gate.ps1 -RequireContentAssets
```

Saida:
- `exit 0`: conforme
- `exit 1`: violacao (com regra + arquivo + linha)

