# OMNI - B1.1 Validacao dos 3 Testes (2026-02-23)

## Resultado consolidado

1. **Setup normal (`devdefaults 0`)**: PASS
- Sistemas sobem (`Status/ActionGate/Movement`), registry inicializa, sem fallback.

2. **Falha real (`devdefaults 0` + asset default ausente)**: PASS
- Fail-fast explicito no `ActionGate` com path esperado e recomendacao acionavel.
- Registry aborta inicializacao.

3. **DEV mode (`devdefaults 1` no mesmo cenario quebrado)**: PASS
- Runtime inicializa via fallback.
- Rastro explicito no log (`DEV_DEFAULTS ACTIVE`, `DEV_FALLBACK`).

---

## Evidencias por teste

### Teste 1 - Setup normal
- `omni.devdefaults = "0"` aplicado.
- `Status system initialized`.
- `Action definitions loaded from profile`.
- `ActionGate system initialized. Definitions=1`.
- `Movement system initialized`.
- `SystemRegistry initialized. Systems: 3`.
- Sem `Fail-fast`, sem `DEV_DEFAULTS`, sem `SkipPackage` no trecho validado.

### Teste 2 - Falha real (asset ausente)
- `LoadPackage: SkipPackage: /Game/Data/Action/DA_Omni_ActionProfile_Default ... does not exist`.
- `Fail-fast [SystemId=ActionGate]: Action profile asset not found ... /Game/Data/Action/DA_Omni_ActionProfile_Default...`
- `Fail-fast: system 'ActionGate' failed initialization and aborted registry startup.`

Trecho de referencia:
- `Saved/Logs/OmniSandbox.log:1795`
- `Saved/Logs/OmniSandbox.log:1797`
- `Saved/Logs/OmniSandbox.log:1798`

### Teste 3 - DEV mode com mesmo cenario quebrado
- `omni.devdefaults = "1"` aplicado.
- `DEV_DEFAULTS ACTIVE: ActionGate is using fallback definitions.`
- `Action definitions loaded from DEV_FALLBACK profile instance.`
- `SystemRegistry initialized. Systems: 3.`
- `Registry initialized via DEV_FALLBACK manifest class...`

Trecho de referencia:
- `Saved/Logs/OmniSandbox.log:1860`
- `Saved/Logs/OmniSandbox.log:1861`
- `Saved/Logs/OmniSandbox.log:1877`
- `Saved/Logs/OmniSandbox.log:1878`
- `Saved/Logs/OmniSandbox.log:1882`
- `Saved/Logs/OmniSandbox.log:1883`

---

## Conclusao
- Os 3 comportamentos esperados para B1.1 foram confirmados em log.
- Resultado: **B1.1 HARDENED validado em runtime** (modo estrito + falha real + modo DEV com rastro).
