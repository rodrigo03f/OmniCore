# Ideia Futura - Multiplayer com EOS

Data: 2026-02-23
Status: backlog privado (nao enviar para Mimi por enquanto)

## Objetivo

Testar multiplayer usando Epic Online Services (EOS) no projeto OmniSandbox.

## Estrategia sugerida

1. Comecar com Online Subsystem EOS (OSSv1) para validar Host/Join rapido.
2. Depois avaliar migracao para Online Services (OSSv2), se necessario.

## Config minima (DefaultEngine.ini)

```ini
[OnlineSubsystemEOS]
bEnabled=true

[OnlineSubsystem]
DefaultPlatformService=EOS

[/Script/Engine.Engine]
!NetDriverDefinitions=ClearArray
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/SocketSubsystemEOS.NetDriverEOSBase",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")
+NetDriverDefinitions=(DefName="DemoNetDriver",DriverClassName="/Script/Engine.DemoNetDriver",DriverClassNameFallback="/Script/Engine.DemoNetDriver")

[/Script/SocketSubsystemEOS.NetDriverEOSBase]
bIsUsingP2PSockets=true
```

## Setup no Editor

1. Habilitar plugin `Online Subsystem EOS` (e dependencias).
2. Preencher Artifact no Project Settings com:
   - ArtifactName
   - ProductId
   - SandboxId
   - DeploymentId
   - ClientId
   - ClientSecret
   - ClientEncryptionKey

## Teste inicial

1. Login com duas contas Epic (AccountPortal).
2. Fluxo basico:
   - Conta A cria sessao (Host).
   - Conta B encontra e entra (Join).

## Riscos e observacoes

- Sem IDs do portal EOS, nao fecha teste real.
- Primeira meta e conectividade/sessao, nao gameplay completo.
- Manter runtime desacoplado do editor durante a integracao.

## Fontes oficiais

- https://dev.epicgames.com/documentation/en-us/unreal-engine/overview-of-online-services-in-unreal-engine
- https://dev.epicgames.com/documentation/pt-br/unreal-engine/online-subsystem-eos-plugin-in-unreal-engine
- https://dev.epicgames.com/documentation/en-us/unreal-engine/enable-and-configure-online-services-eos-in-unreal-engine
