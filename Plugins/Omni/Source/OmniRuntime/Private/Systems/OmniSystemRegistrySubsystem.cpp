#include "Systems/OmniSystemRegistrySubsystem.h"

#include "Debug/OmniDebugSubsystem.h"
#include "Engine/GameInstance.h"
#include "Manifest/OmniManifest.h"
#include "Systems/OmniRuntimeSystem.h"
#include "Systems/OmniSystemMessageSchemas.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniRegistry, Log, All);

namespace OmniRegistry
{
	static TAutoConsoleVariable<int32> CVarOmniDevDefaults(
		TEXT("omni.devdefaults"),
		0,
		TEXT("Enable Omni DEV defaults fallback.\n0 = OFF (fail-fast)\n1 = ON (fallback allowed)"),
		ECVF_Default
	);
}

void UOmniSystemRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PublishRegistryDiagnostics(false);
	if (IsDevDefaultsEnabled())
	{
		UE_LOG(
			LogOmniRegistry,
			Warning,
			TEXT("[Omni][DevMode][Registry] Enabled: devdefaults=ON (omni.devdefaults=1 ou bAllowDevDefaults=True) | Not for production | May affect determinism")
		);
	}
	if (UOmniDebugSubsystem* DebugSubsystem = TryGetDebugSubsystem())
	{
		DebugSubsystem->SetMetric(TEXT("Omni.Profile.Action"), TEXT("Pending"));
		DebugSubsystem->SetMetric(TEXT("Omni.Profile.Status"), TEXT("Pending"));
		DebugSubsystem->SetMetric(TEXT("Omni.Profile.Movement"), TEXT("Pending"));
	}

	if (TryInitializeFromAutoManifest())
	{
		return;
	}

	if (TryInitializeFromConfiguredFallback())
	{
		return;
	}

	UE_LOG(
		LogOmniRegistry,
		Error,
		TEXT("[Omni][Registry][Init] Fail-fast: manifest nao inicializado e fallback indisponivel | Configure OmniManifest em Project Settings > Omni")
	);
}

void UOmniSystemRegistrySubsystem::Deinitialize()
{
	ShutdownSystemsInternal(false);
	Super::Deinitialize();
}

void UOmniSystemRegistrySubsystem::Tick(float DeltaTime)
{
	for (UOmniRuntimeSystem* System : ActiveSystems)
	{
		if (!System || !System->IsTickEnabled())
		{
			continue;
		}

		System->TickSystem(DeltaTime);
	}
}

TStatId UOmniSystemRegistrySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UOmniSystemRegistrySubsystem, STATGROUP_Tickables);
}

bool UOmniSystemRegistrySubsystem::IsTickable() const
{
	return bRegistryInitialized && !IsTemplate();
}

UWorld* UOmniSystemRegistrySubsystem::GetTickableGameObjectWorld() const
{
	return GetWorld();
}

bool UOmniSystemRegistrySubsystem::InitializeFromManifest(UOmniManifest* Manifest)
{
	ShutdownSystemsInternal(false);
	PublishRegistryDiagnostics(false);
	if (UOmniDebugSubsystem* DebugSubsystem = TryGetDebugSubsystem())
	{
		DebugSubsystem->SetMetric(TEXT("Omni.Profile.Action"), TEXT("Pending"));
		DebugSubsystem->SetMetric(TEXT("Omni.Profile.Status"), TEXT("Pending"));
		DebugSubsystem->SetMetric(TEXT("Omni.Profile.Movement"), TEXT("Pending"));
	}

	if (!Manifest)
	{
		UE_LOG(
			LogOmniRegistry,
			Warning,
			TEXT("[Omni][Registry][Init] Manifest nulo | Configure um UOmniManifest valido em Project Settings > Omni")
		);
		return false;
	}

	TMap<FName, FResolvedSystemSpec> Specs;
	if (!BuildSpecs(Manifest, Specs))
	{
		return false;
	}

	TArray<FName> InitializationOrder;
	if (!BuildInitializationOrder(Specs, InitializationOrder))
	{
		return false;
	}

	for (const FName SystemId : InitializationOrder)
	{
		const FResolvedSystemSpec* Spec = Specs.Find(SystemId);
		if (!Spec || !Spec->SystemClass)
		{
			UE_LOG(
				LogOmniRegistry,
				Error,
				TEXT("[Omni][Registry][Init] Spec invalido para SystemId=%s | Verifique Manifest e GetDependencies()"),
				*SystemId.ToString()
			);
			ShutdownSystemsInternal(false);
			return false;
		}

		UOmniRuntimeSystem* System = NewObject<UOmniRuntimeSystem>(this, Spec->SystemClass);
		if (!System)
		{
			UE_LOG(
				LogOmniRegistry,
				Error,
				TEXT("[Omni][Registry][Init] Falha ao instanciar SystemId=%s | Verifique classe no Manifest"),
				*SystemId.ToString()
			);
			ShutdownSystemsInternal(false);
			return false;
		}

		System->InitializeSystem(this, Manifest);
		if (!System->IsInitializationSuccessful())
		{
			UE_LOG(
				LogOmniRegistry,
				Error,
				TEXT("[Omni][Registry][Init] Fail-fast: SystemId=%s falhou na inicializacao | Corrija configuracao de profile/library no Manifest"),
				*SystemId.ToString()
			);
			ShutdownSystemsInternal(false);
			PublishRegistryDiagnostics(false);
			return false;
		}

		ActiveSystems.Add(System);
		SystemsById.Add(SystemId, System);
	}

	ActiveManifest = Manifest;
	bRegistryInitialized = true;

	UE_LOG(
		LogOmniRegistry,
		Log,
		TEXT("[Omni][Registry][Init] Inicializado | systems=%d manifest=%s"),
		ActiveSystems.Num(),
		*GetNameSafe(Manifest)
	);

	PublishRegistryDiagnostics(true);
	return true;
}

void UOmniSystemRegistrySubsystem::ShutdownSystems()
{
	ShutdownSystemsInternal(true);
}

bool UOmniSystemRegistrySubsystem::IsRegistryInitialized() const
{
	return bRegistryInitialized;
}

TArray<FName> UOmniSystemRegistrySubsystem::GetActiveSystemIds() const
{
	TArray<FName> Result;
	SystemsById.GenerateKeyArray(Result);
	Result.Sort(FNameLexicalLess());
	return Result;
}

UOmniRuntimeSystem* UOmniSystemRegistrySubsystem::GetSystemById(const FName SystemId) const
{
	if (const TObjectPtr<UOmniRuntimeSystem>* Found = SystemsById.Find(SystemId))
	{
		return Found->Get();
	}

	return nullptr;
}

UOmniManifest* UOmniSystemRegistrySubsystem::GetActiveManifest() const
{
	return ActiveManifest.Get();
}

bool UOmniSystemRegistrySubsystem::DispatchCommand(const FOmniCommandMessage& Command)
{
	if (!bRegistryInitialized || Command.TargetSystem == NAME_None)
	{
		return false;
	}

	FString ValidationError;
	if (!FOmniMessageSchemaValidator::ValidateCommand(Command, ValidationError))
	{
		UE_LOG(
			LogOmniRegistry,
			Warning,
			TEXT("[Omni][Registry][DispatchCommand] Payload invalido: source=%s target=%s command=%s error=%s | Corrija schema do command antes de enviar"),
			*Command.SourceSystem.ToString(),
			*Command.TargetSystem.ToString(),
			*Command.CommandName.ToString(),
			*ValidationError
		);
		return false;
	}

	UOmniRuntimeSystem* TargetSystem = GetSystemById(Command.TargetSystem);
	if (!TargetSystem)
	{
		UE_LOG(
			LogOmniRegistry,
			Warning,
			TEXT("[Omni][Registry][DispatchCommand] Target system ausente: %s | Verifique TargetSystem no command e inicializacao do Registry"),
			*Command.TargetSystem.ToString()
		);
		return false;
	}

	UE_LOG(
		LogOmniRegistry,
		Verbose,
		TEXT("[Omni][Registry][DispatchCommand] source=%s target=%s command=%s"),
		*Command.SourceSystem.ToString(),
		*Command.TargetSystem.ToString(),
		*Command.CommandName.ToString()
	);

	return TargetSystem->HandleCommand(Command);
}

bool UOmniSystemRegistrySubsystem::ExecuteQuery(FOmniQueryMessage& Query)
{
	Query.ResetResponse();

	if (!bRegistryInitialized || Query.TargetSystem == NAME_None)
	{
		return false;
	}

	FString ValidationError;
	if (!FOmniMessageSchemaValidator::ValidateQuery(Query, ValidationError))
	{
		UE_LOG(
			LogOmniRegistry,
			Warning,
			TEXT("[Omni][Registry][ExecuteQuery] Payload invalido: source=%s target=%s query=%s error=%s | Corrija schema da query antes de enviar"),
			*Query.SourceSystem.ToString(),
			*Query.TargetSystem.ToString(),
			*Query.QueryName.ToString(),
			*ValidationError
		);
		return false;
	}

	UOmniRuntimeSystem* TargetSystem = GetSystemById(Query.TargetSystem);
	if (!TargetSystem)
	{
		UE_LOG(
			LogOmniRegistry,
			Warning,
			TEXT("[Omni][Registry][ExecuteQuery] Target system ausente: %s | Verifique TargetSystem na query e inicializacao do Registry"),
			*Query.TargetSystem.ToString()
		);
		return false;
	}

	UE_LOG(
		LogOmniRegistry,
		Verbose,
		TEXT("[Omni][Registry][ExecuteQuery] source=%s target=%s query=%s"),
		*Query.SourceSystem.ToString(),
		*Query.TargetSystem.ToString(),
		*Query.QueryName.ToString()
	);

	const bool bHandled = TargetSystem->HandleQuery(Query);
	Query.bHandled = Query.bHandled || bHandled;
	return bHandled;
}

void UOmniSystemRegistrySubsystem::BroadcastEvent(const FOmniEventMessage& Event)
{
	if (!bRegistryInitialized)
	{
		return;
	}

	FString ValidationError;
	if (!FOmniMessageSchemaValidator::ValidateEvent(Event, ValidationError))
	{
		UE_LOG(
			LogOmniRegistry,
			Warning,
			TEXT("[Omni][Registry][BroadcastEvent] Payload invalido: source=%s event=%s error=%s | Corrija schema do event antes de publicar"),
			*Event.SourceSystem.ToString(),
			*Event.EventName.ToString(),
			*ValidationError
		);
		return;
	}

	UE_LOG(
		LogOmniRegistry,
		Verbose,
		TEXT("[Omni][Registry][BroadcastEvent] source=%s event=%s target=ALL(%d)"),
		*Event.SourceSystem.ToString(),
		*Event.EventName.ToString(),
		ActiveSystems.Num()
	);

	for (UOmniRuntimeSystem* System : ActiveSystems)
	{
		if (!System)
		{
			continue;
		}

		System->HandleEvent(Event);
	}
}

bool UOmniSystemRegistrySubsystem::IsDevDefaultsEnabled() const
{
	const int32 CVarValue = OmniRegistry::CVarOmniDevDefaults.GetValueOnGameThread();
	return bAllowDevDefaults || CVarValue > 0;
}

bool UOmniSystemRegistrySubsystem::TryInitializeFromAutoManifest()
{
	if (!AutoManifestAssetPath.IsNull())
	{
		UObject* LoadedObject = AutoManifestAssetPath.TryLoad();
		UOmniManifest* Manifest = Cast<UOmniManifest>(LoadedObject);
		if (!Manifest)
		{
			UE_LOG(
				LogOmniRegistry,
				Warning,
				TEXT("[Omni][Registry][AutoManifest] AssetPath invalido para UOmniManifest: %s | Corrija AutoManifestAssetPath"),
				*AutoManifestAssetPath.ToString()
			);
		}
		else if (InitializeFromManifest(Manifest))
		{
			return true;
		}
	}

	if (AutoManifestClassPath.IsNull())
	{
		return false;
	}

	UClass* LoadedManifestClass = AutoManifestClassPath.TryLoadClass<UOmniManifest>();
	if (!LoadedManifestClass || !LoadedManifestClass->IsChildOf(UOmniManifest::StaticClass()))
	{
		UE_LOG(
			LogOmniRegistry,
			Warning,
			TEXT("[Omni][Registry][AutoManifest] ClassPath invalido para UOmniManifest: %s | Corrija AutoManifestClassPath"),
			*AutoManifestClassPath.ToString()
		);
		return false;
	}

	UOmniManifest* ClassManifest = NewObject<UOmniManifest>(this, LoadedManifestClass, NAME_None, RF_Transient);
	if (!ClassManifest)
	{
		UE_LOG(
			LogOmniRegistry,
			Warning,
			TEXT("[Omni][Registry][AutoManifest] Falha ao instanciar manifest: %s | Verifique construtor/default object"),
			*AutoManifestClassPath.ToString()
		);
		return false;
	}

	const bool bInitialized = InitializeFromManifest(ClassManifest);
	if (bInitialized)
	{
		if (IsDevDefaultsEnabled())
		{
			UE_LOG(
				LogOmniRegistry,
				Warning,
				TEXT("[Omni][DevMode][Registry] Enabled: devdefaults=ON (omni.devdefaults=1 ou bAllowDevDefaults=True) | ManifestClass=%s | Not for production | May affect determinism"),
				*AutoManifestClassPath.ToString()
			);
		}
		else
		{
			UE_LOG(
				LogOmniRegistry,
				Verbose,
				TEXT("[Omni][Registry][AutoManifest] Inicializado via classe: %s"),
				*AutoManifestClassPath.ToString()
			);
		}
	}

	return bInitialized;
}

bool UOmniSystemRegistrySubsystem::TryInitializeFromConfiguredFallback()
{
	if (!bUseConfiguredFallbackSystems)
	{
		return false;
	}

	if (!IsDevDefaultsEnabled())
	{
		UE_LOG(
			LogOmniRegistry,
			Warning,
			TEXT("[Omni][Registry][Fallback] Configured fallback ignorado | Ative omni.devdefaults=1 apenas em debug")
		);
		return false;
	}

	if (FallbackSystemClasses.Num() == 0)
	{
		UE_LOG(
			LogOmniRegistry,
			Warning,
			TEXT("[Omni][Registry][Fallback] Lista vazia de FallbackSystemClasses | Defina classes ou desative fallback")
		);
		return false;
	}

	UE_LOG(
		LogOmniRegistry,
		Warning,
		TEXT("[Omni][DevMode][Registry] Enabled: fallback estrutural via config | Not for production | May affect determinism")
	);

	UOmniManifest* FallbackManifest = NewObject<UOmniManifest>(this, NAME_None, RF_Transient);
	FallbackManifest->Namespace = TEXT("Omni.Fallback");
	FallbackManifest->BuildVersion = 1;

	for (const FSoftClassPath& SystemClassPath : FallbackSystemClasses)
	{
		if (SystemClassPath.IsNull())
		{
			continue;
		}

		UClass* LoadedSystemClass = SystemClassPath.TryLoadClass<UOmniRuntimeSystem>();
		if (!LoadedSystemClass)
		{
			UE_LOG(
				LogOmniRegistry,
				Warning,
				TEXT("[Omni][Registry][Fallback] Classe nao carregada: %s | Verifique classe no config"),
				*SystemClassPath.ToString()
			);
			continue;
		}

		FOmniSystemManifestEntry& Entry = FallbackManifest->Systems.AddDefaulted_GetRef();
		Entry.bEnabled = true;
		Entry.SystemClass = LoadedSystemClass;
	}

	if (FallbackManifest->Systems.Num() == 0)
	{
		UE_LOG(
			LogOmniRegistry,
			Warning,
			TEXT("[Omni][Registry][Fallback] Nenhuma classe de system carregada | Corrija FallbackSystemClasses")
		);
		return false;
	}

	const bool bInitialized = InitializeFromManifest(FallbackManifest);
	if (bInitialized)
	{
		UE_LOG(
			LogOmniRegistry,
			Warning,
			TEXT("[Omni][DevMode][Registry] Inicializado via DEV_FALLBACK | classes=%d | Not for production | May affect determinism"),
			FallbackManifest->Systems.Num()
		);
	}

	return bInitialized;
}

bool UOmniSystemRegistrySubsystem::BuildSpecs(const UOmniManifest* Manifest, TMap<FName, FResolvedSystemSpec>& OutSpecs) const
{
	if (!Manifest)
	{
		return false;
	}

	OutSpecs.Reset();

	for (const FOmniSystemManifestEntry& Entry : Manifest->Systems)
	{
		if (!Entry.bEnabled)
		{
			continue;
		}

		UClass* LoadedClass = Entry.SystemClass.LoadSynchronous();
		if (!LoadedClass)
		{
			UE_LOG(
				LogOmniRegistry,
				Warning,
				TEXT("[Omni][Registry][BuildSpecs] Entry habilitado com classe nula no manifest '%s' | Corrija SystemClass no manifest"),
				*GetNameSafe(Manifest)
			);
			continue;
		}
		if (!LoadedClass->IsChildOf(UOmniRuntimeSystem::StaticClass()))
		{
			UE_LOG(
				LogOmniRegistry,
				Error,
				TEXT("[Omni][Registry][BuildSpecs] Classe invalida: '%s' nao herda UOmniRuntimeSystem | Ajuste SystemClass no manifest"),
				*GetNameSafe(LoadedClass)
			);
			return false;
		}

		const UOmniRuntimeSystem* CDO = Cast<UOmniRuntimeSystem>(LoadedClass->GetDefaultObject());
		FName ResolvedSystemId = Entry.SystemId;
		if (ResolvedSystemId == NAME_None && CDO)
		{
			ResolvedSystemId = CDO->GetSystemId();
		}
		if (ResolvedSystemId == NAME_None)
		{
			ResolvedSystemId = LoadedClass->GetFName();
		}

		if (OutSpecs.Contains(ResolvedSystemId))
		{
			UE_LOG(
				LogOmniRegistry,
				Error,
				TEXT("[Omni][Registry][BuildSpecs] Duplicate SystemId '%s' no manifest '%s' | Mantenha SystemId unico por system"),
				*ResolvedSystemId.ToString(),
				*GetNameSafe(Manifest)
			);
			return false;
		}

		FResolvedSystemSpec Spec;
		Spec.SystemId = ResolvedSystemId;
		Spec.SystemClass = LoadedClass;
		Spec.Dependencies = Entry.Dependencies;

		if (Spec.Dependencies.Num() == 0 && CDO)
		{
			Spec.Dependencies = CDO->GetDependencies();
		}

		OutSpecs.Add(ResolvedSystemId, MoveTemp(Spec));
	}

	if (OutSpecs.Num() == 0)
	{
		UE_LOG(
			LogOmniRegistry,
			Warning,
			TEXT("[Omni][Registry][BuildSpecs] Nenhum system habilitado no manifest '%s' | Habilite ao menos um system no manifesto"),
			*GetNameSafe(Manifest)
		);
	}

	return true;
}

bool UOmniSystemRegistrySubsystem::BuildInitializationOrder(
	const TMap<FName, FResolvedSystemSpec>& Specs,
	TArray<FName>& OutInitializationOrder
) const
{
	OutInitializationOrder.Reset();
	if (Specs.Num() == 0)
	{
		return true;
	}

	TMap<FName, int32> InDegree;
	TMap<FName, TArray<FName>> OutEdges;

	for (const TPair<FName, FResolvedSystemSpec>& Pair : Specs)
	{
		InDegree.Add(Pair.Key, 0);
	}

	for (const TPair<FName, FResolvedSystemSpec>& Pair : Specs)
	{
		const FName SystemId = Pair.Key;
		const FResolvedSystemSpec& Spec = Pair.Value;

		for (const FName DependencyId : Spec.Dependencies)
		{
			if (DependencyId == NAME_None || DependencyId == SystemId)
			{
				continue;
			}

			if (!Specs.Contains(DependencyId))
			{
				UE_LOG(
					LogOmniRegistry,
					Warning,
					TEXT("[Omni][Registry][Dependencies] Dependencia ausente: %s -> %s | Adicione em GetDependencies() ou Manifest"),
					*SystemId.ToString(),
					*DependencyId.ToString()
				);
				continue;
			}

			OutEdges.FindOrAdd(DependencyId).Add(SystemId);
			InDegree[SystemId] = InDegree[SystemId] + 1;
		}
	}

	TArray<FName> ReadyQueue;
	for (const TPair<FName, int32>& Pair : InDegree)
	{
		if (Pair.Value == 0)
		{
			ReadyQueue.Add(Pair.Key);
		}
	}
	ReadyQueue.Sort(FNameLexicalLess());

	while (ReadyQueue.Num() > 0)
	{
		const FName Current = ReadyQueue[0];
		ReadyQueue.RemoveAt(0);
		OutInitializationOrder.Add(Current);

		if (const TArray<FName>* Dependents = OutEdges.Find(Current))
		{
			for (const FName DependentId : *Dependents)
			{
				int32& Degree = InDegree.FindChecked(DependentId);
				Degree--;
				if (Degree == 0)
				{
					ReadyQueue.Add(DependentId);
				}
			}
			ReadyQueue.Sort(FNameLexicalLess());
		}
	}

	if (OutInitializationOrder.Num() != Specs.Num())
	{
		UE_LOG(
			LogOmniRegistry,
			Error,
			TEXT("[Omni][Registry][Dependencies] Ciclo detectado na inicializacao | Remova dependencia circular em GetDependencies()")
		);
		return false;
	}

	return true;
}

UOmniDebugSubsystem* UOmniSystemRegistrySubsystem::TryGetDebugSubsystem() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetSubsystem<UOmniDebugSubsystem>();
	}

	return nullptr;
}

void UOmniSystemRegistrySubsystem::PublishRegistryDiagnostics(const bool bManifestLoaded) const
{
	UOmniDebugSubsystem* DebugSubsystem = TryGetDebugSubsystem();
	if (!DebugSubsystem)
	{
		return;
	}

	DebugSubsystem->SetMetric(TEXT("Omni.ManifestLoaded"), bManifestLoaded ? TEXT("True") : TEXT("False"));
	DebugSubsystem->SetMetric(TEXT("Omni.DevDefaults"), IsDevDefaultsEnabled() ? TEXT("ON") : TEXT("OFF"));
}

void UOmniSystemRegistrySubsystem::ShutdownSystemsInternal(const bool bLogSummary)
{
	if (ActiveSystems.Num() > 0)
	{
		for (int32 Index = ActiveSystems.Num() - 1; Index >= 0; --Index)
		{
			if (UOmniRuntimeSystem* System = ActiveSystems[Index])
			{
				System->ShutdownSystem();
			}
		}
	}

	if (bLogSummary && (ActiveSystems.Num() > 0 || bRegistryInitialized))
	{
		UE_LOG(LogOmniRegistry, Log, TEXT("[Omni][Registry][Shutdown] Concluido | systemsParados=%d"), ActiveSystems.Num());
	}

	ActiveSystems.Reset();
	SystemsById.Reset();
	ActiveManifest = nullptr;
	bRegistryInitialized = false;
	PublishRegistryDiagnostics(false);
}
