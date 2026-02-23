#include "Systems/OmniSystemRegistrySubsystem.h"

#include "Manifest/OmniManifest.h"
#include "Systems/OmniRuntimeSystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogOmniRegistry, Log, All);

void UOmniSystemRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (TryInitializeFromAutoManifest())
	{
		return;
	}

	if (TryInitializeFromConfiguredFallback())
	{
		return;
	}

	UE_LOG(LogOmniRegistry, Verbose, TEXT("Registry started without auto-initialization source."));
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

	if (!Manifest)
	{
		UE_LOG(LogOmniRegistry, Warning, TEXT("Cannot initialize registry: Manifest is null."));
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
			UE_LOG(LogOmniRegistry, Error, TEXT("Invalid system spec for '%s' during initialization."), *SystemId.ToString());
			ShutdownSystemsInternal(false);
			return false;
		}

		UOmniRuntimeSystem* System = NewObject<UOmniRuntimeSystem>(this, Spec->SystemClass);
		if (!System)
		{
			UE_LOG(LogOmniRegistry, Error, TEXT("Failed to instantiate system '%s'."), *SystemId.ToString());
			ShutdownSystemsInternal(false);
			return false;
		}

		System->InitializeSystem(this, Manifest);
		ActiveSystems.Add(System);
		SystemsById.Add(SystemId, System);
	}

	ActiveManifest = Manifest;
	bRegistryInitialized = true;

	UE_LOG(
		LogOmniRegistry,
		Log,
		TEXT("SystemRegistry initialized. Systems: %d. Manifest: %s"),
		ActiveSystems.Num(),
		*GetNameSafe(Manifest)
	);

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
				TEXT("AutoManifestAssetPath nao aponta para UOmniManifest: %s"),
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
			TEXT("AutoManifestClassPath nao aponta para classe UOmniManifest valida: %s"),
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
			TEXT("Falha ao instanciar manifest via classe: %s"),
			*AutoManifestClassPath.ToString()
		);
		return false;
	}

	return InitializeFromManifest(ClassManifest);
}

bool UOmniSystemRegistrySubsystem::TryInitializeFromConfiguredFallback()
{
	if (!bUseConfiguredFallbackSystems || FallbackSystemClasses.Num() == 0)
	{
		return false;
	}

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
				TEXT("Classe de fallback nao carregada: %s"),
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
		UE_LOG(LogOmniRegistry, Warning, TEXT("Fallback configurado, mas nenhuma classe de system foi carregada."));
		return false;
	}

	const bool bInitialized = InitializeFromManifest(FallbackManifest);
	if (bInitialized)
	{
		UE_LOG(
			LogOmniRegistry,
			Log,
			TEXT("Registry inicializado via fallback (%d classes)."),
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
			UE_LOG(LogOmniRegistry, Warning, TEXT("Skipping enabled entry with null class in manifest '%s'."), *GetNameSafe(Manifest));
			continue;
		}
		if (!LoadedClass->IsChildOf(UOmniRuntimeSystem::StaticClass()))
		{
			UE_LOG(LogOmniRegistry, Error, TEXT("Class '%s' is not a UOmniRuntimeSystem."), *GetNameSafe(LoadedClass));
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
			UE_LOG(LogOmniRegistry, Error, TEXT("Duplicate SystemId '%s' in manifest '%s'."), *ResolvedSystemId.ToString(), *GetNameSafe(Manifest));
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
		UE_LOG(LogOmniRegistry, Warning, TEXT("No enabled systems found in manifest '%s'."), *GetNameSafe(Manifest));
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
					TEXT("System '%s' depends on missing system '%s'. Dependency will be ignored."),
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
		UE_LOG(LogOmniRegistry, Error, TEXT("Cycle detected in system dependencies. Registry initialization aborted."));
		return false;
	}

	return true;
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
		UE_LOG(LogOmniRegistry, Log, TEXT("SystemRegistry shutdown. Systems stopped: %d"), ActiveSystems.Num());
	}

	ActiveSystems.Reset();
	SystemsById.Reset();
	ActiveManifest = nullptr;
	bRegistryInitialized = false;
}
