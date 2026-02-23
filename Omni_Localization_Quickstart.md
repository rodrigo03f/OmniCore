# Omni Localization Quickstart

This project is prepared for localized UI using Unreal's `FText` and `LOCTEXT`.

## Current setup

- Debug overlay UI texts now use localization keys (`LOCTEXT`) in:
  - `Plugins/Omni/Source/OmniRuntime/Private/Debug/UI/OmniDebugOverlayWidget.cpp`
- Initial staged cultures are configured in:
  - `Config/DefaultGame.ini`

Configured cultures:

- `en`
- `pt-BR`
- `es`
- `fr`
- `de`
- `ja`

## How to add translations

1. Open Unreal Editor.
2. Go to `Tools -> Localization Dashboard`.
3. Create/Select your game localization target.
4. Add cultures you want to support.
5. Run `Gather Text`.
6. Export translations (`.po`), translate, then import.
7. Run `Compile Text`.
8. Test by launching with a culture, for example:
   - `-culture=pt-BR`
   - `-culture=en`

## Rule for Omni code

- Use `FText` for anything visible to players.
- Use `LOCTEXT`/`NSLOCTEXT` for static labels.
- Keep technical runtime data (timestamps, IDs, raw debug messages) as dynamic content.
