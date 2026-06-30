#pragma once
#include "framework.h"
#include "BotNames.h"
#include "POI_Locs.h"

#include "Quests.h"
#include "Misc.h"

static std::vector<UAthenaCharacterItemDefinition*> CIDs{};
static std::vector<UAthenaPickaxeItemDefinition*> Pickaxes{};
static std::vector<UAthenaBackpackItemDefinition*> Backpacks{};
static std::vector<UAthenaGliderItemDefinition*> Gliders{};
static std::vector<UAthenaSkyDiveContrailItemDefinition*> Contrails{};
inline std::vector<UAthenaDanceItemDefinition*> Dances{};

static std::vector<const char*> KnownGoodCIDPaths = {
    "/Game/Athena/Items/Cosmetics/Characters/CID_556_Athena_Commando_F_RebirthDefaultA.CID_556_Athena_Commando_F_RebirthDefaultA",
    "/Game/Athena/Items/Cosmetics/Characters/CID_NPC_Athena_Commando_M_HenchmanBad.CID_NPC_Athena_Commando_M_HenchmanBad",
    "/Game/Athena/Items/Cosmetics/Characters/CID_NPC_Athena_Commando_M_HenchmanGood.CID_NPC_Athena_Commando_M_HenchmanGood",
};

static UAthenaCharacterItemDefinition* GetValidCID() {
    static std::vector<UAthenaCharacterItemDefinition*> LoadedCIDs{};
    static bool bLoadedKnownGoodCIDs = false;

    if (!bLoadedKnownGoodCIDs)
    {
        bLoadedKnownGoodCIDs = true;
        for (auto Path : KnownGoodCIDPaths)
        {
            UAthenaCharacterItemDefinition* CID = StaticLoadObject<UAthenaCharacterItemDefinition>(Path);
            if (CID && CID->HeroDefinition)
                LoadedCIDs.push_back(CID);
        }
    }

    if (LoadedCIDs.empty()) {
        return nullptr;
    }
    int idx = rand() % LoadedCIDs.size();
    return LoadedCIDs[idx];
}

enum class EBotState : uint8 {
    Warmup,
    PreBus,
    Bus,
    Skydiving,
    Gliding,
    Landed,
    Fleeing,
    Looting,
    LookingForPlayers,
    MovingToSafeZone,
    Stuck
};

enum class BotAction : uint8 {
    PUSH_AGGRESSIVE = 0,
    RETREAT_HEAL = 1,
    BUILD_BOX = 2,
    BUILD_RAMP_RUSH = 3,
    BUILD_HIGHGROUND = 4,
    HOLD_SHOOT_AR = 5,
    EDIT_PEEK = 6,
    LOOT_NEARBY = 7,
    ROTATE_ZONE = 8,
    SPRAY_AND_PRAY = 9
};

enum class EBotWarmupChoice {
    Emote,
    MoveToPlayerEmote,
    MAX
};

enum class EBotStrafeType {
    StrafeLeft,
    StrafeRight
};

enum class ELootableType {
    None = -1,
    Chest = 0,
    Pickup = 1
};

std::vector<class PlayerBot*> PlayerBotArray{};
static std::vector<AActor*> ReservedLootables{};

static bool IsLootableReserved(AActor* Lootable)
{
    if (!Lootable)
        return false;
    for (size_t i = 0; i < ReservedLootables.size(); i++)
    {
        if (ReservedLootables[i] == Lootable)
            return true;
    }
    return false;
}

static void ReserveLootable(AActor* Lootable)
{
    if (Lootable && !IsLootableReserved(Lootable))
        ReservedLootables.push_back(Lootable);
}

static void ReleaseLootable(AActor* Lootable)
{
    if (!Lootable)
        return;
    for (size_t i = 0; i < ReservedLootables.size();)
    {
        if (!ReservedLootables[i] || ReservedLootables[i] == Lootable)
            ReservedLootables.erase(ReservedLootables.begin() + i);
        else
            i++;
    }
}

struct DeathContext;
struct LearningStats;
struct CombatState;
struct PlayerBot;

// Set true once a usable nav mesh has been located and pushed onto the bots. On this server
// build the baked RecastNavMesh never streams in, so this stays false and the bots steer
// themselves with direct movement input instead of nav-mesh path following (the fix for bots
// "running in place" after they land, since MoveToActor/MoveToLocation do nothing with no nav).
inline bool g_BotsHaveNavData = false;

struct DeathContext {
    FString WeaponName;
    FVector DeathLocation;
    FVector KillerLocation;
    float Distance;
    EBotState BotStateAtDeath;
    bool WasBuilding;
    bool WasMoving;
    bool bDiedToStorm;
    float TimeAlive;
    FString HowDied;
};

struct LearningStats {
    int GamesPlayed = 0;
    int TimesDied = 0;
    int TimesKilledPlayer = 0;
    int TimesWon = 0;

    float AvgSurvivalTime = 0.f;
    float BestSurvivalTime = 0.f;

    int DeathsByRanged = 0;
    int DeathsByClose = 0;
    int DeathsByBuilding = 0;
    int DeathsByStorm = 0;
    int DeathsByFalling = 0;
    int DeathsWithoutHealing = 0;
    int DeathsWithoutShield = 0;

    float OptimalDistance = 1000.f;
    float PreferredBuildDistance = 500.f;

    bool bLearnedCrouchShoot = false;
    bool bLearnedStrafe = false;
    bool bLearnedBuilding = false;
    bool bLearnedRetreatWhenLow = false;
    bool bLearnedRushEnemy = false;
    bool bLearnedUseHealing = false;
    bool bLearnedUseShield = false;
    bool bLearnedRotateBuild = false;
    bool bLearnedPrioritizeZone = false;
    bool bLearnedAvoidStorm = false;

    float AvgHealthAtDeath = 50.f;
    float AvgShieldAtDeath = 0.f;

    int ConsecutiveStormDeaths = 0;
    int ConsecutiveDeathsSameWay = 0;
    int DeathsWhileFullHealth = 0;
    int DeathsWhileFullShield = 0;
    FString LastDeathReason;
};

struct CombatState {
    AActor* CurrentTarget = nullptr;
    float AimStartTime = 0.f;
    bool bIsAiming = false;
    bool bIsFiring = false;
    float FireEndTime = 0.f;
    float NextFireTime = 0.f;
};

static std::map<struct PlayerBot*, CombatState> CombatStates{};
static std::map<struct PlayerBot*, LearningStats> BotLearning{};
static int GlobalGamesPlayed = 0;

namespace CombatAI {
    void RecordDeath(struct PlayerBot* bot, const DeathContext& Context);
    FString AnalyzeDeathSituation(struct PlayerBot* bot, AActor* Killer);
    bool IsMinigun(AActor* Weapon);
    bool ShouldUseBurstFire(AActor* Weapon);
    float GetBurstDuration(AActor* Weapon);
    float GetBurstCooldown(AActor* Weapon);
    AActor* FindNearestEnemy(struct PlayerBot* bot);
    static bool CanShootAt(struct PlayerBot* bot, AActor* Target);
    static void Tick(struct PlayerBot* bot);
}

struct PlayerBot
{
public:
    // Additional State Enums : Seprated so its cleaner
    EBotWarmupChoice BotWarmupChoice = EBotWarmupChoice::MAX;

public:
    // So we can track the current tick that the bot is doing
    uint64_t tick_counter = 0;

    // The Pawn of the bot
    AFortPlayerPawnAthena* Pawn = nullptr;

    // The playercontroller of the bot
    ABP_PhoebePlayerController_C* PC = nullptr;

    // The playerstate of the bot
    AFortPlayerStateAthena* PlayerState = nullptr;

    // The current botstate, has different behaviours for different states
    EBotState BotState = EBotState::Warmup;

    // The cached botstate, usually if a botstate needs to revert back to the previous one
    EBotState CachedBotState = EBotState::Warmup;

    // The building that the bot collided with (doesent even works smd)
    ABuildingActor* StuckBuildingActor = nullptr;

    // Reservation of lootables, stops pileup and tracks current lootable
    AActor* TargetLootable = nullptr;

    // Just incase we have to do specific stuff based on the type of lootable it is
    ELootableType TargetLootableType = ELootableType::None;

    // The nearest player, bot, factionbot
    AActor* NearestPlayerActor = nullptr;

    // The displayname of the bot
    FString DisplayName = L"Bot";

    // Is the bot stressed, will be determined every tick.
    bool bIsStressed = false;

    // Is the bot dead, basically marking the bot to be removed from the playerbotarray later
    bool bIsDead = false;

    // Has the bot thanked the bus driver
    bool bHasThankedBusDriver = false;

    // Has the bot jumped from the bus, if not then set the botstate to bus.
    bool bHasJumpedFromBus = false;

    // The dropzone that the bot will attempt to land at
    FVector TargetDropZone = FVector();

    // The closest distance achieved to the targetdropzone, will be used mostly for determining bus drop
    float ClosestDistToDropZone = FLT_MAX;

    // Is the bot currently strafing (combat technique & Unstuck method)
    bool bIsCurrentlyStrafing = false;

    // The strafe type used by the bot, determines what direction
    EBotStrafeType StrafeType = EBotStrafeType::StrafeLeft;

    // When should the current strafe end?
    float StrafeEndTime = 0.0f;

    // General purpose timer
    float TimeToNextAction = 0.f;

    // Prevents bots from spam-building every tick while under pressure.
    float NextBuildTime = 0.f;

    // Separate cooldown for non-combat traversal builds.
    float NextTravelBuildTime = 0.f;

    float NextBuildFailureResetTime = 0.f;

    int ConsecutiveBuildFailures = 0;

    float NextDoorOpenTime = 0.f;

    float NextHealTime = 0.f;

    // The start time of this current lootable
    float LootTargetStartTime = 0.f;

    // The distance between the bot and the lootable
    float LastLootTargetDistance = 0.f;

    float NextLootScanTime = 0.f;

    FVector RoamTarget = FVector();
    float NextRoamTime = 0.f;

    FVector PendingChestLootLocation = FVector();
    float PendingChestLootCollectUntil = 0.f;
    float NextChestLootCollectTime = 0.f;

    float NextMoveCommandTime = 0.f;

    float StuckStartTime = 0.f;

    float NextObstacleScanTime = 0.f;
    float NextFullAITickTime = 0.f;
    float NextTargetScanTime = 0.f;
    float NextUtilityActionTime = 0.f;
    float NextFocusClearTime = 0.f;
    float NextCrouchActionTime = 0.f;
    float NextRunCommandTime = 0.f;
    float NextDropCheckTime = 0.f;
    FVector LastPositionSample = FVector();
    float NextPositionSampleTime = 0.f;
    int ConsecutiveWallStuckTicks = 0;

    FVector LastKnownEnemyLocation = FVector();
    float LastKnownEnemyTime = 0.f;

    FVector LastKnownEnemyPos = FVector();
    float LastEnemySeenTime = 0.f;
    int ConsecutiveBuilds = 0;
    float LastActionTime = 0.f;
    BotAction CurrentAction = BotAction::LOOT_NEARBY;
    BotAction LastAction = BotAction::LOOT_NEARBY;
    float ActionCommitTimer = 0.f;
    int EditPeekCount = 0;
    bool IsBoxed = false;
    float NextBotBrainTickTime = 0.f;
    float LastBotBrainThinkTime = 0.f;
    float LastBotBrainDistToEnemy = 999999.f;
    bool bLastBotBrainEnemyVisible = false;
    float LastHealthShieldSample = 0.f;
    float LastDamageTime = 0.f;
    float LastWeaponSwitchTime = 0.f;
    AActor* BrainTarget = nullptr;

public:
    void OnDied(AFortPlayerStateAthena* KillerState, AActor* DamageCauser, FName BoneName)
    {
        static auto Acc_DistanceShot = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_DistanceShot.AccoladeId_DistanceShot");
        static auto Acc_LongShot = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_051_LongShot.AccoladeId_051_LongShot");
        static auto Acc_LudicrousShot = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_052_LudicrousShot.AccoladeId_052_LudicrousShot");
        static auto Acc_ImpossibleShot = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_053_ImpossibleShot.AccoladeId_053_ImpossibleShot");
        static auto Acc_Headshot = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_048_HeadshotElim.AccoladeId_048_HeadshotElim");

        static auto Acc_Survival_Bronze = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_026_Survival_Default_Bronze.AccoladeId_026_Survival_Default_Bronze");
        static auto Acc_Survival_Silver = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_027_Survival_Default_Silver.AccoladeId_027_Survival_Default_Silver");
        static auto Acc_Survival_Gold = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_028_Survival_Default_Gold.AccoladeId_028_Survival_Default_Gold");

        bIsDead = true;

        bool bIsStormDeath = false;

        if (!DamageCauser || !PlayerState || !Pawn) {
            bIsStormDeath = true;
        }

        if (DamageCauser && !KillerState) {
            std::string CauserName = DamageCauser->GetName();
            if (CauserName.find("Storm") != std::string::npos ||
                CauserName.find("Zone") != std::string::npos ||
                CauserName.find("SafeZone") != std::string::npos) {
                bIsStormDeath = true;
            }
        }

        if (!KillerState && !bIsStormDeath) {
            bIsStormDeath = true;
        }

        if (!KillerState || !DamageCauser || !PlayerState || !Pawn) {
            DeathContext Context{};
            Context.DeathLocation = Pawn ? Pawn->K2_GetActorLocation() : FVector();
            Context.BotStateAtDeath = BotState;
            Context.Distance = 0.f;
            Context.TimeAlive = 0.f;
            Context.bDiedToStorm = bIsStormDeath;
            Context.HowDied = bIsStormDeath ? L"Storm" : L"Unknown";

            if (Pawn) {
                CombatAI::RecordDeath(this, Context);
            }
            return;
        }

        FDeathInfo& DeathInfo = PlayerState->DeathInfo;

        DeathInfo.bDBNO = Pawn->bWasDBNOOnDeath;
        DeathInfo.DeathLocation = Pawn->K2_GetActorLocation();
        DeathInfo.DeathTags = Pawn->DeathTags;
        DeathInfo.Downer = KillerState ? KillerState : nullptr;
        AFortPawn* KillerPawn = KillerState ? KillerState->GetCurrentPawn() : nullptr;
        DeathInfo.Distance = (KillerPawn && Pawn) ? KillerPawn->GetDistanceTo(Pawn) : 0.f;
        DeathInfo.FinisherOrDowner = KillerState ? KillerState : nullptr;
        DeathInfo.DeathCause = PlayerState->ToDeathCause(DeathInfo.DeathTags, DeathInfo.bDBNO);
        DeathInfo.bInitialized = true;
        PlayerState->OnRep_DeathInfo();

        DeathContext Context{};
        Context.DeathLocation = Pawn->K2_GetActorLocation();
        Context.BotStateAtDeath = BotState;
        Context.Distance = DeathInfo.Distance;
        Context.TimeAlive = 0.f;
        Context.bDiedToStorm = bIsStormDeath;
        Context.HowDied = bIsStormDeath ? L"Storm" : CombatAI::AnalyzeDeathSituation(this, KillerPawn);

        CombatAI::RecordDeath(this, Context);

        bool bIsKillerABot = KillerState->bIsABot;

        if (!PC->Inventory)
            return;

        auto KillerPC = (AFortPlayerControllerAthena*)KillerState->GetOwner();

        if (KillerPC && KillerPC->IsA(AFortPlayerControllerAthena::StaticClass()) && !bIsKillerABot)
        {
            KillerState->KillScore++;

            Quests::GiveAccolade(KillerPC, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_012_Elimination.AccoladeId_012_Elimination"));
            Quests::GiveAccolade(KillerPC, GetDefFromEvent(EAccoladeEvent::Kill, KillerState->KillScore));

            // giving assist accolade cuz idfk how to track assists
            Quests::GiveAccolade((AFortPlayerControllerAthena*)KillerState->Owner, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_013_Assist.AccoladeId_013_Assist"));

            for (size_t i = 0; i < KillerState->PlayerTeam->TeamMembers.Num(); i++)
            {
                ((AFortPlayerStateAthena*)KillerState->PlayerTeam->TeamMembers[i]->PlayerState)->TeamKillScore++;
                ((AFortPlayerStateAthena*)KillerState->PlayerTeam->TeamMembers[i]->PlayerState)->OnRep_TeamKillScore();
            }

            KillerState->ClientReportKill(PlayerState);
            KillerState->ClientReportTeamKill(KillerState->KillScore); 
            KillerState->OnRep_Kills();

            /*auto KillerPawn = KillerPC->MyFortPawn;

            if (GameState->PlayersLeft && GameState->PlayerBotsLeft <= 1 && !GameState->IsRespawningAllowed(PlayerState)) // This crashes it lol
            {
                if (KillerPawn != Pawn)
                {
                    UFortWeaponItemDefinition* KillerWeaponDef = nullptr;

                    if (auto ProjectileBase = Cast<AFortProjectileBase>(DamageCauser))
                        KillerWeaponDef = ((AFortWeapon*)ProjectileBase->GetOwner())->WeaponData;
                    if (auto Weapon = Cast<AFortWeapon>(DamageCauser))
                        KillerWeaponDef = Weapon->WeaponData;

                    KillerPC->PlayWinEffects(KillerPawn, KillerWeaponDef, DeathInfo.DeathCause, false);
                    KillerPC->ClientNotifyWon(KillerPawn, KillerWeaponDef, DeathInfo.DeathCause);
                }

                KillerState->Place = 1;
                KillerState->OnRep_Place();
                GameState->WinningPlayerState = KillerState;
                GameState->WinningTeam = KillerState->TeamIndex;
                GameState->OnRep_WinningPlayerState();
                GameState->OnRep_WinningTeam();
                GameMode->EndMatch();
            }*/
        }

        if (bIsKillerABot) {
            return;
        }

        if (!bFirstElimTriggered) {
            Quests::GiveAccolade(KillerPC, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_017_First_Elimination.AccoladeId_017_First_Elimination"));
            bFirstElimTriggered = true;
        }

        float Distance = DeathInfo.Distance / 100.0f;

        if (Distance >= 100.0f && Distance < 150.0f)
        {
            Quests::GiveAccolade(KillerPC, Acc_DistanceShot); // 100-149m accolade
        }
        else if (Distance >= 150.0f && Distance < 200.0f)
        {
            Quests::GiveAccolade(KillerPC, Acc_LongShot); // 150-199m accolade
        }
        else if (Distance >= 200.0f && Distance < 250.0f)
        {
            Quests::GiveAccolade(KillerPC, Acc_LudicrousShot); // 200-249m accolade
        }
        else if (Distance >= 250.0f)
        {
            Quests::GiveAccolade(KillerPC, Acc_ImpossibleShot); // 250+m accolade
        }

        if (BoneName.ToString() == "head")
        {
            KillerPC->HeadShots++;
            Quests::GiveAccolade(KillerPC, Acc_Headshot); // headshot accolade

            if (KillerPC->HeadShots == 4) {
                Quests::GiveAccolade(KillerPC, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_049_Headhunter.AccoladeId_049_Headhunter"));
            }
        }

        if (KillerPC) {
            float CurrentTime = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());

            if (UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) > KillerPC->LastKillTimeWindow) {
                KillerPC->KillsInKillTimeWindow = 0;
                KillerPC->LastRecordedKillsInKillTimeWindow = 0;
            }
            KillerPC->LastKillTimeWindow = CurrentTime + 10.f;
            KillerPC->KillsInKillTimeWindow++;
        }

    }

public:
    FRotator UprightRotation(FRotator Rot)
    {
        Rot.Pitch = 0.f;
        Rot.Roll = 0.f;
        return Rot;
    }

    void EnsurePawnUpright()
    {
        if (!Pawn)
            return;

        FRotator Rot = Pawn->K2_GetActorRotation();
        if (fabsf(Rot.Pitch) > 1.f || fabsf(Rot.Roll) > 1.f)
        {
            Rot.Pitch = 0.f;
            Rot.Roll = 0.f;
            Pawn->K2_SetActorRotation(Rot, true);
        }
    }

    void SetLookRotation(FRotator Rot, bool bRotateControllerActor = true)
    {
        if (!PC)
            return;

        PC->SetControlRotation(Rot);
        if (bRotateControllerActor)
            PC->K2_SetActorRotation(UprightRotation(Rot), true);
        EnsurePawnUpright();
    }

    bool ShouldScanObstacles(float Cooldown = 1.25f)
    {
        float Now = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());
        if (Now < NextObstacleScanTime)
            return false;

        NextObstacleScanTime = Now + Cooldown + (float(rand() % 60) * 0.01f);
        return true;
    }

    bool HasTimerElapsed(float& NextTime, float Cooldown, float RandomJitter = 0.25f)
    {
        float Now = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());
        if (NextTime <= 0.f)
        {
            float InitialSpread = Cooldown + (RandomJitter > 0.f ? RandomJitter : 0.f);
            NextTime = Now + ((float)(rand() % 1000) / 1000.f) * InitialSpread;
            return false;
        }

        if (Now < NextTime)
            return false;

        float Jitter = RandomJitter > 0.f ? (float(rand() % 1000) / 1000.f) * RandomJitter : 0.f;
        NextTime = Now + Cooldown + Jitter;
        return true;
    }

    bool ShouldRunFullAITick(float Cooldown = 0.16f)
    {
        float Now = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());
        if (NextFullAITickTime <= 0.f)
        {
            NextFullAITickTime = Now + ((float)(rand() % 1000) / 1000.f) * Cooldown;
            return false;
        }

        if (Now < NextFullAITickTime)
            return false;

        NextFullAITickTime = Now + Cooldown + (float(rand() % 12) * 0.01f);
        return true;
    }

    void SetStuck(AActor* OtherActor, FHitResult& Hit) {
        if (BotState < EBotState::Landed)
            return;

        if (BotState == EBotState::Stuck)
            return;

        if (BotState == EBotState::LookingForPlayers || (NearestPlayerActor && HasGun()))
            return;

        FVector Vel = Pawn ? Pawn->GetVelocity() : FVector();
        float Speed = sqrtf(Vel.X * Vel.X + Vel.Y * Vel.Y);
        if (Speed > 90.f)
            return;

        if (OtherActor && OtherActor->IsA(ABuildingSMActor::StaticClass()))
        {
            ABuildingSMActor* Actor = (ABuildingSMActor*)OtherActor;
            float Health = Actor->GetHealth();
            FFindFloorResult Res;
            float DistToActor = Pawn ? Pawn->GetDistanceTo(Actor) : FLT_MAX;

            if (Actor->bCanBeDamaged == 1 && Health > 1 && Health < 1200 && DistToActor < 240.f)
            {
                Pawn->CharacterMovement->K2_FindFloor(Pawn->CapsuleComponent->K2_GetComponentLocation(), &Res);
                if (Res.HitResult.Actor.Get() == OtherActor)
                    return;
                StuckBuildingActor = Actor;
            }
        }

        if (!StuckBuildingActor)
            return;

        CachedBotState = BotState;
        BotState = EBotState::Stuck;
        StuckStartTime = 0.f;
    }

public:
    // Give an item to the bot
    void GiveItemBot(UFortItemDefinition* Def, int Count = 1, int LoadedAmmo = 0)
    {
        if (!Def) {
            return;
        }

        UFortWorldItem* Item = (UFortWorldItem*)Def->CreateTemporaryItemInstanceBP(Count, 0);
        Item->OwnerInventory = PC->Inventory;
        Item->ItemEntry.LoadedAmmo = LoadedAmmo;
        PC->Inventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
        PC->Inventory->Inventory.ItemInstances.Add(Item);
        PC->Inventory->Inventory.MarkItemDirty(Item->ItemEntry);
        PC->Inventory->HandleInventoryLocalUpdate();
    }

    FFortItemEntry* GetEntry(UFortItemDefinition* Def)
    {
        for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
        {
            if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition == Def)
                return &PC->Inventory->Inventory.ReplicatedEntries[i];
        }

        return nullptr;
    }

    void Emote()
    {
        auto EmoteDef = Dances[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Dances.size() - 1)];
        if (!EmoteDef)
            return;

        static UClass* EmoteAbilityClass = StaticLoadObject<UClass>("/Game/Abilities/Emotes/GAB_Emote_Generic.GAB_Emote_Generic_C");

        FGameplayAbilitySpec Spec{};
        AbilitySpecConstructor(&Spec, reinterpret_cast<UGameplayAbility*>(EmoteAbilityClass->DefaultObject), 1, -1, EmoteDef);
        GiveAbilityAndActivateOnce(reinterpret_cast<AFortPlayerStateAthena*>(PC->PlayerState)->AbilitySystemComponent, &Spec.Handle, Spec);
    }

    void Run()
    {
        if (!Pawn || !Pawn->CharacterMovement)
            return;
        if (Pawn->CharacterMovement->MaxWalkSpeed < 560.f || Pawn->CharacterMovement->MaxWalkSpeed > 585.f)
            Pawn->CharacterMovement->MaxWalkSpeed = 575.f;
    }

    void Walk()
    {
        if (!Pawn || !Pawn->CharacterMovement)
            return;
        if (Pawn->CharacterMovement->MaxWalkSpeed > 310.f || Pawn->CharacterMovement->MaxWalkSpeed < 275.f)
            Pawn->CharacterMovement->MaxWalkSpeed = 300.f;
    }

    void ForceStrafe(bool override) {
        if (!bIsCurrentlyStrafing && override)
        {
            bIsCurrentlyStrafing = true;
            if (UKismetMathLibrary::RandomBool()) {
                StrafeType = EBotStrafeType::StrafeLeft;
            }
            else {
                StrafeType = EBotStrafeType::StrafeRight;
            }
            StrafeEndTime = Statics->GetTimeSeconds(UWorld::GetWorld()) + Math->RandomFloatInRange(2.0f, 5.0f);
        }
        else
        {
            if (Statics->GetTimeSeconds(UWorld::GetWorld()) < StrafeEndTime)
            {
                if (StrafeType == EBotStrafeType::StrafeLeft) {
                    Pawn->AddMovementInput((Pawn->GetActorRightVector() * -1.0f), 0.65f, true);
                }
                else {
                    Pawn->AddMovementInput(Pawn->GetActorRightVector(), 0.65f, true);
                }
            }
            else
            {
                bIsCurrentlyStrafing = false;
            }
        }
    }

    void LookAt(AActor* Actor)
    {
        if (!Pawn || PC->GetFocusActor() == Actor)
            return;

        if (!Actor)
        {
            PC->K2_ClearFocus();
            return;
        }

        PC->K2_SetFocus(Actor);
    }

    bool IsPickaxeEquiped() {
        if (!Pawn || !Pawn->CurrentWeapon)
            return false;

        if (Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
        {
            return true;
        }
        return false;
    }

    bool HasGun()
    {
        if (!PC || !PC->Inventory)
            return false;

        for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
        {
            auto& Entry = PC->Inventory->Inventory.ReplicatedEntries[i];
            if (Entry.ItemDefinition) {
                std::string ItemName = Entry.ItemDefinition->Name.ToString();
                if (ItemName.contains("Shotgun") || ItemName.contains("SMG") || ItemName.contains("Assault")
                    || ItemName.contains("Sniper") || ItemName.contains("Rocket") || ItemName.contains("Pistol")) {
                    return true;
                    break;
                }
            }
        }
        return false;
    }

    void EquipPickaxe()
    {
        if (!Pawn || !Pawn->CurrentWeapon || !Pawn->CurrentWeapon->WeaponData || !PC || !PC->Inventory)
            return;

        if (!Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
        {
            for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
            {
                auto& Entry = PC->Inventory->Inventory.ReplicatedEntries[i];
                if (Entry.ItemDefinition && Entry.ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) &&
                    (Entry.ItemGuid.A != 0 || Entry.ItemGuid.B != 0 || Entry.ItemGuid.C != 0 || Entry.ItemGuid.D != 0))
                {
                    if (Entry.ItemGuid.A != 0 || Entry.ItemGuid.B != 0 || Entry.ItemGuid.C != 0 || Entry.ItemGuid.D != 0)
                        Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Entry.ItemDefinition, Entry.ItemGuid);
                    break;
                }
            }
        }
    }

    UFortItemDefinition* FindConsumable() {
        for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++) {
            auto& Entry = PC->Inventory->Inventory.ReplicatedEntries[i];
            if (Entry.ItemDefinition) {
                std::string ItemName = Entry.ItemDefinition->Name.ToString();
                if (ItemName.contains("Bandage") || ItemName.contains("Medkit") ||
                    ItemName.contains("SmallShield") || ItemName.contains("Slurp") ||
                    ItemName.contains("Healing") || ItemName.contains("Flop")) {
                    if (Entry.Count > 0) {
                        return Entry.ItemDefinition;
                    }
                }
            }
        }
        return nullptr;
    }

    UFortItemDefinition* FindShieldItem() {
        for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++) {
            auto& Entry = PC->Inventory->Inventory.ReplicatedEntries[i];
            if (Entry.ItemDefinition) {
                std::string ItemName = Entry.ItemDefinition->Name.ToString();
                if (ItemName.contains("SmallShield") || ItemName.contains("Shield") ||
                    ItemName.contains("Slurp") || ItemName.contains("Spicy") ||
                    ItemName.contains("Mushroom")) {
                    if (Entry.Count > 0) {
                        return Entry.ItemDefinition;
                    }
                }
            }
        }
        return nullptr;
    }

    void SimpleSwitchToWeapon() {
        if (!HasGun()) {
            return;
        }

        if (!Pawn || !Pawn->CurrentWeapon || !Pawn->CurrentWeapon->WeaponData || !PC || !PC->Inventory || bIsDead)
            return;

        if (!Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass())) {
            return;
        }

        if (Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
        {
            for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
            {
                auto& Entry = PC->Inventory->Inventory.ReplicatedEntries[i];
                if (Entry.ItemDefinition) {
                    std::string ItemName = Entry.ItemDefinition->Name.ToString();
                    if (ItemName.contains("Shotgun") || ItemName.contains("SMG") || ItemName.contains("Assault")
                        || ItemName.contains("Grenade") || ItemName.contains("Sniper") || ItemName.contains("Rocket") || ItemName.contains("Pistol")) {
                        if (Entry.ItemGuid.A != 0 || Entry.ItemGuid.B != 0 || Entry.ItemGuid.C != 0 || Entry.ItemGuid.D != 0)
                            Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Entry.ItemDefinition, Entry.ItemGuid);
                        break;
                    }
                }
            }
        }
    }

    // I will add more stuff to this over time based on what the bot needs/dont needs
    bool ShouldPickup(AFortPickup* Pickup) {
        if (!Pickup) {
            return false;
        }
        auto Def = Pickup->PrimaryPickupItemEntry.ItemDefinition;
        if (!Def) return false;

        std::string ItemName = Def->Name.ToString();

        // Dont bother with ammo cuz bots dont use it and bots usually pickup stuff in range anyway
        if (Def->IsA(UFortAmmoItemDefinition::StaticClass()))
            return false;

        return true;
    }

    void Pickup(AFortPickup* Pickup) {
        if (!Pickup)
            return;

        GiveItemBot(Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo);

        Pickup->PickupLocationData.bPlayPickupSound = true;
        Pickup->PickupLocationData.FlyTime = 0.3f;
        Pickup->PickupLocationData.ItemOwner = Pawn;
        Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
        Pickup->PickupLocationData.PickupTarget = Pawn;
        Pickup->OnRep_PickupLocationData();

        Pickup->bPickedUp = true;
        Pickup->OnRep_bPickedUp();
    }

    void PickupAllItemsInRange(float Range = 320.f) {
        if (!Pawn || !PC) {
            return;
        }

        static auto PickupClass = AFortPickupAthena::StaticClass();
        TArray<AActor*> Array;
        UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), PickupClass, &Array);

        for (size_t i = 0; i < Array.Num(); i++)
        {
            if (!Array[i] || Array[i]->bHidden)
                continue;

            if (!ShouldPickup((AFortPickupAthena*)Array[i])) {
                continue;
            }

            if (Array[i]->GetDistanceTo(Pawn) < Range)
            {
                Pickup((AFortPickupAthena*)Array[i]);
            }
        }

        Array.Free();
    }

    ABuildingActor* FindNearestChest()
    {
        static auto ChestClass = StaticLoadObject<UClass>("/Game/Building/ActorBlueprints/Containers/Tiered_Chest_Athena.Tiered_Chest_Athena_C");
        static auto FactionChestClass = StaticLoadObject<UClass>("/Game/Building/ActorBlueprints/Containers/Tiered_Chest_Athena_FactionChest.Tiered_Chest_Athena_FactionChest_C");
        TArray<AActor*> Array;
        TArray<AActor*> FactionChests;
        UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), ChestClass, &Array);
        UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), FactionChestClass, &FactionChests);

        AActor* NearestChest = nullptr;

        for (size_t i = 0; i < Array.Num(); i++)
        {
            AActor* Chest = Array[i];
            if (Chest->bHidden && Chest != TargetLootable)
                continue;
            if (IsLootableReserved(Chest) && Chest != TargetLootable)
                continue;
            ABuildingContainer* Container = Cast<ABuildingContainer>(Chest);
            if (Container)
                if (Container->bAlreadySearched && Chest != TargetLootable)
                    continue;

            if (!NearestChest || Chest->GetDistanceTo(Pawn) < NearestChest->GetDistanceTo(Pawn))
            {
                NearestChest = Array[i];
            }
        }

        for (size_t i = 0; i < FactionChests.Num(); i++)
        {
            AActor* Chest = FactionChests[i];
            if (Chest->bHidden && Chest != TargetLootable)
                continue;
            if (IsLootableReserved(Chest) && Chest != TargetLootable)
                continue;
            ABuildingContainer* Container = Cast<ABuildingContainer>(Chest);
            if (Container)
                if (Container->bAlreadySearched && Chest != TargetLootable)
                    continue;

            if (!NearestChest || Chest->GetDistanceTo(Pawn) < NearestChest->GetDistanceTo(Pawn))
            {
                NearestChest = FactionChests[i];
            }
        }
        Array.Free();
        FactionChests.Free();
        return (ABuildingActor*)NearestChest;
    }

    AFortPickupAthena* FindNearestPickup()
    {
        static auto PickupClass = AFortPickupAthena::StaticClass();
        TArray<AActor*> Array;
        UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), PickupClass, &Array);
        AActor* NearestPickup = nullptr;

        for (size_t i = 0; i < Array.Num(); i++)
        {
            if (Array[i]->bHidden && Array[i] != TargetLootable)
                continue;
            if (IsLootableReserved(Array[i]) && Array[i] != TargetLootable)
                continue;

            if (!ShouldPickup((AFortPickupAthena*)Array[i])) {
                continue;
            }

            if (!NearestPickup || Array[i]->GetDistanceTo(Pawn) < NearestPickup->GetDistanceTo(Pawn))
            {
                NearestPickup = Array[i];
            }
        }

        Array.Free();
        return (AFortPickupAthena*)NearestPickup;
    }

    bool GetNearestLootable() {
        static auto ChestClass = StaticLoadObject<UClass>("/Game/Building/ActorBlueprints/Containers/Tiered_Chest_Athena.Tiered_Chest_Athena_C");
        static auto FactionChestClass = StaticLoadObject<UClass>("/Game/Building/ActorBlueprints/Containers/Tiered_Chest_Athena_FactionChest.Tiered_Chest_Athena_FactionChest_C");
        TArray<AActor*> ChestArray;
        TArray<AActor*> FactionChests;
        UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), ChestClass, &ChestArray);
        UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), FactionChestClass, &FactionChests);

        static auto PickupClass = AFortPickupAthena::StaticClass();
        TArray<AActor*> PickupArray;
        UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), PickupClass, &PickupArray);

        AActor* NearestPickup = nullptr;
        AActor* NearestChest = nullptr;

        for (size_t i = 0; i < ChestArray.Num(); i++)
        {
            AActor* Chest = ChestArray[i];
            if (Chest->bHidden && Chest != TargetLootable)
                continue;
            if (IsLootableReserved(Chest) && Chest != TargetLootable)
                continue;
            ABuildingContainer* Container = Cast<ABuildingContainer>(Chest);
            if (Container)
                if (Container->bAlreadySearched && Chest != TargetLootable)
                    continue;

            if (!NearestChest || Chest->GetDistanceTo(Pawn) < NearestChest->GetDistanceTo(Pawn))
            {
                NearestChest = ChestArray[i];
            }
        }

        for (size_t i = 0; i < FactionChests.Num(); i++)
        {
            AActor* Chest = FactionChests[i];
            if (Chest->bHidden && Chest != TargetLootable)
                continue;
            if (IsLootableReserved(Chest) && Chest != TargetLootable)
                continue;
            ABuildingContainer* Container = Cast<ABuildingContainer>(Chest);
            if (Container)
                if (Container->bAlreadySearched && Chest != TargetLootable)
                    continue;

            if (!NearestChest || Chest->GetDistanceTo(Pawn) < NearestChest->GetDistanceTo(Pawn))
            {
                NearestChest = FactionChests[i];
            }
        }

        for (size_t i = 0; i < PickupArray.Num(); i++)
        {
            if (PickupArray[i]->bHidden && PickupArray[i] != TargetLootable)
                continue;
            if (IsLootableReserved(PickupArray[i]) && PickupArray[i] != TargetLootable)
                continue;

            if (!ShouldPickup((AFortPickupAthena*)PickupArray[i])) {
                continue;
            }

            if (!NearestPickup || PickupArray[i]->GetDistanceTo(Pawn) < NearestPickup->GetDistanceTo(Pawn))
            {
                NearestPickup = PickupArray[i];
            }
        }

        bool bChestCloser = false;
        if (NearestChest && NearestPickup)
            bChestCloser = NearestPickup->GetDistanceTo(Pawn) > NearestChest->GetDistanceTo(Pawn);
        else
            bChestCloser = NearestChest != nullptr;

        ChestArray.Free();
        FactionChests.Free();
        PickupArray.Free();
        return bChestCloser;
    }

    ABuildingSMActor* FindNearestBuildingSMActor()
    {
        static TArray<AActor*> Array;
        static bool PopulatedArray = false;
        if (!PopulatedArray)
        {
            PopulatedArray = true;
            UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), ABuildingSMActor::StaticClass(), &Array);
        }

        AActor* NearestActor = nullptr;

        for (size_t i = 0; i < Array.Num(); i++)
        {
            if (!NearestActor || (((ABuildingSMActor*)NearestActor)->GetHealth() < 1500 && ((ABuildingSMActor*)NearestActor)->GetHealth() > 1 && Array[i]->GetDistanceTo(Pawn) < NearestActor->GetDistanceTo(Pawn)))
            {
                NearestActor = Array[i];
            }
        }

        return (ABuildingSMActor*)NearestActor;
    }

    FVector FindNearestPlayerOrBot() {
        auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

        AActor* NearestPlayer = nullptr;

        for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
        {
            auto AlivePC = GameMode->AlivePlayers[i];
            if (!AlivePC || !AlivePC->Pawn || !AlivePC->PlayerState)
                continue;

            if (!NearestPlayer || AlivePC->Pawn->GetDistanceTo(Pawn) < NearestPlayer->GetDistanceTo(Pawn))
            {
                AFortPlayerStateAthena* PS = (AFortPlayerStateAthena*)AlivePC->PlayerState;
                if (PS->TeamIndex != PlayerState->TeamIndex) {
                    NearestPlayer = AlivePC->Pawn;
                }
            }
        }

        for (size_t i = 0; i < GameMode->AliveBots.Num(); i++)
        {
            auto AliveBotPC = GameMode->AliveBots[i];
            if (!AliveBotPC || !AliveBotPC->Pawn || !AliveBotPC->PlayerState || AliveBotPC->Pawn == Pawn)
                continue;

            if (!NearestPlayer || AliveBotPC->Pawn->GetDistanceTo(Pawn) < NearestPlayer->GetDistanceTo(Pawn))
            {
                AFortPlayerStateAthena* PS = (AFortPlayerStateAthena*)AliveBotPC->PlayerState;
                if (PS->TeamIndex != PlayerState->TeamIndex) {
                    NearestPlayer = AliveBotPC->Pawn;
                }
            }
        }

        for (size_t i = 0; i < FactionBots.size(); i++) {
            if (!FactionBots[i] || !FactionBots[i]->Pawn)
                continue;
            if (FactionBots[i]->PC && FactionBots[i]->PC->PlayerState && ((AFortPlayerStateAthena*)FactionBots[i]->PC->PlayerState)->TeamIndex == PlayerState->TeamIndex)
                continue;

            if (!NearestPlayer || FactionBots[i]->Pawn->GetDistanceTo(Pawn) < NearestPlayer->GetDistanceTo(Pawn))
            {
                NearestPlayer = FactionBots[i]->Pawn;
            }
        }

        if (!NearestPlayer) return FVector();
        return NearestPlayer->K2_GetActorLocation();
    }

    AActor* GetNearestPlayerActor() {
        auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

        AActor* NearestPlayer = nullptr;

        for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
        {
            auto AlivePC = GameMode->AlivePlayers[i];
            if (!AlivePC || !AlivePC->Pawn || !AlivePC->PlayerState)
                continue;

            if (!NearestPlayer || AlivePC->Pawn->GetDistanceTo(Pawn) < NearestPlayer->GetDistanceTo(Pawn))
            {
                AFortPlayerStateAthena* PS = (AFortPlayerStateAthena*)AlivePC->PlayerState;
                if (PS->TeamIndex != PlayerState->TeamIndex) {
                    NearestPlayer = AlivePC->Pawn;
                }
            }
        }

        for (size_t i = 0; i < GameMode->AliveBots.Num(); i++)
        {
            auto AliveBotPC = GameMode->AliveBots[i];
            if (!AliveBotPC || !AliveBotPC->Pawn || !AliveBotPC->PlayerState || AliveBotPC->Pawn == Pawn)
                continue;

            if (!NearestPlayer || AliveBotPC->Pawn->GetDistanceTo(Pawn) < NearestPlayer->GetDistanceTo(Pawn))
            {
                AFortPlayerStateAthena* PS = (AFortPlayerStateAthena*)AliveBotPC->PlayerState;
                if (PS->TeamIndex != PlayerState->TeamIndex) {
                    NearestPlayer = AliveBotPC->Pawn;
                }
            }
        }

        for (size_t i = 0; i < FactionBots.size(); i++) {
            if (!FactionBots[i] || !FactionBots[i]->Pawn)
                continue;
            if (FactionBots[i]->PC && FactionBots[i]->PC->PlayerState && ((AFortPlayerStateAthena*)FactionBots[i]->PC->PlayerState)->TeamIndex == PlayerState->TeamIndex)
                continue;

            if (!NearestPlayer || FactionBots[i]->Pawn->GetDistanceTo(Pawn) < NearestPlayer->GetDistanceTo(Pawn))
            {
                NearestPlayer = FactionBots[i]->Pawn;
            }
        }

        return NearestPlayer;
    }

    void UpdateLootableReservation(AActor* Lootable, bool RemoveReservation) {
        if (RemoveReservation) {
            if (TargetLootable) {
                ReleaseLootable(TargetLootable);
                TargetLootable = nullptr;
            }
            return;
        }

        if (!Lootable)
            return;

        if (TargetLootable && TargetLootable != Lootable)
            ReleaseLootable(TargetLootable);

        TargetLootable = Lootable;
        ReserveLootable(Lootable);
    }

    // -- Building helpers --

    float SnapBuildHorizontal(float Value, float Offset)
    {
        constexpr float Grid = 512.f;
        return roundf((Value - Offset) / Grid) * Grid + Offset;
    }

    float SnapBuildVertical(float Value)
    {
        constexpr float Grid = 384.f;
        return roundf(Value / Grid) * Grid;
    }

    FVector SnapBuildLocation(FVector Loc)
    {
        Loc.X = SnapBuildHorizontal(Loc.X, 0.f);
        Loc.Y = SnapBuildHorizontal(Loc.Y, 0.f);
        Loc.Z = SnapBuildVertical(Loc.Z);
        return Loc;
    }

    FRotator SnapBuildRotation(FRotator Rot)
    {
        Rot.Pitch = 0.f;
        Rot.Roll  = 0.f;
        Rot.Yaw   = roundf(Rot.Yaw / 90.f) * 90.f;
        return Rot;
    }

    bool IsWallBuildClass(UClass* BuildingClass)
    {
        return BuildingClass && BuildingClass->DefaultObject && BuildingClass->DefaultObject->IsA(ABuildingWall::StaticClass());
    }

    FVector SnapBuildLocation(FVector Loc, FRotator Rot, UClass* BuildingClass)
    {
        Rot = SnapBuildRotation(Rot);

        if (!IsWallBuildClass(BuildingClass))
            return SnapBuildLocation(Loc);

        // Match the build grid used by normal player walls.
        float SnappedYaw = fmodf(fabsf(Rot.Yaw), 360.f);
        bool bNorthSouthWall = SnappedYaw < 45.f || SnappedYaw > 315.f || (SnappedYaw > 135.f && SnappedYaw < 225.f);
        Loc.X = SnapBuildHorizontal(Loc.X, bNorthSouthWall ? 0.f : 256.f);
        Loc.Y = SnapBuildHorizontal(Loc.Y, bNorthSouthWall ? 256.f : 0.f);
        Loc.Z = SnapBuildVertical(Loc.Z);
        return Loc;
    }

    FVector DirectionFromBuildYaw(float Yaw)
    {
        float SnappedYaw = roundf(Yaw / 90.f) * 90.f;
        int Dir = ((int)(SnappedYaw / 90.f) % 4 + 4) % 4;
        if (Dir == 0) return FVector{ 1.f, 0.f, 0.f };
        if (Dir == 1) return FVector{ 0.f, 1.f, 0.f };
        if (Dir == 2) return FVector{ -1.f, 0.f, 0.f };
        return FVector{ 0.f, -1.f, 0.f };
    }

    bool SpawnBotBuild(UClass* BuildingClass, FVector Loc, FRotator Rot)
    {
        if (!BuildingClass || !PC || !Pawn || !PlayerState)
            return false;

        float Now = Statics->GetTimeSeconds(UWorld::GetWorld());
        if (ConsecutiveBuildFailures >= 2 && Now < NextBuildFailureResetTime)
            return false;
        if (Now >= NextBuildFailureResetTime)
            ConsecutiveBuildFailures = 0;

        Rot = SnapBuildRotation(Rot);
        Loc = SnapBuildLocation(Loc, Rot, BuildingClass);

        TArray<AActor*> BuildingsToRemove;
        char CantBuildResult = 0;
        if (CantBuild(UWorld::GetWorld(), BuildingClass, Loc, Rot, false, &BuildingsToRemove, &CantBuildResult)) {
            ConsecutiveBuildFailures++;
            NextBuildFailureResetTime = Now + 25.f;
            return false;
        }

        ABuildingSMActor* NewBuilding = SpawnActor<ABuildingSMActor>(Loc, Rot, PC, BuildingClass, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (!NewBuilding) {
            ConsecutiveBuildFailures++;
            NextBuildFailureResetTime = Now + 25.f;
            return false;
        }

        NewBuilding->bPlayerPlaced = true;
        NewBuilding->EditingPlayer = nullptr;
        NewBuilding->InitializeKismetSpawnedBuildingActor(NewBuilding, (AFortPlayerController*)PC, true);
        NewBuilding->TeamIndex = PlayerState->TeamIndex;
        NewBuilding->Team = EFortTeam(NewBuilding->TeamIndex);
        NewBuilding->SetNetDormancy(ENetDormancy::DORM_DormantAll);
        ConsecutiveBuildFailures = 0;
        return true;
    }

    bool TryBuildBeginnerCover(bool bUnderPressure)
    {
        float Now = Statics->GetTimeSeconds(UWorld::GetWorld());
        if (Now < NextBuildTime || !Pawn || !PC || bIsDead)
            return false;

        static UClass* WallClass = APBWA_W1_Solid_C::StaticClass();
        if (!WallClass)
            return false;

        FVector Base    = Pawn->K2_GetActorLocation();
        FRotator Facing = PC->GetControlRotation();
        Facing.Pitch = 0.f;
        Facing.Roll  = 0.f;

        FVector BuildBase  = SnapBuildLocation(Base);
        FRotator BuildRot  = SnapBuildRotation(Facing);
        FVector Forward    = DirectionFromBuildYaw(BuildRot.Yaw);
        bool bBuiltAny     = false;

        // One wall in front – keeps it cheap and avoids collision spam
        bBuiltAny |= SpawnBotBuild(WallClass, BuildBase + (Forward * 512.f), BuildRot);

        NextBuildTime = Now + Math->RandomFloatInRange(bUnderPressure ? 12.f : 18.f, bUnderPressure ? 20.f : 28.f);
        return bBuiltAny;
    }

    bool TryBuildForTraversal(FVector Destination)
    {
        float Now = Statics->GetTimeSeconds(UWorld::GetWorld());
        // Traversal building disabled – re-enable by removing the early return below
        NextTravelBuildTime = Now + 30.f;
        return false;
    }

    void TryOpenNearbyDoor(float Range = 600.f)
    {
        if (!Pawn || bIsDead)
            return;

        float Now = Statics->GetTimeSeconds(UWorld::GetWorld());
        if (Now < NextDoorOpenTime)
            return;
        NextDoorOpenTime = Now + 1.5f;

        TArray<AActor*> Walls;
        UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), ABuildingWall::StaticClass(), &Walls);

        ABuildingWall* NearestDoor = nullptr;
        float NearestDist = FLT_MAX;

        for (int i = 0; i < Walls.Num(); i++)
        {
            ABuildingWall* Wall = (ABuildingWall*)Walls[i];
            if (!Wall)
                continue;

            float Dist = Wall->GetDistanceTo(Pawn);
            if (Dist > Range)
                continue;

            bool bIsDoor = Wall->bSwingingDoor || Wall->bSlidingDoor || Wall->bAutomaticSlidingDoor || Wall->DoorMesh;

            if (!bIsDoor)
                continue;

            if (Wall->IsDoorOpen())
                continue;

            if (Dist < NearestDist) {
                NearestDist = Dist;
                NearestDoor = Wall;
            }
        }

        Walls.Free();

        if (NearestDoor) {
            FVector DoorLoc = NearestDoor->K2_GetActorLocation();
            FVector BotLoc = Pawn->K2_GetActorLocation();
            FVector ToDoor = (DoorLoc - BotLoc).Normalize();

            FRotator DoorRot = Math->FindLookAtRotation(BotLoc, DoorLoc);
            SetLookRotation(DoorRot);

            if (NearestDist < 150.f) {
                NearestDoor->bDoorOpen = true;
                NearestDoor->bLocalDoorOpen = true;
                NearestDoor->bDoorCollisionDisabled = true;
                NearestDoor->bLocalDoorCollisionDisabled = true;
                NearestDoor->OnRep_bDoorOpen();
                NearestDoor->OnRep_bDoorCollisionDisabled();
                NearestDoor->VerifyDoorOpenMatchesServer();
                NearestDoor->VerifyDoorCollisionMatchesServer();
            }
            else {
                FVector DoorApproachLoc = DoorLoc - ToDoor * 100.f;
                PC->MoveToLocation(DoorApproachLoc, 50.f, true, false, false, true, nullptr, true);
            }
        }
    }

void TryBreakDoorOrWall(float Range = 500.f)
    {
        if (!Pawn || bIsDead || Pawn->bIsDBNO)
            return;

        float Now = Statics->GetTimeSeconds(UWorld::GetWorld());
        if (Now < NextDoorOpenTime)
            return;
        NextDoorOpenTime = Now + 0.5f;

        TArray<AActor*> Walls;
        UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), ABuildingSMActor::StaticClass(), &Walls);

        ABuildingWall* NearestBlocker = nullptr;
        float NearestDist = FLT_MAX;

        FVector Forward = Pawn->GetActorForwardVector();

        for (int i = 0; i < Walls.Num(); i++)
        {
            ABuildingWall* Wall = (ABuildingWall*)Walls[i];
            if (!Wall)
                continue;

            float Dist = Wall->GetDistanceTo(Pawn);
            if (Dist > Range)
                continue;

            FVector ToWall = (Wall->K2_GetActorLocation() - Pawn->K2_GetActorLocation()).Normalize();
            float DotProduct = Forward.X * ToWall.X + Forward.Y * ToWall.Y;

            if (DotProduct > 0.35f && Dist < NearestDist) {
                NearestDist = Dist;
                NearestBlocker = Wall;
            }
        }

        Walls.Free();

        if (NearestBlocker) {
            FVector WallLoc = NearestBlocker->K2_GetActorLocation();
            FVector BotLoc = Pawn->K2_GetActorLocation();
            float VertOffset = WallLoc.Z - BotLoc.Z;
            bool bIsJumpable = (VertOffset > -100.f && VertOffset < 200.f);

            FRotator WallRot = Math->FindLookAtRotation(BotLoc, WallLoc);
            SetLookRotation(WallRot);
            LookAt(NearestBlocker);

            if (bIsJumpable && NearestDist < 300.f) {
                FVector Forward = Pawn->GetActorForwardVector();
                float ForwardSpeed = 360.f + (rand() % 80);
                float UpSpeed = 260.f + (VertOffset * 0.35f);
                if (UpSpeed < 220.f) UpSpeed = 220.f;
                if (UpSpeed > 360.f) UpSpeed = 360.f;
                FVector JumpVel = (Forward * ForwardSpeed) + FVector(0, 0, UpSpeed);
                Pawn->LaunchCharacter(JumpVel, true, false);
                NextDoorOpenTime = Now + 0.8f;
            }
            else if (NearestDist < 200.f) {
                if (!IsPickaxeEquiped()) {
                    EquipPickaxe();
                }

                float WallHealth = NearestBlocker->GetHealth();
                if (WallHealth > 0 && WallHealth < 1500) {
                    Pawn->PawnStartFire(0);
                    NextDoorOpenTime = Now + 0.1f;
                }
            }
            else if (NearestDist < 350.f) {
                FVector ToWall = (WallLoc - BotLoc).Normalize();
                FVector ApproachLoc = WallLoc - ToWall * 100.f;
                PC->MoveToLocation(ApproachLoc, 50.f, true, false, false, true, nullptr, true);
            }
        }
    }

    bool TryDropDown(FVector Destination)
    {
        if (!Pawn || !PC || bIsDead)
            return false;

        float Now = Statics->GetTimeSeconds(UWorld::GetWorld());
        if (Now < NextDropCheckTime)
            return false;

        FVector BotLoc = Pawn->K2_GetActorLocation();

        float ZDiff = BotLoc.Z - Destination.Z;
        if (ZDiff < 250.f)
            return false;

        if (ZDiff > 680.f)
            return false;

        float HorizDist = sqrtf(
            (Destination.X - BotLoc.X) * (Destination.X - BotLoc.X) +
            (Destination.Y - BotLoc.Y) * (Destination.Y - BotLoc.Y));
        if (HorizDist > 2200.f)
            return false;

        FVector ToDest = Destination - BotLoc;
        ToDest.Z = 0.f;
        ToDest.Normalize();

        TArray<AActor*> NearbyActors;
        UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), ABuildingSMActor::StaticClass(), &NearbyActors);

        ABuildingSMActor* BlockingRailing = nullptr;
        float NearestDist = FLT_MAX;

        for (int i = 0; i < NearbyActors.Num(); i++)
        {
            ABuildingSMActor* Actor = (ABuildingSMActor*)NearbyActors[i];
            if (!Actor) continue;

            FVector ActorLoc = Actor->K2_GetActorLocation();
            float Dist = Math->Vector_Distance(BotLoc, ActorLoc);
            if (Dist > 450.f || Dist < 30.f) continue;

            FVector ToActor = (ActorLoc - BotLoc).Normalize();
            float Dot = ToDest.X * ToActor.X + ToDest.Y * ToActor.Y;
            if (Dot < 0.05f) continue;

            float VertOffset = ActorLoc.Z - BotLoc.Z;
            if (VertOffset < -80.f || VertOffset > 260.f) continue;

            float Health = Actor->GetHealth();
            if (Health <= 0.f || Health >= 1400.f) continue;

            if (Dist < NearestDist)
            {
                NearestDist = Dist;
                BlockingRailing = Actor;
            }
        }

        NearbyActors.Free();

        if (BlockingRailing)
        {
            FVector RailLoc = BlockingRailing->K2_GetActorLocation();
            FRotator LookRot = Math->FindLookAtRotation(BotLoc, RailLoc);
            SetLookRotation(LookRot);

            if (!IsPickaxeEquiped())
                EquipPickaxe();

            if (NearestDist < 270.f)
            {
                Pawn->PawnStartFire(0);
                NextDropCheckTime = Now + 0.25f;
            }
            else
            {
                Pawn->PawnStopFire(0);
                PC->MoveToLocation(RailLoc, 80.f, true, false, false, true, nullptr, true);
                Pawn->AddMovementInput(ToDest, 0.55f, true);
                NextDropCheckTime = Now + 0.4f;
            }
            return true;
        }

        FRotator DropRot = Math->FindLookAtRotation(BotLoc, BotLoc + ToDest * 512.f);
        DropRot.Pitch = -15.f;
        DropRot.Roll = 0.f;
        SetLookRotation(DropRot);

        FVector JumpVel = (ToDest * 420.f) + FVector(0.f, 0.f, 240.f);
        Pawn->LaunchCharacter(JumpVel, true, false);
        Pawn->AddMovementInput(ToDest, 0.45f, true);

        NextDropCheckTime = Now + 1.6f;
        return true;
    }

    void DetectWorldGeoStuck(FVector BotLoc, AActor* RepathActor = nullptr, FVector RepathLocation = FVector())
    {
        if (!Pawn || bIsDead || BotLoc.IsZero())
            return;

        float Now = Statics->GetTimeSeconds(UWorld::GetWorld());
        if (Now < NextPositionSampleTime)
            return;

        float Moved = LastPositionSample.IsZero() ? 9999.f : Math->Vector_Distance(BotLoc, LastPositionSample);
        if (Moved < 75.f && !LastPositionSample.IsZero())
        {
            ConsecutiveWallStuckTicks++;
            if (ConsecutiveWallStuckTicks >= 2)
            {
                FVector Right = Pawn->GetActorRightVector();
                Right.Z = 0.f;
                Right.Normalize();

                float Side = (ConsecutiveWallStuckTicks % 2 == 0) ? 1.f : -1.f;
                Pawn->AddMovementInput(Right * Side, 0.75f, true);
                if (ConsecutiveWallStuckTicks % 3 == 0)
                    Pawn->Jump();

                if (RepathActor)
                    MoveToActorThrottled(RepathActor, 500.f, 0.f);
                else if (!RepathLocation.IsZero())
                    MoveToLocationThrottled(RepathLocation, 300.f, 0.f);
            }
        }
        else
        {
            ConsecutiveWallStuckTicks = 0;
        }

        LastPositionSample = BotLoc;
        NextPositionSampleTime = Now + 1.5f;
    }

    struct ObstacleInfo {
        ABuildingSMActor* Actor;
        float Distance;
        float HorizontalDot;
        float VerticalOffset;
        bool bIsJumpable;
    };

    void ScanForObstacles3D(TArray<ObstacleInfo>& OutObstacles, float Range) {
        if (!Pawn) return;

        TArray<AActor*> AllActors;
        UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), ABuildingSMActor::StaticClass(), &AllActors);

        FVector BotLoc = Pawn->K2_GetActorLocation();
        FVector Forward = Pawn->GetActorForwardVector();

        for (AActor* Actor : AllActors) {
            FVector ActorLoc = Actor->K2_GetActorLocation();
            FVector Delta = ActorLoc - BotLoc;
            float Dist = sqrtf((Delta.X * Delta.X) + (Delta.Y * Delta.Y) + (Delta.Z * Delta.Z));
            if (Dist > Range || Dist < 50) continue;

            FVector ToActor = (ActorLoc - BotLoc).Normalize();

            float HorzDot = Forward.X * ToActor.X + Forward.Y * ToActor.Y;
            if (HorzDot < 0.35f) continue;

            float VertOffset = ActorLoc.Z - BotLoc.Z;
            bool bJumpable = (VertOffset > -100.f && VertOffset < 200.f);

            ObstacleInfo Info;
            Info.Actor = (ABuildingSMActor*)Actor;
            Info.Distance = Dist;
            Info.HorizontalDot = HorzDot;
            Info.VerticalOffset = VertOffset;
            Info.bIsJumpable = bJumpable;
            OutObstacles.Add(Info);
        }

        for (int i = 0; i < OutObstacles.Num(); i++) {
            for (int j = i + 1; j < OutObstacles.Num(); j++) {
                if (OutObstacles[j].Distance < OutObstacles[i].Distance) {
                    ObstacleInfo Temp = OutObstacles[i];
                    OutObstacles[i] = OutObstacles[j];
                    OutObstacles[j] = Temp;
                }
            }
        }
    }

    void TryJumpOverObstacle(ObstacleInfo& Obstacle) {
        if (!Pawn || bIsDead) return;

        FVector Vel = Pawn->GetVelocity();
        if (fabsf(Vel.Z) > 100.f) return;

        FVector Forward = Pawn->GetActorForwardVector();
        float ForwardSpeed = 500.f + (rand() % 200);
        float UpSpeed = 350.f + (Obstacle.VerticalOffset * 0.5f);
        if (UpSpeed < 250.f) UpSpeed = 250.f;
        if (UpSpeed > 500.f) UpSpeed = 500.f;

        FVector JumpVel = (Forward * ForwardSpeed) + FVector(0, 0, UpSpeed);
        Pawn->LaunchCharacter(JumpVel, true, false);
    }

    void PickupAllItemsNear(FVector Origin, float Range = 320.f)
    {
        if (!Pawn || !PC)
            return;

        static auto PickupClass = AFortPickupAthena::StaticClass();
        TArray<AActor*> Array;
        UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), PickupClass, &Array);

        for (size_t i = 0; i < Array.Num(); i++)
        {
            if (!Array[i] || Array[i]->bHidden)
                continue;

            if (!ShouldPickup((AFortPickupAthena*)Array[i]))
                continue;

            if (Math->Vector_Distance(Array[i]->K2_GetActorLocation(), Origin) < Range)
                Pickup((AFortPickupAthena*)Array[i]);
        }

        Array.Free();
    }

    void ScheduleChestLootCollection(FVector ChestLocation)
    {
        PendingChestLootLocation = ChestLocation;
        float Now = Statics->GetTimeSeconds(UWorld::GetWorld());
        PendingChestLootCollectUntil = Now + 4.f;
        NextChestLootCollectTime    = Now;
    }

    void TryCollectPendingChestLoot()
    {
        if (PendingChestLootLocation.IsZero() || !Pawn || !PC)
            return;

        float Now = Statics->GetTimeSeconds(UWorld::GetWorld());
        if (Now > PendingChestLootCollectUntil)
        {
            PendingChestLootLocation     = FVector();
            PendingChestLootCollectUntil = 0.f;
            NextChestLootCollectTime     = 0.f;
            return;
        }

        if (Now < NextChestLootCollectTime)
            return;

        NextChestLootCollectTime = Now + 0.45f;
        PickupAllItemsNear(PendingChestLootLocation, 950.f);
        SimpleSwitchToWeapon();
    }

    bool CanIssueMoveCommand(float Cooldown = 0.55f)
    {
        float Now = Statics->GetTimeSeconds(UWorld::GetWorld());
        if (Now < NextMoveCommandTime)
            return false;
        NextMoveCommandTime = Now + Cooldown;
        return true;
    }

    // Steer the pawn straight toward a world location with raw movement input. Used as the
    // fallback when the server has no nav mesh, so MoveToActor/MoveToLocation (which need nav
    // path following) would otherwise leave the bot running in place. Horizontal only -- the
    // movement component handles ground following / gravity.
    void DirectMoveToward(FVector Dest, float Scale = 1.f)
    {
        if (!Pawn || Dest.IsZero())
            return;
        FVector Loc = Pawn->K2_GetActorLocation();
        FVector Dir = Dest - Loc;
        Dir.Z = 0.f;
        float LenSq = (Dir.X * Dir.X) + (Dir.Y * Dir.Y);
        if (LenSq < 1.f)
            return;
        float Len = sqrtf(LenSq);
        Dir.X /= Len;
        Dir.Y /= Len;
        Pawn->AddMovementInput(Dir, Scale, true);
    }

    void MoveToActorThrottled(AActor* Target, float AcceptanceRadius, float Cooldown = 0.55f)
    {
        if (!Target || !PC || !Pawn)
            return;

        if (!g_BotsHaveNavData)
        {
            // No nav mesh on this build -- path-following MoveToActor is a no-op, so drive the
            // pawn directly toward the target every frame instead. The action handlers call us
            // each frame (not just on the think pass), so this produces smooth movement.
            FVector TLoc = Target->K2_GetActorLocation();
            FVector Loc = Pawn->K2_GetActorLocation();
            FVector Flat = TLoc - Loc; Flat.Z = 0.f;
            if (((Flat.X * Flat.X) + (Flat.Y * Flat.Y)) > (AcceptanceRadius * AcceptanceRadius))
                DirectMoveToward(TLoc);
            return;
        }

        if (!CanIssueMoveCommand(Cooldown))
            return;
        PC->MoveToActor(Target, AcceptanceRadius, true, false, true, nullptr, true);
    }

    void MoveToLocationThrottled(FVector Dest, float AcceptanceRadius, float Cooldown = 0.55f)
    {
        if (!PC || !Pawn || Dest.IsZero())
            return;

        if (!g_BotsHaveNavData)
        {
            FVector Loc = Pawn->K2_GetActorLocation();
            FVector Flat = Dest - Loc; Flat.Z = 0.f;
            if (((Flat.X * Flat.X) + (Flat.Y * Flat.Y)) > (AcceptanceRadius * AcceptanceRadius))
                DirectMoveToward(Dest);
            return;
        }

        if (!CanIssueMoveCommand(Cooldown))
            return;
        PC->MoveToLocation(Dest, AcceptanceRadius, true, false, false, true, nullptr, true);
    }

    void __fastcall GiveLategameLoadout()
    {
        // Bots should get weapons from world loot, not a spawned starter loadout.
        // Building timers are reset so late-game bots can build immediately.
        NextBuildTime = 0.f;
    }
};

#include "BotBrain.h"

namespace CombatAI {
    static bool IsMinigun(AActor* Weapon) {
        if (!Weapon) return false;
        std::string Name = Weapon->GetName();
        return Name.find("Minigun") != std::string::npos;
    }

    static bool IsShotgun(AActor* Weapon) {
        if (!Weapon) return false;
        std::string Name = Weapon->GetName();
        return Name.find("Shotgun") != std::string::npos;
    }

    static bool ShouldUseBurstFire(AActor* Weapon) {
        if (!Weapon) return false;
        std::string Name = Weapon->GetName();
        return Name.find("Minigun") != std::string::npos ||
               Name.find("LMG") != std::string::npos ||
               Name.find("SMG") != std::string::npos;
    }

    static float GetBurstDuration(AActor* Weapon) {
        if (!Weapon) return 0.3f;
        std::string Name = Weapon->GetName();
        if (Name.find("Minigun") != std::string::npos) {
            return 1.5f + (rand() % 10) * 0.1f;
        }
        if (Name.find("SMG") != std::string::npos) {
            return 0.8f + (rand() % 5) * 0.1f;
        }
        return 0.5f;
    }

    static float GetBurstCooldown(AActor* Weapon) {
        if (!Weapon) return 0.5f;
        std::string Name = Weapon->GetName();
        if (Name.find("Minigun") != std::string::npos) {
            return 0.8f + (rand() % 10) * 0.1f;
        }
        if (Name.find("SMG") != std::string::npos) {
            return 0.3f + (rand() % 5) * 0.05f;
        }
        return 0.3f;
    }

    static AActor* FindNearestEnemy(PlayerBot* bot) {
        auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
        AActor* NearestEnemy = nullptr;
        float NearestDist = FLT_MAX;
        auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;

        for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++) {
            auto PC = GameMode->AlivePlayers[i];
            if (!PC || !PC->Pawn || !PC->PlayerState) continue;
            if (((AFortPlayerStateAthena*)PC->PlayerState)->TeamIndex == bot->PlayerState->TeamIndex) continue;
            auto Pawn = (AFortPlayerPawnAthena*)PC->Pawn;
            if (Pawn == bot->Pawn) continue;
            if (Pawn->bIsDBNO || Pawn->GetHealth() <= 0) continue;
            float Dist = Math->Vector_Distance(bot->Pawn->K2_GetActorLocation(), Pawn->K2_GetActorLocation());
            if (Dist < NearestDist && Dist < 6000.f) {
                if (bot->PC->LineOfSightTo(Pawn, FVector(), true)) {
                    NearestDist = Dist;
                    NearestEnemy = Pawn;
                }
            }
        }

        for (size_t i = 0; i < GameMode->AliveBots.Num(); i++) {
            auto PC = GameMode->AliveBots[i];
            if (!PC || !PC->Pawn || !PC->PlayerState) continue;
            if (((AFortPlayerStateAthena*)PC->PlayerState)->TeamIndex == bot->PlayerState->TeamIndex) continue;
            auto Pawn = (AFortPlayerPawnAthena*)PC->Pawn;
            if (Pawn == bot->Pawn) continue;
            if (Pawn->bIsDBNO || Pawn->GetHealth() <= 0) continue;
            float Dist = Math->Vector_Distance(bot->Pawn->K2_GetActorLocation(), Pawn->K2_GetActorLocation());
            if (Dist < NearestDist && Dist < 6000.f) {
                if (bot->PC->LineOfSightTo(Pawn, FVector(), true)) {
                    NearestDist = Dist;
                    NearestEnemy = Pawn;
                }
            }
        }

        for (auto hench : FactionBots) {
            if (!hench || !hench->Pawn || !hench->PC || !hench->PC->PlayerState) continue;
            if (((AFortPlayerStateAthena*)hench->PC->PlayerState)->TeamIndex == bot->PlayerState->TeamIndex) continue;
            auto Pawn = hench->Pawn;
            if (Pawn->bIsDBNO || Pawn->GetHealth() <= 0) continue;
            float Dist = Math->Vector_Distance(bot->Pawn->K2_GetActorLocation(), Pawn->K2_GetActorLocation());
            if (Dist < NearestDist && Dist < 6000.f) {
                if (bot->PC->LineOfSightTo(Pawn, FVector(), true)) {
                    NearestDist = Dist;
                    NearestEnemy = Pawn;
                }
            }
        }

        return NearestEnemy;
    }

    static bool CanShootAt(PlayerBot* bot, AActor* Target) {
        if (!Target) return false;
        auto FortPawn = (AFortPlayerPawnAthena*)Target;
        if (FortPawn->GetHealth() <= 0) return false;
        if (FortPawn->bIsDBNO) return false;
        auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
        FVector BotPos = bot->Pawn->K2_GetActorLocation();
        FVector TargetPos = Target->K2_GetActorLocation();
        FRotator LookRot = Math->FindLookAtRotation(BotPos, TargetPos);
        FRotator CurrentRot = bot->PC->K2_GetActorRotation();
        FRotator Delta = Math->NormalizedDeltaRotator(LookRot, CurrentRot);
        float YawDiff = abs(Delta.Yaw);
        return YawDiff < 35.f && bot->PC->LineOfSightTo(Target, FVector(), true);
    }

    void RecordDeath(PlayerBot* bot, const DeathContext& Context) {
        LearningStats& Stats = BotLearning[bot];
        Stats.GamesPlayed++;
        Stats.TimesDied++;
        Stats.LastDeathReason = Context.HowDied;

        if (Context.bDiedToStorm) {
            Stats.DeathsByStorm++;
            Stats.bLearnedAvoidStorm = true;
            Stats.ConsecutiveStormDeaths++;
            Stats.bLearnedPrioritizeZone = true;
            Stats.ConsecutiveDeathsSameWay = 0;

            if (Stats.ConsecutiveStormDeaths >= 1) {
                Stats.OptimalDistance = 500.f;
            }

            wchar_t Buffer[32];
            swprintf(Buffer, 32, L"Storm Death #%d", Stats.ConsecutiveStormDeaths);
            Stats.LastDeathReason = FString(Buffer);
            return;
        }

        Stats.ConsecutiveStormDeaths = 0;

        float HealthAtDeath = bot->Pawn->GetHealth();
        float ShieldAtDeath = bot->Pawn->GetShield();

        Stats.AvgHealthAtDeath = (Stats.AvgHealthAtDeath * (Stats.GamesPlayed - 1) + HealthAtDeath) / Stats.GamesPlayed;
        Stats.AvgShieldAtDeath = (Stats.AvgShieldAtDeath * (Stats.GamesPlayed - 1) + ShieldAtDeath) / Stats.GamesPlayed;

        if (Context.Distance > 2000.f) {
            Stats.DeathsByRanged++;
        }
        else if (Context.Distance < 500.f) {
            Stats.DeathsByClose++;
        }

        if (Context.WasBuilding) {
            Stats.DeathsByBuilding++;
            Stats.bLearnedBuilding = true;
        }

        if (HealthAtDeath > 75.f && ShieldAtDeath > 50.f) {
            Stats.DeathsWhileFullHealth++;
            Stats.DeathsWhileFullShield++;
            Stats.bLearnedRotateBuild = true;
        }
        else if (HealthAtDeath > 50.f) {
            Stats.DeathsWithoutHealing++;
            if (Stats.DeathsWithoutHealing >= 2 && !Stats.bLearnedUseHealing) {
                Stats.bLearnedUseHealing = true;
            }
        }

        if (ShieldAtDeath > 25.f) {
            Stats.DeathsWithoutShield++;
            if (Stats.DeathsWithoutShield >= 2 && !Stats.bLearnedUseShield) {
                Stats.bLearnedUseShield = true;
            }
        }

        if (Context.TimeAlive > Stats.BestSurvivalTime) {
            Stats.BestSurvivalTime = Context.TimeAlive;
        }
        Stats.AvgSurvivalTime = (Stats.AvgSurvivalTime * (Stats.GamesPlayed - 1) + Context.TimeAlive) / Stats.GamesPlayed;

        if (Stats.OptimalDistance < 500.f && Context.Distance > 1500.f) {
            Stats.OptimalDistance = 1500.f;
        }
        else if (Stats.OptimalDistance > 1500.f && Context.Distance < 800.f) {
            Stats.OptimalDistance = 600.f;
        }

        if (Context.BotStateAtDeath == EBotState::Fleeing) {
            Stats.bLearnedRetreatWhenLow = true;
        }

        if (Stats.TimesDied >= 2) {
            float DeathRate = (float)Stats.TimesDied / Stats.GamesPlayed;
            if (DeathRate > 0.4f) {
                Stats.bLearnedRushEnemy = true;
            }
        }

        Stats.ConsecutiveDeathsSameWay = (Stats.LastDeathReason == Context.HowDied) ? Stats.ConsecutiveDeathsSameWay + 1 : 0;

        if (Stats.ConsecutiveDeathsSameWay >= 2) {
            Stats.OptimalDistance = Stats.OptimalDistance == 1000.f ? 800.f : 1000.f;
            Stats.bLearnedBuilding = true;
            Stats.ConsecutiveDeathsSameWay = 0;
        }
    }

    static void RecordKill(PlayerBot* bot) {
        LearningStats& Stats = BotLearning[bot];
        Stats.TimesKilledPlayer++;
        Stats.bLearnedCrouchShoot = true;
        Stats.bLearnedStrafe = true;
    }

    static void RecordWin(PlayerBot* bot) {
        LearningStats& Stats = BotLearning[bot];
        Stats.TimesWon++;
    }

    static FString AnalyzeDeathSituation(PlayerBot* bot, AActor* Killer) {
        if (!Killer) return L"Unknown";

        float Dist = bot->Pawn->GetDistanceTo(Killer);
        if (Dist > 2000.f) return L"Ranged Attack";
        if (Dist < 500.f) return L"Close Combat";

        if (bot->Pawn->CurrentWeapon) {
            std::string WeaponName = bot->Pawn->CurrentWeapon->GetName();
            if (WeaponName.find("Shotgun") != std::string::npos) {
                return L"Shotgun";
            }
        }

        return L"Mixed Combat";
    }

    static void Tick(PlayerBot* bot) {
        auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;
        auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
        float CurrentTime = Statics->GetTimeSeconds(UWorld::GetWorld());

        if (bot->BotState == EBotState::Warmup || bot->BotState == EBotState::Bus || bot->BotState == EBotState::Skydiving || bot->BotState == EBotState::Gliding) {
            bot->Pawn->PawnStopFire(0);
            CombatStates[bot].CurrentTarget = nullptr;
            return;
        }

        if (!bot->HasGun()) {
            bot->Pawn->PawnStopFire(0);
            CombatStates[bot].CurrentTarget = nullptr;
            return;
        }

        CombatState& State = CombatStates[bot];
        LearningStats& Learn = BotLearning[bot];
        AActor* CurrentWeapon = bot->Pawn->CurrentWeapon;

        if (bot->tick_counter % 10 == 0 || !State.CurrentTarget) {
            AActor* NewTarget = FindNearestEnemy(bot);
            if (NewTarget != State.CurrentTarget) {
                State.CurrentTarget = NewTarget;
                State.bIsAiming = false;
            }
        }

        if (!State.CurrentTarget) {
            bot->Pawn->PawnStopFire(0);
            return;
        }

        auto TargetPawn = (AFortPlayerPawnAthena*)State.CurrentTarget;
        if (TargetPawn->GetHealth() <= 0 || TargetPawn->bIsDBNO) {
            bot->Pawn->PawnStopFire(0);
            State.CurrentTarget = nullptr;
            return;
        }

        FVector BotPos = bot->Pawn->K2_GetActorLocation();
        FVector TargetPos = State.CurrentTarget->K2_GetActorLocation();
        float Distance = Math->Vector_Distance(BotPos, TargetPos);

        bool bHasLineOfSight = bot->PC->LineOfSightTo(State.CurrentTarget, FVector(), true);
        if (!bHasLineOfSight || Distance > 6000.f) {
            bot->Pawn->PawnStopFire(0);
            if (!bot->LastKnownEnemyLocation.IsZero() && CurrentTime - bot->LastKnownEnemyTime < 8.f) {
                bot->MoveToLocationThrottled(bot->LastKnownEnemyLocation, 450.f, 1.0f);
            }
            State.CurrentTarget = nullptr;
            return;
        }

        bot->LastKnownEnemyLocation = TargetPos;
        bot->LastKnownEnemyTime = CurrentTime;

        FRotator LookRot = Math->FindLookAtRotation(BotPos, TargetPos);
        bot->SetLookRotation(LookRot);

        bool bIsMinigun = IsMinigun(CurrentWeapon);
        bool bUseBurstFire = ShouldUseBurstFire(CurrentWeapon);

        if (Distance > 2000.f) {
            bot->Pawn->PawnStopFire(0);
            if (bot->tick_counter % 5 == 0) {
                bot->MoveToActorThrottled(State.CurrentTarget, 500.f, 0.8f);
            }
        }
        else if (Distance > 800.f) {
            if (bot->tick_counter % 15 == 0) {
                bot->MoveToActorThrottled(State.CurrentTarget, 300.f, 0.8f);
            }
            if (CanShootAt(bot, State.CurrentTarget)) {
                if (bUseBurstFire) {
                    float CurrentTime = Statics->GetTimeSeconds(UWorld::GetWorld());
                    if (!State.bIsFiring) {
                        if (CurrentTime >= State.NextFireTime) {
                            State.bIsFiring = true;
                            State.FireEndTime = CurrentTime + GetBurstDuration(CurrentWeapon);
                            bot->Pawn->PawnStartFire(0);
                        }
                    }
                    else {
                        if (CurrentTime >= State.FireEndTime) {
                            bot->Pawn->PawnStopFire(0);
                            State.bIsFiring = false;
                            State.NextFireTime = CurrentTime + GetBurstCooldown(CurrentWeapon);
                        }
                    }
                }
                else {
                    bot->Pawn->PawnStartFire(0);
                }
            }
            else {
                bot->Pawn->PawnStopFire(0);
            }
        }
        else if (Distance > 300.f) {
            if (!State.bIsAiming) {
                State.bIsAiming = true;
                State.AimStartTime = Statics->GetTimeSeconds(UWorld::GetWorld()) + 0.5f;
            }
            if (Statics->GetTimeSeconds(UWorld::GetWorld()) >= State.AimStartTime) {
                if (CanShootAt(bot, State.CurrentTarget)) {
                    if (bUseBurstFire) {
                        if (!State.bIsFiring) {
                            if (CurrentTime >= State.NextFireTime) {
                                State.bIsFiring = true;
                                State.FireEndTime = CurrentTime + GetBurstDuration(CurrentWeapon);
                                bot->Pawn->PawnStartFire(0);
                                if (Learn.bLearnedCrouchShoot && Math->RandomBoolWithWeight(0.3f)) {
                                    bot->Pawn->Crouch(false);
                                }
                            }
                        }
                        else {
                            if (CurrentTime >= State.FireEndTime) {
                                bot->Pawn->PawnStopFire(0);
                                State.bIsFiring = false;
                                State.NextFireTime = CurrentTime + GetBurstCooldown(CurrentWeapon);
                            }
                        }
                    }
                    else {
                        bot->Pawn->PawnStartFire(0);
                        if (Learn.bLearnedCrouchShoot && Math->RandomBoolWithWeight(0.2f)) {
                            bot->Pawn->Crouch(false);
                        }
                    }
                }
                else {
                    bot->Pawn->PawnStopFire(0);
                }
            }
            if (bot->tick_counter % 30 == 0) {
                bot->ForceStrafe(true);
            }
        }
        else {
            if (!State.bIsAiming) {
                State.bIsAiming = true;
                State.AimStartTime = Statics->GetTimeSeconds(UWorld::GetWorld()) + 0.3f;
            }
            if (Statics->GetTimeSeconds(UWorld::GetWorld()) >= State.AimStartTime) {
                if (CanShootAt(bot, State.CurrentTarget)) {
                    if (bUseBurstFire) {
                        if (!State.bIsFiring) {
                            if (CurrentTime >= State.NextFireTime) {
                                State.bIsFiring = true;
                                State.FireEndTime = CurrentTime + GetBurstDuration(CurrentWeapon);
                                bot->Pawn->PawnStartFire(0);
                            }
                        }
                        else {
                            if (CurrentTime >= State.FireEndTime) {
                                bot->Pawn->PawnStopFire(0);
                                State.bIsFiring = false;
                                State.NextFireTime = CurrentTime + GetBurstCooldown(CurrentWeapon);
                            }
                        }
                    }
                    else {
                        bot->Pawn->PawnStartFire(0);
                    }
                }
                else {
                    bot->Pawn->PawnStopFire(0);
                }
            }
            if (bot->tick_counter % 10 == 0) {
                bot->ForceStrafe(true);
            }
        }
    }
}

class BotsBTService_AIEvaluator {
public:
    // When stressed the bot will handle combat situations with players or other bots differently
    bool IsStressed(AFortPlayerPawnAthena* Pawn, ABP_PhoebePlayerController_C* PC) {
        // If the bots health is 75 or below then they are stressed
        if (Pawn->GetHealth() <= 75) {
            return true;
        }
        return false;
    }

public:
    void StartSkydivingFromBus(PlayerBot* bot, FVector JumpLoc, FRotator JumpRot)
    {
        if (!bot || !bot->Pawn)
            return;

        JumpRot = bot->UprightRotation(JumpRot);
        bot->Pawn->K2_TeleportTo(JumpLoc, JumpRot);
        bot->Pawn->BeginSkydiving(true);
        bot->Pawn->BP_ForceLockParachuteOpen(false);
        bot->SetLookRotation(FRotator(-80.f, JumpRot.Yaw, 0.f), false);
        bot->BotState = EBotState::Skydiving;
        bot->bHasJumpedFromBus = true;
    }

    void DeployGlider(PlayerBot* bot)
    {
        if (!bot || !bot->Pawn)
            return;

        bot->Pawn->BP_ForceOpenParachute();
        bot->Pawn->BP_ForceLockParachuteOpen(true);
        bot->BotState = EBotState::Gliding;
    }

    void Tick(PlayerBot* bot) {
        FVector BotPos = bot->Pawn->K2_GetActorLocation();
        FVector Vel = bot->Pawn->GetVelocity();
        float Speed = sqrtf(Vel.X * Vel.X + Vel.Y * Vel.Y);

        if (bot->HasTimerElapsed(bot->NextLootScanTime, 1.8f, 0.8f)) {
            // Lets update the reservation every 2 seconds because its cleaner
            AActor* NearestChest = bot->FindNearestChest();
            AActor* NearestPickup = (AActor*)bot->FindNearestPickup();
            if (!NearestChest || !NearestPickup) {}
            else {
                AActor* Nearest = NearestChest;
                ELootableType NearestLootable = ELootableType::Chest;
                if (NearestChest->GetDistanceTo(bot->Pawn) > NearestPickup->GetDistanceTo(bot->Pawn)) {
                    NearestLootable = ELootableType::Pickup;
                    Nearest = NearestPickup;
                }
                if (bot->TargetLootable) {
                    if (Nearest != bot->TargetLootable) {
                        bot->UpdateLootableReservation(nullptr, true);
                        bot->UpdateLootableReservation(Nearest, false);
                    }
                }
                else {
                    bot->UpdateLootableReservation(Nearest, false);
                }
                bot->TargetLootableType = NearestLootable;
            }
        }

        if (bot->HasTimerElapsed(bot->NextTargetScanTime, 0.9f, 0.45f)) {
            bot->NearestPlayerActor = bot->GetNearestPlayerActor();
        }

        if (bot->HasTimerElapsed(bot->NextUtilityActionTime, 1.8f, 1.0f)) {
            FVector Vel = bot->Pawn ? bot->Pawn->GetVelocity() : FVector();
            float Speed = sqrtf((Vel.X * Vel.X) + (Vel.Y * Vel.Y));
            if (Speed < 120.f || bot->ConsecutiveWallStuckTicks > 0)
            {
                bot->TryOpenNearbyDoor(600.f);
                bot->TryBreakDoorOrWall(500.f);
            }
        }

        bot->TryCollectPendingChestLoot();

        if (bot->HasTimerElapsed(bot->NextFocusClearTime, 2.0f, 0.6f)) {
            // Every 2 seconds clear the focus just incase the bot is doing something else
            bot->PC->K2_ClearFocus();
        }

        if (bot->Pawn->bIsCrouched && bot->HasTimerElapsed(bot->NextCrouchActionTime, 1.0f, 0.4f)) {
            bot->Pawn->UnCrouch(false);
        }

        bot->bIsStressed = IsStressed(bot->Pawn, bot->PC);

        if (bot->bIsCurrentlyStrafing) {
            bot->ForceStrafe(false);
        }

        if (Speed >= 100 && bot->BotState > EBotState::Landed && bot->HasTimerElapsed(bot->NextRunCommandTime, 3.0f, 1.0f)) {
            bot->Run();
        }
    }
};

class BotsBTService_Warmup{
public:
    void DetermineBotWarmupChoice(PlayerBot* bot) {
        if (UKismetMathLibrary::GetDefaultObj()->RandomBool()) {
            bot->BotWarmupChoice = EBotWarmupChoice::Emote;
        }
        else {
            bot->BotWarmupChoice = EBotWarmupChoice::MoveToPlayerEmote;
        }
    }

public:
    void Tick(PlayerBot* bot) {
        if (bot->BotWarmupChoice == EBotWarmupChoice::MAX) {
            DetermineBotWarmupChoice(bot);
        }
        else if (bot->BotWarmupChoice == EBotWarmupChoice::Emote) {
            if (bot->tick_counter % 300 == 0) {
                bot->Emote();
            }
        }
        else {
            if (bot->tick_counter % 150 == 0) {
                bot->NearestPlayerActor = bot->GetNearestPlayerActor();
                auto BotPos = bot->Pawn->K2_GetActorLocation();
                if (bot->NearestPlayerActor) {
                    FVector Nearest = bot->NearestPlayerActor->K2_GetActorLocation();
                    if (!Nearest.IsZero()) {
                        float Dist = Math->Vector_Distance(BotPos, Nearest);
                        if (Dist < 200.f + rand() % 300) {
                            bot->LookAt(bot->NearestPlayerActor);
                            if (UKismetMathLibrary::GetDefaultObj()->RandomBool()) {
                                bot->Emote();
                            }
                        }
                        else {
                            bot->LookAt(bot->NearestPlayerActor);
                            bot->MoveToActorThrottled(bot->NearestPlayerActor, 100.f, 0.8f);
                        }
                    }
                }
            }
        }
    }
};

class BotsBTService_AIDropZone {
public:
    static void StartBotSkydivingFromBus(PlayerBot* bot, FVector JumpLoc, FRotator JumpRot)
    {
        if (!bot || !bot->Pawn)
            return;

        JumpRot = bot->UprightRotation(JumpRot);
        bot->Pawn->K2_TeleportTo(JumpLoc, JumpRot);
        bot->Pawn->BeginSkydiving(true);
        bot->Pawn->BP_ForceLockParachuteOpen(false);
        bot->SetLookRotation(FRotator(-80.f, JumpRot.Yaw, 0.f), false);
        bot->BotState = EBotState::Skydiving;
        bot->bHasJumpedFromBus = true;
    }

    static void DeployBotGlider(PlayerBot* bot)
    {
        if (!bot || !bot->Pawn)
            return;

        bot->Pawn->BP_ForceOpenParachute();
        bot->Pawn->BP_ForceLockParachuteOpen(true);
        bot->BotState = EBotState::Gliding;
    }

    void ChooseDropZone(PlayerBot* bot) {
        if (DropZoneLocations.empty()) return;
        
        bot->TargetDropZone = DropZoneLocations[rand() % DropZoneLocations.size()];

        // makes it more realisetic cause they dont clutter together
        bot->TargetDropZone.X += Math->RandomFloatInRange(-800.f, 800.f);
        bot->TargetDropZone.Y += Math->RandomFloatInRange(-800.f, 800.f);
    }

public:
    void Tick(PlayerBot* bot) {
        auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
        auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
        auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
        auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

        if (bot->TargetDropZone.IsZero()) {
            ChooseDropZone(bot);
            return;
        }

        if (bot->BotState == EBotState::Bus) {
            bot->Pawn->SetShield(0);
            if (bot->bHasJumpedFromBus) {
                bot->BotState = EBotState::Skydiving;
                return;
            }

            if (!bot->bHasThankedBusDriver && GameState->GamePhase == EAthenaGamePhase::Aircraft && Math->RandomBoolWithWeight(0.0005f))
            {
                bot->bHasThankedBusDriver = true;
                bot->PC->ThankBusDriver();
            }

            AActor* Bus = GameState->GetAircraft(0);
            if (!Bus) {
                return;
            }

            FVector BusLocation = Bus->K2_GetActorLocation();
            FVector DropTarget = bot->TargetDropZone;
            DropTarget.Z = BusLocation.Z;

            if (GameState->GamePhase > EAthenaGamePhase::Aircraft) {
                Log("Force Jump");
                FVector JumpLoc = BusLocation;
                JumpLoc.Z += 500;
                JumpLoc.X += (rand() % 400) - 200;
                JumpLoc.Y += (rand() % 400) - 200;

                FRotator JumpRot = FRotator(0.f, 0.f, 0.f);
                StartBotSkydivingFromBus(bot, JumpLoc, JumpRot);

                return;
            }

            float DistanceToDrop = Math->Vector_Distance(BusLocation, DropTarget);
            if (DistanceToDrop < 18000.f || bot->ClosestDistToDropZone < 18000.f) {
                if (!bot->bHasThankedBusDriver && Math->RandomBoolWithWeight(0.5f)) {
                    bot->bHasThankedBusDriver = true;
                    bot->PC->ThankBusDriver();
                }

                FVector JumpLoc = BusLocation;
                JumpLoc.Z += 500;
                JumpLoc.X += (rand() % 400) - 200;
                JumpLoc.Y += (rand() % 400) - 200;
                FRotator JumpRot = FRotator(0.f, 0.f, 0.f);
                StartBotSkydivingFromBus(bot, JumpLoc, JumpRot);
            }
            else if (DistanceToDrop < bot->ClosestDistToDropZone) {
                bot->ClosestDistToDropZone = DistanceToDrop;
            }
            else {
                if (!bot->bHasThankedBusDriver && Math->RandomBoolWithWeight(0.5f)) {
                    bot->bHasThankedBusDriver = true;
                    bot->PC->ThankBusDriver();
                }
                if (Math->RandomBoolWithWeight(0.75)) {
                    FVector JumpLoc = BusLocation;
                    JumpLoc.Z += 500;
                    JumpLoc.X += (rand() % 400) - 200;
                    JumpLoc.Y += (rand() % 400) - 200;
                    FRotator JumpRot = FRotator(0.f, 0.f, 0.f);
                    StartBotSkydivingFromBus(bot, JumpLoc, JumpRot);
                }
            }

            return;
        }

        auto BotPos = bot->Pawn->K2_GetActorLocation();

        if (bot->BotState == EBotState::Skydiving) {
            FVector BotPos = bot->Pawn->K2_GetActorLocation();
            FVector Vel = bot->Pawn->GetVelocity();

            float TargetZ = bot->TargetDropZone.IsZero() ? 0.f : bot->TargetDropZone.Z;
            float HeightAboveTarget = BotPos.Z - TargetZ;
            float DistToDrop = bot->TargetDropZone.IsZero() ? FLT_MAX : Math->Vector_Distance(BotPos, bot->TargetDropZone);

            if (bot->Pawn->bIsInWaterVolume) {
                bot->BotState = EBotState::Landed;
                return;
            }

            if (HeightAboveTarget < 5200.f || DistToDrop < 6500.f) {
                DeployBotGlider(bot);
                return;
            }

            if (!bot->TargetDropZone.IsZero()) {
                // Steer toward the drop zone. Feeding the steep look-at rotation (pitch -80)
                // straight into AddMovementInput points the input vector nearly straight down,
                // so the bot barely tracks horizontally and just freefalls. Drive the
                // horizontal direction toward the target directly (full magnitude) and only
                // use the steep pitch for the camera/look so they visibly dive toward their POI.
                FRotator LookRot = Math->FindLookAtRotation(BotPos, bot->TargetDropZone);
                bot->SetLookRotation(FRotator(-65.f, LookRot.Yaw, 0.f));

                FVector ToTarget = bot->TargetDropZone - BotPos;
                ToTarget.Z = 0.f;
                ToTarget = ToTarget.Normalize();
                bot->Pawn->AddMovementInput(ToTarget, 1.f, true);
            }
        }
        else if (bot->BotState == EBotState::Gliding) {
            FVector BotPos = bot->Pawn->K2_GetActorLocation();
            FVector Vel = bot->Pawn->GetVelocity();
            float VerticalSpeed = Vel.Z;
            float HeightAboveTarget = bot->TargetDropZone.IsZero() ? BotPos.Z : BotPos.Z - bot->TargetDropZone.Z;

            if (bot->Pawn->bIsInWaterVolume || HeightAboveTarget < 350.f) {
                bot->Pawn->BP_ForceLockParachuteOpen(false);
                bot->BotState = EBotState::Landed;
            }

            if (!bot->TargetDropZone.IsZero()) {
                float Dist = Math->Vector_Distance(BotPos, bot->TargetDropZone);
                FRotator TargetRot = Math->FindLookAtRotation(BotPos, bot->TargetDropZone);
                TargetRot.Pitch = 0.f;
                TargetRot.Roll = 0.f;

                bot->SetLookRotation(TargetRot);

                if (Dist > 500.f) {
                    // Glide straight at the target horizontally instead of trusting the pawn's
                    // (async-updated) facing, so they actually close on the POI under canopy.
                    FVector ToTarget = bot->TargetDropZone - BotPos;
                    ToTarget.Z = 0.f;
                    ToTarget = ToTarget.Normalize();
                    bot->Pawn->AddMovementInput(ToTarget, 1.f, true);
                }
            }
        }
    }
};

class BotsBTService_Loot {
public:

public:
    void Tick(PlayerBot* bot) {
        if (!bot || !bot->Pawn)
            return;

        FVector BotLoc = bot->Pawn->K2_GetActorLocation();
        if (bot->TargetLootable) {
            if (bot->HasGun()) {
                bot->BotState = EBotState::LookingForPlayers;
                return;
            }
            bot->DetectWorldGeoStuck(BotLoc, bot->TargetLootable, bot->TargetLootable->K2_GetActorLocation());
            if (!BotLoc.IsZero()) {
                float Dist = Math->Vector_Distance(BotLoc, bot->TargetLootable->K2_GetActorLocation());

                auto BotPos = bot->Pawn->K2_GetActorLocation();
                auto TestRot = Math->FindLookAtRotation(BotPos, bot->TargetLootable->K2_GetActorLocation());

                bot->SetLookRotation(TestRot);
                bot->LookAt(bot->TargetLootable);

                float CurrentTime = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());

                if (bot->TargetLootable) {
                    FVector BotLoc2 = bot->Pawn->K2_GetActorLocation();
                    float Dist2 = Math->Vector_Distance(BotLoc2, bot->TargetLootable->K2_GetActorLocation());

                    if (bot->LootTargetStartTime == 0.f) {
                        bot->LootTargetStartTime = CurrentTime;
                        bot->LastLootTargetDistance = Dist2;
                    }

                    float Elapsed = CurrentTime - bot->LootTargetStartTime;

                    // If the route is blocked, drop this reservation and find another item instead of teleporting into geometry.
                    if ((Elapsed > 8.f && Dist2 > bot->LastLootTargetDistance - 100.f)) {
                        bot->UpdateLootableReservation(nullptr, true);
                        bot->LootTargetStartTime = 0.f;
                        bot->BotState = EBotState::Looting;
                        return;
                    }

                    if (Dist2 < 300.f) {
                        bot->LootTargetStartTime = 0.f;
                    }

                    bot->LastLootTargetDistance = Dist2;
                }

                if (Dist < 300.f) {
                    bot->PC->StopMovement();
                    bot->Pawn->PawnStopFire(0);
                    if (!bot->TimeToNextAction && bot->TargetLootableType == ELootableType::Chest) {
                        bot->TimeToNextAction = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());
                    }
                    else if (UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) - bot->TimeToNextAction >= 1.5f && bot->TargetLootableType == ELootableType::Chest) {
                        FVector ChestLoc = bot->TargetLootable->K2_GetActorLocation();
                        Looting::SpawnLoot((ABuildingContainer*)bot->TargetLootable);
                        bot->UpdateLootableReservation(nullptr, true); // releases reservation + clears TargetLootable
                        bot->ScheduleChestLootCollection(ChestLoc);
                        bot->TimeToNextAction = 0;
                        bot->BotState = EBotState::Looting;
                    }
                    else if (bot->TargetLootableType == ELootableType::Pickup) {
                        AFortPickup* Pickup = bot->FindNearestPickup();
                        if (Pickup)
                        {
                            bot->Pickup(Pickup);
                        }
                        bot->UpdateLootableReservation(nullptr, true);
                        bot->TimeToNextAction = 0;
                        bot->BotState = EBotState::LookingForPlayers;
                    }
                }
                else if (Dist < 2000.f) {
                    bot->Pawn->PawnStopFire(0);
                    bot->MoveToActorThrottled(bot->TargetLootable, 50.f, 0.8f);
                    bot->TryDropDown(bot->TargetLootable->K2_GetActorLocation());
                }
                else {
                    bot->MoveToActorThrottled(bot->TargetLootable, 50.f, 0.8f);
                    bot->TryDropDown(bot->TargetLootable->K2_GetActorLocation());
                }
            }
        }
        else {
            if (bot->HasGun()) {
                bot->BotState = EBotState::LookingForPlayers;
                return;
            }

            auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
            FVector EscapeTarget = bot->TargetDropZone;
            if (EscapeTarget.IsZero() && GameState && GameState->SafeZoneIndicator)
                EscapeTarget = GameState->SafeZoneIndicator->NextCenter;

            bot->DetectWorldGeoStuck(BotLoc, nullptr, EscapeTarget);

            if (!EscapeTarget.IsZero())
                bot->TryDropDown(EscapeTarget);

            if (bot->HasTimerElapsed(bot->NextRoamTime, 3.5f, 1.0f))
            {
                FVector RoamPt = EscapeTarget;
                if (!RoamPt.IsZero()) {
                    RoamPt.X += Math->RandomFloatInRange(-600.f, 600.f);
                    RoamPt.Y += Math->RandomFloatInRange(-600.f, 600.f);
                    bot->MoveToLocationThrottled(RoamPt, 300.f, 0.5f);
                    bot->RoamTarget = RoamPt;
                }
            }
        }
    }
};

namespace PlayerBots {
    void FreeDeadBots() {
        for (size_t i = 0; i < PlayerBotArray.size();)
        {
            if (PlayerBotArray[i]->bIsDead) {
                BotBrain::CleanupDeadBotData(PlayerBotArray[i]);
                CombatStates.erase(PlayerBotArray[i]);
                BotLearning.erase(PlayerBotArray[i]);
                delete PlayerBotArray[i];
                PlayerBotArray.erase(PlayerBotArray.begin() + i);
                Log("Freed a dead bot from the array!");
            }
            else {
                ++i;
            }
        }
    }

    // Returns the world's main nav data, locating/registering it if the nav system hasn't.
    // On the dedicated server MainNavData is frequently null AND NavDataSet is empty even
    // though the baked RecastNavMesh from Apollo_Nav_Gameplay has streamed in as an actor --
    // so as a last resort we find the RecastNavMesh actor directly and register it. Returns
    // null if no nav mesh exists in the world yet (still streaming).
    inline ANavigationData* EnsureWorldNavData()
    {
        UWorld* World = UWorld::GetWorld();
        if (!World)
            return nullptr;
        auto NavSys = (UNavigationSystemV1*)World->NavigationSystem;
        if (!NavSys)
            return nullptr;

        if (NavSys->MainNavData)
            return NavSys->MainNavData;

        if (NavSys->NavDataSet.Num() > 0)
        {
            NavSys->MainNavData = NavSys->NavDataSet[0];
            return NavSys->MainNavData;
        }

        // NavDataSet empty -- find the baked RecastNavMesh actor directly and register it.
        TArray<AActor*> NavActors;
        UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(World, ARecastNavMesh::StaticClass(), &NavActors);
        ANavigationData* Found = nullptr;
        if (NavActors.Num() > 0)
        {
            Found = (ANavigationData*)NavActors[0];
            NavSys->MainNavData = Found;
            NavSys->NavDataSet.Add(Found);
        }
        NavActors.Free();
        return Found;
    }

    // Push the world nav data onto every spawned bot's path-following component. Bots spawn
    // during warmup, usually before Apollo_Nav_Gameplay finishes registering its
    // RecastNavMesh, so their MyNavData starts null and MoveToActor/MoveToLocation do
    // nothing (no roaming/looting/chasing). Returns true once nav data exists and was
    // assigned -- the tick calls this until it takes.
    inline bool TryAssignNavDataToBots()
    {
        ANavigationData* Nav = EnsureWorldNavData();
        if (!Nav)
            return false;

        g_BotsHaveNavData = true;
        int assigned = 0;
        for (size_t i = 0; i < PlayerBotArray.size(); i++)
        {
            PlayerBot* bot = PlayerBotArray[i];
            if (bot && bot->PC && bot->PC->PathFollowingComponent)
            {
                bot->PC->PathFollowingComponent->MyNavData = Nav;
                bot->PC->PathFollowingComponent->OnNavDataRegistered(Nav);
                assigned++;
            }
        }

        static bool bLoggedNavAssign = false;
        if (!bLoggedNavAssign)
        {
            char navbuf[160];
            sprintf_s(navbuf, "Nav mesh registered; assigned MainNavData to %d bot(s) -- they can path now.", assigned);
            Log(navbuf);
            bLoggedNavAssign = true;
        }
        return true;
    }

    void SpawnPlayerBots(AActor* SpawnLocator, EBotState StartingState = EBotState::Warmup, AFortPlayerControllerAthena* TeamPC = nullptr)
    {
        if (!Globals::bBotsEnabled)
            return;

        auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

        static auto BotBP = StaticLoadObject<UClass>("/Game/Athena/AI/Phoebe/BP_PlayerPawn_Athena_Phoebe.BP_PlayerPawn_Athena_Phoebe_C");
        static UBehaviorTree* BehaviorTree = StaticLoadObject<UBehaviorTree>("/Game/Athena/AI/Phoebe/BehaviorTrees/BT_Phoebe.BT_Phoebe");
        static UBlackboardData* BlackboardData = StaticLoadObject<UBlackboardData>("/Game/Athena/AI/Phoebe/BehaviorTrees/BB_Phoebe.BB_Phoebe");

        if (!BotBP || !BehaviorTree)
            return;

        PlayerBot* bot = new PlayerBot{};

        AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

        static auto BotMutator = (AFortAthenaMutator_Bots*)GameMode->ServerBotManager->CachedBotMutator;

        bot->Pawn = BotMutator->SpawnBot(BotBP, SpawnLocator, SpawnLocator->K2_GetActorLocation(), SpawnLocator->K2_GetActorRotation(), false);

        if (!bot->Pawn || !bot->Pawn->Controller)
            return;

        bot->PC = Cast<ABP_PhoebePlayerController_C>(bot->Pawn->Controller);
        bot->PlayerState = Cast<AFortPlayerStateAthena>(bot->PC->PlayerState);

        if (!bot->PC || !bot->PlayerState)
            return;

        auto CID = GetValidCID();

        if (CID && CID->HeroDefinition)
        {
            if (CID->HeroDefinition->Specializations.IsValid())
            {
                for (size_t i = 0; i < CID->HeroDefinition->Specializations.Num(); i++)
                {
                    UFortHeroSpecialization* Spec = StaticLoadObject<UFortHeroSpecialization>(UKismetStringLibrary::GetDefaultObj()->Conv_NameToString(CID->HeroDefinition->Specializations[i].ObjectID.AssetPathName).ToString());
                    if (Spec)
                    {
                        for (size_t j = 0; j < Spec->CharacterParts.Num(); j++)
                        {
                            UCustomCharacterPart* Part = StaticLoadObject<UCustomCharacterPart>(UKismetStringLibrary::GetDefaultObj()->Conv_NameToString(Spec->CharacterParts[j].ObjectID.AssetPathName).ToString());
                            if (Part)
                            {
                                bot->PlayerState->CharacterData.Parts[(uintptr_t)Part->CharacterPartType] = Part;
                            }
                        }
                    }
                }
            }
        }
        if (CID) {
            bot->PC->CosmeticLoadoutBC.Character = CID;
        }
        EnsureValidAthenaLoadout(bot->PC->CosmeticLoadoutBC, "player bot controller");
        if (!Backpacks.empty() && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.3)) {
            auto Backpack = Backpacks[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Backpacks.size() - 1)];
            for (size_t j = 0; j < Backpack->CharacterParts.Num(); j++)
            {
                UCustomCharacterPart* Part = Backpack->CharacterParts[j];
                if (Part)
                {
                    bot->PlayerState->CharacterData.Parts[(uintptr_t)Part->CharacterPartType] = Part;
                }
            }
        }
        if (!Gliders.empty()) {
            auto Glider = Gliders[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Gliders.size() - 1)];
            bot->PC->CosmeticLoadoutBC.Glider = Glider;
        }
        if (!Contrails.empty() && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.95)) {
            auto Contrail = Contrails[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Contrails.size() - 1)];
            bot->PC->CosmeticLoadoutBC.SkyDiveContrail = Contrail;
        }
        for (size_t i = 0; i < Dances.size(); i++)
        {
            bot->PC->CosmeticLoadoutBC.Dances.Add(Dances.at(i));
        }

        bot->Pawn->CosmeticLoadout = bot->PC->CosmeticLoadoutBC;
        TryApplyPawnCosmeticLoadout(bot->Pawn, "player bot pawn");

        if (Pickaxes.empty()) {
            Log("Pickaxes array is empty!");
            UFortWeaponMeleeItemDefinition* PickDef = StaticLoadObject<UFortWeaponMeleeItemDefinition>("/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");
            if (PickDef) {
                bot->GiveItemBot(PickDef);
                auto Entry = bot->GetEntry(PickDef);
                if (Entry && Entry->ItemDefinition && (Entry->ItemGuid.A != 0 || Entry->ItemGuid.B != 0 || Entry->ItemGuid.C != 0 || Entry->ItemGuid.D != 0))
                    bot->Pawn->EquipWeaponDefinition((UFortWeaponMeleeItemDefinition*)Entry->ItemDefinition, Entry->ItemGuid);
            }
            else {
                Log("Default Pickaxe dont exist!");
            }
        }
        else {
            auto PickDef = Pickaxes[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Pickaxes.size() - 1)];
            if (!PickDef)
            {
                Log("Cooked!");
                UFortWeaponMeleeItemDefinition* PickDef = StaticLoadObject<UFortWeaponMeleeItemDefinition>("/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");
                if (PickDef) {
                    bot->GiveItemBot(PickDef);
                    auto Entry = bot->GetEntry(PickDef);
                    if (Entry && Entry->ItemDefinition && (Entry->ItemGuid.A != 0 || Entry->ItemGuid.B != 0 || Entry->ItemGuid.C != 0 || Entry->ItemGuid.D != 0))
                        bot->Pawn->EquipWeaponDefinition((UFortWeaponMeleeItemDefinition*)Entry->ItemDefinition, Entry->ItemGuid);
                }
                else {
                    Log("Default Pickaxe dont exist!");
                }
            }
            else {
                if (PickDef && PickDef->WeaponDefinition)
                {
                    bot->GiveItemBot(PickDef->WeaponDefinition);
                }

                auto Entry = bot->GetEntry(PickDef->WeaponDefinition);
                if (Entry && Entry->ItemDefinition && (Entry->ItemGuid.A != 0 || Entry->ItemGuid.B != 0 || Entry->ItemGuid.C != 0 || Entry->ItemGuid.D != 0))
                    bot->Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Entry->ItemDefinition, Entry->ItemGuid);
            }
        }

        if (BotDisplayNames.size() != 0) {
            std::srand(static_cast<unsigned int>(std::time(0)));
            int randomIndex = std::rand() % BotDisplayNames.size();
            std::string rdName = BotDisplayNames[randomIndex];
            BotDisplayNames.erase(BotDisplayNames.begin() + randomIndex);

            int size_needed = MultiByteToWideChar(CP_UTF8, 0, rdName.c_str(), (int)rdName.size(), NULL, 0);
            std::wstring wideString(size_needed, 0);
            MultiByteToWideChar(CP_UTF8, 0, rdName.c_str(), (int)rdName.size(), &wideString[0], size_needed);


            FString CVName = FString(wideString.c_str());
            GameMode->ChangeName(bot->PC, CVName, true);
            bot->DisplayName = CVName;

            bot->PlayerState->OnRep_PlayerName();
        }

        for (auto SkillSet : bot->PC->DigestedBotSkillSets)
        {
            if (!SkillSet)
                continue;

            if (auto AimingSkill = Cast<UFortAthenaAIBotAimingDigestedSkillSet>(SkillSet))
                bot->PC->CacheAimingDigestedSkillSet = AimingSkill;

            if (auto AttackingSkill = Cast<UFortAthenaAIBotAttackingDigestedSkillSet>(SkillSet))
                bot->PC->CacheAttackingSkillSet = AttackingSkill;

            if (auto HarvestSkill = Cast<UFortAthenaAIBotHarvestDigestedSkillSet>(SkillSet))
                bot->PC->CacheHarvestDigestedSkillSet = HarvestSkill;

            if (auto InventorySkill = Cast<UFortAthenaAIBotInventoryDigestedSkillSet>(SkillSet))
                bot->PC->CacheInventoryDigestedSkillSet = InventorySkill;

            if (auto LootingSkill = Cast<UFortAthenaAIBotLootingDigestedSkillSet>(SkillSet))
                bot->PC->CacheLootingSkillSet = LootingSkill;

            if (auto MovementSkill = Cast<UFortAthenaAIBotMovementDigestedSkillSet>(SkillSet))
                bot->PC->CacheMovementSkillSet = MovementSkill;

            if (auto PerceptionSkill = Cast<UFortAthenaAIBotPerceptionDigestedSkillSet>(SkillSet))
                bot->PC->CachePerceptionDigestedSkillSet = PerceptionSkill;

            if (auto PlayStyleSkill = Cast<UFortAthenaAIBotPlayStyleDigestedSkillSet>(SkillSet))
                bot->PC->CachePlayStyleSkillSet = PlayStyleSkill;
        }

        bot->PC->BehaviorTree = BehaviorTree;
        bot->PC->RunBehaviorTree(BehaviorTree);
        bot->PC->UseBlackboard(BehaviorTree->BlackboardAsset, &bot->PC->Blackboard);
        bot->PC->UseBlackboard(BehaviorTree->BlackboardAsset, &bot->PC->Blackboard1);

        static auto Name1 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhaseStep"));
        static auto Name2 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhase"));
        bot->PC->Blackboard->SetValueAsEnum(Name1, (uint8)EAthenaGamePhaseStep::Warmup);
        bot->PC->Blackboard->SetValueAsEnum(Name2, (uint8)EAthenaGamePhase::Warmup);

        // Give this bot the world nav data so MoveToActor/MoveToLocation can path. The nav
        // mesh (Apollo_Nav_Gameplay's RecastNavMesh) often hasn't finished streaming when
        // bots spawn during warmup, so EnsureWorldNavData may return null here -- the tick's
        // TryAssignNavDataToBots then back-fills every bot once the mesh registers.
        ANavigationData* BotNav = EnsureWorldNavData();
        bot->PC->PathFollowingComponent->MyNavData = BotNav;
        if (BotNav)
            bot->PC->PathFollowingComponent->OnNavDataRegistered(BotNav);
        else
        {
            static bool bLoggedNoNav = false;
            if (!bLoggedNoNav)
            {
                Log("No NavData yet (nav mesh still streaming); the tick will back-fill bots once it registers.");
                bLoggedNoNav = true;
            }
        }

        bot->PlayerState->OnRep_CharacterData();

        bot->Pawn->CapsuleComponent->SetGenerateOverlapEvents(true);
        bot->Pawn->CharacterMovement->bCanWalkOffLedges = true;

        bot->Pawn->SetMaxHealth(100);
        bot->Pawn->SetHealth(100);
        bot->Pawn->SetMaxShield(100);
        bot->Pawn->SetShield(0);

        bot->BotState = StartingState;

        if (TeamPC) {
            AFortPlayerStateAthena* TeamPS = (AFortPlayerStateAthena*)TeamPC->PlayerState;

            uint8 OldTeamIdx = bot->PlayerState->TeamIndex;

            bot->PlayerState->bIsABot = false;

            bot->PlayerState->TeamIndex = TeamPS->TeamIndex;
            bot->PlayerState->SquadId = TeamPS->SquadId;
            bot->PlayerState->OnRep_TeamIndex(OldTeamIdx);
            bot->PlayerState->OnRep_SquadId();

            TeamPS->PlayerTeam->TeamMembers.Add(bot->PC);
            bot->PlayerState->PlayerTeam = TeamPS->PlayerTeam;

            bot->PlayerState->OnRep_PlayerTeam();
            TeamPS->OnRep_PlayerTeam();

            bot->PlayerState->bIsABot = true;
        }

        PlayerBotArray.push_back(bot); // gotta do this so we can tick them all
        //Log("Bot Spawned With DisplayName: " + bot->DisplayName.ToString());
    }

    void Tick() {
        if (!PlayerBotArray.empty()) {
            if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.001f))
            {
                FreeDeadBots();
            }
        }
        else {
            return;
        }

        auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
        auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
        auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
        auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

        if (!GameState || !GameMode)
            return;

        size_t BotCount = PlayerBotArray.size();
        size_t BotStart = BotBrain::GetFrameStart();

        for (size_t BotIter = 0; BotIter < BotCount; BotIter++) {
            PlayerBot* bot = PlayerBotArray[(BotStart + BotIter) % BotCount];
            if (!bot || !bot->Pawn || !bot->PC || !bot->PlayerState)
                continue;

            bot->EnsurePawnUpright();

            if (GameState->GamePhase <= EAthenaGamePhase::Warmup) {
                if (bot->tick_counter <= 150) {
                    bot->tick_counter++;
                    continue;
                }
            }

            if (bot->BotState >= EBotState::Landed && bot->BotState != EBotState::Stuck) {
                BotBrain::Tick(bot, GameState);
                bot->tick_counter++;
                continue;
            }

            bool bAirState = bot->BotState == EBotState::Bus || bot->BotState == EBotState::Skydiving || bot->BotState == EBotState::Gliding;
            bool bAlwaysResponsiveState = bAirState || bot->BotState == EBotState::Warmup || bot->BotState == EBotState::PreBus;
            if (!bAlwaysResponsiveState && !bot->ShouldRunFullAITick(0.16f)) {
                bot->tick_counter++;
                continue;
            }

            if (bot->BotState == EBotState::Warmup) {
                BotsBTService_Warmup Warmup;
                Warmup.Tick(bot);
            }
            else if (bot->BotState == EBotState::PreBus) {
                bot->Pawn->SetHealth(100);
                bot->Pawn->SetShield(100);
                if (!bot->bHasThankedBusDriver && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.0005f))
                {
                    bot->bHasThankedBusDriver = true;
                    bot->PC->ThankBusDriver();
                }
                // PreBus -> Bus fallback. The only native promoter is OnAircraftEnteredDropZone,
                // which can be missed -- stranding bots in PreBus so they never jump. Once the
                // bus is actually flying, advance to Bus so the drop-zone tick (below) can jump
                // them off and steer them to their target POI.
                if (GameState && GameState->GamePhase >= EAthenaGamePhase::Aircraft && GameState->GetAircraft(0))
                {
                    static auto NameIsInBus = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_IsInBus"));
                    if (bot->PC && bot->PC->Blackboard)
                        bot->PC->Blackboard->SetValueAsBool(NameIsInBus, true);
                    bot->BotState = EBotState::Bus;
                }
            }
            else if (bot->BotState == EBotState::Bus || bot->BotState == EBotState::Skydiving || bot->BotState == EBotState::Gliding) {
                if (Globals::LateGame && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.5f)) {
                    if (bot->bHasJumpedFromBus) {
                        bot->BotState = EBotState::LookingForPlayers;
                        continue; // was: return (exited whole loop)
                    }

                    AActor* Aircraft = GameState->GetAircraft(0);
                    if (Aircraft) {
                        FVector AircraftLoc = Aircraft->K2_GetActorLocation();
                        FVector SpawnLoc = AircraftLoc;
                        SpawnLoc.X += (rand() % 500) - 250;
                        SpawnLoc.Y += (rand() % 500) - 250;
                        BotsBTService_AIDropZone::StartBotSkydivingFromBus(bot, SpawnLoc, {});
                    }
                    bot->GiveLategameLoadout();
                    bot->BotState = EBotState::LookingForPlayers;
                    bot->bHasJumpedFromBus = true;
                    continue; // was: return (exited whole loop)
                }

                BotsBTService_AIDropZone DropZoneEv;
                DropZoneEv.Tick(bot);
            }
            else if (bot->BotState == EBotState::Landed) {
                FVector BotLoc = bot->Pawn->K2_GetActorLocation();
                if (bot->NearestPlayerActor) {
                    float Dist = Math->Vector_Distance(BotLoc, bot->NearestPlayerActor->K2_GetActorLocation());
                    if (Dist < 6000.f) {
                        bot->BotState = EBotState::Looting;
                    }
                    else {
                        bot->BotState = EBotState::Looting;
                    }
                }
                else {
                    bot->BotState = EBotState::Looting;
                }

                if (GameState && GameState->SafeZoneIndicator && bot->TargetDropZone.IsZero()) {
                    bot->TargetDropZone = GameState->SafeZoneIndicator->NextCenter;
                }
            }
            else if (bot->BotState == EBotState::Fleeing) {
                FVector BotLoc = bot->Pawn->K2_GetActorLocation();
                FVector Nearest = bot->FindNearestPlayerOrBot();
                if (!Nearest.IsZero()) {
                    float Dist = Math->Vector_Distance(BotLoc, Nearest);
                    if (bot->HasGun()) {
                        bot->BotState = EBotState::LookingForPlayers;
                        continue;
                    }

                    if (Dist < 3000.f) {
                        auto TestRot = Math->FindLookAtRotation(Nearest, BotLoc);

                        bot->SetLookRotation(TestRot);
                        FVector AwayFromEnemy = BotLoc + (BotLoc - Nearest).Normalize() * 1200.f;
                        bot->MoveToLocationThrottled(AwayFromEnemy, 200.f, 0.6f);
                        bot->DetectWorldGeoStuck(BotLoc, nullptr, AwayFromEnemy);
                    }
                    else {
                        bot->BotState = EBotState::Looting;
                    }
                }
                else {
                    bot->BotState = EBotState::Looting;
                }
            }
            else if (bot->BotState == EBotState::Looting) {
                FVector BotLoc = bot->Pawn->K2_GetActorLocation();
                bot->DetectWorldGeoStuck(BotLoc, nullptr, bot->TargetDropZone);
                BotsBTService_Loot LootAI;
                LootAI.Tick(bot);
            }
            else if (bot->BotState == EBotState::LookingForPlayers) {
                if (!bot->HasGun()) {
                    bot->BotState = EBotState::Looting;
                }
                if (bot->IsPickaxeEquiped()) {
                    bot->SimpleSwitchToWeapon();
                }

                FVector BotLoc = bot->Pawn->K2_GetActorLocation();
                bot->DetectWorldGeoStuck(BotLoc, bot->NearestPlayerActor, bot->LastKnownEnemyLocation);

                if (bot->ShouldScanObstacles(1.2f)) {
                    TArray<PlayerBot::ObstacleInfo> Obstacles;
                    bot->ScanForObstacles3D(Obstacles, 400.f);

                    for (auto& Obs : Obstacles) {
                        if (Obs.bIsJumpable && Obs.Distance < 300.f) {
                            bot->TryJumpOverObstacle(Obs);
                            break;
                        }
                        if (!Obs.bIsJumpable && Obs.VerticalOffset >= 200.f && Obs.VerticalOffset < 600.f && Obs.Distance < 400.f) {
                            int Wood = BotBrain::GetWood(bot);
                            if (Wood >= 20) {
                                FVector SnapBase = bot->SnapBuildLocation(BotLoc);
                                FRotator FaceRot = bot->SnapBuildRotation(bot->PC->GetControlRotation());
                                FVector Forward = bot->Pawn->GetActorForwardVector();
                                UClass* RampClass = APBWA_W1_StairW_C::StaticClass();
                                if (RampClass)
                                    bot->SpawnBotBuild(RampClass, SnapBase + Forward * 256.f, FaceRot);
                            }
                            break;
                        }
                    }
                }

                if (bot->NearestPlayerActor) {
                    FVector Nearest = bot->NearestPlayerActor->K2_GetActorLocation();

                    if (!Nearest.IsZero()) {
                        float Dist = Math->Vector_Distance(BotLoc, Nearest);

                        if (bot->PC->LineOfSightTo(bot->NearestPlayerActor, BotLoc, true)) {
                            bot->LastKnownEnemyLocation = Nearest;
                            bot->LastKnownEnemyTime = Statics->GetTimeSeconds(UWorld::GetWorld());

                            if (bot->tick_counter % 3 == 0) {
                                float RandomXmod = Math->RandomFloatInRange(-30, 30);
                                float RandomYmod = Math->RandomFloatInRange(-30, 30);
                                float RandomZmod = Math->RandomFloatInRange(-20, 20);

                                FVector TargetPosMod{ Nearest.X + RandomXmod, Nearest.Y + RandomYmod, Nearest.Z + RandomZmod };

                                FRotator Rot = Math->FindLookAtRotation(BotLoc, TargetPosMod);

                                bot->SetLookRotation(Rot);
                            }

                            if (bot->tick_counter % 20 == 0) {
                                if (!bot->Pawn->bIsCrouched && Math->RandomBoolWithWeight(0.03f)) {
                                    bot->Pawn->Crouch(false);
                                }
                            }

                            if (!bot->bIsStressed) {
                                if (Dist > 600.f && bot->tick_counter % 8 == 0)
                                    bot->MoveToActorThrottled(bot->NearestPlayerActor, Math->RandomFloatInRange(400.f, 650.f), 0.8f);
                            }
                            else {
                                if (!bot->LastKnownEnemyLocation.IsZero() && bot->tick_counter % 8 == 0) {
                                    FVector AwayDir = BotLoc + (BotLoc - Nearest).Normalize() * 800.f;
                                    bot->MoveToLocationThrottled(AwayDir, 200.f, 0.8f);
                                }
                            }

                            if (bot->tick_counter % 30 == 0)
                                bot->ForceStrafe(true);
                            if (bot->bIsCurrentlyStrafing)
                                bot->ForceStrafe(false);

                            if (bot->PC->LineOfSightTo(bot->NearestPlayerActor, BotLoc, true)) {
                                bot->Pawn->PawnStartFire(0);
                            }
                            else {
                                bot->Pawn->PawnStopFire(0);
                            }
                        }
                        else {
                            float Now = Statics->GetTimeSeconds(UWorld::GetWorld());
                            if (!bot->LastKnownEnemyLocation.IsZero() && Now - bot->LastKnownEnemyTime < 8.f) {
                                bot->Pawn->PawnStopFire(0);
                                bot->MoveToLocationThrottled(bot->LastKnownEnemyLocation, 450.f, 1.0f);
                                bot->TryDropDown(bot->LastKnownEnemyLocation);
                            }
                            else {
                                bot->BotState = EBotState::MovingToSafeZone;
                            }
                        }
                    }
                }
                else {
                    FVector ZoneTarget = FVector();
                    if (GameState && GameState->SafeZoneIndicator) {
                        ZoneTarget = GameState->SafeZoneIndicator->NextCenter;
                    }

                    if (!ZoneTarget.IsZero() && bot->tick_counter % 10 == 0) {
                        bot->MoveToLocationThrottled(ZoneTarget, 500.f, 1.0f);
                    }
                }
            }
            else if (bot->BotState == EBotState::MovingToSafeZone) {
                FVector BotLoc = bot->Pawn->K2_GetActorLocation();

                LearningStats& Learn = BotLearning[bot];

                if (bot->NearestPlayerActor) {
                    float Dist = Math->Vector_Distance(BotLoc, bot->NearestPlayerActor->K2_GetActorLocation());
                    float StormDist = 10000.f;

                    if (GameState && GameState->SafeZoneIndicator) {
                        FVector ZoneCenter = GameState->SafeZoneIndicator->NextCenter;
                        StormDist = Math->Vector_Distance(BotLoc, ZoneCenter);
                    }

                    if (Dist < StormDist * 0.3f && bot->HasGun()) {
                        bot->BotState = EBotState::LookingForPlayers;
                    }
                    else if (Dist < 1000.f && Learn.bLearnedPrioritizeZone) {
                        bot->BotState = EBotState::LookingForPlayers;
                    }
                }

                if (GameState && GameState->SafeZoneIndicator)
                {
                    FVector ZoneTarget = GameState->SafeZoneIndicator->NextCenter;
                    float ZoneRadius = GameState->SafeZoneIndicator->Radius;

                    if (ZoneTarget.IsZero()) {
                        ZoneTarget = bot->TargetDropZone;
                    }

                    if (!ZoneTarget.IsZero()) {
                        bot->DetectWorldGeoStuck(BotLoc, nullptr, ZoneTarget);
                        FVector ToZone = (ZoneTarget - BotLoc).Normalize();

                        if (bot->tick_counter % 3 == 0) {
                            FRotator TargetRot = Math->FindLookAtRotation(BotLoc, ZoneTarget);
                            TargetRot.Pitch = 0.f;
                            bot->SetLookRotation(TargetRot);
                        }

                        if (bot->tick_counter % 5 == 0) {
                            bot->MoveToLocationThrottled(ZoneTarget, ZoneRadius * 0.8f, 1.0f);
                        }

                        bot->TryDropDown(ZoneTarget);

                        FVector Vel = bot->Pawn->GetVelocity();
                        float Speed = sqrtf((Vel.X * Vel.X) + (Vel.Y * Vel.Y));
                        if (Speed < 50.f && bot->ConsecutiveWallStuckTicks == 0) {
                            bot->Pawn->AddMovementInput(ToZone, 0.4f, true);
                        }

                        if (bot->ShouldScanObstacles(1.4f)) {
                            TArray<PlayerBot::ObstacleInfo> Obstacles;
                            bot->ScanForObstacles3D(Obstacles, 500.f);

                            if (Obstacles.Num() > 0) {
                                PlayerBot::ObstacleInfo& Nearest = Obstacles[0];

                                if (Nearest.bIsJumpable && Nearest.Distance < 300.f) {
                                    bot->TryJumpOverObstacle(Nearest);
                                    bot->Pawn->PawnStopFire(0);
                                }
                                else if (!Nearest.bIsJumpable && Nearest.Distance < 250.f) {
                                    ABuildingSMActor* BuildingActor = Cast<ABuildingSMActor>(Nearest.Actor);
                                    if (!BuildingActor) {
                                        FVector Right = bot->Pawn->GetActorRightVector();
                                        float Side = (bot->tick_counter % 2 == 0) ? 1.f : -1.f;
                                        bot->Pawn->AddMovementInput(Right * Side, 0.65f, true);
                                        if (bot->tick_counter % 6 == 0) bot->Pawn->Jump();
                                    }
                                    else {
                                        if (!bot->IsPickaxeEquiped()) {
                                            bot->EquipPickaxe();
                                        }
                                        FRotator WallRot = Math->FindLookAtRotation(BotLoc, Nearest.Actor->K2_GetActorLocation());
                                        bot->SetLookRotation(WallRot);
                                        float H = BuildingActor->GetHealth();
                                        if (H > 0.f && H < 1500.f) {
                                            bot->Pawn->PawnStartFire(0);
                                        }
                                        else {
                                            FVector Right = bot->Pawn->GetActorRightVector();
                                            float Side = (bot->tick_counter % 2 == 0) ? 1.f : -1.f;
                                            bot->Pawn->AddMovementInput(Right * Side, 0.65f, true);
                                            if (bot->tick_counter % 6 == 0) bot->Pawn->Jump();
                                        }
                                    }
                                }
                                else if (Nearest.Distance < 500.f) {
                                    bot->Pawn->PawnStopFire(0);

                                    FVector ToObs = (Nearest.Actor->K2_GetActorLocation() - BotLoc).Normalize();
                                    ToObs.Z = 0;
                                    FVector AvoidDir = ToObs;
                                    FVector Forward = bot->Pawn->GetActorForwardVector();
                                    FVector Perpendicular = FVector(-AvoidDir.Y, AvoidDir.X, 0);

                                    float DotForward = Forward.X * AvoidDir.X + Forward.Y * AvoidDir.Y;
                                    if (DotForward < 0) {
                                        Perpendicular = FVector(AvoidDir.Y, -AvoidDir.X, 0);
                                    }

                                    if (bot->tick_counter % 3 == 0) {
                                        bot->Pawn->AddMovementInput(Perpendicular, 0.65f, true);
                                    }
                                }
                                else {
                                    bot->Pawn->PawnStopFire(0);
                                }
                            }
                        }
                    }
                }
            }
            else if (bot->BotState == EBotState::Stuck) {
                bool bBreakingStuckActor = false;

                float CurrentTime = Statics->GetTimeSeconds(UWorld::GetWorld());
                if (bot->StuckStartTime == 0.f) {
                    bot->StuckStartTime = CurrentTime;
                }

                float StuckDuration = CurrentTime - bot->StuckStartTime;

                if (bot->NearestPlayerActor && bot->HasGun()) {
                    bot->Pawn->PawnStopFire(0);
                    bot->StuckBuildingActor = nullptr;
                    bot->StuckStartTime = 0.f;
                    bot->BotState = EBotState::LookingForPlayers;
                    bot->tick_counter++;
                    continue;
                }

                FVector BotLoc = bot->Pawn->K2_GetActorLocation();
                FVector Forward = bot->Pawn->GetActorForwardVector();

                bot->Pawn->UnCrouch(false);

                if (!bot->IsPickaxeEquiped()) {
                    bot->EquipPickaxe();
                }

                TArray<PlayerBot::ObstacleInfo> Obstacles;
                if (bot->ShouldScanObstacles(0.55f)) {
                    bot->ScanForObstacles3D(Obstacles, 500.f);
                }

                if (Obstacles.Num() > 0) {
                    PlayerBot::ObstacleInfo& Nearest = Obstacles[0];
                    FVector WallLoc = Nearest.Actor->K2_GetActorLocation();
                    FRotator WallRot = Math->FindLookAtRotation(BotLoc, WallLoc);
                    bot->SetLookRotation(WallRot);

                    if (Nearest.bIsJumpable) {
                        bot->TryJumpOverObstacle(Nearest);
                        bot->Pawn->AddMovementInput(Forward, 1.0f, true);
                        bBreakingStuckActor = false;
                    }
else if (Nearest.Distance < 250.f) {
						ABuildingSMActor* BuildingActor = Cast<ABuildingSMActor>(Nearest.Actor);
						if (!BuildingActor)
						{
							FVector Right = Forward * -1.f;
							float Side = (bot->tick_counter % 2 == 0) ? 1.f : -1.f;
							bot->Pawn->AddMovementInput(bot->Pawn->GetActorRightVector() * Side, 0.65f, true);
							if (bot->tick_counter % 6 == 0)
								bot->Pawn->Jump();
						}
						else if (BuildingActor->GetHealth() > 0 && BuildingActor->GetHealth() < 1500) {
                            bot->Pawn->PawnStartFire(0);
                            bBreakingStuckActor = true;

                            if (BuildingActor->GetHealth() <= 10.f) {
                                bot->Pawn->PawnStopFire(0);
                                bot->StuckBuildingActor = nullptr;
                                bot->StuckStartTime = 0.f;
                                bot->BotState = bot->CachedBotState;
                                continue;
                            }
                        }
                        else if (BuildingActor->GetHealth() <= 0 || BuildingActor->GetHealth() >= 1500) {
							FVector Right = bot->Pawn->GetActorRightVector();
							float Side = (bot->tick_counter % 2 == 0) ? 1.f : -1.f;
							bot->Pawn->AddMovementInput(Right * Side, 0.65f, true);
							if (bot->tick_counter % 6 == 0)
								bot->Pawn->Jump();
						}
						else if (Nearest.Distance < 100.f) {
                            FVector BackDir = Forward * -1.f;
                            FVector SlideTarget = BotLoc + BackDir * 200.f;
                            SlideTarget.X += (rand() % 200) - 100;
                            SlideTarget.Y += (rand() % 200) - 100;
                            bot->MoveToLocationThrottled(SlideTarget, 50.f, 0.5f);
                        }
                    }
                    else {
                        if (bot->tick_counter % 3 == 0) {
                            FVector ApproachTarget = WallLoc - (WallLoc - BotLoc).Normalize() * 150.f;
                            bot->MoveToLocationThrottled(ApproachTarget, 50.f, 0.5f);
                        }
                        bBreakingStuckActor = false;
                    }
                }
                else {
                    FVector JumpVel = (Forward * 300.f) + FVector(0, 0, 250.f);
                    bot->Pawn->LaunchCharacter(JumpVel, true, false);
                    bot->Pawn->AddMovementInput(Forward, 0.5f, true);

                    if (StuckDuration > 1.2f && !bot->TargetDropZone.IsZero()) {
                        bot->TryDropDown(bot->TargetDropZone);
                    }

                    if (bot->tick_counter % 5 == 0) {
                        bot->ForceStrafe(true);
                    }
                    bBreakingStuckActor = false;
                }

                if (StuckDuration > 3.f && bot->tick_counter % 3 == 0) {
                    bot->Pawn->PawnStopFire(0);

                    auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
                    FVector ZoneTarget = FVector();
                    float ZoneRadius = 10000.f;
                    if (GameState && GameState->SafeZoneIndicator) {
                        ZoneTarget = GameState->SafeZoneIndicator->NextCenter;
                        ZoneRadius = GameState->SafeZoneIndicator->Radius;
                    }

                    float RandomAngle = float(rand() % 360) * 3.14159f / 180.f;
                    float TeleportDist = 500.f + float(rand() % 500);

                    FVector Right = bot->Pawn->GetActorRightVector();
                    float Side = (rand() % 2 == 0) ? 1.f : -1.f;
                    FVector TeleportOffset = (Forward * TeleportDist) + (Right * TeleportDist * 0.5f * Side);
                    FVector NewLoc = BotLoc + TeleportOffset;

                    if (!ZoneTarget.IsZero()) {
                        float DistToZone = Math->Vector_Distance(NewLoc, ZoneTarget);
                        if (DistToZone > ZoneRadius * 0.9f) {
                            FVector ConstrainDir = (NewLoc - ZoneTarget).Normalize();
                            NewLoc = ZoneTarget + ConstrainDir * ZoneRadius * 0.85f;
                        }
                    }

                    NewLoc.Z = BotLoc.Z + 50.f;

                    bot->Pawn->K2_TeleportTo(NewLoc, bot->UprightRotation(bot->Pawn->K2_GetActorRotation()));

                    bot->StuckBuildingActor = nullptr;
                    bot->StuckStartTime = 0.f;
                    bot->BotState = bot->CachedBotState;
                    continue;
                }

                bot->tick_counter++;
                continue;
            }
            
            bot->tick_counter++;
        }
    }
}
