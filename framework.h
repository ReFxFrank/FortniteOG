#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <algorithm>
#include <numeric>
#include <intrin.h>
#include <sstream>
#include <array>
#include <tlhelp32.h>
#include <future>
#include <set>
#include <variant>

#include "minhook/MinHook.h"
#include "SDK/SDK.hpp"
#include "Globals.h"

#pragma comment(lib, "minhook/minhook.lib")

using namespace SDK;

static auto ImageBase = InSDKUtils::GetImageBase();

static UNetDriver* (*CreateNetDriver)(void*, void*, FName) = decltype(CreateNetDriver)(ImageBase + 0x4573990);
static bool (*InitListen)(void*, void*, FURL&, bool, FString&) = decltype(InitListen)(ImageBase + 0xD44C40);
static void (*SetWorld)(void*, void*) = decltype(SetWorld)(ImageBase + 0x42C2B20);
static bool (*InitHost)(UObject* Beacon) = decltype(InitHost)(ImageBase + 0xd446f0);
static void (*PauseBeaconRequests)(UObject* Beacon, bool bPause) = decltype(PauseBeaconRequests)(ImageBase + 0x20dfad0);

static void(*GiveAbility)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle*, FGameplayAbilitySpec) = decltype(GiveAbility)(ImageBase + 0x6b19e0);
static void (*AbilitySpecConstructor)(FGameplayAbilitySpec*, UGameplayAbility*, int, int, UObject*) = decltype(AbilitySpecConstructor)(ImageBase + 0x6d6dd0);
static bool (*InternalTryActivateAbility)(UAbilitySystemComponent* AbilitySystemComp, FGameplayAbilitySpecHandle AbilityToActivate, FPredictionKey InPredictionKey, UGameplayAbility** OutInstancedAbility, void* OnGameplayAbilityEndedDelegate, const FGameplayEventData* TriggerEventData) = decltype(InternalTryActivateAbility)(ImageBase + 0x6b33f0);
static FGameplayAbilitySpecHandle(*GiveAbilityAndActivateOnce)(UAbilitySystemComponent* ASC, FGameplayAbilitySpecHandle*, FGameplayAbilitySpec) = decltype(GiveAbilityAndActivateOnce)(ImageBase + 0x6b1b00);

static FVector* (*PickSupplyDropLocationOG)(AFortAthenaMapInfo* MapInfo, FVector* outLocation, __int64 Center, float Radius) = decltype(PickSupplyDropLocationOG)(ImageBase + 0x18848f0);

inline static ABuildingSMActor* (*ReplaceBuildingActor)(ABuildingSMActor* BuildingSMActor, unsigned int a2, UObject* a3, unsigned int a4, int a5, bool bMirrored, AFortPlayerControllerAthena* PC) = decltype(ReplaceBuildingActor)(ImageBase + 0x1B951B0);
static __int64 (*CantBuild)(UWorld*, UObject*, FVector, FRotator, char, void*, char*) = decltype(CantBuild)(ImageBase + 0x1E57790);

static void* (*ApplyCharacterCustomization)(void* a1, void* a2) = decltype(ApplyCharacterCustomization)(ImageBase + 0x2363ff0);

static void (*BotManagerSetup)(__int64 BotManaager, __int64 Pawn, __int64 BehaviorTree, __int64 a4, DWORD* SkillLevel, __int64 a7, __int64 StartupInventory, __int64 BotNameSettings, __int64 a10, BYTE* CanRespawnOnDeath, unsigned __int8 BitFieldDataThing, BYTE* CustomSquadId, FFortAthenaAIBotRunTimeCustomizationData InRuntimeBotData) = decltype(BotManagerSetup)(ImageBase + 0x19D93F0);

static char(__fastcall* RegisterComponentWithWorld)(UObject* Component, UObject* World) = decltype(RegisterComponentWithWorld)(ImageBase + 0x3FF94E0);

static void(*RemoveFromAlivePlayers)(AFortGameModeAthena*, AFortPlayerControllerAthena*, APlayerState*, AFortPlayerPawn*, UFortWeaponItemDefinition*, uint8_t DeathCause, char) = decltype(RemoveFromAlivePlayers)(ImageBase + 0x18ECBB0);
static void (*AddToAlivePlayers)(AFortGameModeAthena* GameMode, AFortPlayerControllerAthena* Player) = decltype(AddToAlivePlayers)(ImageBase + 0x18c35b0);

static void* (*StaticFindObjectOG)(UClass*, UObject* Package, const wchar_t* OrigInName, bool ExactClass) = decltype(StaticFindObjectOG)(ImageBase + 0x2E1C4B0);
static void* (*StaticLoadObjectOG)(UClass* Class, UObject* InOuter, const TCHAR* Name, const TCHAR* Filename, uint32_t LoadFlags, UObject* Sandbox, bool bAllowObjectReconciliation, void*) = decltype(StaticLoadObjectOG)(ImageBase + 0x2E1D7A0);

AFortAthenaMutator_Bots* BotMutator = nullptr;
TArray<FVector> PickedSupplyDropLocations;
TArray<APlayerController*> GivenLootPlayers;

static TArray<AActor*> PlayerStarts;

bool bFirstElimTriggered = false;
bool bFirstEliminated = false;
bool DontPlayAnimation = false;

bool bFirstChestSearched = false;
bool bFirstSupplyDropSearched = false;

// text manipulation utils
namespace TextManipUtils {
	// Found this from stack overflow :fire:
	std::vector<std::string> SplitWhitespace(std::string const& input) {
		std::istringstream buffer(input);
		std::vector<std::string> ret;

		std::copy(std::istream_iterator<std::string>(buffer),
			std::istream_iterator<std::string>(),
			std::back_inserter(ret));
		return ret;
	}
}

void Log(const std::string& msg)
{
	static bool firstCall = true;

	if (firstCall)
	{
		std::ofstream logFile("OGS_log.txt", std::ios::trunc);
		if (logFile.is_open())
		{
			logFile << "[OGS]: Log file initialized!\n";
			logFile.close();
		}
		firstCall = false;
	}

	std::ofstream logFile("OGS_log.txt", std::ios::app);
	if (logFile.is_open())
	{
		logFile << "[OGS]: " << msg << std::endl;
		logFile.close();
	}

	std::cout << "[OGS]: " << msg << std::endl;
}

inline void EnsureGameNetDriverDefinition()
{
	UEngine* Engine = UEngine::GetEngine();
	if (!Engine)
		return;

	FName GameNetDriver = UKismetStringLibrary::Conv_StringToName(FString(L"GameNetDriver"));
	for (int32 i = 0; i < Engine->NetDriverDefinitions.Num(); i++)
	{
		if (Engine->NetDriverDefinitions[i].DefName == GameNetDriver)
			return;
	}

	FName IpNetDriver = UKismetStringLibrary::Conv_StringToName(FString(L"/Script/OnlineSubsystemUtils.IpNetDriver"));
	FNetDriverDefinition Definition{};
	Definition.DefName = GameNetDriver;
	Definition.DriverClassName = IpNetDriver;
	Definition.DriverClassNameFallback = IpNetDriver;

	Engine->NetDriverDefinitions.Add(Definition);
	Log("Added missing GameNetDriver definition.");
}

void HookVTable(void* Base, int Idx, void* Detour, void** OG)
{
	DWORD oldProtection;

	void** VTable = *(void***)Base;

	if (OG)
	{
		*OG = VTable[Idx];
	}

	VirtualProtect(&VTable[Idx], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtection);

	VTable[Idx] = Detour;

	VirtualProtect(&VTable[Idx], sizeof(void*), oldProtection, NULL);
}

// pasted shit from ploosh wow
template <typename T = void*>
__forceinline static void ExecHook(UFunction* func, void* detour, T& og = nullptr)
{
	if (!func)
		return;
	if (!std::is_same_v<T, void*>)
		og = (T)func->ExecFunction;

	func->ExecFunction = reinterpret_cast<UFunction::FNativeFuncPtr>(detour);
}

class FOutputDevice
{
public:
	bool bSuppressEventTag;
	bool bAutoEmitLineTerminator;
	uint8_t _Padding1[0x6];
};

class FFrame : public FOutputDevice
{
public:
	void** VTable;
	UFunction* Node;
	UObject* Object;
	uint8* Code;
	uint8* Locals;
	void* MostRecentProperty;
	uint8_t* MostRecentPropertyAddress;
	uint8_t _Padding1[0x40];
	FField* PropertyChainForCompiledIn;

public:
	void StepCompiledIn(void* const Result, bool ForceExplicitProp = false)
	{
		if (Code && !ForceExplicitProp)
		{
			((void (*)(FFrame*, UObject*, void* const))(ImageBase + 0x2E1DD00))(this, Object, Result);
		}
		else
		{
			FField* _Prop = PropertyChainForCompiledIn;
			PropertyChainForCompiledIn = _Prop->Next;
			((void (*)(FFrame*, void* const, FField*))(ImageBase + 0x2E1DD30))(this, Result, _Prop);
		}
	}

	template <typename T>
	T& StepCompiledInRef()
	{
		T TempVal{};
		MostRecentPropertyAddress = nullptr;

		if (Code)
		{
			((void (*)(FFrame*, UObject*, void* const))(ImageBase + 0x2E1DD00))(this, Object, &TempVal);
		}
		else
		{
			FField* _Prop = PropertyChainForCompiledIn;
			PropertyChainForCompiledIn = _Prop->Next;
			((void (*)(FFrame*, void* const, FField*))(ImageBase + 0x2E1DD30))(this, &TempVal, _Prop);
		}

		return MostRecentPropertyAddress ? *(T*)MostRecentPropertyAddress : TempVal;
	}
};

inline FQuat RotatorToQuat(FRotator Rotation)
{
	FQuat Quat;
	const float DEG_TO_RAD = 3.14159f / 180.0f;
	const float DIVIDE_BY_2 = DEG_TO_RAD / 2.0f;

	float SP = sin(Rotation.Pitch * DIVIDE_BY_2);
	float CP = cos(Rotation.Pitch * DIVIDE_BY_2);
	float SY = sin(Rotation.Yaw * DIVIDE_BY_2);
	float CY = cos(Rotation.Yaw * DIVIDE_BY_2);
	float SR = sin(Rotation.Roll * DIVIDE_BY_2);
	float CR = cos(Rotation.Roll * DIVIDE_BY_2);

	Quat.X = CR * SP * SY - SR * CP * CY;
	Quat.Y = -CR * SP * CY - SR * CP * SY;
	Quat.Z = CR * CP * SY - SR * SP * CY;
	Quat.W = CR * CP * CY + SR * SP * SY;

	return Quat;
}

template <typename T>
static inline T* StaticFindObject(std::wstring ObjectName)
{
	return (T*)StaticFindObjectOG(T::StaticClass(), nullptr, ObjectName.c_str(), false);
}

template<typename T>
inline T* Cast(UObject* Object)
{
	if (!Object || !Object->IsA(T::StaticClass()))
		return nullptr;
	return (T*)Object;
}

inline void MarkControllerServerFinishedLoading(AFortPlayerControllerAthena* PC, const char* Reason)
{
	if (!PC)
		return;

	PC->bHasClientFinishedLoading = true;
	PC->bHasServerFinishedLoading = true;
	PC->bReadyToStartMatch = true;
	PC->bLoadingScreenDropped = true;
	PC->ClientStopWaitLTMLoading();
	PC->ForceNetUpdate();

	std::string Message = "Marked server finished loading";
	if (Reason && Reason[0])
		Message += std::string(" after ") + Reason;
	Message += ".";
	Log(Message);
}

template<typename T = UObject>
static inline T* StaticLoadObject(const std::string& Name)
{
	auto ConvName = std::wstring(Name.begin(), Name.end());

	T* Object = StaticFindObject<T>(ConvName);

	if (!Object)
	{
		Object = (T*)StaticLoadObjectOG(T::StaticClass(), nullptr, ConvName.c_str(), nullptr, 0, nullptr, false, nullptr);
	}

	return Object;
}

inline UAthenaCharacterItemDefinition* GetFallbackAthenaCharacterDefinition()
{
	static UAthenaCharacterItemDefinition* Fallback = nullptr;

	if (Fallback && Fallback->HeroDefinition)
		return Fallback;

	const char* FallbackPaths[] = {
		"/Game/Athena/Items/Cosmetics/Characters/CID_556_Athena_Commando_F_RebirthDefaultA.CID_556_Athena_Commando_F_RebirthDefaultA",
		"/Game/Athena/Items/Cosmetics/Characters/CID_NPC_Athena_Commando_M_HenchmanBad.CID_NPC_Athena_Commando_M_HenchmanBad",
		"/Game/Athena/Items/Cosmetics/Characters/CID_NPC_Athena_Commando_M_HenchmanGood.CID_NPC_Athena_Commando_M_HenchmanGood"
	};

	for (const char* Path : FallbackPaths)
	{
		UAthenaCharacterItemDefinition* Candidate = StaticLoadObject<UAthenaCharacterItemDefinition>(Path);
		if (Candidate && Candidate->HeroDefinition)
		{
			Fallback = Candidate;
			return Fallback;
		}
	}

	return nullptr;
}

inline bool EnsureValidAthenaLoadout(FFortAthenaLoadout& Loadout, const char* Owner)
{
	if (Loadout.Character && Loadout.Character->HeroDefinition)
		return true;

	UAthenaCharacterItemDefinition* Fallback = GetFallbackAthenaCharacterDefinition();
	if (!Fallback || !Fallback->HeroDefinition)
	{
		std::string Message = "No valid fallback character for cosmetic loadout";
		if (Owner && Owner[0])
			Message += std::string(" (") + Owner + ")";
		Message += ".";
		Log(Message);
		return false;
	}

	Loadout.Character = Fallback;
	Loadout.bIsDefaultCharacter = true;

	std::string Message = "Applied fallback character to cosmetic loadout";
	if (Owner && Owner[0])
		Message += std::string(" (") + Owner + ")";
	Message += ".";
	Log(Message);
	return true;
}

inline UFortCustomizationAssetLoader* EnsurePawnCustomizationAssetLoader(AFortPlayerPawn* Pawn, const char* Owner)
{
	if (!Pawn)
		return nullptr;

	if (Pawn->CustomizationAssetLoader)
		return Pawn->CustomizationAssetLoader;

	Pawn->CustomizationAssetLoader = (UFortCustomizationAssetLoader*)UGameplayStatics::SpawnObject(UFortCustomizationAssetLoader::StaticClass(), Pawn);

	std::string Message = Pawn->CustomizationAssetLoader
		? "Created missing pawn CustomizationAssetLoader"
		: "Failed to create pawn CustomizationAssetLoader";
	if (Owner && Owner[0])
		Message += std::string(" (") + Owner + ")";
	Message += ".";
	Log(Message);

	return Pawn->CustomizationAssetLoader;
}

inline bool TryApplyPawnCosmeticLoadout(AFortPlayerPawn* Pawn, const char* Owner)
{
	if (!Pawn)
		return false;

	if (!EnsureValidAthenaLoadout(Pawn->CosmeticLoadout, Owner))
		return false;

	if (!EnsurePawnCustomizationAssetLoader(Pawn, Owner))
	{
		std::string Message = "Skipped pawn cosmetic OnRep; CustomizationAssetLoader is not initialized";
		if (Owner && Owner[0])
			Message += std::string(" (") + Owner + ")";
		Message += ".";
		Log(Message);
		return false;
	}

	// FIX: CosmeticLoadout is a replicated property - OnRep_CosmeticLoadout() is meant
	// to be invoked by the engine on each CLIENT once the replicated value arrives, not
	// called manually from server code. OnRep_ functions can assume client-only context
	// (local player/UI/customization state) that isn't guaranteed to exist when forced
	// from here, which was producing a null-pointer access violation
	// (EXCEPTION_ACCESS_VIOLATION reading 0x000000c8) inside SpawnDefaultPawnFor.
	// The caller already calls Pawn->ForceNetUpdate() right after this, which is enough
	// to replicate the property and let each client (including the listen-server host)
	// correctly run OnRep_CosmeticLoadout() locally with proper context.
	return true;
}

template<typename T>
T* GetDefaultObject()
{
	return (T*)T::StaticClass()->DefaultObject;
}

static inline FQuat FRotToQuat(FRotator Rotation) {
	FQuat Quat;
	const float DEG_TO_RAD = 3.14159f / 180.0f;
	const float DIVIDE_BY_2 = DEG_TO_RAD / 2.0f;

	float SP = sin(Rotation.Pitch * DIVIDE_BY_2);
	float CP = cos(Rotation.Pitch * DIVIDE_BY_2);
	float SY = sin(Rotation.Yaw * DIVIDE_BY_2);
	float CY = cos(Rotation.Yaw * DIVIDE_BY_2);
	float SR = sin(Rotation.Roll * DIVIDE_BY_2);
	float CR = cos(Rotation.Roll * DIVIDE_BY_2);

	Quat.X = CR * SP * SY - SR * CP * CY;
	Quat.Y = -CR * SP * CY - SR * CP * SY;
	Quat.Z = CR * CP * SY - SR * SP * CY;
	Quat.W = CR * CP * CY + SR * SP * SY;

	return Quat;
}

template<typename T>
inline T* SpawnActor(FVector Loc, FRotator Rot = FRotator(), AActor* Owner = nullptr, SDK::UClass* Class = T::StaticClass(), ESpawnActorCollisionHandlingMethod Handle = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
{
	FTransform Transform{};
	Transform.Scale3D = FVector{ 1,1,1 };
	Transform.Translation = Loc;
	Transform.Rotation = FRotToQuat(Rot);

	return (T*)UGameplayStatics::FinishSpawningActor(UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, Handle, Owner), Transform);
}

template<typename T>
static inline T* SpawnAActor(FVector Loc = { 0,0,0 }, FRotator Rot = { 0,0,0 }, AActor* Owner = nullptr)
{
	FTransform Transform{};
	Transform.Scale3D = { 1,1,1 };
	Transform.Translation = Loc;
	Transform.Rotation = FRotToQuat(Rot);

	AActor* NewActor = UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), T::StaticClass(), Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, Owner);
	return (T*)UGameplayStatics::FinishSpawningActor(NewActor, Transform);
}

template<typename T>
static inline T* SpawnActorClass(FVector Loc = { 0,0,0 }, FRotator Rot = { 0,0,0 }, UClass* Class = nullptr, AActor* Owner = nullptr)
{
	FTransform Transform{};
	Transform.Scale3D = { 1,1,1 };
	Transform.Translation = Loc;
	Transform.Rotation = RotatorToQuat(Rot);

	AActor* NewActor = UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, Owner);
	return (T*)UGameplayStatics::FinishSpawningActor(NewActor, Transform);
}

template<typename T>
T* Actors(UClass* Class = T::StaticClass(), FVector Loc = {}, FRotator Rot = {}, AActor* Owner = nullptr)
{
	return SpawnActor<T>(Loc, Rot, Owner, Class);
}

AActor* DuplicateActor(AActor* Original)
{
	if (!Original) return nullptr;

	const FVector Location = Original->K2_GetActorLocation();
	const FRotator Rotation = Original->K2_GetActorRotation();
	const FTransform Transform = Original->GetTransform();

	auto* Class = Original->Class;
	auto* Owner = Original->GetOwner();

	auto* Duplicated = (AActor*)UGameplayStatics::FinishSpawningActor(
		UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn, Owner),
		Transform
	);

	if (Duplicated)
	{
		//FName DupedTag = UKismetStringLibrary::Conv_StringToName(L"_ServerSpawned_");

		Duplicated->Name = Original->Name;
		Duplicated->Tags = Original->Tags;

		//Duplicated->Tags.Add(DupedTag);
	}

	FGameplayTag* OriginalFactionTag = (FGameplayTag*)((uintptr_t)Original + 0xB0);
	FGameplayTag* DuplicateFactionTag = (FGameplayTag*)((uintptr_t)Duplicated + 0xB0);

	if (OriginalFactionTag && DuplicateFactionTag)
	{
		//Log(OriginalFactionTag->TagName.ToString());
		*DuplicateFactionTag = *OriginalFactionTag;
	}

	return Duplicated;
}

inline bool AdjustPickupLocationForWater(FVector& Loc)
{
	static TArray<AActor*> WaterBodies;
	static bool bLoadedWaterBodies = false;

	if (!bLoadedWaterBodies)
	{
		UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), AFortWaterBodyActor::StaticClass(), &WaterBodies);
		bLoadedWaterBodies = true;
	}

	bool bFoundWater = false;
	FVector BestSurface{};
	float BestZDelta = FLT_MAX;

	for (int i = 0; i < WaterBodies.Num(); i++)
	{
		auto WaterBody = (AFortWaterBodyActor*)WaterBodies[i];
		if (!WaterBody)
			continue;

		FVector WaterPlaneLocation{};
		FVector WaterPlaneNormal{};
		FVector WaterSurfacePosition{};
		FVector WaterVelocity{};
		float WaterDepth = 0.f;
		int32 WaterBodyIdx = 0;
		WaterBody->GetWaterSurfaceInfo(Loc, &WaterPlaneLocation, &WaterPlaneNormal, &WaterSurfacePosition, &WaterDepth, &WaterBodyIdx, &WaterVelocity, true, true);

		float ZDelta = fabsf(WaterSurfacePosition.Z - Loc.Z);
		if (WaterDepth > 8.f && ZDelta < 4500.f && ZDelta < BestZDelta)
		{
			bFoundWater = true;
			BestSurface = WaterSurfacePosition;
			BestZDelta = ZDelta;
		}
	}

	if (bFoundWater)
	{
		Loc.Z = BestSurface.Z + 85.f;
		return true;
	}

	return false;
}

AFortPickupAthena* SpawnPickup(UFortItemDefinition* ItemDef, int OverrideCount, int LoadedAmmo, FVector Loc, EFortPickupSourceTypeFlag SourceType, EFortPickupSpawnSource Source, AFortPawn* Pawn = nullptr)
{
	const bool bStaticDrop = Source == EFortPickupSpawnSource::PlayerElimination || SourceType == EFortPickupSourceTypeFlag::Container || SourceType == EFortPickupSourceTypeFlag::FloorLoot;
	bool bSpawnedInWater = !bStaticDrop && AdjustPickupLocationForWater(Loc);
	auto SpawnedPickup = SpawnActor<AFortPickupAthena>(Loc, {}, nullptr, AFortPickupAthena::StaticClass(), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!SpawnedPickup)
		return nullptr;
	SpawnedPickup->bReplicates = true;
	SpawnedPickup->bRandomRotation = true;

	auto& PickupEntry = SpawnedPickup->PrimaryPickupItemEntry;
	PickupEntry.ItemDefinition = ItemDef;
	PickupEntry.Count = OverrideCount;
	PickupEntry.LoadedAmmo = LoadedAmmo;
	PickupEntry.ReplicationKey++;
	SpawnedPickup->OnRep_PrimaryPickupItemEntry();
	SpawnedPickup->PawnWhoDroppedPickup = Pawn;
	SpawnedPickup->PickupSourceTypeFlags = SourceType;
	SpawnedPickup->PickupSpawnSource = Source;
	FHitResult SpawnLocHit;

	SpawnedPickup->SetReplicateMovement(!bStaticDrop);
	if (!bStaticDrop)
	{
		SpawnedPickup->TossPickup(Loc, Pawn, -1, true, false, SourceType, Source);
		SpawnedPickup->K2_SetActorLocation(Loc, false, &SpawnLocHit, true);
		SpawnedPickup->MovementComponent = (UProjectileMovementComponent*)GetDefaultObject<UGameplayStatics>()->SpawnObject(UProjectileMovementComponent::StaticClass(), SpawnedPickup);
		if (SpawnedPickup->MovementComponent)
		{
			SpawnedPickup->MovementComponent->bReplicates = true;
			SpawnedPickup->MovementComponent->Buoyancy = bSpawnedInWater ? 10.0f : SpawnedPickup->MovementComponent->Buoyancy;
			SpawnedPickup->MovementComponent->ProjectileGravityScale = bSpawnedInWater ? 0.f : SpawnedPickup->MovementComponent->ProjectileGravityScale;
			if (bSpawnedInWater)
			{
				SpawnedPickup->MovementComponent->Velocity = FVector{};
				SpawnedPickup->MovementComponent->InitialSpeed = 0.f;
				SpawnedPickup->MovementComponent->MaxSpeed = 0.f;
				SpawnedPickup->K2_SetActorLocation(Loc, false, &SpawnLocHit, true);
			}
		}
	}
	else
	{
		SpawnedPickup->bTossedFromContainer = false;
		SpawnedPickup->bCombinePickupsWhenTossCompletes = false;
		SpawnedPickup->bServerStoppedSimulation = false;
		SpawnedPickup->bClientUseInterpolationOnly = false;
		SpawnedPickup->PickupLocationData.PickupTarget = nullptr;
		SpawnedPickup->PickupLocationData.CombineTarget = nullptr;
		SpawnedPickup->PickupLocationData.ItemOwner = Pawn;
		SpawnedPickup->PickupLocationData.LootInitialPosition.X = Loc.X;
		SpawnedPickup->PickupLocationData.LootInitialPosition.Y = Loc.Y;
		SpawnedPickup->PickupLocationData.LootInitialPosition.Z = Loc.Z;
		SpawnedPickup->PickupLocationData.LootFinalPosition.X = Loc.X;
		SpawnedPickup->PickupLocationData.LootFinalPosition.Y = Loc.Y;
		SpawnedPickup->PickupLocationData.LootFinalPosition.Z = Loc.Z;
		SpawnedPickup->PickupLocationData.FinalTossRestLocation.X = Loc.X;
		SpawnedPickup->PickupLocationData.FinalTossRestLocation.Y = Loc.Y;
		SpawnedPickup->PickupLocationData.FinalTossRestLocation.Z = Loc.Z;
		SpawnedPickup->PickupLocationData.FlyTime = 0.f;
		SpawnedPickup->PickupLocationData.StartDirection.X = 0.f;
		SpawnedPickup->PickupLocationData.StartDirection.Y = 0.f;
		SpawnedPickup->PickupLocationData.StartDirection.Z = 0.f;
		SpawnedPickup->PickupLocationData.TossState = EFortPickupTossState::AtRest;
		SpawnedPickup->PickupLocationData.bPlayPickupSound = false;
		SpawnedPickup->PickupLocationData.PickupGuid = PickupEntry.ItemGuid;
		SpawnedPickup->K2_SetActorLocation(Loc, false, &SpawnLocHit, true);
		SpawnedPickup->OnRep_PickupLocationData();
	}

	if (!bStaticDrop && SourceType == EFortPickupSourceTypeFlag::Container)
	{
		SpawnedPickup->bTossedFromContainer = true;
		SpawnedPickup->OnRep_TossedFromContainer();
	}

	SpawnedPickup->K2_SetActorLocation(Loc, false, &SpawnLocHit, true);
	SpawnedPickup->ForceNetUpdate();

	return SpawnedPickup;
}

std::map<AFortPickup*, float> PickupLifetimes;
AFortPickupAthena* SpawnStack(APlayerPawn_Athena_C* Pawn, UFortItemDefinition* Def, int Count, bool giveammo = false, int ammo = 0)
{
	auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

	FVector Loc = Pawn->K2_GetActorLocation();
	bool bSpawnedInWater = AdjustPickupLocationForWater(Loc);
	AFortPickupAthena* Pickup = SpawnActor<AFortPickupAthena>(Loc, {}, nullptr, AFortPickupAthena::StaticClass(), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Pickup)
		return nullptr;
	Pickup->bReplicates = true;
	PickupLifetimes[Pickup] = Statics->GetTimeSeconds(UWorld::GetWorld());
	Pickup->PawnWhoDroppedPickup = Pawn;
	Pickup->PrimaryPickupItemEntry.Count = Count;
	Pickup->PrimaryPickupItemEntry.ItemDefinition = Def;
	if (giveammo)
	{
		Pickup->PrimaryPickupItemEntry.LoadedAmmo = ammo;
	}
	Pickup->PrimaryPickupItemEntry.ReplicationKey++;

	Pickup->OnRep_PrimaryPickupItemEntry();
	Pickup->TossPickup(Loc, Pawn, 6, true, true, EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::Unset);
	FHitResult SpawnLocHit;
	Pickup->K2_SetActorLocation(Loc, false, &SpawnLocHit, true);

	Pickup->MovementComponent = (UProjectileMovementComponent*)Statics->SpawnObject(UProjectileMovementComponent::StaticClass(), Pickup);
	Pickup->MovementComponent->bReplicates = true;
	Pickup->MovementComponent->Buoyancy = bSpawnedInWater ? 10.0f : Pickup->MovementComponent->Buoyancy;
	Pickup->MovementComponent->ProjectileGravityScale = bSpawnedInWater ? 0.f : Pickup->MovementComponent->ProjectileGravityScale;
	if (bSpawnedInWater)
	{
		Pickup->MovementComponent->Velocity = FVector{};
		Pickup->MovementComponent->InitialSpeed = 0.f;
		Pickup->MovementComponent->MaxSpeed = 0.f;
		Pickup->K2_SetActorLocation(Loc, false, &SpawnLocHit, true);
	}
	((UProjectileMovementComponent*)Pickup->MovementComponent)->SetComponentTickEnabled(true);

	return Pickup;
}

static AFortPickupAthena* SpawnPickup(FFortItemEntry ItemEntry, FVector Location, EFortPickupSourceTypeFlag PickupSource = EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource SpawnSource = EFortPickupSpawnSource::Unset)
{
	auto Pickup = SpawnPickup(ItemEntry.ItemDefinition, ItemEntry.Count, ItemEntry.LoadedAmmo, Location, PickupSource, SpawnSource);
	return Pickup;
}

inline void ShowFoundation(ABuildingFoundation* BuildingFoundation) {
	if (!BuildingFoundation)
		return;

	FTransform FoundationTransform = BuildingFoundation->GetTransform();
	BuildingFoundation->bServerStreamedInLevel = true;
	BuildingFoundation->FoundationEnabledState = EDynamicFoundationEnabledState::Enabled;
	BuildingFoundation->DynamicFoundationRepData.Translation = FoundationTransform.Translation;
	BuildingFoundation->DynamicFoundationRepData.Rotation = FoundationTransform.Rotation;
	BuildingFoundation->DynamicFoundationRepData.EnabledState = EDynamicFoundationEnabledState::Enabled;
	BuildingFoundation->DynamicFoundationTransform = FoundationTransform;
	BuildingFoundation->SetDynamicFoundationEnabled(true);
	BuildingFoundation->OnRep_ServerStreamedInLevel();
	BuildingFoundation->OnRep_DynamicFoundationRepData();
	BuildingFoundation->OnRep_LevelToStream();
	BuildingFoundation->OnLevelStreamedIn();
	BuildingFoundation->OnLevelShown();
}

FVector PickSupplyDropLocation(SDK::AFortAthenaMapInfo* MapInfo, SDK::FVector Center, float Radius)
{
	if (!PickSupplyDropLocationOG)
		return SDK::FVector(0, 0, 0);

	const float MinDistance = 10000.0f;

	for (int i = 0; i < 20; i++)
	{
		SDK::FVector loc = FVector(0, 0, 0);
		PickSupplyDropLocationOG(MapInfo, &loc, (__int64)&Center, Radius);

		bool bTooClose = false;
		for (const auto& other : PickedSupplyDropLocations)
		{
			float dx = loc.X - other.X;
			float dy = loc.Y - other.Y;
			float dz = loc.Z - other.Z;

			float distSquared = dx * dx + dy * dy + dz * dz;

			if (distSquared < MinDistance * MinDistance)
			{
				bTooClose = true;
				break;
			}
		}

		if (!bTooClose)
		{
			PickedSupplyDropLocations.Add(loc);
			return loc;
		}
	}

	return SDK::FVector(0, 0, 0);
}

template<typename T>
inline std::vector<T*> GetAllObjectsOfClass(UClass* Class = T::StaticClass())
{
	std::vector<T*> Objects{};

	for (int i = 0; i < UObject::GObjects->Num(); ++i)
	{
		UObject* Object = UObject::GObjects->GetByIndex(i);

		if (!Object)
			continue;

		if (Object->GetFullName().contains("Default"))
			continue;

		if (Object->GetFullName().contains("Test"))
			continue;

		if (Object->IsA(Class) && !Object->IsDefaultObject())
		{
			Objects.push_back((T*)Object);
		}
	}

	return Objects;
}

inline bool IsUnsafeStartupLevelPath(const std::string& Path)
{
	return Path.contains("Hoagie")
		|| Path.contains("NeoTilted")
		|| Path.contains("Yacht")
		|| Path.contains("Apollo_Yacht")
		|| Path.contains("BridgeGirder_Trim")
		|| Path.contains("Apollo_Nav_Gameplay")
		|| Path.contains("Apollo_ItemCollect")
		|| Path.contains("ItemCollection")
		|| Path.contains("QuestInteractable")
		|| Path.contains("BlueprintMaterialTextureNodes")
		|| (!Globals::bEventEnabled && Path.contains("CycloneJerky"));
}

inline std::string SoftWorldPathToString(const TSoftObjectPtr<UWorld>& World)
{
	return UKismetStringLibrary::Conv_NameToString(World.ObjectID.AssetPathName).ToString();
}

inline void SanitizeLoadedLevelOverlayConfigs(const char* Reason)
{
	if (Globals::bEventEnabled)
		return;

	int RemovedOverlays = 0;
	int TouchedConfigs = 0;

	for (int i = 0; i < UObject::GObjects->Num(); ++i)
	{
		UObject* Object = UObject::GObjects->GetByIndex(i);
		if (!Object || Object->IsDefaultObject() || !Object->IsA(UFortLevelOverlayConfig::StaticClass()))
			continue;

		UFortLevelOverlayConfig* Config = (UFortLevelOverlayConfig*)Object;
		int OriginalCount = Config->OverlayList.Num();
		if (OriginalCount <= 0)
			continue;

		for (int32 OverlayIdx = Config->OverlayList.Num() - 1; OverlayIdx >= 0; OverlayIdx--)
		{
			std::string SourceWorld = SoftWorldPathToString(Config->OverlayList[OverlayIdx].SourceWorld);
			std::string OverlayWorld = SoftWorldPathToString(Config->OverlayList[OverlayIdx].OverlayWorld);

			if (Globals::Automatics || IsUnsafeStartupLevelPath(SourceWorld) || IsUnsafeStartupLevelPath(OverlayWorld))
			{
				if (!OverlayWorld.empty())
					Log("Skipping unsafe level overlay: " + OverlayWorld);
				Config->OverlayList.Remove(OverlayIdx);
				RemovedOverlays++;
			}
		}

		if (Config->OverlayList.Num() != OriginalCount)
			TouchedConfigs++;
	}

	if (RemovedOverlays > 0)
	{
		std::string Message = "Removed " + std::to_string(RemovedOverlays) + " startup level overlay entries from " + std::to_string(TouchedConfigs) + " config(s)";
		if (Reason && Reason[0])
			Message += std::string(" during ") + Reason;
		Message += ".";
		Log(Message);
	}
}

inline bool LevelOverlayConfigContainsUnsafePath(UFortLevelOverlayConfig* Config)
{
	if (!Config)
		return false;

	for (int32 OverlayIdx = 0; OverlayIdx < Config->OverlayList.Num(); OverlayIdx++)
	{
		std::string SourceWorld = SoftWorldPathToString(Config->OverlayList[OverlayIdx].SourceWorld);
		std::string OverlayWorld = SoftWorldPathToString(Config->OverlayList[OverlayIdx].OverlayWorld);

		if (IsUnsafeStartupLevelPath(SourceWorld) || IsUnsafeStartupLevelPath(OverlayWorld))
			return true;
	}

	return false;
}

inline void SanitizeLoadedGameFeatureLevelOverlayConfigs(const char* Reason)
{
	if (Globals::bEventEnabled)
		return;

	int DetachedConfigs = 0;

	for (int i = 0; i < UObject::GObjects->Num(); ++i)
	{
		UObject* Object = UObject::GObjects->GetByIndex(i);
		if (!Object || Object->IsDefaultObject() || !Object->IsA(UFortGameFeatureData::StaticClass()))
			continue;

		UFortGameFeatureData* FeatureData = (UFortGameFeatureData*)Object;
		UFortLevelOverlayConfig* Config = FeatureData->LevelOverlayConfig;
		if (!Config)
			continue;

		std::string FeatureName = FeatureData->GetFullName();
		std::string ConfigName = Config->GetFullName();
		bool ShouldDetach = Globals::Automatics
			|| IsUnsafeStartupLevelPath(FeatureName)
			|| IsUnsafeStartupLevelPath(ConfigName)
			|| LevelOverlayConfigContainsUnsafePath(Config);

		if (!ShouldDetach)
			continue;

		FeatureData->LevelOverlayConfig = nullptr;
		DetachedConfigs++;

		std::string Message = "Detached game feature level overlay config";
		if (Reason && Reason[0])
			Message += std::string(" during ") + Reason;
		Message += ": " + FeatureName;
		if (!ConfigName.empty())
			Message += " -> " + ConfigName;
		Message += ".";
		Log(Message);
	}

	if (DetachedConfigs > 0)
	{
		std::string Message = "Detached " + std::to_string(DetachedConfigs) + " game feature level overlay config(s)";
		if (Reason && Reason[0])
			Message += std::string(" during ") + Reason;
		Message += ".";
		Log(Message);
	}
}

inline void DisableFortLevelOverlayManager(const char* Reason)
{
	if (Globals::bEventEnabled)
		return;

	UWorld* World = UWorld::GetWorld();
	if (!World)
		return;

	AWorldSettings* WorldSettings = World->K2_GetWorldSettings();
	if (!WorldSettings || !WorldSettings->IsA(AFortWorldSettings::StaticClass()))
		return;

	AFortWorldSettings* FortWorldSettings = (AFortWorldSettings*)WorldSettings;
	if (!FortWorldSettings->LevelOverlayManager)
		return;

	std::string ManagerName = FortWorldSettings->LevelOverlayManager->GetFullName();
	FortWorldSettings->LevelOverlayManager = nullptr;

	std::string Message = "Disabled FortLevelOverlayManager";
	if (Reason && Reason[0])
		Message += std::string(" during ") + Reason;
	if (!ManagerName.empty())
		Message += ": " + ManagerName;
	Message += ".";
	Log(Message);
}

inline std::string StreamingLevelPathToString(ULevelStreaming* Level)
{
	if (!Level)
		return "";

	std::string Path = Level->GetFullName();

	std::string PackageName = Level->PackageNameToLoad.ToString();
	if (!PackageName.empty())
		Path += " " + PackageName;

	std::string WorldAsset = UKismetStringLibrary::Conv_NameToString(Level->WorldAsset.ObjectID.AssetPathName).ToString();
	if (!WorldAsset.empty())
		Path += " " + WorldAsset;

	FName WorldAssetPackage = Level->GetWorldAssetPackageFName();
	std::string WorldAssetPackageName = WorldAssetPackage.ToString();
	if (!WorldAssetPackageName.empty())
		Path += " " + WorldAssetPackageName;

	return Path;
}

inline bool IsUnsafeStreamingLevel(ULevelStreaming* Level)
{
	return IsUnsafeStartupLevelPath(StreamingLevelPathToString(Level));
}

inline void DisableStreamingLevel(ULevelStreaming* Level)
{
	if (!Level)
		return;

	Level->SetShouldBeVisible(false);
	Level->SetShouldBeLoaded(false);
	Level->SetIsRequestingUnloadAndRemoval(true);
	Level->bShouldBlockOnLoad = false;
	Level->bDisableDistanceStreaming = true;
	Level->LoadedLevel = nullptr;
}

inline int RemoveUnsafeStreamingLevelsFromArray(TArray<ULevelStreaming*>& Levels, const char* Reason)
{
	int Removed = 0;

	for (int32 i = Levels.Num() - 1; i >= 0; i--)
	{
		ULevelStreaming* Level = Levels[i];
		if (!IsUnsafeStreamingLevel(Level))
			continue;

		std::string LevelPath = StreamingLevelPathToString(Level);
		DisableStreamingLevel(Level);
		Levels.Remove(i);
		Removed++;

		std::string Message = "Removed unsafe streaming level";
		if (Reason && Reason[0])
			Message += std::string(" during ") + Reason;
		if (!LevelPath.empty())
			Message += ": " + LevelPath;
		Message += ".";
		Log(Message);
	}

	return Removed;
}

inline void SanitizeWorldStreamingLevels(const char* Reason)
{
	if (Globals::bEventEnabled)
		return;

	UWorld* World = UWorld::GetWorld();
	if (!World)
		return;

	int Removed = 0;
	Removed += RemoveUnsafeStreamingLevelsFromArray(World->StreamingLevels, Reason);
	Removed += RemoveUnsafeStreamingLevelsFromArray(World->StreamingLevelsToConsider.StreamingLevels, Reason);

	if (Removed > 0)
	{
		std::string Message = "Removed " + std::to_string(Removed) + " unsafe world streaming level reference(s)";
		if (Reason && Reason[0])
			Message += std::string(" during ") + Reason;
		Message += ".";
		Log(Message);
	}
}

inline bool IsUnsafePlacedActorName(const std::string& Name)
{
	// Never touch spawners - the Hoagie vehicle spawner is used intentionally
	// (see Vehicles.h), so any "Spawner" actor is always left alone.
	if (Name.contains("Spawner"))
		return false;

	return Name.contains("Yacht_Girder_Trim")
		|| Name.contains("Apollo_Yacht_Girder_Trim")
		|| Name.contains("BridgeGirder_Trim_E")
		|| Name.contains("NeoTilted_SpotLight")
		|| Name.contains("NeoTilted")
		|| Name.contains("Apollo_Yacht");
}

inline void DestroyUnsafePlacedActors(const char* Reason)
{
    if (Globals::bEventEnabled)
        return;

    UWorld* World = UWorld::GetWorld();
    if (!World)
        return;

    static UWorld* LastWorld = nullptr;
    static std::chrono::steady_clock::time_point NextScanTime{};
    static std::chrono::steady_clock::time_point WorldChangeTime{};
    static std::set<AActor*> NeutralizedActors;

    if (World != LastWorld)
    {
        LastWorld = World;
        WorldChangeTime = std::chrono::steady_clock::now();
        NextScanTime = std::chrono::steady_clock::now();
        NeutralizedActors.clear();
    }

    auto Now = std::chrono::steady_clock::now();
    if (Now < NextScanTime)
        return;

    auto ElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(Now - WorldChangeTime).count();
    NextScanTime = Now + std::chrono::milliseconds(ElapsedMs < 20000 ? 100 : 2000);

    TArray<AActor*> AllActors;
    UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(World, AActor::StaticClass(), &AllActors);

    // Do not destroy placed actors while the world is still coming up. K2_DestroyActor
    // runs full engine teardown and can re-enter hooks while streaming/map init is in
    // progress. Neutralizing the actor is enough to stop tick/nav/collision from
    // touching the broken content without risking teardown during startup.
    constexpr int MaxNeutralizesPerScan = 75;

    int Neutralized = 0;
    for (AActor* Actor : AllActors)
    {
        if (Neutralized >= MaxNeutralizesPerScan)
            break;

        if (!Actor || NeutralizedActors.count(Actor))
            continue;

        std::string Name = Actor->GetName();
        std::string FullName = Actor->GetFullName();
        std::string ClassName = Actor->Class ? Actor->Class->GetFullName() : "";

        if (!IsUnsafePlacedActorName(Name) && !IsUnsafePlacedActorName(FullName) && !IsUnsafePlacedActorName(ClassName))
            continue;

        // Disable everything so nav/tick/collision cannot touch broken content.
        Actor->SetActorTickEnabled(false);
        Actor->SetActorEnableCollision(false);
        Actor->SetActorHiddenInGame(true);
        NeutralizedActors.insert(Actor);
        Neutralized++;
    }

    AllActors.Free();

    if (Neutralized > 0)
    {
        std::string Message = "Neutralized " + std::to_string(Neutralized) + " unsafe actor(s)";
        if (Reason && Reason[0]) Message += std::string(" during ") + Reason;
        Message += ".";
        Log(Message);
    }
}

inline void GuardUnsafeStartupLevelStreaming(const char* Reason)
{
	SanitizeLoadedLevelOverlayConfigs(Reason);
	SanitizeLoadedGameFeatureLevelOverlayConfigs(Reason);
	DisableFortLevelOverlayManager(Reason);
	SanitizeWorldStreamingLevels(Reason);
}

int CountActorsWithName(FName TargetName, UClass* Class)
{
	TArray<AActor*> FoundActors;
	auto* Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;
	Statics->GetAllActorsOfClass(UWorld::GetWorld(), Class, &FoundActors);

	int Count = 0;
	for (AActor* Actor : FoundActors)
	{
		if (Actor && Actor->GetName() == TargetName.ToString())
			Count++;
	}
	return Count;
}

AFortPlayerControllerAthena* GetPCFromId(FUniqueNetIdRepl& ID)
{
	for (auto& PlayerState : UWorld::GetWorld()->GameState->PlayerArray)
	{
		auto PlayerStateAthena = Cast<AFortPlayerStateAthena>(PlayerState);
		if (!PlayerStateAthena)
			continue;
		if (PlayerStateAthena->AreUniqueIDsIdentical(ID, PlayerState->UniqueId))
			return Cast<AFortPlayerControllerAthena>(PlayerState->Owner);
	}

	return nullptr;
}

enum class EAccoladeEvent : uint8
{
	Kill,
	Search,
	MAX
};

inline UFortAccoladeItemDefinition* GetDefFromEvent(EAccoladeEvent Event, int Count, UObject* Object = nullptr)
{
	UFortAccoladeItemDefinition* Def = nullptr;

	switch (Event)
	{
	case EAccoladeEvent::Kill:
		if (Count == 1)
		{
			Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_014_Elimination_Bronze.AccoladeId_014_Elimination_Bronze");
		}
		else if (Count == 4)
		{
			Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_015_Elimination_Silver.AccoladeId_015_Elimination_Silver");
		}
		else if (Count == 8)
		{
			Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_016_Elimination_Gold.AccoladeId_016_Elimination_Gold");
		}
		break;
	case EAccoladeEvent::Search:
		if (Count == 3)
		{
			Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_008_SearchChests_Bronze.AccoladeId_008_SearchChests_Bronze");
		}
		else if (Count == 7)
		{
			Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_009_SearchChests_Silver.AccoladeId_009_SearchChests_Silver");
		}
		else if (Count == 12)
		{
			Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_010_SearchChests_Gold.AccoladeId_010_SearchChests_Gold");
		}
		break;
	case EAccoladeEvent::MAX:
		break;
	default:
		break;
	}

	return Def;
}

namespace EBTExecutionMode
{
	enum Type
	{
		SingleRun,
		Looped,
	};
}

namespace EBTActiveNode
{
	enum Type
	{
		Composite,
		ActiveTask,
		AbortingTask,
		InactiveTask,
	};
}

namespace EBTTaskStatus
{
	enum Type
	{
		Active,
		Aborting,
		Inactive,
	};
}
