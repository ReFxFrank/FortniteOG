#pragma once
#include "framework.h"
#include "Globals.h"
#include "GameMode.h"
#include "Quests.h"
#include "Bots.h"
#include "AdminPanel.h"

namespace AccoladeTickingService {
	float NextPlaytimeXPDrop = 0.f;

	// Survival Accolades
	bool AccoladeId_026_Survival_Default_Bronze = false;
	bool AccoladeId_027_Survival_Default_Silver = false;
	bool AccoladeId_028_Survival_Default_Gold = false;
	//arena
	bool Placement25 = false;
	bool Placement5 = false;

	// first player to land accolade
	bool AccoladeId_018_First_Landing = false;

	void Tick(AFortGameModeAthena* GameMode, AFortGameStateAthena* GameState) {
		if (!GameMode || !GameState)
			return;

		float CurrentTime = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());

		if (NextPlaytimeXPDrop == 0.f) {
			NextPlaytimeXPDrop = CurrentTime + 120.f;
		}

		if (CurrentTime >= NextPlaytimeXPDrop) { // Broken or smth idk
			NextPlaytimeXPDrop = CurrentTime + 120.f; // 2 Minute interval
			Log("Playtime XP!");
			for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
			{
				if (GameMode->AlivePlayers[i])
					Quests::GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_054_Playtime.AccoladeId_054_Playtime"));
			}
		}

		int32 AliveCount = GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num();

		if (AliveCount == 50 && !AccoladeId_026_Survival_Default_Bronze)
		{
			AccoladeId_026_Survival_Default_Bronze = true;
			for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
			{
				if (GameMode->AlivePlayers[i])
					Quests::GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_026_Survival_Default_Bronze.AccoladeId_026_Survival_Default_Bronze"));

			}
		}


		if (AliveCount == 25 && !AccoladeId_027_Survival_Default_Silver)
		{
			AccoladeId_027_Survival_Default_Silver = true;
			for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
			{
				if (GameMode->AlivePlayers[i])
					Quests::GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_027_Survival_Default_Silver.AccoladeId_027_Survival_Default_Silver"));

			}
		}
		if (AliveCount == 10 && !AccoladeId_028_Survival_Default_Gold)
		{
			AccoladeId_028_Survival_Default_Gold = true;
			for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
			{
				if (GameMode->AlivePlayers[i])
					Quests::GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_028_Survival_Default_Gold.AccoladeId_028_Survival_Default_Gold"));
			}
		}

		if (Globals::Arena)
		{


		    if (AliveCount == 25 && !Placement25)
			{
				Placement25 = true;
				for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
				{
					if (GameMode->AlivePlayers[i])
						GameMode->AlivePlayers[i]->ClientReportTournamentPlacementPointsScored(25, 60);

				}
			}

			else if (AliveCount == 5 && !Placement5)
			{
				Placement5 = true;
				for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
				{
					if (GameMode->AlivePlayers[i])
						GameMode->AlivePlayers[i]->ClientReportTournamentPlacementPointsScored(5, 30);

				}
			}

		}

		for (AFortPlayerControllerAthena* Player : GameMode->AlivePlayers) { 
			if (!Player || !Player->Pawn)
				continue;

			// KillStreak Accolades!
			if (CurrentTime <= Player->LastKillTimeWindow && Player->KillsInKillTimeWindow != Player->LastRecordedKillsInKillTimeWindow) {
				if (Player->KillsInKillTimeWindow == 2) {
					Quests::GiveAccolade(Player, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_002_DoubleKill.AccoladeId_002_DoubleKill"));
				}
				else if (Player->KillsInKillTimeWindow == 3) {
					Quests::GiveAccolade(Player, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_003_TrippleKill.AccoladeId_003_TrippleKill"));
				}
				else if (Player->KillsInKillTimeWindow == 4) {
					Quests::GiveAccolade(Player, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_004_QuadraKill.AccoladeId_004_QuadraKill"));
				}
				else if (Player->KillsInKillTimeWindow == 5) {
					Quests::GiveAccolade(Player, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_005_PentaKill.AccoladeId_005_PentaKill"));
				}
				else if (Player->KillsInKillTimeWindow == 6) {
					Quests::GiveAccolade(Player, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_006_MonsterKill.AccoladeId_006_MonsterKill"));
				}

				Player->LastRecordedKillsInKillTimeWindow = Player->KillsInKillTimeWindow;
			}

			// Maybe proper?
			if (!AccoladeId_018_First_Landing && !Player->IsInAircraft()) {
				FVector Vel = Player->Pawn->GetVelocity();
				float Speed = Vel.Z;
				if (Speed == 0.f) {
					AccoladeId_018_First_Landing = true;
					Quests::GiveAccolade(Player, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_018_First_Landing.AccoladeId_018_First_Landing"));

					// Eh give discover landmark and poi accolade cuz idk how to track it either (for ones idk how to track i will put them in the proper places when i figure it out)
					Quests::GiveAccolade(Player, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_032_DiscoverLandmark.AccoladeId_032_DiscoverLandmark"));
					Quests::GiveAccolade(Player, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_033_DiscoverPOI.AccoladeId_033_DiscoverPOI"));
				}
			}
		}
	}
}

namespace Tick {
	void (*ServerReplicateActors)(void*) = decltype(ServerReplicateActors)(ImageBase + 0x1023F60);

	// The dedicated server dies on the first ServerReplicateActors pass after a
	// player is possessed (the OGS log always stops right after
	// ServerAcknowledgePossession). That crash is inside the engine's
	// ReplicationGraph, so the OGS log can't tell us where. SEH-wrap the call:
	// on a fault we record the faulting instruction as an ImageBase-relative RVA
	// and SKIP replication for that frame instead of letting the process vanish.
	// This keeps the server alive AND hands us the exact crash address to fix.
	// NOTE: __try/__except cannot live in a function that needs C++ object
	// unwinding (MSVC C2712) and the filter must not build a std::string, so the
	// RVA is captured here and all logging happens at the call site.
	static unsigned __int64 g_ReplFaultRva = 0;

	static int ReplFaultFilter(EXCEPTION_POINTERS* ep)
	{
		g_ReplFaultRva = (unsigned __int64)((uintptr_t)ep->ExceptionRecord->ExceptionAddress - ImageBase);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	static bool SafeServerReplicate(void* RepDriver)
	{
		__try
		{
			ServerReplicateActors(RepDriver);
			return true;
		}
		__except (ReplFaultFilter(GetExceptionInformation()))
		{
			return false;
		}
	}

	inline void (*TickFlushOG)(UNetDriver*, float);
	void TickFlush(UNetDriver* Driver, float DeltaTime)
	{
		if (!Driver)
			return;

		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		if (Driver->ReplicationDriver)
		{
			static bool bLoggedFirst = false;
			static bool bLoggedOk = false;
			static bool bLoggedFault = false;

			if (!bLoggedFirst)
			{
				Log("Starting first manual ServerReplicateActors pass (SEH-guarded)...");
				bLoggedFirst = true;
			}

			if (SafeServerReplicate(Driver->ReplicationDriver))
			{
				if (!bLoggedOk)
				{
					Log("ServerReplicateActors pass survived.");
					bLoggedOk = true;
				}
			}
			else if (!bLoggedFault)
			{
				char buf[192];
				sprintf_s(buf, "ServerReplicateActors FAULTED at ImageBase+0x%llX -- skipping replication this frame to keep the server alive. Map this RVA to find the crash site.", (unsigned long long)g_ReplFaultRva);
				LogError(buf);
				bLoggedFault = true;
			}
		}
		else
		{
			Log("ReplicationDriver Doesent Exist!");
		}

		if (!GameState || !GameMode)
			return TickFlushOG(Driver, DeltaTime);

		GameMode::KeepWarmupUntilHumanReady(GameMode, GameState, "TickFlush");

		if (GameState->GamePhase == EAthenaGamePhase::Warmup
			&& GameMode::HasReadyHumanPlayer(GameMode)
			&& (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num()) >= Globals::MinPlayersForEarlyStart
			&& GameState->WarmupCountdownEndTime > UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 10.f) {

			auto TS = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
			auto DR = 10.f;

			GameState->WarmupCountdownEndTime = TS + DR;
			GameMode->WarmupCountdownDuration = DR;
			GameState->WarmupCountdownStartTime = TS;
			GameMode->WarmupEarlyCountdownDuration = DR;
		}

		if (Globals::bBotsEnabled && !Globals::bEventEnabled) {
			static bool InitialisedPlayerStarts = false;
			static float NextWarmupBotSpawnTime = 0.f;
			if (!InitialisedPlayerStarts)
			{
				UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), AFortPlayerStartWarmup::StaticClass(), &PlayerStarts);
				InitialisedPlayerStarts = true;
			}

			float CurrentTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
			int MaxPlayers = GameMode->GameSession && GameMode->GameSession->MaxPlayers > 0 ? GameMode->GameSession->MaxPlayers : 100;
			int BotFillTarget = MaxPlayers - GameMode->AlivePlayers.Num();
			if (BotFillTarget > Globals::MaxBotsToSpawn)
				BotFillTarget = Globals::MaxBotsToSpawn;

			if (((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->GamePhase == EAthenaGamePhase::Warmup &&
				GameMode->AlivePlayers.Num() > 0
				&& PlayerStarts.Num() > 0
				&& (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num()) < MaxPlayers
				&& GameMode->AliveBots.Num() < BotFillTarget
				&& GameState->WarmupCountdownEndTime > CurrentTime
				&& CurrentTime >= NextWarmupBotSpawnTime)
			{
				int BotsNeeded = BotFillTarget - GameMode->AliveBots.Num();
				float SpawnDelay = 0.25f;
				if (GameMode->AliveBots.Num() > 70)
					SpawnDelay = 0.45f;
				else if (GameMode->AliveBots.Num() > 35)
					SpawnDelay = 0.35f;

				NextWarmupBotSpawnTime = CurrentTime + SpawnDelay;
				AActor* SpawnLocator = PlayerStarts[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, PlayerStarts.Num() - 1)];

				if (SpawnLocator && BotsNeeded > 0)
				{
					PlayerBots::SpawnPlayerBots(SpawnLocator);
				}
			}
		}

		if (GameState->WarmupCountdownEndTime - UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) <= 0 && GameState->GamePhase == EAthenaGamePhase::Warmup)
		{
			if (GameMode::HasReadyHumanPlayer(GameMode))
				GameMode::StartAircraftPhase(GameMode, 0);
			else
				GameMode::HoldWarmupCountdown(GameMode, GameState, 30.f, "waiting for player spawn");
		}

		if (GameState->GamePhase > EAthenaGamePhase::Warmup && !Globals::bCreativeEnabled) {
			AccoladeTickingService::Tick(GameMode, GameState);
		}

		if (Globals::bBossesEnabled && !Globals::bEventEnabled && GameState->GamePhase > EAthenaGamePhase::Warmup)
		{
			Bosses::TickBots(DeltaTime);
		}

		if (Globals::bBotsEnabled && !Globals::bEventEnabled) {
			int AliveTotal = (int)PlayerBotArray.size();
			int BrainBudget = (GameState->GamePhase >= EAthenaGamePhase::SafeZones && GameState->GamePhase < EAthenaGamePhase::EndGame) ? 8 : 5;
			BotBrain::BeginFrame(AliveTotal, BrainBudget);
			PlayerBots::Tick();
		}

		if (Globals::bAdminPanelEnabled) {
			AdminPanel::ProcessPending();
		}

		return TickFlushOG(Driver, DeltaTime);
	}

	inline float GetMaxTickRate()
	{
		return 30.f;
	}

	void Hook() {
		MH_CreateHook((LPVOID)(ImageBase + 0x42C3ED0), TickFlush, (LPVOID*)&TickFlushOG);
		MH_CreateHook((LPVOID)(ImageBase + 0x4576310), GetMaxTickRate, nullptr);

		Log("Tick Hooked!");
	}
}
