#pragma once
#include "framework.h"

namespace Misc {
    void nullFunc() {}

    int True() {
        return 1;
    }

    int False() {
        return 0;
    }

    static inline void (*KickPlayerOG)(AGameSession*, AController*);
    static void KickPlayer(AGameSession*, AController*) {
        return;
    }

static void RegisterPlayerHook(AGameSession* Session, AController* NewPlayer, void* UniqueId, bool bWasFromInvite) {
	Log("RegisterPlayer Hook Called! Reservation bypassed.");
	return;
}

    void (*DispatchRequestOG)(__int64 a1, unsigned __int64* a2, int a3);
    void DispatchRequest(__int64 a1, unsigned __int64* a2, int a3)
    {
        return DispatchRequestOG(a1, a2, 3);
    }

    void (*K2_DestroyActorOG)(AActor* This, __int64 a2);
    void K2_DestroyActor(AActor* This, __int64 a2)
    {
        if (!This)
            return;

        UWorld* World = UWorld::GetWorld();
        AFortGameStateAthena* GameState = World ? (AFortGameStateAthena*)World->GameState : nullptr;

        std::string Name = This->GetName();
        //Log(Name);
        const bool bUsesDestroyStateHack = Name.contains("BGA") || Name.contains("StaticGenerator");

        if (!GameState)
            return K2_DestroyActorOG(This, a2);

		if (GameState->GamePhase <= EAthenaGamePhase::Warmup) {
            if (bUsesDestroyStateHack) {
                This->bActorIsBeingDestroyed = true;
            }
        }
        else {
            if (bUsesDestroyStateHack) {
                if (This->bActorIsBeingDestroyed) {
                    This->bActorIsBeingDestroyed = false;
                }
            }
        }

        return K2_DestroyActorOG(This, a2);
    }

    void LateGameAircraftThread(FVector BattleBusLocation)
    {
        auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
        auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

        Sleep(2500);

        GameState->bAircraftIsLocked = false;

        GameMode->OnAircraftExitedDropZone(GameState->GetAircraft(0));

        GameState->GamePhase = EAthenaGamePhase::SafeZones;

        GameState->SafeZonesStartTime = 1;
    }

    uint8 FirstIdx = 3;
    uint8 LastIdx = 102;
    uint8 NextIdx = 3;
    int CurrentPlayersOnTeam = 0;
    int MaxPlayersOnTeam;

    inline __int64 PickTeam(__int64 a1, unsigned __int8 a2, __int64 a3)
    {
        if (MaxPlayersOnTeam <= 0)
            MaxPlayersOnTeam = 1;

        if (NextIdx < FirstIdx || NextIdx > LastIdx)
            NextIdx = FirstIdx;

        uint8 Ret = NextIdx;
        CurrentPlayersOnTeam++;

        if (CurrentPlayersOnTeam >= MaxPlayersOnTeam)
        {
            CurrentPlayersOnTeam = 0;
            if (NextIdx >= LastIdx)
                NextIdx = FirstIdx;
            else
                NextIdx++;
        }
        return Ret;
    };

    void Hook() {
        // NOTE: Each address must only be hooked ONCE. Duplicate hooks corrupt MinHook's trampolines.
        MH_CreateHook((LPVOID)(ImageBase + 0x2D95E00), False, nullptr);   // collectgarbage (suppress)
        MH_CreateHook((LPVOID)(ImageBase + 0x4155600), KickPlayer, (LPVOID*)&KickPlayerOG); // KickPlayer -> no-op
        MH_CreateHook((LPVOID)(ImageBase + 0x20EED20), RegisterPlayerHook, nullptr); // RegisterPlayer reservation bypass
        MH_CreateHook((LPVOID)(ImageBase + 0x1E23840), False, nullptr);   // change gamesession id (suppress)
        MH_CreateHook((LPVOID)(ImageBase + 0x108D740), DispatchRequest, (LPVOID*)&DispatchRequestOG);

        MH_CreateHook((LPVOID)(ImageBase + 0x2DBCBA0), True, nullptr);    // CanCreateContext

        MH_CreateHook((LPVOID)(ImageBase + 0x3ca10c0), nullFunc, nullptr);
        // 0x2d95e00 already hooked above - DO NOT duplicate
        // 0x1e23840 already hooked above - DO NOT duplicate
        MH_CreateHook((LPVOID)(ImageBase + 0x3262100), nullFunc, nullptr);
        MH_CreateHook((LPVOID)(ImageBase + 0x2d95dc0), nullFunc, nullptr);

        MH_CreateHook((LPVOID)(ImageBase + 0x7F0220), K2_DestroyActor, (LPVOID*)&K2_DestroyActorOG);

        MH_CreateHook((LPVOID)(ImageBase + 0x18E6B20), PickTeam, nullptr);

        Log("Misc Hooked!");
    }
}
