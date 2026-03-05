# SYS_UI_HUD_U1 — Design Specification (HUD mínima do Protótipo)

Status: DESIGN FROZEN  
Roadmap: RM_PrototypePlayable_P1  
System: SYS_UI_HUD  
Update: U1 (HUD HP + Stamina + Exhausted)

---

## 1. Objetivo do U1

Entregar uma HUD mínima, funcional e determinística para validar o protótipo jogável:

- Barra de HP
- Barra de Stamina
- Indicador textual de EXHAUSTED

A HUD deve refletir o estado real do runtime sem depender de DebugWidget.

---

## 2. Princípios (Leis aplicadas)

- Widget UMG **não consulta Systems** diretamente.
- Um Bridge/SubSystem é responsável por coletar snapshots do runtime.
- HUD consome apenas **Snapshot** estruturado.
- Sem acoplamento entre UI e Systems concretos.

---

## 3. Fonte de Verdade

Snapshot consumido:

FOmniAttributeSnapshot (ou equivalente), contendo no mínimo:

- HPCurrent, HPMax
- StaminaCurrent, StaminaMax
- bExhausted

---

## 4. Arquitetura

### 4.1 Widget (C++)

Classe C++:

- UOmniHudWidget : UUserWidget

BindWidget obrigatório:

- PB_HP (UProgressBar)
- PB_Stamina (UProgressBar)
- TXT_Exhausted (UTextBlock)

Atualização:

- NativeTick lê snapshot via Bridge e atualiza UI

Regras:

- HPPercent = HPCurrent / max(HPMax, 1)
- StaminaPercent = StaminaCurrent / max(StaminaMax, 1)
- TXT_Exhausted visível apenas quando bExhausted == true

---

### 4.2 Layout (UMG Blueprint)

Blueprint:

- WBP_OmniHUD_Minimal

Parent class:

- UOmniHudWidget

Nome dos widgets no Designer deve bater exatamente:

- PB_HP
- PB_Stamina
- TXT_Exhausted

---

### 4.3 Bridge (Subsystem)

Subsystem recomendado:

- UOmniUIBridgeSubsystem (UGameInstanceSubsystem)

Responsabilidade:

- Consultar SYS_Attributes (via Registry/Query/caminho canônico do runtime)
- Manter CachedSnapshot
- Expor:

  - GetLatestAttributeSnapshot(OutSnapshot)

Widget consulta apenas o Bridge.

---

## 5. Instanciação

Ponto recomendado:

- PlayerController::BeginPlay

Fluxo:

- CreateWidget(WBP_OmniHUD_Minimal)
- AddToViewport()

---

## 6. Critérios de Aceite

Funcional (visual em PIE):

- Sprint drena stamina → barra desce
- Ao chegar no limite → EXHAUSTED aparece e sprint para
- Regen após delay → barra sobe
- omni.damage X → HP barra desce
- omni.heal X → HP barra sobe

Qualidade:

- Sem spam de logs
- Sem consultas diretas a Systems no widget
- Build OK
- Conformance Gate OK
- Smoke OK (mesmo que headless não renderize, não pode quebrar)

---

Design fechado em: 2026-03-02
