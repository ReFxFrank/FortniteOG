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

	// Native UNetDriver::TickFlush does NOT run the ReplicationGraph in this
	// server-as-client setup, so we drive it ourselves every frame -- this is what
	// builds and delivers the outgoing bunches to each client. CONFIRMED load-
	// bearing: removing it regressed the client to conns=1 but pawn=0 with no
	// client RPCs (the client never received its PlayerController, so it never sent
	// ServerSetClientHasFinishedLoading). SEH-wrapped so a transient bad actor in
	// the graph can't vanish the server; the fault RVA is captured for diagnosis.
	void (*ServerReplicateActors)(void*) = decltype(ServerReplicateActors)(ImageBase + 0x1023F60);

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

	// Post-possession visibility. When the server does NOT crash, the OGS log
	// otherwise goes silent right after ServerAck, so there is no way to see why
	// a connected client is stuck on the loading screen. Emit a throttled status
	// line (~every 3s) covering the match phase, net-connection state, and -- the
	// key signal -- whether each player controller has a pawn, has ACKNOWLEDGED
	// that pawn, and finished loading. A client that never enters the world shows
	// up here as conns=0 (never connected), ackPawn=0 (pawn not acknowledged), or
	// a phase/warmup counter that never advances.
	inline void LogJoinDiagnostics(UNetDriver* Driver, AFortGameModeAthena* GameMode, AFortGameStateAthena* GameState)
	{
		float Now = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
		static float NextDiagTime = 0.f;
		if (Now < NextDiagTime)
			return;
		NextDiagTime = Now + 3.f;

		int conns = Driver ? Driver->ClientConnections.Num() : -1;

		char head[256];
		sprintf_s(head, "[DIAG] phase=%d step=%d players=%d bots=%d conns=%d warmupLeft=%.1f worldReady=%d",
			(int)GameState->GamePhase, (int)GameState->GamePhaseStep,
			GameMode->AlivePlayers.Num(), GameMode->AliveBots.Num(),
			conns, GameState->WarmupCountdownEndTime - Now,
			GameMode->bWorldIsReady ? 1 : 0);
		Log(head);

		if (Driver)
		{
			for (int i = 0; i < Driver->ClientConnections.Num() && i < 8; i++)
			{
				UNetConnection* C = Driver->ClientConnections[i];
				if (!C)
					continue;
				char cl[200];
				sprintf_s(cl, "[DIAG]   conn[%d]: pc=%d viewTarget=%d",
					i, C->PlayerController ? 1 : 0, C->ViewTarget ? 1 : 0);
				Log(cl);
			}
		}

		for (int i = 0; i < GameMode->AlivePlayers.Num() && i < 8; i++)
		{
			AFortPlayerControllerAthena* PC = GameMode->AlivePlayers[i];
			if (!PC)
				continue;
			bool hasPawn = PC->Pawn != nullptr;
			bool ackPawn = (PC->Pawn != nullptr) && (PC->AcknowledgedPawn == PC->Pawn);
			// waiting/spectator are what actually drive the client's warmup free camera;
			// log them so we can see whether the player is still "waiting" on the server's
			// side even when the pawn is possessed + acknowledged.
			int waiting = PC->bPlayerIsWaiting ? 1 : 0;
			int onlySpec = PC->PlayerState ? (PC->PlayerState->bOnlySpectator ? 1 : 0) : -1;
			int isSpec = PC->PlayerState ? (PC->PlayerState->bIsSpectator ? 1 : 0) : -1;
			int inAir = PC->IsInAircraft() ? 1 : 0;
			char pl[320];
			sprintf_s(pl, "[DIAG]   PC[%d]: pawn=%d ackPawn=%d finishedLoading=%d readyToStart=%d waiting=%d onlySpec=%d isSpec=%d inAircraft=%d",
				i, hasPawn ? 1 : 0, ackPawn ? 1 : 0,
				PC->bHasServerFinishedLoading ? 1 : 0, PC->bReadyToStartMatch ? 1 : 0,
				waiting, onlySpec, isSpec, inAir);
			Log(pl);
		}
	}

	inline void (*TickFlushOG)(UNetDriver*, float);

	// SEH-guarded native net tick. Native UNetDriver::TickFlush is what actually
	// SENDS the queued replication bunches to clients, so it must run to completion
	// or the client never leaves the loading screen. If it ever faults we record
	// the address (as an ImageBase RVA) and skip just that frame instead of letting
	// the dedicated server vanish. Kept unwinding-free so __try/__except is legal
	// (C2712); DoNativeTick does the logging outside the __try.
	static unsigned __int64 g_TickFaultRva = 0;
	static int TickFaultFilter(EXCEPTION_POINTERS* ep)
	{
		g_TickFaultRva = (unsigned __int64)((uintptr_t)ep->ExceptionRecord->ExceptionAddress - ImageBase);
		return EXCEPTION_EXECUTE_HANDLER;
	}
	static bool SafeNativeTickFlush(UNetDriver* Driver, float DeltaTime)
	{
		__try
		{
			TickFlushOG(Driver, DeltaTime);
			return true;
		}
		__except (TickFaultFilter(GetExceptionInformation()))
		{
			return false;
		}
	}
	inline void DoNativeTick(UNetDriver* Driver, float DeltaTime)
	{
		static bool bLoggedTickFault = false;
		if (!SafeNativeTickFlush(Driver, DeltaTime) && !bLoggedTickFault)
		{
			char buf[208];
			sprintf_s(buf, "Native TickFlush FAULTED at ImageBase+0x%llX -- skipping native net tick this frame to keep the server alive.", (unsigned long long)g_TickFaultRva);
			LogError(buf);
			bLoggedTickFault = true;
		}
	}

	// SEH-guarded StartAircraftPhase. The warmup->drop transition is driven from the
	// (otherwise unguarded) warmup-expiry path below. StartNoAircraftFallback guards
	// its own native steps, but wrap the outer call too so ANY fault in the
	// transition keeps the server alive and logs an address instead of vanishing.
	static unsigned __int64 g_AircraftFaultRva = 0;
	static int AircraftFaultFilter(EXCEPTION_POINTERS* ep)
	{
		g_AircraftFaultRva = (unsigned __int64)((uintptr_t)ep->ExceptionRecord->ExceptionAddress - ImageBase);
		return EXCEPTION_EXECUTE_HANDLER;
	}
	static bool SafeStartAircraftPhase(AFortGameModeAthena* GM)
	{
		__try { GameMode::StartAircraftPhase(GM, 0); return true; }
		__except (AircraftFaultFilter(GetExceptionInformation())) { return false; }
	}

	// SEH-guarded bot spawn + bot AI tick. Bots are newly enabled in the no-aircraft
	// flow, so treat them as untrusted: a fault inside a single bot's spawn or AI tick
	// must skip that frame, not vanish the dedicated server. Kept unwinding-free so
	// __try/__except is legal (C2712); the fault RVA is captured for diagnosis.
	static unsigned __int64 g_BotFaultRva = 0;
	static int BotFaultFilter(EXCEPTION_POINTERS* ep)
	{
		g_BotFaultRva = (unsigned __int64)((uintptr_t)ep->ExceptionRecord->ExceptionAddress - ImageBase);
		return EXCEPTION_EXECUTE_HANDLER;
	}
	static bool SafeBotSpawn(AActor* SpawnLocator)
	{
		__try { PlayerBots::SpawnPlayerBots(SpawnLocator); return true; }
		__except (BotFaultFilter(GetExceptionInformation())) { return false; }
	}
	static bool SafeBotTick(AFortGameStateAthena* GameState)
	{
		__try
		{
			int AliveTotal = (int)PlayerBotArray.size();
			int BrainBudget = (GameState->GamePhase >= EAthenaGamePhase::SafeZones && GameState->GamePhase < EAthenaGamePhase::EndGame) ? 8 : 5;
			BotBrain::BeginFrame(AliveTotal, BrainBudget);
			PlayerBots::Tick();
			return true;
		}
		__except (BotFaultFilter(GetExceptionInformation())) { return false; }
	}

	// SEH-guarded human bus boarding. Runs every tick during the Aircraft phase and
	// seats any not-yet-aboard human onto the live bus. Boarding pokes native RPCs
	// (ClientEnterAircraft / ServerSetInAircraft) so a fault here must skip the frame,
	// not vanish the server. Unwinding-free so __try/__except is legal (C2712).
	static unsigned __int64 g_BoardFaultRva = 0;
	static int BoardFaultFilter(EXCEPTION_POINTERS* ep)
	{
		g_BoardFaultRva = (unsigned __int64)((uintptr_t)ep->ExceptionRecord->ExceptionAddress - ImageBase);
		return EXCEPTION_EXECUTE_HANDLER;
	}
	static bool SafeBoardHumansOntoBus(AFortGameModeAthena* GameMode, AFortGameStateAthena* GameState)
	{
		__try { GameMode::BoardHumansOntoBus(GameMode, GameState); return true; }
		__except (BoardFaultFilter(GetExceptionInformation())) { return false; }
	}

	// SEH-guarded re-handshake of every human right after the match goes InProgress, so the
	// prelobby player can actually walk instead of free-camming. Respawns + re-runs the
	// client restart, which pokes native engine paths -- a fault must skip, not kill the
	// server. Unwinding-free so __try/__except is legal (C2712).
	static unsigned __int64 g_WarmupCtrlFaultRva = 0;
	static int WarmupCtrlFaultFilter(EXCEPTION_POINTERS* ep)
	{
		g_WarmupCtrlFaultRva = (unsigned __int64)((uintptr_t)ep->ExceptionRecord->ExceptionAddress - ImageBase);
		return EXCEPTION_EXECUTE_HANDLER;
	}
	static bool SafeReassertWarmupControl(AFortGameModeAthena* GameMode)
	{
		__try
		{
			for (int32 i = 0; i < GameMode->AlivePlayers.Num(); i++)
				PC::ReassertWarmupControl(GameMode->AlivePlayers[i]);
			return true;
		}
		__except (WarmupCtrlFaultFilter(GetExceptionInformation())) { return false; }
	}

	void TickFlush(UNetDriver* Driver, float DeltaTime)
	{
		if (!Driver)
			return;

		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		// Drive the ReplicationGraph ourselves -- native TickFlush does not do it in
		// this setup, and without it the client never receives its PlayerController
		// and can't progress past initial loading. SEH-guarded so a transient bad
		// actor can't vanish the server.
		if (Driver->ReplicationDriver)
		{
			static bool bLoggedReplFault = false;
			if (!SafeServerReplicate(Driver->ReplicationDriver) && !bLoggedReplFault)
			{
				char buf[208];
				sprintf_s(buf, "ServerReplicateActors FAULTED at ImageBase+0x%llX -- skipped this frame.", (unsigned long long)g_ReplFaultRva);
				LogError(buf);
				bLoggedReplFault = true;
			}
		}

		if (!GameState || !GameMode)
			return DoNativeTick(Driver, DeltaTime);

		if (Driver->ReplicationDriver) // only report the game net driver, not beacons
			LogJoinDiagnostics(Driver, GameMode, GameState);

		// The warmup pawn spawn can fail on its single ServerReadyToStartMatch
		// attempt (World->NetDriver / PlayerState / GameState->MapInfo not ready yet,
		// see TryManualWarmupSpawn's early guards), which leaves the player with no
		// pawn forever -- and the Setup->Warmup advance below needs a pawn
		// (HasReadyHumanPlayer). Retry the spawn from the tick (throttled ~1s) for any
		// finished-loading controller that still has no pawn, until it succeeds.
		{
			static float NextWarmupSpawnRetry = 0.f;
			float NowRetry = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
			if (NowRetry >= NextWarmupSpawnRetry)
			{
				NextWarmupSpawnRetry = NowRetry + 1.0f;
				for (int32 i = 0; i < GameMode->AlivePlayers.Num(); i++)
				{
					AFortPlayerControllerAthena* PC = GameMode->AlivePlayers[i];
					if (PC && !PC->Pawn && PC->bHasServerFinishedLoading)
						PC::TryManualWarmupSpawn(PC, "tick warmup retry");
				}
			}
		}

		// Advance the match out of Setup once a human is fully ready. The native
		// ServerReadyToStartMatch (which normally drives Setup -> Warmup) is skipped
		// to avoid the unsafe aircraft startup, so the Athena GamePhase otherwise
		// stays stuck at Setup (1) forever -- and EVERY warmup/match-start path below
		// is gated on GamePhase == Warmup (2), so nothing runs: no countdown, no bot
		// fill, no StartAircraftPhase. The player just stands in Setup. Flip it to
		// Warmup with a fresh countdown; the existing logic then runs the countdown,
		// fills bots, and on expiry calls StartAircraftPhase -> no-aircraft fallback,
		// which skydives the player in and begins SafeZones.
		// Drive Setup -> Warmup in BOTH modes: our ReadyToStartMatch hook returns false,
		// so the engine never advances the Athena GamePhase on its own (it stays stuck at
		// Setup). In native-aircraft mode the warmup expiry below then calls the real
		// StartAircraftPhase; in no-aircraft mode it calls the fallback.
		if (GameState->GamePhase == EAthenaGamePhase::Setup
			&& GameMode::HasReadyHumanPlayer(GameMode))
		{
			EAthenaGamePhase OldPhase = GameState->GamePhase;
			float Now = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
			GameState->GamePhase = EAthenaGamePhase::Warmup;
			GameState->GamePhaseStep = EAthenaGamePhaseStep::Warmup;
			GameState->WarmupCountdownStartTime = Now;
			GameState->WarmupCountdownEndTime = Now + 30.f;
			GameMode->WarmupCountdownDuration = 30.f;
			GameMode->WarmupEarlyCountdownDuration = 30.f;
			GameState->OnRep_GamePhase(OldPhase);
			GameState->ForceNetUpdate();
			Log("Advanced game phase Setup -> Warmup (human ready); match starts after warmup.");

			// Real-bus mode: start the match (MatchState -> InProgress) right as we enter
			// warmup. The client's "WAITING FOR PLAYERS" camera + input lock is keyed off
			// MatchState, so until the match starts the player can't move -- this is the
			// prelobby freecam. StartMatch resets the phase/countdown back to warmup, which is
			// harmless here (we're entering warmup anyway), so we re-assert our 30s window
			// right after so the bus still launches on schedule. No-aircraft mode starts the
			// match later via StartNoAircraftFallback, so skip it there.
			if (!GameMode::ShouldForceNoAircraftStartup())
			{
				GameMode::EnsureMatchInProgress(GameMode);
				float Now2 = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
				GameState->GamePhase = EAthenaGamePhase::Warmup;
				GameState->GamePhaseStep = EAthenaGamePhaseStep::Warmup;
				GameState->WarmupCountdownStartTime = Now2;
				GameState->WarmupCountdownEndTime = Now2 + 30.f;
				GameMode->WarmupCountdownDuration = 30.f;
				GameMode->WarmupEarlyCountdownDuration = 30.f;
				GameState->ForceNetUpdate();

				// Re-run the player's client-restart handshake now that the match is
				// InProgress -- the warmup spawn's handshake ran while MatchState was still
				// WaitingToStart, so the client ignored it and stayed in the freecam camera.
				static bool bLoggedWarmupCtrlFault = false;
				if (!SafeReassertWarmupControl(GameMode) && !bLoggedWarmupCtrlFault)
				{
					char buf[200];
					sprintf_s(buf, "ReassertWarmupControl FAULTED at ImageBase+0x%llX -- prelobby control may be limited.", (unsigned long long)g_WarmupCtrlFaultRva);
					LogError(buf);
					bLoggedWarmupCtrlFault = true;
				}
			}
		}

		// In native aircraft mode the engine drives the warmup->aircraft progression;
		// our warmup-hold/early-start would fight it, so only run them in no-aircraft mode.
		if (GameMode::ShouldForceNoAircraftStartup())
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

				// Count our custom bots via PlayerBotArray, not only GameMode->AliveBots:
				// the custom PlayerBots may not register in AliveBots, and capping on
				// AliveBots alone would let this loop spawn far past MaxBotsToSpawn across
				// the warmup. Take the larger of the two so native bots still count.
				int BotCount = (int)PlayerBotArray.size();
				if ((int)GameMode->AliveBots.Num() > BotCount)
					BotCount = (int)GameMode->AliveBots.Num();

			if (((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->GamePhase == EAthenaGamePhase::Warmup &&
				GameMode->AlivePlayers.Num() > 0
				&& PlayerStarts.Num() > 0
				&& (GameMode->AlivePlayers.Num() + BotCount) < MaxPlayers
				&& BotCount < BotFillTarget
				&& GameState->WarmupCountdownEndTime > CurrentTime
				&& CurrentTime >= NextWarmupBotSpawnTime)
			{
				int BotsNeeded = BotFillTarget - BotCount;
				float SpawnDelay = 0.25f;
				if (BotCount > 70)
					SpawnDelay = 0.45f;
				else if (BotCount > 35)
					SpawnDelay = 0.35f;

				NextWarmupBotSpawnTime = CurrentTime + SpawnDelay;
				AActor* SpawnLocator = PlayerStarts[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, PlayerStarts.Num() - 1)];

				if (SpawnLocator && BotsNeeded > 0)
				{
					static bool bLoggedBotSpawnFault = false;
					if (!SafeBotSpawn(SpawnLocator) && !bLoggedBotSpawnFault)
					{
						char buf[200];
						sprintf_s(buf, "Bot spawn FAULTED at ImageBase+0x%llX -- skipped to keep the server alive.", (unsigned long long)g_BotFaultRva);
						LogError(buf);
						bLoggedBotSpawnFault = true;
					}
				}
			}
		}

		// On warmup expiry, trigger the aircraft phase ourselves in BOTH modes (the engine
		// won't, since our ReadyToStartMatch returns false). With bSkipUnsafeAircraftPhase
		// false this routes through to native StartAircraftPhaseOG (the real flying bus);
		// with it true it runs the no-aircraft fallback.
		if (GameState->WarmupCountdownEndTime - UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) <= 0 && GameState->GamePhase == EAthenaGamePhase::Warmup)
		{
			if (GameMode::HasReadyHumanPlayer(GameMode))
			{
				static bool bLoggedAircraftFault = false;
				if (!SafeStartAircraftPhase(GameMode) && !bLoggedAircraftFault)
				{
					char buf[200];
					sprintf_s(buf, "StartAircraftPhase FAULTED at ImageBase+0x%llX -- skipped to keep the server alive.", (unsigned long long)g_AircraftFaultRva);
					LogError(buf);
					bLoggedAircraftFault = true;
				}
			}
			else
				GameMode::HoldWarmupCountdown(GameMode, GameState, 30.f, "waiting for player spawn");
		}

		if (GameState->GamePhase > EAthenaGamePhase::Warmup && !Globals::bCreativeEnabled) {
			AccoladeTickingService::Tick(GameMode, GameState);
		}

		// While the bus is flying, keep seating any human who hasn't boarded yet (the
		// bus actor often isn't live the instant StartAircraftPhase returns, so this
		// retries each tick until everyone is aboard). Riding the bus is what lets the
		// player jump when THEY choose instead of the client rendering a falling pawn.
		if (GameState->GamePhase == EAthenaGamePhase::Aircraft && !GameMode::ShouldForceNoAircraftStartup())
		{
			static bool bLoggedBoardFault = false;
			if (!SafeBoardHumansOntoBus(GameMode, GameState) && !bLoggedBoardFault)
			{
				char buf[200];
				sprintf_s(buf, "BoardHumansOntoBus FAULTED at ImageBase+0x%llX -- skipped to keep the server alive.", (unsigned long long)g_BoardFaultRva);
				LogError(buf);
				bLoggedBoardFault = true;
			}
		}

		if (Globals::bBossesEnabled && !Globals::bEventEnabled && GameState->GamePhase > EAthenaGamePhase::Warmup)
		{
			Bosses::TickBots(DeltaTime);
		}

		if (Globals::bBotsEnabled && !Globals::bEventEnabled) {
			static bool bLoggedBotTickFault = false;
			if (!SafeBotTick(GameState) && !bLoggedBotTickFault)
			{
				char buf[200];
				sprintf_s(buf, "Bot tick FAULTED at ImageBase+0x%llX -- skipped this frame to keep the server alive.", (unsigned long long)g_BotFaultRva);
				LogError(buf);
				bLoggedBotTickFault = true;
			}
		}

		if (Globals::bAdminPanelEnabled) {
			AdminPanel::ProcessPending();
		}

		return DoNativeTick(Driver, DeltaTime);
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
