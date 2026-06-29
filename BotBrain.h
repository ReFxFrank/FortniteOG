#pragma once

struct BotNeuralNet
{
    float W1[24][20];
    float B1[24];
    float W2[10][24];
    float B2[10];
    bool bInitialized = false;

    float SeedWeight(int A, int B, int Salt, float Scale)
    {
        unsigned int X = 2166136261u;
        X ^= (unsigned int)(A + 31 * Salt); X *= 16777619u;
        X ^= (unsigned int)(B + 131 * Salt); X *= 16777619u;
        X ^= (X >> 13);
        X *= 1274126177u;
        float Unit = float(X % 2001) / 1000.f - 1.f;
        return Unit * Scale;
    }

    void Initialize()
    {
        if (bInitialized)
            return;

        float Xavier1 = sqrtf(6.f / 44.f);
        float Xavier2 = sqrtf(6.f / 34.f);

        for (int H = 0; H < 24; H++)
        {
            B1[H] = 0.f;
            for (int I = 0; I < 20; I++)
            {
                float Scale = Xavier1 * 0.45f;
                bool bCombat = H < 6;
                bool bResource = H >= 6 && H < 12;
                bool bZone = H >= 12 && H < 18;
                bool bOpportunity = H >= 18;

                if (bCombat && (I == 7 || I == 8 || I == 10 || I == 11 || I == 12 || I == 13))
                    Scale = Xavier1 * 1.45f;
                else if (bResource && (I <= 6 || I == 9 || I == 19))
                    Scale = Xavier1 * 1.25f;
                else if (bZone && (I == 14 || I == 15 || I == 16 || I == 17))
                    Scale = Xavier1 * 1.35f;
                else if (bOpportunity && (I == 9 || I == 17 || I == 18 || I == 19))
                    Scale = Xavier1 * 1.35f;

                W1[H][I] = SeedWeight(H, I, 11, Scale);
            }
        }

        for (int O = 0; O < 10; O++)
        {
            B2[O] = 0.f;
            for (int H = 0; H < 24; H++)
                W2[O][H] = SeedWeight(O, H, 29, Xavier2 * 0.35f);
        }

        for (int H = 0; H < 6; H++)
        {
            W2[(int)BotAction::PUSH_AGGRESSIVE][H] += 0.45f;
            W2[(int)BotAction::BUILD_RAMP_RUSH][H] += 0.35f;
            W2[(int)BotAction::HOLD_SHOOT_AR][H] += 0.38f;
            W2[(int)BotAction::SPRAY_AND_PRAY][H] += 0.22f;
        }
        for (int H = 6; H < 12; H++)
        {
            W2[(int)BotAction::RETREAT_HEAL][H] += 0.38f;
            W2[(int)BotAction::BUILD_BOX][H] += 0.30f;
            W2[(int)BotAction::LOOT_NEARBY][H] += 0.20f;
        }
        for (int H = 12; H < 18; H++)
        {
            W2[(int)BotAction::ROTATE_ZONE][H] += 0.42f;
            W2[(int)BotAction::BUILD_HIGHGROUND][H] += 0.28f;
            W2[(int)BotAction::EDIT_PEEK][H] += 0.18f;
        }
        for (int H = 18; H < 24; H++)
        {
            W2[(int)BotAction::LOOT_NEARBY][H] += 0.45f;
            W2[(int)BotAction::RETREAT_HEAL][H] += 0.22f;
            W2[(int)BotAction::EDIT_PEEK][H] += 0.16f;
        }

        B2[(int)BotAction::HOLD_SHOOT_AR] += 0.4f;
        B2[(int)BotAction::BUILD_BOX] += 0.3f;
        bInitialized = true;
    }

    void Forward(float Inputs[20], float Outputs[10])
    {
        Initialize();

        float Hidden[24];
        for (int H = 0; H < 24; H++)
        {
            float Sum = B1[H];
            for (int I = 0; I < 20; I++)
                Sum += W1[H][I] * Inputs[I];
            Hidden[H] = Sum > 0.f ? Sum : 0.f;
        }

        float Logits[10];
        float MaxLogit = -FLT_MAX;
        for (int O = 0; O < 10; O++)
        {
            float Sum = B2[O];
            for (int H = 0; H < 24; H++)
                Sum += W2[O][H] * Hidden[H];

            if (O == (int)BotAction::RETREAT_HEAL && Inputs[0] < 0.3f)
                Sum += 0.2f;

            Logits[O] = Sum;
            if (Sum > MaxLogit)
                MaxLogit = Sum;
        }

        float Total = 0.f;
        for (int O = 0; O < 10; O++)
        {
            Outputs[O] = expf(Logits[O] - MaxLogit);
            Total += Outputs[O];
        }

        if (Total <= 0.f)
            Total = 1.f;

        for (int O = 0; O < 10; O++)
            Outputs[O] /= Total;
    }
};

namespace BotBrain
{
    static std::map<PlayerBot*, BotNeuralNet> BotNets;
    static std::map<PlayerBot*, float> ActionCooldownUntil;
    static std::map<PlayerBot*, FVector> ExploreTargets;
    static std::map<PlayerBot*, float> ExploreTargetUntil;
    static std::map<PlayerBot*, FVector> ObstacleLastPos;
    static std::map<PlayerBot*, float> ObstacleStuckSince;
    static TArray<AActor*> CachedWalls;
    static TArray<AActor*> CachedPickups;
    static float NextWorldScanTime = 0.f;
    static int FrameBrainBudget = 5;
    static size_t RoundRobinCursor = 0;
    static size_t RoundRobinStart = 0;

    inline void RefreshWorldCache(float Now)
    {
        if (Now < NextWorldScanTime)
            return;

        NextWorldScanTime = Now + 0.75f;
        CachedWalls.Free();
        CachedPickups.Free();

        UWorld* World = UWorld::GetWorld();
        if (!World)
            return;

        TArray<AActor*> AllWalls;
        UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(World, ABuildingSMActor::StaticClass(), &AllWalls);
        for (int I = 0; I < AllWalls.Num(); I++)
        {
            ABuildingSMActor* Wall = (ABuildingSMActor*)AllWalls[I];
            if (Wall && Wall->bPlayerPlaced)
                CachedWalls.Add(Wall);
        }
        AllWalls.Free();

        UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(World, AFortPickupAthena::StaticClass(), &CachedPickups);
    }

    inline void CleanupDeadBotData(PlayerBot* Bot)
    {
        BotNets.erase(Bot);
        ActionCooldownUntil.erase(Bot);
        ExploreTargets.erase(Bot);
        ExploreTargetUntil.erase(Bot);
        ObstacleLastPos.erase(Bot);
        ObstacleStuckSince.erase(Bot);
    }
    inline bool IsValidGuid(const FGuid& Guid)
    {
        return Guid.A != 0 || Guid.B != 0 || Guid.C != 0 || Guid.D != 0;
    }

    inline bool SafeEquipPickaxe(PlayerBot* Bot)
    {
        if (!Bot || !Bot->Pawn || !Bot->PC || !Bot->PC->Inventory)
            return false;

        auto& Entries = Bot->PC->Inventory->Inventory.ReplicatedEntries;
        for (int I = 0; I < Entries.Num(); I++)
        {
            auto& Entry = Entries[I];
            if (Entry.ItemDefinition && Entry.ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) && IsValidGuid(Entry.ItemGuid))
            {
                Bot->Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Entry.ItemDefinition, Entry.ItemGuid);
                return true;
            }
        }
        return false;
    }

    inline float Clamp01(float Value)
    {
        if (Value < 0.f) return 0.f;
        if (Value > 1.f) return 1.f;
        return Value;
    }

    inline bool NameContains(UFortItemDefinition* Def, const char* Token)
    {
        if (!Def || !Token)
            return false;
        std::string Name = Def->Name.ToString();
        return Name.find(Token) != std::string::npos;
    }

    inline bool IsARName(UFortItemDefinition* Def)
    {
        return NameContains(Def, "Assault") || NameContains(Def, "Rifle") || NameContains(Def, "Burst") || NameContains(Def, "AR");
    }

    inline bool IsShotgunName(UFortItemDefinition* Def)
    {
        return NameContains(Def, "Shotgun");
    }

    inline bool IsGunName(UFortItemDefinition* Def)
    {
        return IsARName(Def) || IsShotgunName(Def) ||
               NameContains(Def, "SMG") || NameContains(Def, "Sniper") ||
               NameContains(Def, "Rocket") || NameContains(Def, "Pistol");
    }

    inline bool HasAnyWeapon(PlayerBot* Bot)
    {
        if (!Bot || !Bot->PC || !Bot->PC->Inventory)
            return false;

        auto& Entries = Bot->PC->Inventory->Inventory.ReplicatedEntries;
        for (int I = 0; I < Entries.Num(); I++)
        {
            UFortItemDefinition* Def = Entries[I].ItemDefinition;
            if (Def && IsGunName(Def))
                return true;
        }
        return false;
    }

    inline bool IsShieldName(UFortItemDefinition* Def)
    {
        return NameContains(Def, "Shield") || NameContains(Def, "Slurp") || NameContains(Def, "SmallShield") || NameContains(Def, "Mushroom");
    }

    inline bool IsHealName(UFortItemDefinition* Def)
    {
        return IsShieldName(Def) || NameContains(Def, "Bandage") || NameContains(Def, "Medkit") || NameContains(Def, "Healing") || NameContains(Def, "Flop");
    }

    inline int GetInventoryCount(PlayerBot* Bot, const char* A, const char* B = nullptr, const char* C = nullptr)
    {
        if (!Bot || !Bot->PC || !Bot->PC->Inventory)
            return 0;

        int Count = 0;
        auto& Entries = Bot->PC->Inventory->Inventory.ReplicatedEntries;
        for (int I = 0; I < Entries.Num(); I++)
        {
            UFortItemDefinition* Def = Entries[I].ItemDefinition;
            if (!Def)
                continue;

            if (NameContains(Def, A) || (B && NameContains(Def, B)) || (C && NameContains(Def, C)))
                Count += Entries[I].Count;
        }
        return Count;
    }

    inline bool HasWeaponOfType(PlayerBot* Bot, bool bShotgun)
    {
        if (!Bot || !Bot->PC || !Bot->PC->Inventory)
            return false;

        auto& Entries = Bot->PC->Inventory->Inventory.ReplicatedEntries;
        for (int I = 0; I < Entries.Num(); I++)
        {
            UFortItemDefinition* Def = Entries[I].ItemDefinition;
            if (!Def)
                continue;

            if (bShotgun ? IsShotgunName(Def) : IsARName(Def))
                return true;
        }
        return false;
    }

    inline FFortItemEntry* FindEntry(PlayerBot* Bot, bool bShotgun, bool bHeal, bool bShieldOnly = false)
    {
        if (!Bot || !Bot->PC || !Bot->PC->Inventory)
            return nullptr;

        auto& Entries = Bot->PC->Inventory->Inventory.ReplicatedEntries;
        for (int I = 0; I < Entries.Num(); I++)
        {
            UFortItemDefinition* Def = Entries[I].ItemDefinition;
            if (!Def || Entries[I].Count <= 0)
                continue;

            if (bHeal)
            {
                if ((bShieldOnly && IsShieldName(Def)) || (!bShieldOnly && IsHealName(Def)))
                    return &Entries[I];
            }
            else if (bShotgun ? IsShotgunName(Def) : IsARName(Def))
            {
                return &Entries[I];
            }
        }
        return nullptr;
    }

    inline void EquipEntry(PlayerBot* Bot, FFortItemEntry* Entry)
    {
        if (!Bot || !Bot->Pawn || !Entry || !Entry->ItemDefinition)
            return;

        UFortWeaponItemDefinition* DefToEquip = (UFortWeaponItemDefinition*)Entry->ItemDefinition;
        if (Entry->ItemDefinition->IsA(UFortGadgetItemDefinition::StaticClass()))
            DefToEquip = ((UFortGadgetItemDefinition*)Entry->ItemDefinition)->GetWeaponItemDefinition();

        if (DefToEquip && IsValidGuid(Entry->ItemGuid))
            Bot->Pawn->EquipWeaponDefinition(DefToEquip, Entry->ItemGuid);
    }

    inline void SwitchToBestGun(PlayerBot* Bot)
    {
        FFortItemEntry* Entry = FindEntry(Bot, false, false);
        if (!Entry) Entry = FindEntry(Bot, true, false);
        if (!Entry)
        {
            auto& Entries = Bot->PC->Inventory->Inventory.ReplicatedEntries;
            for (int I = 0; I < Entries.Num(); I++)
            {
                UFortItemDefinition* Def = Entries[I].ItemDefinition;
                if (Def && IsGunName(Def) && Entries[I].Count > 0)
                {
                    Entry = &Entries[I];
                    break;
                }
            }
        }
        EquipEntry(Bot, Entry);
    }

    inline void SwitchToAR(PlayerBot* Bot)
    {
        SwitchToBestGun(Bot);
    }

    inline void SwitchToShotgun(PlayerBot* Bot)
    {
        FFortItemEntry* Entry = FindEntry(Bot, true, false);
        if (!Entry) SwitchToBestGun(Bot);
        else EquipEntry(Bot, Entry);
    }

    inline void SelectWeaponForRange(PlayerBot* Bot, float DistToEnemy)
    {
        if (!Bot || !Bot->PC || !Bot->PC->Inventory)
            return;

        auto& Entries = Bot->PC->Inventory->Inventory.ReplicatedEntries;
        FFortItemEntry* BestLoadedEntry = nullptr;
        FFortItemEntry* BestFallbackEntry = nullptr;
        int BestLoadedPriority = -1;
        int BestFallbackPriority = -1;

        for (int I = 0; I < Entries.Num(); I++)
        {
            UFortItemDefinition* Def = Entries[I].ItemDefinition;
            if (!Def || Entries[I].Count <= 0 || !IsGunName(Def))
                continue;

            bool bIsShotgun = IsShotgunName(Def);
            bool bIsAR = IsARName(Def);
            bool bIsSMG = NameContains(Def, "SMG");
            bool bIsSniper = NameContains(Def, "Sniper");
            bool bIsRocket = NameContains(Def, "Rocket");
            bool bIsPistol = NameContains(Def, "Pistol");
            int Priority = 0;

            if (DistToEnemy < 250.f)
            {
                if (bIsShotgun) Priority = 5;
                else if (bIsSMG) Priority = 4;
                else if (bIsPistol) Priority = 3;
                else if (bIsAR) Priority = 2;
                else if (bIsRocket) Priority = 1;
            }
            else if (DistToEnemy < 600.f)
            {
                if (bIsSMG) Priority = 5;
                else if (bIsAR) Priority = 4;
                else if (bIsShotgun) Priority = 3;
                else if (bIsPistol) Priority = 2;
                else if (bIsRocket) Priority = 1;
            }
            else if (DistToEnemy < 1800.f)
            {
                if (bIsAR) Priority = 5;
                else if (bIsSMG) Priority = 4;
                else if (bIsPistol) Priority = 3;
                else if (bIsShotgun) Priority = 2;
                else if (bIsRocket && DistToEnemy > 400.f) Priority = 1;
            }
            else
            {
                if (bIsSniper) Priority = 5;
                else if (bIsAR) Priority = 4;
                else if (bIsSMG) Priority = 3;
                else if (bIsPistol) Priority = 2;
            }

            if (Priority <= 0)
                continue;

            if (Entries[I].LoadedAmmo > 0)
            {
                if (Priority > BestLoadedPriority)
                {
                    BestLoadedPriority = Priority;
                    BestLoadedEntry = &Entries[I];
                }
            }
            else if (Priority > BestFallbackPriority)
            {
                BestFallbackPriority = Priority;
                BestFallbackEntry = &Entries[I];
            }
        }

        if (BestLoadedEntry)
            EquipEntry(Bot, BestLoadedEntry);
        else if (BestFallbackEntry)
            EquipEntry(Bot, BestFallbackEntry);
    }

    inline int GetWood(PlayerBot* Bot)
    {
        return GetInventoryCount(Bot, "Wood");
    }

    inline bool CanBuild(PlayerBot* Bot)
    {
        return GetWood(Bot) >= 20;
    }

    inline bool HasFirableWeapon(PlayerBot* Bot)
    {
        return Bot && Bot->Pawn && !Bot->IsPickaxeEquiped() && HasAnyWeapon(Bot);
    }

    inline bool HasAmmoLoaded(PlayerBot* Bot)
    {
        if (!Bot || !Bot->Pawn || !Bot->Pawn->CurrentWeapon || !Bot->PC || !Bot->PC->Inventory)
            return false;

        auto& Entries = Bot->PC->Inventory->Inventory.ReplicatedEntries;
        for (int I = 0; I < Entries.Num(); I++)
        {
            if (Entries[I].ItemDefinition == Bot->Pawn->CurrentWeapon->WeaponData)
                return Entries[I].LoadedAmmo > 0;
        }
        return true;
    }

    inline bool CanFireNow(PlayerBot* Bot, float DistToEnemy)
    {
        if (!HasFirableWeapon(Bot))
            return false;

        if (!HasAmmoLoaded(Bot))
        {
            SelectWeaponForRange(Bot, DistToEnemy);
            return HasAmmoLoaded(Bot);
        }
        return true;
    }

    inline void TryWeaponSwapOnEmpty(PlayerBot* Bot, float DistToEnemy, float Now)
    {
        if (!Bot || !Bot->Pawn || !Bot->Pawn->CurrentWeapon)
            return;
        if ((Now - Bot->LastWeaponSwitchTime) < 0.5f)
            return;

        if (!HasAmmoLoaded(Bot))
        {
            SelectWeaponForRange(Bot, DistToEnemy);
            Bot->LastWeaponSwitchTime = Now;
        }
    }

    inline bool IsDeadOrDownedPawn(AActor* Target)
    {
        if (!Target || !Target->IsA(AFortPlayerPawnAthena::StaticClass()))
            return false;

        AFortPlayerPawnAthena* Pawn = (AFortPlayerPawnAthena*)Target;
        return Pawn->GetHealth() <= 0.f || Pawn->bIsDBNO;
    }

    inline AActor* ResolveTarget(PlayerBot* Bot, float Now)
    {
        if (!Bot || !Bot->Pawn || !Bot->PC)
            return nullptr;

        AActor* VisibleTarget = CombatAI::FindNearestEnemy(Bot);
        if (VisibleTarget)
        {
            Bot->BrainTarget = VisibleTarget;
            Bot->NearestPlayerActor = VisibleTarget;
            Bot->LastKnownEnemyPos = VisibleTarget->K2_GetActorLocation();
            Bot->LastKnownEnemyLocation = Bot->LastKnownEnemyPos;
            Bot->LastEnemySeenTime = Now;
            Bot->LastKnownEnemyTime = Now;
            return VisibleTarget;
        }

        if (Bot->BrainTarget)
        {
            if (IsDeadOrDownedPawn(Bot->BrainTarget))
            {
                Bot->PickupAllItemsNear(Bot->LastKnownEnemyPos, 900.f);
                Bot->BrainTarget = nullptr;
                Bot->NearestPlayerActor = nullptr;
                Bot->CurrentAction = BotAction::LOOT_NEARBY;
            }
        }

        if (Bot->NearestPlayerActor && Now - Bot->LastEnemySeenTime <= 4.f)
        {
            if (IsDeadOrDownedPawn(Bot->NearestPlayerActor))
                Bot->NearestPlayerActor = nullptr;

            if (Bot->NearestPlayerActor)
                return Bot->NearestPlayerActor;
        }

        if (!Bot->NearestPlayerActor || Bot->HasTimerElapsed(Bot->NextTargetScanTime, 0.9f, 0.35f))
        {
            Bot->NearestPlayerActor = Bot->GetNearestPlayerActor();
            if (IsDeadOrDownedPawn(Bot->NearestPlayerActor))
                Bot->NearestPlayerActor = nullptr;
        }

        return Bot->NearestPlayerActor;
    }

    inline float Dist(PlayerBot* Bot, FVector A, FVector B)
    {
        auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
        return Math->Vector_Distance(A, B);
    }

    inline float HorizontalDist(FVector A, FVector B)
    {
        float DX = A.X - B.X;
        float DY = A.Y - B.Y;
        return sqrtf((DX * DX) + (DY * DY));
    }

    inline FVector PickExploreTarget(PlayerBot* Bot, AFortGameStateAthena* GameState, float Now)
    {
        if (!Bot || !Bot->Pawn)
            return FVector();

        FVector BotLoc = Bot->Pawn->K2_GetActorLocation();
        FVector Current = ExploreTargets[Bot];
        bool bNeedNew = Current.IsZero() || Now >= ExploreTargetUntil[Bot] || HorizontalDist(BotLoc, Current) < 650.f;

        if (!bNeedNew)
            return Current;

        float BestScore = -FLT_MAX;
        FVector Best = FVector();
        FVector ZoneCenter = FVector();
        float ZoneRadius = 0.f;
        if (GameState && GameState->SafeZoneIndicator)
        {
            ZoneCenter = GameState->SafeZoneIndicator->NextCenter;
            ZoneRadius = GameState->SafeZoneIndicator->Radius;
        }

        for (int Attempt = 0; Attempt < 8; Attempt++)
        {
            float Angle = (float(rand() % 360) * 3.14159265f) / 180.f;
            float Range = 1400.f + float(rand() % 2800);
            FVector Candidate = BotLoc + FVector(cosf(Angle) * Range, sinf(Angle) * Range, 0.f);
            Candidate.Z = BotLoc.Z;

            float Score = Range;
            if (!Bot->TargetDropZone.IsZero())
                Score += HorizontalDist(Candidate, Bot->TargetDropZone) * 0.25f;
            if (!ZoneCenter.IsZero() && ZoneRadius > 0.f)
            {
                float ZoneDist = HorizontalDist(Candidate, ZoneCenter);
                if (ZoneDist > ZoneRadius)
                    Score -= (ZoneDist - ZoneRadius) * 0.8f;
                else
                    Score += 600.f;
            }

            if (Score > BestScore)
            {
                BestScore = Score;
                Best = Candidate;
            }
        }

        if (Best.IsZero())
            Best = BotLoc + FVector(1800.f, 0.f, 0.f);

        ExploreTargets[Bot] = Best;
        ExploreTargetUntil[Bot] = Now + 4.f + float(rand() % 4);
        return Best;
    }

    inline void MoveTowardLocation(PlayerBot* Bot, FVector Dest, float AcceptanceRadius, float MovementScale, float Cooldown)
    {
        if (!Bot || !Bot->Pawn || Dest.IsZero())
            return;

        FVector BotLoc = Bot->Pawn->K2_GetActorLocation();
        if (HorizontalDist(BotLoc, Dest) < AcceptanceRadius)
            return;

        Bot->Run();
        Bot->MoveToLocationThrottled(Dest, AcceptanceRadius, Cooldown);
    }

    inline bool IsInStorm(PlayerBot* Bot, AFortGameStateAthena* GameState)
    {
        if (!Bot || !Bot->Pawn || !GameState || !GameState->SafeZoneIndicator)
            return false;

        FVector BotLoc = Bot->Pawn->K2_GetActorLocation();
        FVector Zone = GameState->SafeZoneIndicator->NextCenter;
        float Radius = GameState->SafeZoneIndicator->Radius;
        return Radius > 0.f && Dist(Bot, BotLoc, Zone) > Radius;
    }

    inline float DistanceToSafeZone(PlayerBot* Bot, AFortGameStateAthena* GameState)
    {
        if (!Bot || !Bot->Pawn || !GameState || !GameState->SafeZoneIndicator)
            return 0.f;

        FVector BotLoc = Bot->Pawn->K2_GetActorLocation();
        FVector Zone = GameState->SafeZoneIndicator->NextCenter;
        float Radius = GameState->SafeZoneIndicator->Radius;
        float CenterDist = Dist(Bot, BotLoc, Zone);
        return CenterDist > Radius ? CenterDist - Radius : 0.f;
    }

    inline int NearbyWallCount(PlayerBot* Bot, float Range = 650.f)
    {
        if (!Bot || !Bot->Pawn)
            return 0;

        int Count = 0;
        for (int I = 0; I < CachedWalls.Num(); I++)
        {
            if (CachedWalls[I] && CachedWalls[I]->GetDistanceTo(Bot->Pawn) < Range)
                Count++;
            if (Count > 3)
                break;
        }
        return Count;
    }

    inline float NearbyLootValue(PlayerBot* Bot, float Range = 1100.f)
    {
        if (!Bot || !Bot->Pawn)
            return 0.f;

        float Score = 0.f;
        for (int I = 0; I < CachedPickups.Num(); I++)
        {
            AFortPickupAthena* Pickup = (AFortPickupAthena*)CachedPickups[I];
            if (!Pickup || Pickup->bHidden || Pickup->GetDistanceTo(Bot->Pawn) > Range)
                continue;

            UFortItemDefinition* Def = Pickup->PrimaryPickupItemEntry.ItemDefinition;
            if (!Def)
                continue;

            if (Def->IsA(UFortWeaponRangedItemDefinition::StaticClass()))
                Score += 240.f;
            else if (IsHealName(Def))
                Score += 180.f;
            else if (Def->IsA(UFortAmmoItemDefinition::StaticClass()) || Def->IsA(UFortResourceItemDefinition::StaticClass()))
                Score += 60.f;
            else
                Score += 100.f;
        }
        return Score;
    }

    inline bool AllyDiedNearby(PlayerBot* Bot, float Range = 2600.f)
    {
        if (!Bot || !Bot->Pawn || !Bot->PlayerState)
            return false;

        for (auto Other : PlayerBotArray)
        {
            if (!Other || Other == Bot || !Other->Pawn || !Other->PlayerState || !Other->bIsDead)
                continue;

            if (Other->PlayerState->TeamIndex == Bot->PlayerState->TeamIndex && Other->Pawn->GetDistanceTo(Bot->Pawn) < Range)
                return true;
        }
        return false;
    }

    inline int FocusedBotCount(AActor* Target)
    {
        if (!Target)
            return 0;

        int Count = 0;
        for (auto Bot : PlayerBotArray)
        {
            if (!Bot || Bot->BrainTarget != Target)
                continue;

            if (Bot->CurrentAction == BotAction::PUSH_AGGRESSIVE ||
                Bot->CurrentAction == BotAction::BUILD_RAMP_RUSH ||
                Bot->CurrentAction == BotAction::BUILD_HIGHGROUND)
                Count++;
        }
        return Count;
    }

    inline void AimAt(PlayerBot* Bot, FVector Target, float Sway = 5.f, AActor* TargetActor = nullptr)
    {
        if (!Bot || !Bot->Pawn)
            return;

        FVector BotLoc = Bot->Pawn->K2_GetActorLocation();
        auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;

        float Dist3D = Math->Vector_Distance(BotLoc, Target);
        float TravelTime = Dist3D / 30000.f;
        FVector Lead = FVector();
        if (TargetActor && TargetActor->IsA(AFortPlayerPawnAthena::StaticClass()))
        {
            FVector Vel = ((AFortPlayerPawnAthena*)TargetActor)->GetVelocity();
            Vel.Z *= 0.4f;
            Lead = Vel * TravelTime;
        }

        FVector AimTarget = Target + Lead;
        AimTarget.Z += 45.f;
        float FinalSway = Clamp01(Dist3D / 4000.f) * Sway * 0.5f;

        FRotator Rot = Math->FindLookAtRotation(BotLoc, AimTarget);
        Rot.Yaw += Math->RandomFloatInRange(-FinalSway, FinalSway);
        Rot.Pitch += Math->RandomFloatInRange(-FinalSway * 0.6f, FinalSway * 0.6f);
        Bot->SetLookRotation(Rot);
    }

    inline FVector DirectionToTarget(PlayerBot* Bot, AActor* Target)
    {
        if (!Bot || !Bot->Pawn || !Target)
            return FVector();

        FVector Dir = Target->K2_GetActorLocation() - Bot->Pawn->K2_GetActorLocation();
        Dir.Normalize();
        Dir.Z = 0.f;
        return Dir;
    }

    inline FRotator YawToTarget(PlayerBot* Bot, AActor* Target)
    {
        FRotator Rot = Bot->PC ? Bot->PC->GetControlRotation() : FRotator();
        if (Bot && Bot->Pawn && Target)
        {
            auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
            Rot = Math->FindLookAtRotation(Bot->Pawn->K2_GetActorLocation(), Target->K2_GetActorLocation());
        }
        Rot.Pitch = 0.f;
        Rot.Roll = 0.f;
        return Bot->SnapBuildRotation(Rot);
    }

    inline bool BuildWall(PlayerBot* Bot, FVector Base, FRotator Rot, float Offset = 512.f, UClass* WallClass = nullptr)
    {
        if (!Bot || !CanBuild(Bot))
            return false;

        if (!WallClass)
            WallClass = APBWA_W1_Solid_C::StaticClass();

        FVector Forward = Bot->DirectionFromBuildYaw(Rot.Yaw);
        return Bot->SpawnBotBuild(WallClass, Base + (Forward * Offset), Rot);
    }

    inline void BuildBox(PlayerBot* Bot, AActor* Target, bool bEnemyVisible = false)
    {
        if (!Bot || !Bot->Pawn || !CanBuild(Bot))
            return;

        FVector Base = Bot->SnapBuildLocation(Bot->Pawn->K2_GetActorLocation());
        FRotator Facing = Target ? YawToTarget(Bot, Target) : Bot->SnapBuildRotation(Bot->PC->GetControlRotation());

        UClass* WallClass = APBWA_W1_Solid_C::StaticClass();
        UClass* WindowClass = APBWA_W1_WindowC_C::StaticClass();
        UClass* FloorClass = APBWA_W1_Floor_C::StaticClass();

        bool bBuilt = false;
        bBuilt |= BuildWall(Bot, Base, Facing, 512.f, WindowClass ? WindowClass : WallClass);
        bBuilt |= BuildWall(Bot, Base, FRotator(0.f, Facing.Yaw + 90.f, 0.f), 512.f, WallClass);
        bBuilt |= BuildWall(Bot, Base, FRotator(0.f, Facing.Yaw + 180.f, 0.f), 512.f, WallClass);
        bBuilt |= BuildWall(Bot, Base, FRotator(0.f, Facing.Yaw - 90.f, 0.f), 512.f, WallClass);

        if (FloorClass)
            bBuilt |= Bot->SpawnBotBuild(FloorClass, Base + FVector(0.f, 0.f, 384.f), Facing);

        if (bBuilt)
        {
            Bot->IsBoxed = true;
            Bot->ConsecutiveBuilds++;
        }

        if (Target)
        {
            float DistToTarget = Dist(Bot, Bot->Pawn->K2_GetActorLocation(), Target->K2_GetActorLocation());
            SelectWeaponForRange(Bot, DistToTarget);
            AimAt(Bot, Target->K2_GetActorLocation(), 3.f, Target);
            if (bEnemyVisible && CanFireNow(Bot, DistToTarget))
                Bot->Pawn->PawnStartFire(0);
            else
                Bot->Pawn->PawnStopFire(0);
        }
    }

    inline void BuildRampRush(PlayerBot* Bot, AActor* Target)
    {
        if (!Bot || !Bot->Pawn || !Target || !CanBuild(Bot))
            return;

        FVector Base = Bot->SnapBuildLocation(Bot->Pawn->K2_GetActorLocation());
        FRotator Rot = YawToTarget(Bot, Target);
        FVector Forward = Bot->DirectionFromBuildYaw(Rot.Yaw);
        UClass* RampClass = APBWA_W1_StairW_C::StaticClass();
        UClass* WallClass = APBWA_W1_Solid_C::StaticClass();

        bool bBuilt = false;
        if (RampClass)
            bBuilt |= Bot->SpawnBotBuild(RampClass, Base + Forward * 256.f, Rot);
        if (WallClass)
            bBuilt |= Bot->SpawnBotBuild(WallClass, Base - Forward * 512.f, FRotator(0.f, Rot.Yaw + 180.f, 0.f));

        if (bBuilt)
            Bot->ConsecutiveBuilds++;

        Bot->Run();
        Bot->MoveToActorThrottled(Target, 350.f, 0.65f);
    }

    inline void BuildHighground(PlayerBot* Bot, AActor* Target)
    {
        if (!Bot || !Bot->Pawn || !Target || !CanBuild(Bot))
            return;

        FVector BotLoc = Bot->Pawn->K2_GetActorLocation();
        FVector TargetLoc = Target->K2_GetActorLocation();
        if (BotLoc.Z > TargetLoc.Z + 400.f)
        {
            Bot->CurrentAction = BotAction::HOLD_SHOOT_AR;
            Bot->ConsecutiveBuilds = 0;
            return;
        }

        if (Bot->ConsecutiveBuilds > 12)
            return;

        FVector Base = Bot->SnapBuildLocation(BotLoc);
        FRotator Rot = YawToTarget(Bot, Target);
        float YawOffset = float((Bot->ConsecutiveBuilds / 3) % 4) * 90.f;
        Rot.Yaw += YawOffset;
        Rot = Bot->SnapBuildRotation(Rot);
        FVector Forward = Bot->DirectionFromBuildYaw(Rot.Yaw);
        UClass* WallClass = APBWA_W1_Solid_C::StaticClass();
        UClass* RampClass = APBWA_W1_StairW_C::StaticClass();
        UClass* FloorClass = APBWA_W1_Floor_C::StaticClass();

        bool bBuilt = false;
        int Cycle = Bot->ConsecutiveBuilds % 3;
        if (Cycle == 0 && WallClass)
            bBuilt = Bot->SpawnBotBuild(WallClass, Base + Forward * 512.f, Rot);
        else if (Cycle == 1 && RampClass)
            bBuilt = Bot->SpawnBotBuild(RampClass, Base + Forward * 256.f, Rot);
        else if (Cycle == 2 && FloorClass)
        {
            bBuilt = Bot->SpawnBotBuild(FloorClass, Base + FVector(0.f, 0.f, 384.f), Rot);
            Bot->Pawn->Jump();
        }

        if (bBuilt)
            Bot->ConsecutiveBuilds++;

        Bot->Run();
        FVector Vel = Bot->Pawn->GetVelocity();
        float Speed = sqrtf((Vel.X * Vel.X) + (Vel.Y * Vel.Y));
        if (Speed < 80.f)
            Bot->Pawn->AddMovementInput(Forward, 0.35f, true);
    }

    inline void UseHeal(PlayerBot* Bot)
    {
        if (!Bot || !Bot->Pawn)
            return;

        FFortItemEntry* Entry = nullptr;
        if (Bot->Pawn->GetShield() < 75.f)
            Entry = FindEntry(Bot, false, true, true);
        if (!Entry)
            Entry = FindEntry(Bot, false, true, false);

        if (Entry)
        {
            EquipEntry(Bot, Entry);
            Bot->Pawn->PawnStartFire(0);
        }
    }

    inline void ExecuteRetreatHeal(PlayerBot* Bot, AActor* Target)
    {
        if (!Bot || !Bot->Pawn)
            return;

        if (Bot->Pawn->GetHealth() + Bot->Pawn->GetShield() >= 150.f)
        {
            Bot->CurrentAction = BotAction::HOLD_SHOOT_AR;
            return;
        }

        FVector BotLoc = Bot->Pawn->K2_GetActorLocation();
        FVector Away = FVector(1.f, 0.f, 0.f);
        if (Target)
        {
            Away = BotLoc - Target->K2_GetActorLocation();
            Away.Normalize();
        }
        Away.Z = 0.f;

        auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
        FRotator AwayRot = Math->FindLookAtRotation(BotLoc, BotLoc + Away * 512.f);
        AwayRot.Pitch = 0.f;
        AwayRot.Roll = 0.f;
        Bot->SetLookRotation(AwayRot);

        if (CanBuild(Bot))
            BuildWall(Bot, Bot->SnapBuildLocation(BotLoc), FRotator(0.f, AwayRot.Yaw + 180.f, 0.f), 512.f);

        FVector Retreat = BotLoc + Away * 2000.f;
        Retreat.X += float((rand() % 600) - 300);
        Retreat.Y += float((rand() % 600) - 300);
        Bot->Walk();
        Bot->MoveToLocationThrottled(Retreat, 250.f, 0.65f);
        FVector Vel = Bot->Pawn->GetVelocity();
        float Speed = sqrtf((Vel.X * Vel.X) + (Vel.Y * Vel.Y));
        if (Speed < 80.f)
            Bot->Pawn->AddMovementInput(Away, 0.45f, true);
        float RetreatDuration = Bot->LastActionTime > 0.f ? (UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) - Bot->LastActionTime) : 0.f;
        if (NearbyWallCount(Bot, 500.f) > 0 || RetreatDuration > 1.5f)
            UseHeal(Bot);
    }

    inline bool TryBreakObstacle(PlayerBot* Bot, FVector TargetLocation)
    {
        if (!Bot || !Bot->Pawn || !Bot->PC || TargetLocation.IsZero())
            return false;
        auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
        float Now = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());
        FVector BotLoc = Bot->Pawn->K2_GetActorLocation();
        float DistToGoal = Math->Vector_Distance(BotLoc, TargetLocation);
        FVector& LastPos = ObstacleLastPos[Bot];
        float MovedDist = LastPos.IsZero() ? 9999.f : Math->Vector_Distance(BotLoc, LastPos);

        if (MovedDist < 50.f && DistToGoal > 200.f)
        {
            float& StuckTime = ObstacleStuckSince[Bot];
            if (StuckTime <= 0.f)
                StuckTime = Now;

            if ((Now - StuckTime) > 1.8f)
            {
                if (!Bot->IsPickaxeEquiped() && !SafeEquipPickaxe(Bot))
                    return false;

                FVector Dir = TargetLocation - BotLoc;
                Dir.Normalize();
                FRotator FaceTarget = Math->FindLookAtRotation(BotLoc, TargetLocation);
                FaceTarget.Pitch = -10.f;
                Bot->SetLookRotation(FaceTarget);
                Bot->Pawn->PawnStartFire(0);
                Bot->Pawn->AddMovementInput(Dir, 0.45f, true);
                LastPos = BotLoc;
                return true;
            }
        }
        else
        {
            ObstacleStuckSince[Bot] = 0.f;
            LastPos = BotLoc;
            if (Bot->IsPickaxeEquiped() && HasAnyWeapon(Bot))
                Bot->SimpleSwitchToWeapon();
        }

        return false;
    }
    inline void ExecuteLoot(PlayerBot* Bot, AFortGameStateAthena* GameState, float Now)
    {
        if (!Bot || !Bot->Pawn)
            return;

        bool bShouldScanLoot = false;
        if (Bot->NextLootScanTime <= 0.f || Now >= Bot->NextLootScanTime)
        {
            bShouldScanLoot = true;
            Bot->NextLootScanTime = Now + 0.75f + (float(rand() % 55) * 0.01f);
        }

        if (bShouldScanLoot)
        {
            AActor* Pickup = (AActor*)Bot->FindNearestPickup();
            AActor* Chest = (AActor*)Bot->FindNearestChest();
            AActor* Best = Pickup ? Pickup : Chest;
            ELootableType Type = Pickup ? ELootableType::Pickup : ELootableType::Chest;

            if (Pickup && Chest && Chest->GetDistanceTo(Bot->Pawn) < Pickup->GetDistanceTo(Bot->Pawn))
            {
                Best = Chest;
                Type = ELootableType::Chest;
            }

            if (Best)
            {
                Bot->UpdateLootableReservation(Best, false);
                Bot->TargetLootableType = Type;
            }
        }

        if (!Bot->TargetLootable)
        {
            Bot->Pawn->PawnStopFire(0);
            Bot->LookAt(nullptr);
            FVector Explore = PickExploreTarget(Bot, GameState, Now);
            MoveTowardLocation(Bot, Explore, 350.f, 0.85f, 0.7f);
            return;
        }

        float DistToLoot = Bot->TargetLootable->GetDistanceTo(Bot->Pawn);
        Bot->LookAt(Bot->TargetLootable);

        if (DistToLoot > 300.f)
        {
            if (TryBreakObstacle(Bot, Bot->TargetLootable->K2_GetActorLocation()))
                return;
            Bot->MoveToActorThrottled(Bot->TargetLootable, 75.f, 0.65f);
            return;
        }

        bool bLootLOS = Bot->PC->LineOfSightTo(Bot->TargetLootable, Bot->Pawn->K2_GetActorLocation(), true);
        if (!bLootLOS && DistToLoot < 600.f)
        {
            if (!Bot->IsPickaxeEquiped() && !SafeEquipPickaxe(Bot))
                return;
            FVector Dir = Bot->TargetLootable->K2_GetActorLocation() - Bot->Pawn->K2_GetActorLocation();
            Dir.Normalize();
            auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
            FRotator FaceRot = Math->FindLookAtRotation(Bot->Pawn->K2_GetActorLocation(), Bot->TargetLootable->K2_GetActorLocation());
            FaceRot.Pitch = -15.f;
            Bot->SetLookRotation(FaceRot);
            Bot->Pawn->PawnStartFire(0);
            Bot->Pawn->AddMovementInput(Dir, 0.45f, true);
            return;
        }

        Bot->Pawn->PawnStopFire(0);
        if (Bot->TargetLootableType == ELootableType::Pickup)
        {
            Bot->Pickup((AFortPickup*)Bot->TargetLootable);
            Bot->UpdateLootableReservation(nullptr, true);
            Bot->SimpleSwitchToWeapon();
            return;
        }

        FVector ChestLoc = Bot->TargetLootable->K2_GetActorLocation();
        ABuildingContainer* Chest = (ABuildingContainer*)Bot->TargetLootable;
        Bot->UpdateLootableReservation(nullptr, true);
        if (Chest && !Chest->bAlreadySearched)
            Looting::SpawnLoot(Chest);
        Bot->ScheduleChestLootCollection(ChestLoc);
    }

    inline void ExecuteRotateZone(PlayerBot* Bot, AFortGameStateAthena* GameState)
    {
        if (!Bot || !Bot->Pawn || !GameState || !GameState->SafeZoneIndicator)
            return;

        FVector Zone = GameState->SafeZoneIndicator->NextCenter;
        float Radius = GameState->SafeZoneIndicator->Radius;
        if (Zone.IsZero())
        {
            Zone = Bot->TargetDropZone;
            Radius = 800.f;
        }
        if (Zone.IsZero())
            return;

        FVector BotLoc = Bot->Pawn->K2_GetActorLocation();
        FVector ToZone = Zone - BotLoc;
        ToZone.Z = 0.f;
        float HDist = sqrtf((ToZone.X * ToZone.X) + (ToZone.Y * ToZone.Y));
        FVector Target = Zone;
        if (HDist > 0.f && Radius > 100.f)
        {
            FVector Dir = ToZone;
            Dir.Normalize();
            Target = Zone - Dir * (Radius * 0.75f);
            Target.Z = BotLoc.Z;
        }

        Bot->Pawn->PawnStopFire(0);
        Bot->LookAt(nullptr);

        if (Bot->tick_counter % 5 == 0)
        {
            auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
            FRotator LR = Math->FindLookAtRotation(BotLoc, Zone);
            LR.Pitch = 0.f;
            LR.Roll = 0.f;
            Bot->SetLookRotation(LR);
        }

        MoveTowardLocation(Bot, Target, 300.f, 0.85f, 0.7f);
        TryBreakObstacle(Bot, Target);
        Bot->TryDropDown(Target);
    }

    inline BotAction ArgMax(float Outputs[10])
    {
        int Best = 0;
        for (int I = 1; I < 10; I++)
        {
            if (Outputs[I] > Outputs[Best])
                Best = I;
        }
        return (BotAction)Best;
    }

    inline BotAction ApplyRules(PlayerBot* Bot, AFortGameStateAthena* GameState, AActor* Target, BotAction Proposed, float DistToEnemy, bool bEnemyVisible, float Health, float Shield)
    {
        FVector BotLoc = Bot && Bot->Pawn ? Bot->Pawn->K2_GetActorLocation() : FVector();
        FVector TargetLoc = Target ? Target->K2_GetActorLocation() : FVector();
        bool bHasAnyGun = HasAnyWeapon(Bot);
        bool bInStorm = IsInStorm(Bot, GameState);

        if (bInStorm && (!Target || !bEnemyVisible || DistToEnemy > 900.f))
            return BotAction::ROTATE_ZONE;

        if (Health < 50.f && CanBuild(Bot) && Target)
            return BotAction::BUILD_BOX;

        if (AllyDiedNearby(Bot) && (Health + Shield) < 120.f)
            return BotAction::RETREAT_HEAL;

        if (bInStorm)
            return BotAction::ROTATE_ZONE;

        bool bCombatAction = Proposed == BotAction::PUSH_AGGRESSIVE || Proposed == BotAction::BUILD_RAMP_RUSH ||
            Proposed == BotAction::BUILD_HIGHGROUND || Proposed == BotAction::HOLD_SHOOT_AR ||
            Proposed == BotAction::EDIT_PEEK || Proposed == BotAction::SPRAY_AND_PRAY;
        if (!bHasAnyGun && bCombatAction)
            return BotAction::LOOT_NEARBY;

        bool bBuildAction = Proposed == BotAction::BUILD_BOX || Proposed == BotAction::BUILD_RAMP_RUSH || Proposed == BotAction::BUILD_HIGHGROUND || Proposed == BotAction::EDIT_PEEK;
        if (bBuildAction && !CanBuild(Bot))
            return Target ? BotAction::HOLD_SHOOT_AR : BotAction::LOOT_NEARBY;

        if (Proposed == BotAction::HOLD_SHOOT_AR && Target && DistToEnemy < 250.f)
            return BotAction::PUSH_AGGRESSIVE;

        if (Proposed == BotAction::BUILD_RAMP_RUSH && DistToEnemy > 2500.f)
            return BotAction::HOLD_SHOOT_AR;

        if (bBuildAction && GetWood(Bot) < 30 && bEnemyVisible && DistToEnemy < 600.f)
            return BotAction::HOLD_SHOOT_AR;

        if (Health < 40.f && !Bot->FindConsumable() && CanBuild(Bot) && Target)
            return BotAction::BUILD_BOX;

        if (Target && !TargetLoc.IsZero() && !BotLoc.IsZero() && TargetLoc.Z > BotLoc.Z + 400.f && DistToEnemy < 2000.f && CanBuild(Bot))
            return BotAction::BUILD_HIGHGROUND;

        if (Target && !TargetLoc.IsZero() && !BotLoc.IsZero() && BotLoc.Z > TargetLoc.Z + 400.f && bEnemyVisible)
            return BotAction::HOLD_SHOOT_AR;

        if (Bot->IsBoxed && Target && DistToEnemy < 800.f && bEnemyVisible && CanBuild(Bot))
            return BotAction::EDIT_PEEK;

        if (Proposed == BotAction::PUSH_AGGRESSIVE || Proposed == BotAction::BUILD_RAMP_RUSH)
        {
            int FocusedOnUs = 0;
            for (auto Other : PlayerBotArray)
            {
                if (Other && Other != Bot && Other->BrainTarget == Bot->Pawn)
                    FocusedOnUs++;
            }
            if (FocusedOnUs >= 2)
                return BotAction::BUILD_BOX;
        }

        if (Proposed == BotAction::BUILD_RAMP_RUSH && (!bEnemyVisible || DistToEnemy > 2000.f))
            return Target ? BotAction::HOLD_SHOOT_AR : BotAction::LOOT_NEARBY;

        if ((Proposed == BotAction::PUSH_AGGRESSIVE || Proposed == BotAction::BUILD_RAMP_RUSH || Proposed == BotAction::BUILD_HIGHGROUND) &&
            Target && FocusedBotCount(Target) >= 2)
            return BotAction::HOLD_SHOOT_AR;

        if (!Target && Proposed != BotAction::ROTATE_ZONE)
        {
            if (bHasAnyGun && NearbyLootValue(Bot) < 120.f)
                return BotAction::ROTATE_ZONE;
            return BotAction::LOOT_NEARBY;
        }

        if ((Health + Shield) < 70.f && Bot->FindConsumable())
            return BotAction::RETREAT_HEAL;

        return Proposed;
    }

    inline bool CommitAction(PlayerBot* Bot, BotAction NewAction, float Now)
    {
        if (!Bot)
            return false;

        float Until = ActionCooldownUntil[Bot];
        if (Now < Until && NewAction != Bot->CurrentAction)
            return false;

        if (NewAction != Bot->CurrentAction)
        {
            Bot->LastAction = Bot->CurrentAction;
            Bot->CurrentAction = NewAction;
            Bot->LastActionTime = Now;
            Bot->ActionCommitTimer = Now + 0.3f;
            Bot->EditPeekCount = 0;
            if (NewAction != BotAction::BUILD_HIGHGROUND)
                Bot->ConsecutiveBuilds = 0;
            ActionCooldownUntil[Bot] = Now + 0.3f;
        }
        return true;
    }

    inline void ExtractFeatures(PlayerBot* Bot, AFortGameStateAthena* GameState, AActor* Target, float Inputs[20], float Now, bool bEnemyVisible)
    {
        for (int I = 0; I < 20; I++)
            Inputs[I] = 0.f;

        if (!Bot || !Bot->Pawn)
            return;

        float Health = Bot->Pawn->GetHealth();
        float Shield = Bot->Pawn->GetShield();
        float HealthShield = Health + Shield;
        if (Bot->LastHealthShieldSample <= 0.f)
            Bot->LastHealthShieldSample = HealthShield;
        if (HealthShield + 1.f < Bot->LastHealthShieldSample)
            Bot->LastDamageTime = Now;
        Bot->LastHealthShieldSample = HealthShield;

        FVector BotLoc = Bot->Pawn->K2_GetActorLocation();
        FVector TargetLoc = Target ? Target->K2_GetActorLocation() : Bot->LastKnownEnemyPos;
        float DistToEnemy = Target ? Dist(Bot, BotLoc, TargetLoc) : 5000.f;

        Inputs[0] = Clamp01(Health / 100.f);
        Inputs[1] = Clamp01(Shield / 100.f);
        Inputs[2] = Clamp01(float(GetInventoryCount(Bot, "Wood")) / 999.f);
        Inputs[3] = Clamp01(float(GetInventoryCount(Bot, "Stone", "Brick")) / 999.f);
        Inputs[4] = Clamp01(float(GetInventoryCount(Bot, "Metal")) / 999.f);
        Inputs[5] = Clamp01(float(GetInventoryCount(Bot, "Medium", "Bullet")) / 210.f);
        Inputs[6] = Clamp01(float(GetInventoryCount(Bot, "Shell", "Shotgun")) / 60.f);
        Inputs[7] = HasWeaponOfType(Bot, false) ? 1.f : 0.f;
        Inputs[8] = HasWeaponOfType(Bot, true) ? 1.f : 0.f;
        Inputs[9] = Bot->FindConsumable() ? 1.f : 0.f;
        Inputs[10] = bEnemyVisible ? 1.f : 0.f;
        Inputs[11] = Clamp01(1.f - DistToEnemy / 5000.f);
        Inputs[12] = Target && (TargetLoc.Z - BotLoc.Z > 200.f) ? 1.f : 0.f;
        Inputs[13] = Target && (BotLoc.Z - TargetLoc.Z > 200.f) ? 1.f : 0.f;
        Inputs[14] = Target && (BotLoc.Z > TargetLoc.Z + 200.f) ? 1.f : 0.f;
        Inputs[15] = IsInStorm(Bot, GameState) ? 1.f : 0.f;
        Inputs[16] = Clamp01(1.f - DistanceToSafeZone(Bot, GameState) / 10000.f);
        Inputs[17] = NearbyWallCount(Bot) > 0 ? 1.f : 0.f;
        Inputs[18] = Clamp01(NearbyLootValue(Bot) / 500.f);
        Inputs[19] = (Now - Bot->LastDamageTime) < 3.f ? 1.f : 0.f;
    }

    inline void BeginFrame(size_t BotCount, int MaxTicks = 5)
    {
        FrameBrainBudget = MaxTicks;
        if (BotCount == 0)
        {
            RoundRobinStart = 0;
            RoundRobinCursor = 0;
            return;
        }

        RoundRobinStart = RoundRobinCursor % BotCount;
        RoundRobinCursor = (RoundRobinCursor + MaxTicks) % BotCount;
    }

    inline size_t GetFrameStart()
    {
        return RoundRobinStart;
    }

    inline bool CanTickBot(PlayerBot* Bot, float Now)
    {
        if (!Bot || FrameBrainBudget <= 0)
            return false;
        if (Now < Bot->NextBotBrainTickTime)
            return false;

        Bot->NextBotBrainTickTime = Now + 0.12f + (float(rand() % 5) * 0.01f);
        FrameBrainBudget--;
        return true;
    }

    inline bool Tick(PlayerBot* Bot, AFortGameStateAthena* GameState)
    {
        if (!Bot || !Bot->Pawn || !Bot->PC || !Bot->PlayerState || Bot->bIsDead)
            return false;

        FVector BotLoc = Bot->Pawn->K2_GetActorLocation();
        float Now = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());
        bool bRanThinkPass = CanTickBot(Bot, Now);
        AActor* Target = Bot->BrainTarget ? Bot->BrainTarget : Bot->NearestPlayerActor;

        if (IsDeadOrDownedPawn(Target))
        {
            if (Target == Bot->BrainTarget)
                Bot->BrainTarget = nullptr;
            if (Target == Bot->NearestPlayerActor)
                Bot->NearestPlayerActor = nullptr;
            Target = nullptr;
            Bot->bLastBotBrainEnemyVisible = false;
        }

        FVector TargetLoc = Target ? Target->K2_GetActorLocation() : Bot->LastKnownEnemyPos;
        float DistToEnemy = Target ? Dist(Bot, BotLoc, TargetLoc) : Bot->LastBotBrainDistToEnemy;
        bool bEnemyVisible = Target && Bot->bLastBotBrainEnemyVisible && (Now - Bot->LastBotBrainThinkTime) <= 0.45f;

        if (bRanThinkPass)
        {
            RefreshWorldCache(Now);

            Target = ResolveTarget(Bot, Now);
            if (IsDeadOrDownedPawn(Target))
            {
                if (Target == Bot->BrainTarget)
                    Bot->BrainTarget = nullptr;
                if (Target == Bot->NearestPlayerActor)
                    Bot->NearestPlayerActor = nullptr;
                Target = nullptr;
            }

            BotLoc = Bot->Pawn->K2_GetActorLocation();
            TargetLoc = Target ? Target->K2_GetActorLocation() : Bot->LastKnownEnemyPos;
            DistToEnemy = Target ? Dist(Bot, BotLoc, TargetLoc) : FLT_MAX;
            bEnemyVisible = Target && Bot->PC->LineOfSightTo(Target, FVector(), true);

            float Inputs[20];
            float Outputs[10];
            ExtractFeatures(Bot, GameState, Target, Inputs, Now, bEnemyVisible);
            BotNets[Bot].Forward(Inputs, Outputs);

            BotAction Proposed = ArgMax(Outputs);
            BotAction Chosen = ApplyRules(Bot, GameState, Target, Proposed, DistToEnemy, bEnemyVisible, Bot->Pawn->GetHealth(), Bot->Pawn->GetShield());
            CommitAction(Bot, Chosen, Now);

            Bot->LastBotBrainThinkTime = Now;
            Bot->LastBotBrainDistToEnemy = DistToEnemy;
            Bot->bLastBotBrainEnemyVisible = bEnemyVisible;
        }

        Bot->TryCollectPendingChestLoot();
        if (Bot->HasTimerElapsed(Bot->NextUtilityActionTime, 1.8f, 1.0f))
        {
            FVector Vel = Bot->Pawn->GetVelocity();
            float Speed = sqrtf((Vel.X * Vel.X) + (Vel.Y * Vel.Y));
            if (Speed < 120.f || Bot->ConsecutiveWallStuckTicks > 0)
            {
                Bot->TryOpenNearbyDoor(600.f);
                Bot->TryBreakDoorOrWall(500.f);
            }
        }

        switch (Bot->CurrentAction)
        {
        case BotAction::PUSH_AGGRESSIVE:
            if (Target)
            {
                TryWeaponSwapOnEmpty(Bot, DistToEnemy, Now);
                Bot->Run();
                SelectWeaponForRange(Bot, DistToEnemy);
                AimAt(Bot, TargetLoc, 5.f, Target);
                float HeightDiff = TargetLoc.Z - BotLoc.Z;
                if (HeightDiff > 300.f && CanBuild(Bot))
                {
                    CommitAction(Bot, BotAction::BUILD_RAMP_RUSH, Now);
                    BuildRampRush(Bot, Target);
                    break;
                }
                if (HeightDiff < -400.f)
                {
                    auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
                    if (Math->RandomBoolWithWeight(0.08f))
                        Bot->Pawn->Jump();
                }
                if (TryBreakObstacle(Bot, TargetLoc))
                    break;
                Bot->MoveToActorThrottled(Target, 250.f, 0.65f);
                Bot->DetectWorldGeoStuck(BotLoc, Target, FVector());
                if (bEnemyVisible)
                {
                    FVector ToEnemy = TargetLoc - BotLoc;
                    ToEnemy.Z = 0.f;
                    ToEnemy.Normalize();
                    FVector StrafeDir = FVector(-ToEnemy.Y, ToEnemy.X, 0.f);
                    float T = Now - Bot->LastActionTime;
                    float Side = (fmodf(T, 4.f) < 2.f) ? 1.f : -1.f;
                    Bot->Pawn->AddMovementInput(StrafeDir * Side, 0.35f, true);
                }
                if (bEnemyVisible && CanFireNow(Bot, DistToEnemy))
                    Bot->Pawn->PawnStartFire(0);
                else
                    Bot->Pawn->PawnStopFire(0);
            }
            else
            {
                Bot->Pawn->PawnStopFire(0);
                CommitAction(Bot, HasAnyWeapon(Bot) ? BotAction::ROTATE_ZONE : BotAction::LOOT_NEARBY, Now);
            }
            break;
        case BotAction::RETREAT_HEAL:
            ExecuteRetreatHeal(Bot, Target);
            break;
        case BotAction::BUILD_BOX:
            BuildBox(Bot, Target, bEnemyVisible);
            break;
        case BotAction::BUILD_RAMP_RUSH:
            if (Target)
                BuildRampRush(Bot, Target);
            else
            {
                Bot->Pawn->PawnStopFire(0);
                CommitAction(Bot, HasAnyWeapon(Bot) ? BotAction::ROTATE_ZONE : BotAction::LOOT_NEARBY, Now);
            }
            if (Target && DistToEnemy < 500.f)
                SelectWeaponForRange(Bot, DistToEnemy);
            break;
        case BotAction::BUILD_HIGHGROUND:
            if (Target)
                BuildHighground(Bot, Target);
            else
            {
                Bot->Pawn->PawnStopFire(0);
                CommitAction(Bot, HasAnyWeapon(Bot) ? BotAction::ROTATE_ZONE : BotAction::LOOT_NEARBY, Now);
            }
            break;
        case BotAction::HOLD_SHOOT_AR:
            if (Target)
            {
                TryWeaponSwapOnEmpty(Bot, DistToEnemy, Now);
                SelectWeaponForRange(Bot, DistToEnemy);
                AimAt(Bot, TargetLoc, 4.f, Target);
                if (bEnemyVisible && CanFireNow(Bot, DistToEnemy))
                    Bot->Pawn->PawnStartFire(0);
                else
                    Bot->Pawn->PawnStopFire(0);
                if (DistToEnemy > 1500.f)
                    Bot->MoveToActorThrottled(Target, 600.f, 0.9f);
                if (bEnemyVisible)
                {
                    FVector ToEnemy = TargetLoc - BotLoc;
                    ToEnemy.Z = 0.f;
                    ToEnemy.Normalize();
                    FVector StrafeDir = FVector(-ToEnemy.Y, ToEnemy.X, 0.f);
                    float T = Now - Bot->LastActionTime;
                    float Side = (fmodf(T, 4.f) < 2.f) ? 1.f : -1.f;
                    Bot->Pawn->AddMovementInput(StrafeDir * Side, 0.3f, true);
                }
                if (bEnemyVisible && DistToEnemy > 600.f && !Bot->Pawn->bIsCrouched)
                    Bot->Pawn->Crouch(false);
                else if ((!bEnemyVisible || DistToEnemy < 400.f) && Bot->Pawn->bIsCrouched)
                    Bot->Pawn->UnCrouch(false);
            }
            else
            {
                Bot->Pawn->PawnStopFire(0);
                if (Now - Bot->LastEnemySeenTime < 3.f && !Bot->LastKnownEnemyPos.IsZero())
                {
                    Bot->MoveToLocationThrottled(Bot->LastKnownEnemyPos, 300.f, 0.8f);
                    FVector Dir = Bot->LastKnownEnemyPos - BotLoc;
                    Dir.Z = 0.f;
                    Dir.Normalize();
                    FVector Vel = Bot->Pawn->GetVelocity();
                    float Speed = sqrtf((Vel.X * Vel.X) + (Vel.Y * Vel.Y));
                    if (Speed < 80.f)
                        Bot->Pawn->AddMovementInput(Dir, 0.3f, true);
                }
            }
            break;
        case BotAction::EDIT_PEEK:
            if (Target)
            {
                if (!Bot->IsBoxed && CanBuild(Bot))
                    BuildBox(Bot, Target, bEnemyVisible);
                SelectWeaponForRange(Bot, DistToEnemy);
                AimAt(Bot, TargetLoc, 3.f, Target);
                if (bEnemyVisible && Bot->EditPeekCount < 3 && CanFireNow(Bot, DistToEnemy))
                {
                    Bot->Pawn->PawnStartFire(0);
                    Bot->EditPeekCount++;
                }
                else
                {
                    Bot->Pawn->PawnStopFire(0);
                    if (Bot->EditPeekCount >= 3)
                        CommitAction(Bot, BotAction::HOLD_SHOOT_AR, Now);
                }
            }
            else
            {
                Bot->Pawn->PawnStopFire(0);
                CommitAction(Bot, HasAnyWeapon(Bot) ? BotAction::ROTATE_ZONE : BotAction::LOOT_NEARBY, Now);
            }
            break;
        case BotAction::LOOT_NEARBY:
            ExecuteLoot(Bot, GameState, Now);
            break;
        case BotAction::ROTATE_ZONE:
            ExecuteRotateZone(Bot, GameState);
            break;
        case BotAction::SPRAY_AND_PRAY:
            if (!Bot->LastKnownEnemyPos.IsZero() && Now - Bot->LastEnemySeenTime <= 4.f)
            {
                SelectWeaponForRange(Bot, DistToEnemy);
                AimAt(Bot, Bot->LastKnownEnemyPos, 8.f, Target);
                if (CanFireNow(Bot, DistToEnemy))
                    Bot->Pawn->PawnStartFire(0);
                else
                    Bot->Pawn->PawnStopFire(0);
                Bot->MoveToLocationThrottled(Bot->LastKnownEnemyPos, 450.f, 1.0f);
            }
            else
            {
                Bot->Pawn->PawnStopFire(0);
                CommitAction(Bot, HasAnyWeapon(Bot) ? BotAction::ROTATE_ZONE : BotAction::LOOT_NEARBY, Now);
            }
            break;
        default:
            break;
        }

        return true;
    }
}
