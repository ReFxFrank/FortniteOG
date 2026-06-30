#pragma once
#include "framework.h"
#include "Inventory.h"
#include "Abilities.h"
#include "Quests.h"
#include "Net.h"
#include "Bots.h"
#include "Misc.h"

namespace GameMode {
	inline bool bSetupServerComplete = false;
	inline bool bUseManualPlayerStart = true;

	inline bool ShouldForceNoAircraftStartup()
	{
		return Globals::bSkipUnsafeAircraftPhase && !Globals::LateGame && !Globals::bCreativeEnabled && !Globals::bEventEnabled;
	}

	namespace Event {
		static inline UClass* Starter;
		static inline UObject* JerkyLoader;
	}

	inline bool IsUnsafePlaylistLevelPath(const std::string& Path)
	{
		return Path.contains("Hoagie") || Path.contains("NeoTilted");
	}

	template <typename LevelArrayType>
	inline void RemoveUnsafePlaylistLevels(LevelArrayType& Levels, const char* Label)
	{
		for (int32 i = Levels.Num() - 1; i >= 0; i--)
		{
			std::string Path = UKismetStringLibrary::Conv_NameToString(Levels[i].ObjectID.AssetPathName).ToString();
			if (IsUnsafePlaylistLevelPath(Path))
			{
				Log(std::string("Skipping unsafe playlist ") + Label + " level: " + Path);
				Levels.Remove(i);
			}
		}
	}

	inline void SanitizePlaylistLevelStreams(UFortPlaylistAthena* Playlist)
	{
		if (!Playlist || Globals::bEventEnabled)
			return;

		// With the real aircraft phase on, keep the playlist's additional levels -- the
		// native flight-path/aircraft startup relies on them. Only strip them in the
		// no-aircraft startup that needed the streaming-hang workaround.
		if (!Globals::bSkipUnsafeAircraftPhase)
			return;

		if (Globals::Automatics)
		{
			if (Playlist->AdditionalLevels.Num() > 0 || Playlist->AdditionalLevelsServerOnly.Num() > 0)
			{
				Log("Clearing Automatics playlist additional levels to avoid startup streaming hang.");
				Playlist->AdditionalLevels.Clear();
				Playlist->AdditionalLevelsServerOnly.Clear();
			}
			return;
		}

		RemoveUnsafePlaylistLevels(Playlist->AdditionalLevels, "client");
		RemoveUnsafePlaylistLevels(Playlist->AdditionalLevelsServerOnly, "server");
	}

	inline bool EnsureServerListening(AFortGameModeAthena* GameMode)
	{
		UWorld* World = UWorld::GetWorld();
		if (!World)
			return false;

		EnsureGameNetDriverDefinition();

		if (World->NetDriver)
		{
			if (GameMode)
				GameMode->bWorldIsReady = true;
			return true;
		}

		FName NetDriverDef = UKismetStringLibrary::Conv_StringToName(FString(L"GameNetDriver"));
		UNetDriver* NetDriver = CreateNetDriver(UEngine::GetEngine(), World, NetDriverDef);
		if (!NetDriver)
		{
			Log("CreateNetDriver failed; server is not listening yet.");
			return false;
		}

		NetDriver->NetDriverName = NetDriverDef;
		NetDriver->World = World;

		FString Error;
		FURL Url = FURL();
		Url.Port = 7777;

		if (!InitListen(NetDriver, World, Url, false, Error))
		{
			Log("InitListen failed: " + Error.ToString());
			return false;
		}

		SetWorld(NetDriver, World);
		World->NetDriver = NetDriver;
		for (size_t i = 0; i < World->LevelCollections.Num(); i++)
			World->LevelCollections[i].NetDriver = NetDriver;
		SetWorld(World->NetDriver, World);

		if (GameMode)
			GameMode->bWorldIsReady = true;

		Log("Listening: 7777");
		SetConsoleTitleA("OGS 12.41 | Listening: 7777");
		return true;
	}

	inline bool HasReadyHumanPlayer(AFortGameModeAthena* GameMode)
	{
		if (!GameMode)
			return false;

		for (int32 i = 0; i < GameMode->AlivePlayers.Num(); i++)
		{
			AFortPlayerControllerAthena* PC = GameMode->AlivePlayers[i];
			if (PC && PC->Pawn && PC->AcknowledgedPawn == PC->Pawn && PC->bHasServerFinishedLoading)
				return true;
		}

		return false;
	}

	inline void HoldWarmupCountdown(AFortGameModeAthena* GameMode, AFortGameStateAthena* GameState, float Seconds, const char* Reason)
	{
		if (!GameMode || !GameState || GameState->GamePhase != EAthenaGamePhase::Warmup)
			return;

		float Now = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
		float TargetEndTime = Now + Seconds;
		if (GameState->WarmupCountdownEndTime >= TargetEndTime)
			return;

		GameState->WarmupCountdownStartTime = Now;
		GameState->WarmupCountdownEndTime = TargetEndTime;
		GameMode->WarmupCountdownDuration = Seconds;
		GameMode->WarmupEarlyCountdownDuration = Seconds;
		GameState->ForceNetUpdate();

		std::string Message = "Held warmup countdown";
		if (Reason && Reason[0])
			Message += std::string(" during ") + Reason;
		Message += ".";
		Log(Message);
	}

	// Latched true once we intentionally start the match (no-aircraft drop). After
	// that, KeepWarmupUntilHumanReady must NOT drag the phase back to warmup even if a
	// pawn is briefly un-acknowledged (Possess / native RestartPlayer transiently
	// clear AcknowledgedPawn) -- otherwise the phase bounces SafeZones->Warmup every
	// time the countdown expires, the 30s bus/storm/"waiting for players" loop.
	inline bool g_OGSMatchStarted = false;

	inline void KeepWarmupUntilHumanReady(AFortGameModeAthena* GameMode, AFortGameStateAthena* GameState, const char* Reason)
	{
		if (g_OGSMatchStarted || Globals::LateGame || Globals::bCreativeEnabled || HasReadyHumanPlayer(GameMode))
			return;

		if (GameState && GameState->GamePhase > EAthenaGamePhase::Warmup)
		{
			GameState->GamePhase = EAthenaGamePhase::Warmup;
			GameState->GamePhaseStep = EAthenaGamePhaseStep::Warmup;
			GameState->bAircraftIsLocked = true;
			GameState->ForceNetUpdate();

			std::string Message = "Forced game phase back to warmup";
			if (Reason && Reason[0])
				Message += std::string(" during ") + Reason;
			Message += ".";
			Log(Message);
		}

		if (GameState && GameState->GamePhase == EAthenaGamePhase::Warmup)
		{
			float Now = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
			if (GameState->WarmupCountdownEndTime < Now + 10.f)
				HoldWarmupCountdown(GameMode, GameState, 30.f, Reason);
		}
	}

	// Spawns the bot manager + mutator + AI goal manager and loads the bot cosmetic
	// pools. Pulled out of ReadyToStartMatch so it can run under SEH: these object/actor
	// spawns are exactly the bot init that used to be skipped in no-aircraft startup, and
	// there is no global crash handler -- a fault here would otherwise kill setup
	// silently. DoInitBots holds the std::vector work (no __try here, so C2712 is
	// satisfied); the __try lives only in SafeInitBots.
	static void DoInitBots(AFortGameModeAthena* GameMode, AFortGameStateAthena* GameState)
	{
		auto BotManager = (UFortServerBotManagerAthena*)UGameplayStatics::SpawnObject(UFortServerBotManagerAthena::StaticClass(), GameMode);
		if (!BotManager)
		{
			Log("BotManager is nullptr!");
			Globals::bBotsEnabled = false;
			return;
		}

		GameMode->ServerBotManager = BotManager;
		GameMode->ServerBotManagerClass = UFortServerBotManagerAthena::StaticClass();
		BotManager->CachedGameState = GameState;
		BotManager->CachedGameMode = GameMode;

		if (!GameMode->AIDirector)
		{
			// The custom C++ combat AI (FindNearestEnemy / MoveToActor / PawnStartFire)
			// does not need the AIDirector, so don't disable bots over it -- just note that
			// the native behavior-tree director is absent.
			Log("No AIDirector; native bot director absent, custom bot combat will still run.");
		}

		BotMutator = Globals::bBotsEnabled ? SpawnActor<AFortAthenaMutator_Bots>({}) : nullptr;
		if (Globals::bBotsEnabled && !BotMutator)
		{
			Log("BotMutator spawn failed; disabling custom bots.");
			Globals::bBotsEnabled = false;
		}
		else if (BotMutator)
		{
			BotManager->CachedBotMutator = BotMutator;
			BotMutator->CachedGameMode = GameMode;
			BotMutator->CachedGameState = GameState;

			if (Globals::bBotsEnabled)
			{
				AFortAIGoalManager* GoalManager = SpawnActor<AFortAIGoalManager>({});
				GameMode->AIGoalManager = GoalManager;
			}
		}

		if (Globals::bBotsEnabled)
		{
			CIDs.clear();
			for (auto Path : KnownGoodCIDPaths)
			{
				UAthenaCharacterItemDefinition* CID = StaticLoadObject<UAthenaCharacterItemDefinition>(Path);
				if (CID && CID->HeroDefinition)
					CIDs.push_back(CID);
			}
			Pickaxes = GetAllObjectsOfClass<UAthenaPickaxeItemDefinition>();
			Backpacks = GetAllObjectsOfClass<UAthenaBackpackItemDefinition>();
			Gliders = GetAllObjectsOfClass<UAthenaGliderItemDefinition>();
			Contrails = GetAllObjectsOfClass<UAthenaSkyDiveContrailItemDefinition>();
			Dances = GetAllObjectsOfClass<UAthenaDanceItemDefinition>();
		}

		Log("Initialised Bots!");
	}

	static unsigned __int64 g_BotInitFaultRva = 0;
	static int BotInitFaultFilter(EXCEPTION_POINTERS* ep)
	{
		g_BotInitFaultRva = (unsigned __int64)((uintptr_t)ep->ExceptionRecord->ExceptionAddress - ImageBase);
		return EXCEPTION_EXECUTE_HANDLER;
	}
	static bool SafeInitBots(AFortGameModeAthena* GameMode, AFortGameStateAthena* GameState)
	{
		__try { DoInitBots(GameMode, GameState); return true; }
		__except (BotInitFaultFilter(GetExceptionInformation())) { return false; }
	}

	inline bool (*ReadyToStartMatchOG)(AFortGameModeAthena* GameMode);
	inline bool ReadyToStartMatch(AFortGameModeAthena* GameMode)
	{
		static bool LoggedSkippedNativeReady = false;
		if (ReadyToStartMatchOG && !ShouldForceNoAircraftStartup())
			ReadyToStartMatchOG(GameMode);
		else if (!LoggedSkippedNativeReady && ShouldForceNoAircraftStartup())
		{
			LoggedSkippedNativeReady = true;
			Log("Skipped native ReadyToStartMatchOG to block aircraft flight path initialization.");
		}

		static bool SetupServer = false;
		static bool ServerListening = false;

		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		if (!SetupServer) {
			static bool LoadedPlaylist = false;
			if (!LoadedPlaylist) {
				UFortPlaylistAthena* Playlist = nullptr;
				if (Globals::bCreativeEnabled) {
					Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Creative/Playlist_PlaygroundV2.Playlist_PlaygroundV2");
				}
				else if (Globals::bEventEnabled) {
					Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Music/Playlist_Music_High.Playlist_Music_High");
				}
				else if (Globals::Arena) {
					Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Showdown/Playlist_ShowdownAlt_Solo.Playlist_ShowdownAlt_Solo");
				}
				else if (Globals::Automatics)
				{
					Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Auto/Playlist_DefaultSolo.Playlist_DefaultSolo");
					if (!Playlist)
						Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo");
				}
				else if (Globals::BattleLab)
				{
					Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/BattleLab/Playlist_BattleLab.Playlist_BattleLab");
				}
				else if (Globals::Blitz)
				{
					Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Blitz/Playlist_Blitz_Solo.Playlist_Blitz_Solo");
				}
				else if (Globals::StormKing)
				{
					Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/DADBRO/Playlist_DADBRO_Squads.Playlist_DADBRO_Squads");
				}
				else if (Globals::Arsenal)
				{
					Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/gg/Playlist_Gg_Reverse.Playlist_Gg_Reverse");
				}
				else if (Globals::TeamRumble)
				{
					Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Respawn/Playlist_Respawn_Solo.Playlist_Respawn_Solo");
				}
				else if (Globals::SolidGold)
				{
					Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/SolidGold/Playlist_SolidGold_Solo.Playlist_SolidGold_Solo");
				}
				else if (Globals::UnVaulted)
				{
					Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Unvaulted/Playlist_Unvaulted_Solo.Playlist_Unvaulted_Solo");
				}
				else if (Globals::Siphon)
				{
					Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Vamp/Playlist_Vamp_Solo.Playlist_Vamp_Solo");
				}
				else {
					Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Playlist_DefaultSquad.Playlist_DefaultSquad");
					if (!Playlist)
						Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo");
				}
				if (!Playlist) {
					Log("Could not find playlist!");
					return false;
				}
				else {
					LoadedPlaylist = true;
					Log("Found Playlist!");
				}

				SanitizePlaylistLevelStreams(Playlist);
				GuardUnsafeStartupLevelStreaming("playlist setup");

				GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
				GameState->CurrentPlaylistInfo.OverridePlaylist = Playlist;
				GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
				GameState->CurrentPlaylistInfo.MarkArrayDirty();
				GameState->OnRep_CurrentPlaylistInfo();

				GameMode->CurrentPlaylistName = Playlist->PlaylistName;
				GameMode->WarmupRequiredPlayerCount = 1;

				Misc::FirstIdx = Playlist->DefaultFirstTeam;
				Misc::LastIdx = Playlist->DefaultLastTeam >= Playlist->DefaultFirstTeam ? Playlist->DefaultLastTeam : 102;
				Misc::NextIdx = Misc::FirstIdx;
				Misc::MaxPlayersOnTeam = Playlist->MaxSquadSize;

				bool bDBNO = Misc::MaxPlayersOnTeam > 1;

				GameState->bDBNOEnabledForGameMode = bDBNO;
				GameState->bDBNODeathEnabled = bDBNO;

				GameMode->bAlwaysDBNO = bDBNO;
				GameMode->bDBNOEnabled = bDBNO;

				GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
				GameState->CurrentPlaylistInfo.OverridePlaylist = Playlist;
				GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
				GameState->CurrentPlaylistId = Playlist->PlaylistId;
				GameState->CurrentPlaylistInfo.MarkArrayDirty();

				GameMode->GameSession->MaxPlayers = Globals::MaxPlayers; // total session capacity (humans + bots); decoupled from bot count so multiple users can connect
				GameMode->GameSession->MaxSpectators = 0;
				GameMode->GameSession->MaxPartySize = Playlist->MaxSquadSize;
				GameMode->GameSession->MaxSplitscreensPerConnection = 2;
				GameMode->GameSession->bRequiresPushToTalk = false;
				GameMode->GameSession->SessionName = UKismetStringLibrary::Conv_StringToName(FString(L"GameSession"));

				auto TS = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
				auto DR = 90.f;

				GameState->WarmupCountdownEndTime = TS + DR;
				GameMode->WarmupCountdownDuration = DR;
				GameState->WarmupCountdownStartTime = TS;
				GameMode->WarmupEarlyCountdownDuration = DR;

				GameState->CurrentPlaylistId = Playlist->PlaylistId;
				GameState->OnRep_CurrentPlaylistId();

				if (ShouldForceNoAircraftStartup())
				{
					Playlist->bSkipAircraft = true;
					Playlist->SafeZoneStartUp = ESafeZoneStartUp::StartsWithNoAirCraft;
					Playlist->AirCraftBehavior = EAirCraftBehavior::NoAircraft;
					Playlist->AISettings = nullptr;
				}

				GameState->bGameModeWillSkipAircraft = Playlist->bSkipAircraft;
				GameState->CachedSafeZoneStartUp = Playlist->SafeZoneStartUp;
				GameState->AirCraftBehavior = Playlist->AirCraftBehavior;
				GameState->OnRep_Aircraft();

				GameState->WorldLevel = Playlist->LootLevel;
				GameMode->AISettings = ShouldForceNoAircraftStartup() ? nullptr : Playlist->AISettings;
				GameMode->bFlightPathInitialized = ShouldForceNoAircraftStartup();
				GameMode->bSpawnAllStuff = true;
				GameState->DefaultRebootMachineHotfix = 1;

				if (Globals::bEventEnabled) {
					Log("Event is loaded!");

					TArray<AActor*> BuildingFoundations;
					UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), ABuildingFoundation::StaticClass(), &BuildingFoundations);


					for (AActor*& BuildingFoundation : BuildingFoundations) {
						ABuildingFoundation* Foundation = (ABuildingFoundation*)BuildingFoundation;
						if (!Foundation) continue;

						if (BuildingFoundation->GetName().contains("Jerky") ||
							BuildingFoundation->GetName().contains("LF_Athena_POI_19x19")) {
							ShowFoundation((ABuildingFoundation*)BuildingFoundation);
						}
					}

					BuildingFoundations.Free();
				}
				else {
					Log("Using native building foundation streaming.");
				}

				Log("Setup Playlist!");

				if (!EnsureServerListening(GameMode))
					return false;
			}

			if (!GameState->MapInfo) {
				GuardUnsafeStartupLevelStreaming("waiting for map info");
				//Log("Map isnt fully loaded yet, ReadyToStartMatch return false!"); //this spams so why log 
				return false;
			}

			static bool InitialisedBots = false;

			if (!InitialisedBots) {
				if (Globals::bEventEnabled) {
					InitialisedBots = true;
				}
				else if (!Globals::bBotsEnabled)
				{
					InitialisedBots = true;
					GameMode->ServerBotManager = nullptr;
					GameMode->ServerBotManagerClass = nullptr;
					GameMode->AISettings = nullptr;
					Log("Bots disabled by config; skipped bot manager initialization.");
				}
				else
				{
					// Initialise the bot manager / mutator even in no-aircraft startup. This
					// is our own SpawnObject/SpawnActor work and does NOT run the native
					// aircraft flight-path init (that lives in ReadyToStartMatchOG /
					// StartAircraftPhaseOG, which we still skip; AISettings stays null).
					// SEH-guarded so a fault during setup disables bots instead of killing the
					// server -- there is no global crash handler around this path.
					InitialisedBots = true;
					if (!SafeInitBots(GameMode, GameState))
					{
						char buf[200];
						sprintf_s(buf, "Bot initialization FAULTED at ImageBase+0x%llX -- disabling bots, server kept alive.", (unsigned long long)g_BotInitFaultRva);
						LogError(buf);
						Globals::bBotsEnabled = false;
						GameMode->ServerBotManager = nullptr;
						GameMode->ServerBotManagerClass = nullptr;
					}
				}
			}

			if (!ServerListening) {
				ServerListening = true;
				GuardUnsafeStartupLevelStreaming("server listen setup");

				if (Globals::bEventEnabled) {
					Event::Starter = StaticLoadObject<UClass>("/CycloneJerky/Gameplay/BP_Jerky_Scripting.BP_Jerky_Scripting_C");
					Event::JerkyLoader = UObject::FindObject<UObject>("BP_Jerky_Loader_C JerkyLoaderLevel.JerkyLoaderLevel.PersistentLevel.BP_Jerky_Loader_2");
				}

				GameState->OnRep_CurrentPlaylistId();
				GameState->OnRep_CurrentPlaylistInfo();

				if (!GameState->CurrentPlaylistInfo.BasePlaylist) {
					Log("CurrentPlaylistInfo.BasePlaylist is nullptr, delaying match start.");
					ServerListening = false;
					return false;
				}

				for (int i = 0; i < GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevels.Num(); i++)
				{
					FVector Loc{};
					FRotator Rot{};
					bool bSuccess = false;
					((ULevelStreamingDynamic*)ULevelStreamingDynamic::StaticClass()->DefaultObject)->LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevels[i], Loc, Rot, &bSuccess, FString());
					FAdditionalLevelStreamed NewLevel{};
					NewLevel.LevelName = GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevels[i].ObjectID.AssetPathName;
					NewLevel.bIsServerOnly = false;
					GameState->AdditionalPlaylistLevelsStreamed.Add(NewLevel);
				}

				for (int i = 0; i < GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevelsServerOnly.Num(); i++)
				{
					FVector Loc{};
					FRotator Rot{};
					bool bSuccess = false;
					((ULevelStreamingDynamic*)ULevelStreamingDynamic::StaticClass()->DefaultObject)->LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevelsServerOnly[i], Loc, Rot, &bSuccess, FString());
					FAdditionalLevelStreamed NewLevel{};
					NewLevel.LevelName = GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevelsServerOnly[i].ObjectID.AssetPathName;
					NewLevel.bIsServerOnly = true;
					GameState->AdditionalPlaylistLevelsStreamed.Add(NewLevel);
				}
				GameState->OnRep_AdditionalPlaylistLevelsStreamed();
				GameState->OnFinishedStreamingAdditionalPlaylistLevel();
				GameMode->HandleAllPlaylistLevelsVisible();

				if (!EnsureServerListening(GameMode))
				{
					ServerListening = false;
					return false;
				}

				GuardUnsafeStartupLevelStreaming("setup complete");
				SetupServer = true;
				bSetupServerComplete = true;
			}

			/*if (UWorld::GetWorld()->NetDriver->ClientConnections.Num() > 0) {
				std::cout << "Return true\n";
				return true;
			}*/
		}

		/*if (GameMode->bDelayedStart)
		{
			return false;
		}*/

		//std::cout << GameMode->GetMatchState().ToString() << "\n";

		/*if (GameMode->GetMatchState().ToString() == "WaitingToStart")
		{
			if (GameMode->NumPlayers + GameMode->NumBots > 0)
			{
				Log("Enough Players/Bots, Starting match!");
				return true;
			}
		}*/

		if (SetupServer)
		{
			bSetupServerComplete = true;
			return false;
		}

		return false;
		//return UWorld::GetWorld()->NetDriver->ClientConnections.Num() > 0;
	}

	inline APawn* SpawnDefaultPawnFor(AFortGameModeAthena* GameMode, AFortPlayerController* Player, AActor* StartingLoc)
	{
		if (!GameMode || !Player || !StartingLoc)
			return nullptr;

		if (Player->Pawn)
			return 0;

		AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Player;
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		if (!PC || !PlayerState || !GameState)
			return nullptr;

		auto Transform = StartingLoc->GetTransform();
		auto Pawn = (AFortPlayerPawn*)GameMode->SpawnDefaultPawnAtTransform(Player, Transform);
		if (!Pawn)
			return nullptr;

		Abilities::InitAbilitiesForPlayer(PC);

		if (PC->XPComponent)
		{
			PC->XPComponent->bRegisteredWithQuestManager = true;
			PC->XPComponent->OnRep_bRegisteredWithQuestManager();

			PlayerState->SeasonLevelUIDisplay = PC->XPComponent->CurrentLevel;
			PlayerState->OnRep_SeasonLevelUIDisplay();
		}

		auto* QuestManager = PC->GetQuestManager(ESubGame::Athena);
		if (QuestManager)
			QuestManager->InitializeQuestAbilities(Pawn);

		FFortAthenaLoadout& CosmecticLoadoutPC = PC->CosmeticLoadoutPC;
		bool bHasValidCosmeticLoadout = EnsureValidAthenaLoadout(CosmecticLoadoutPC, "player");

		if (bHasValidCosmeticLoadout)
		{
			UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization(PlayerState);
			PlayerState->OnRep_CharacterData();
		}

		PlayerState->SquadId = PlayerState->TeamIndex - 3;
		PlayerState->OnRep_SquadId();

		FGameMemberInfo Member;
		Member.MostRecentArrayReplicationKey = -1;
		Member.ReplicationID = -1;
		Member.ReplicationKey = -1;
		Member.TeamIndex = PlayerState->TeamIndex;
		Member.SquadId = PlayerState->SquadId;
		Member.MemberUniqueId = PlayerState->UniqueId;

		GameState->GameMemberInfoArray.Members.Add(Member);
		GameState->GameMemberInfoArray.MarkItemDirty(Member);

		UAthenaPickaxeItemDefinition* PickDef;
		PickDef = CosmecticLoadoutPC.Pickaxe != nullptr ? CosmecticLoadoutPC.Pickaxe : StaticLoadObject<UAthenaPickaxeItemDefinition>("/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");
		//UFortWeaponMeleeItemDefinition* PickDef = StaticLoadObject<UFortWeaponMeleeItemDefinition>("/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");
		if (PickDef) {
			Inventory::GiveItem(PC, PickDef->WeaponDefinition, 1, 0);
		}
		else {
			Log("Pick Doesent Exist!");
		}
		if (Pawn)
		{
			Pawn->CosmeticLoadout = PC->CosmeticLoadoutPC;
			TryApplyPawnCosmeticLoadout(Pawn, "player pawn");
		}

		for (size_t i = 0; i < GameMode->StartingItems.Num(); i++)
		{
			if (GameMode->StartingItems[i].Count > 0)
			{
				Inventory::GiveItem(PC, GameMode->StartingItems[i].Item, GameMode->StartingItems[i].Count, 0);
			}
		}

		GameState->OnRep_SafeZoneIndicator();
		GameState->OnRep_SafeZonePhase();

		PlayerState->ForceNetUpdate();
		Pawn->ForceNetUpdate();
		PC->ForceNetUpdate();

		if (PlayerState && Pawn && Pawn->CosmeticLoadout.Character && Pawn->CosmeticLoadout.Character->HeroDefinition
			&& EnsurePawnCustomizationAssetLoader(Pawn, "player pawn customization apply"))
			ApplyCharacterCustomization(PlayerState, Pawn);

		return Pawn;
		//return (AFortPlayerPawnAthena*)GameMode->SpawnDefaultPawnAtTransform(Player, Transform);
	}

	inline bool IsValidAircraftZone(const FBox2D& Zone)
	{
		return Zone.bIsValid
			&& Zone.Max.X > Zone.Min.X
			&& Zone.Max.Y > Zone.Min.Y;
	}

	inline bool IsAircraftStartupReady(AFortGameStateAthena* GameState, std::string* Reason = nullptr)
	{
		if (!GameState)
		{
			if (Reason) *Reason = "missing game state";
			return false;
		}

		AFortAthenaMapInfo* MapInfo = GameState->MapInfo;
		if (!MapInfo)
		{
			if (Reason) *Reason = "missing map info";
			return false;
		}

		if (!MapInfo->AircraftClass)
		{
			if (Reason) *Reason = "missing aircraft class";
			return false;
		}

		if (!MapInfo->AircraftDropVolume)
		{
			if (Reason) *Reason = "missing aircraft drop volume";
			return false;
		}

		if (!IsValidAircraftZone(MapInfo->AircraftSpawnZone))
		{
			if (Reason) *Reason = "invalid aircraft spawn zone";
			return false;
		}

		if (!IsValidAircraftZone(MapInfo->AircraftDropZone))
		{
			if (Reason) *Reason = "invalid aircraft drop zone";
			return false;
		}

		return true;
	}

	inline void PutPlayerInFallbackSkydive(AFortPlayerControllerAthena* PC)
	{
		if (!PC)
			return;

		// Clear the warmup "WAITING FOR PLAYERS" waiting/spectator flags up front so the
		// fresh restart below hands the client a playing pawn instead of re-entering the
		// waiting camera.
		PC->bPlayerIsWaiting = false;
		if (PC->PlayerState)
		{
			PC->PlayerState->bIsSpectator = false;
			PC->PlayerState->bOnlySpectator = false;
			PC->PlayerState->ForceNetUpdate();
		}

		// Why the player was stuck in free cam: the client still holds the *warmup* pawn
		// the manual spawn gave it during ServerReadyToStartMatch. StartMatch() does NOT
		// restart a controller that already has a pawn, and flipping the waiting/spectator
		// flags after the fact never re-runs the engine's client restart handshake -- so
		// the client stays in the warmup waiting camera even though the server shows
		// pawn=1 ackPawn=1 viewTarget=1. Do exactly what the real bus-jump does
		// (ServerAttemptAircraftJump): drop the warmup pawn and let the native
		// RestartPlayer spawn a FRESH gameplay pawn, now that the match is InProgress, then
		// drive ClientRestart ourselves (native RestartPlayer does not reliably emit the
		// client RPC in this server-as-client setup). g_OGSMatchStarted is already latched,
		// so the transient un-acknowledge from the re-possess can't bounce the phase back
		// to warmup, and the whole step runs under RunNoAircraftNative's SEH.
		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

		APawn* OldPawn = PC->Pawn;
		if (OldPawn)
		{
			PC->UnPossess();            // GetPawn() == null so RestartPlayer spawns a fresh pawn
			OldPawn->K2_DestroyActor();
		}

		if (GameMode)
			GameMode->RestartPlayer(PC);

		if (PC->Pawn)
		{
			PC->ClientRestart(PC->Pawn);
			PC->ClientRetryClientRestart(PC->Pawn);
		}

		static FName PlayingState = UKismetStringLibrary::Conv_StringToName(L"Playing");
		PC->ClientGotoState(PlayingState);

		if (PC->Pawn)
			PC->Pawn->ForceNetUpdate();
		PC->ForceNetUpdate();
	}

	static unsigned __int64 g_NoAirFaultRva = 0;
	static const char* g_NoAirFaultStep = "";
	static int NoAirFaultFilter(EXCEPTION_POINTERS* ep, const char* step)
	{
		g_NoAirFaultRva = (unsigned __int64)((uintptr_t)ep->ExceptionRecord->ExceptionAddress - ImageBase);
		g_NoAirFaultStep = step;
		return EXCEPTION_EXECUTE_HANDLER;
	}

	// Runs the engine-touching steps of the no-aircraft fallback under SEH so a
	// fault in any one cannot vanish the server and does not stop the others. In
	// particular JumpToSafeZonePhase assumes flight-path/storm setup we deliberately
	// skip and can fault -- but the skydive (which actually drops the player in) and
	// the OnReps (which replicate the phase change to the client) must still run.
	// No C++ object unwinding in here so __try/__except is legal (C2712).
	static void RunNoAircraftNative(AFortGameModeAthena* GameMode, AFortGameStateAthena* GameState,
		EAthenaGamePhase OldPhase, bool* MatchOk, bool* JumpOk, bool* SkydiveOk, bool* RepOk)
	{
		// Drive the GameMode match state to InProgress. The client's "WAITING FOR
		// PLAYERS" spectator camera is keyed off MatchState (not the Athena GamePhase
		// we drive), and the native match-start is skipped to block aircraft -- so
		// without this MatchState stays at WaitingToStart and the player never gets
		// control of their character. Guarded because HandleMatchHasStarted is native.
		__try { GameMode->StartMatch(); *MatchOk = true; }
		__except (NoAirFaultFilter(GetExceptionInformation(), "StartMatch")) { *MatchOk = false; }

		__try { GameMode->JumpToSafeZonePhase(); *JumpOk = true; }
		__except (NoAirFaultFilter(GetExceptionInformation(), "JumpToSafeZonePhase")) { *JumpOk = false; }

		__try
		{
			for (int32 i = 0; i < GameMode->AlivePlayers.Num(); i++)
				PutPlayerInFallbackSkydive(GameMode->AlivePlayers[i]);

			for (size_t b = 0; b < PlayerBotArray.size(); b++)
			{
				auto PlayerBot = PlayerBotArray[b];
				if (PlayerBot && PlayerBot->Pawn)
				{
					// No aircraft to skydive from -- BeginSkydiving(true) faults here. Drop
					// the bot straight to the ground/Landed state instead; the combat state
					// machine (Landed -> Looting -> LookingForPlayers) takes over from there
					// and never touches the bus/aircraft.
					PlayerBot->BotState = EBotState::Landed;
				}
			}
			*SkydiveOk = true;
		}
		__except (NoAirFaultFilter(GetExceptionInformation(), "FallbackSkydive")) { *SkydiveOk = false; }

		__try
		{
			GameState->OnRep_Aircraft();
			GameState->OnRep_GamePhase(OldPhase);
			GameState->OnRep_SafeZoneIndicator();
			GameState->OnRep_SafeZonePhase();
			GameState->ForceNetUpdate();
			*RepOk = true;
		}
		__except (NoAirFaultFilter(GetExceptionInformation(), "OnReps")) { *RepOk = false; }
	}

	static bool SafeStartMatchOnly(AFortGameModeAthena* GameMode)
	{
		__try { GameMode->StartMatch(); return true; }
		__except (NoAirFaultFilter(GetExceptionInformation(), "StartMatch(bus)")) { return false; }
	}

	// Flip the GameMode MatchState to InProgress exactly once. The client's "WAITING FOR
	// PLAYERS" camera -- and the input lock that comes with it -- is keyed off MatchState,
	// NOT the Athena GamePhase we drive. Because ReadyToStartMatch returns false to block
	// the unsafe native startup, the match otherwise never starts, so the player never
	// gets control of their pawn (in warmup OR after deploying from the bus). Unlike the
	// no-aircraft fallback we do NOT JumpToSafeZonePhase -- the real bus drives its own
	// phases. Latched via g_OGSMatchStarted; SEH-guarded since StartMatch ->
	// HandleMatchHasStarted is native and can fault.
	inline void EnsureMatchInProgress(AFortGameModeAthena* GameMode)
	{
		if (g_OGSMatchStarted || !GameMode)
			return;
		g_OGSMatchStarted = true;
		if (!SafeStartMatchOnly(GameMode))
		{
			char buf[176];
			sprintf_s(buf, "StartMatch FAULTED at ImageBase+0x%llX -- player control may be limited.", (unsigned long long)g_NoAirFaultRva);
			LogError(buf);
		}
		else
			Log("Started match (MatchState -> InProgress) so the player gains control.");
	}

	inline void StartNoAircraftFallback(AFortGameModeAthena* GameMode, AFortGameStateAthena* GameState, const std::string& Reason)
	{
		if (!GameMode || !GameState)
			return;

		// StartAircraftPhase can reach the hook twice in the same tick (our warmup-expiry
		// path and the native warmup timer both fire), which would double-run StartMatch /
		// JumpToSafeZonePhase and double-restart every player. g_OGSMatchStarted is only
		// ever set here, so use it to make the drop happen exactly once.
		if (g_OGSMatchStarted)
		{
			Log("No-aircraft fallback already ran; ignoring duplicate StartAircraftPhase.");
			return;
		}

		EAthenaGamePhase OldPhase = GameState->GamePhase;
		float Now = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());

		GameState->bGameModeWillSkipAircraft = true;
		GameState->AirCraftBehavior = EAirCraftBehavior::NoAircraft;
		GameState->CachedSafeZoneStartUp = ESafeZoneStartUp::StartsWithNoAirCraft;
		GameState->bAircraftIsLocked = false;
		GameState->AircraftStartTime = Now;
		GameState->SafeZonesStartTime = Now;
		GameState->GamePhase = EAthenaGamePhase::SafeZones;
		GameState->GamePhaseStep = EAthenaGamePhaseStep::StormForming;
		GameMode->bFlightPathInitialized = true;

		// We are intentionally starting the match now -- stop the warmup-hold from
		// dragging the phase back to warmup from here on.
		g_OGSMatchStarted = true;

		bool MatchOk = false, JumpOk = false, SkydiveOk = false, RepOk = false;
		RunNoAircraftNative(GameMode, GameState, OldPhase, &MatchOk, &JumpOk, &SkydiveOk, &RepOk);

		if (!MatchOk || !JumpOk || !SkydiveOk || !RepOk)
		{
			char buf[256];
			sprintf_s(buf, "No-aircraft fallback partial (match=%d jump=%d skydive=%d rep=%d); last fault in %s at ImageBase+0x%llX -- server kept alive.",
				MatchOk ? 1 : 0, JumpOk ? 1 : 0, SkydiveOk ? 1 : 0, RepOk ? 1 : 0, g_NoAirFaultStep, (unsigned long long)g_NoAirFaultRva);
			LogError(buf);
		}

		Log(std::string("Skipped native aircraft phase because ") + Reason + ".");
	}

	static __int64 (*StartAircraftPhaseOG)(AFortGameModeAthena* GameMode, char a2) = nullptr;
	inline bool g_RealBusLaunched = false;
	__int64 StartAircraftPhase(AFortGameModeAthena* GameMode, char a2)
	{
		Log("StartAircraftPhase hook called.");

		// Once the real bus has actually launched, ignore any re-entrant StartAircraftPhase
		// (e.g. EnsureMatchInProgress -> StartMatch -> HandleMatchHasStarted can call it
		// again) so we never spawn a second bus. Early-return paths below never set this,
		// so blocked/retry cases still get to try again.
		if (g_RealBusLaunched)
		{
			Log("StartAircraftPhase re-entered after the bus already launched; ignoring.");
			return 0;
		}

		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		if (!Globals::LateGame && !Globals::bCreativeEnabled && !HasReadyHumanPlayer(GameMode))
		{
			HoldWarmupCountdown(GameMode, GameState, 30.f, "blocked aircraft phase");
			Log("Blocked StartAircraftPhase until a human player is spawned and loading is complete.");
			return 0;
		}

		if (!Globals::LateGame && !Globals::bCreativeEnabled)
		{
			std::string AircraftBlockReason;
			if (GameState && GameState->CurrentPlaylistInfo.BasePlaylist
				&& (GameState->CurrentPlaylistInfo.BasePlaylist->bSkipAircraft
					|| GameState->CurrentPlaylistInfo.BasePlaylist->AirCraftBehavior == EAirCraftBehavior::NoAircraft
					|| GameState->CurrentPlaylistInfo.BasePlaylist->SafeZoneStartUp == ESafeZoneStartUp::StartsWithNoAirCraft))
			{
				StartNoAircraftFallback(GameMode, GameState, "playlist is configured for no aircraft");
				return 0;
			}

			if (Globals::bSkipUnsafeAircraftPhase)
			{
				StartNoAircraftFallback(GameMode, GameState, "native aircraft flight path initialization is unsafe for this startup");
				return 0;
			}

			if (!IsAircraftStartupReady(GameState, &AircraftBlockReason))
			{
				HoldWarmupCountdown(GameMode, GameState, 15.f, "waiting for aircraft drop-zone data");
				static int32 AircraftReadinessFailures = 0;
				AircraftReadinessFailures++;

				if (AircraftReadinessFailures < 4)
				{
					Log(std::string("Blocked StartAircraftPhase until aircraft startup data is valid: ") + AircraftBlockReason + ".");
					return 0;
				}

				StartNoAircraftFallback(GameMode, GameState, AircraftBlockReason);
				return 0;
			}

			if (Globals::bSkipUnsafeAircraftPhase
				&& GameState
				&& GameState->MapInfo
				&& GameState->MapInfo->FlightInfos.Num() <= 0
				&& GameState->TeamFlightPaths.Num() <= 0)
			{
				StartNoAircraftFallback(GameMode, GameState, "no cached aircraft flight path is available");
				return 0;
			}
		}

		for (auto FactionBot : FactionBots) // idek if im supposed to be setting these for the henchmens
		{
			static auto Name1 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhaseStep"));
			static auto Name2 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhase"));
			FactionBot->PC->Blackboard->SetValueAsEnum(Name1, (uint8)EAthenaGamePhaseStep::BusLocked);
			FactionBot->PC->Blackboard->SetValueAsEnum(Name2, (uint8)EAthenaGamePhase::Aircraft);
		}

		for (auto PlayerBot : PlayerBotArray)
		{

			if (!Globals::LateGame) {
				auto Name1 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhaseStep"));
				auto Name2 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhase"));
				PlayerBot->PC->Blackboard->SetValueAsEnum(Name1, (uint8)EAthenaGamePhaseStep::BusLocked);
				PlayerBot->PC->Blackboard->SetValueAsEnum(Name2, (uint8)EAthenaGamePhase::Aircraft);
			}
			else {
				auto Name1 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhaseStep"));
				auto Name2 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhase"));
				PlayerBot->PC->Blackboard->SetValueAsEnum(Name1, (uint8)EAthenaGamePhaseStep::BusFlying);
				PlayerBot->PC->Blackboard->SetValueAsEnum(Name2, (uint8)EAthenaGamePhase::Aircraft);
			}

			static auto Name4 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_JumpOffBus_ExecutionStatus"));
			static auto Name3 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_IsInBus"));
			PlayerBot->PC->Blackboard->SetValueAsBool(Name3, true);
			PlayerBot->PC->Blackboard->SetValueAsEnum(Name4, (uint8)EExecutionStatus::ExecutionAllowed);
			 
			if (!Globals::LateGame) {
				PlayerBot->BotState = EBotState::PreBus; // Proper!
			}
			else {
				PlayerBot->BotState = EBotState::Bus;
			}
		}

		// Human players are boarded onto the bus from the tick (BoardHumansOntoBus) rather
		// than here -- GetAircraft(0) is often still null the instant StartAircraftPhaseOG
		// returns, so a one-shot board here would silently miss.
		g_RealBusLaunched = true; // latch BEFORE StartMatch so any re-entrant call is ignored

		// Start the match now (MatchState -> InProgress) so the client leaves the "WAITING
		// FOR PLAYERS" camera and accepts input -- this is what gives the player control of
		// their pawn (in the bus and after deploying). StartMatch's native
		// HandleMatchHasStarted resets the phase to warmup, but StartAircraftPhaseOG right
		// below overrides it back to the aircraft phase in the SAME frame, so there's no
		// mid-flight reset / loading-screen flip (the bug from calling StartMatch at deploy).
		EnsureMatchInProgress(GameMode);

		return StartAircraftPhaseOG(GameMode, a2);
	}

	// Seat each manually-spawned human onto the live bus so they RIDE it (instead of the
	// client rendering their grounded warmup pawn as falling, which reads as an instant
	// jump). Sets the controller-component CurrentAircraft authoritatively on the server
	// (so PC->IsInAircraft() and the DIAG reflect it), tells the client to enter via
	// ClientEnterAircraft, and flips the player-state bInAircraft. Idempotent: skips anyone
	// already aboard. The warmup pawn is left intact and is replaced by
	// ServerAttemptAircraftJump's RestartPlayer when the player presses jump.
	inline void BoardHumansOntoBus(AFortGameModeAthena* GameMode, AFortGameStateAthena* GameState)
	{
		if (Globals::LateGame || Globals::bCreativeEnabled || !GameMode || !GameState)
			return;

		// Seat each controller exactly once per match. IsInAircraft() does not reliably
		// report our manual seating back as true, so guarding on it re-issued
		// ClientEnterAircraft every tick -- which floods the log AND keeps resetting the
		// client's ride each frame so it never settles. Track who we've boarded instead.
		static std::set<void*> s_BoardedPCs;

		AFortAthenaAircraft* Bus = GameState->GetAircraft(0);
		if (!Bus)
		{
			// No live bus (pre-aircraft or post-drop): forget seating so a fresh match
			// boards again.
			s_BoardedPCs.clear();
			return;
		}

		for (int32 i = 0; i < GameMode->AlivePlayers.Num(); i++)
		{
			AFortPlayerControllerAthena* HumanPC = GameMode->AlivePlayers[i];
			if (!HumanPC || (HumanPC->PlayerState && HumanPC->PlayerState->bIsABot))
				continue;
			if (s_BoardedPCs.count(HumanPC))
				continue; // already seated this match

			UFortControllerComponent_Aircraft* AircraftComp = HumanPC->GetAircraftComponent();
			if (AircraftComp)
			{
				AircraftComp->CurrentAircraft = (AFortAircraft*)Bus; // authoritative server-side
				AircraftComp->ClientEnterAircraft((AFortAircraft*)Bus); // client riding camera
			}
			if (HumanPC->PlayerState)
				((AFortPlayerStateZone*)HumanPC->PlayerState)->ServerSetInAircraft(true);

			s_BoardedPCs.insert(HumanPC);
			char bbuf[160];
			sprintf_s(bbuf, "Boarded human onto the bus (comp=%d curAircraft=%d inAir=%d).",
				AircraftComp ? 1 : 0,
				(AircraftComp && AircraftComp->CurrentAircraft) ? 1 : 0,
				HumanPC->IsInAircraft() ? 1 : 0);
			Log(bbuf);
		}
	}

	static inline void (*OriginalOnAircraftExitedDropZone)(AFortGameModeAthena* GameMode, AFortAthenaAircraft* FortAthenaAircraft);
	void OnAircraftExitedDropZone(AFortGameModeAthena* GameMode, AFortAthenaAircraft* FortAthenaAircraft)
	{
		Log("OnAircraftExitedDropZone!");

		if (Globals::bBotsEnabled) { // kick all bots out of the bus
			AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
			AActor* Aircraft = GameState ? GameState->GetAircraft(0) : nullptr;
			for (auto PlayerBot : PlayerBotArray) {
				// Only skydive bots off a real bus. The bare BeginSkydiving fallback (no
				// aircraft) faults in this setup; with the native bus restored Aircraft is
				// valid, and if it is momentarily null the per-tick Bus logic ejects them.
				if (PlayerBot->BotState == EBotState::Bus && Aircraft) {
					FVector JumpLoc = Aircraft->K2_GetActorLocation();
					JumpLoc.Z += 500.f;
					JumpLoc.X += (rand() % 400) - 200;
					JumpLoc.Y += (rand() % 400) - 200;
					BotsBTService_AIDropZone::StartBotSkydivingFromBus(PlayerBot, JumpLoc, {});
				}
			}
		}

		if (Globals::bEventEnabled)
		{
			UFunction* StartEventFunc = Event::JerkyLoader->Class->GetFunction("BP_Jerky_Loader_C", "startevent");

			float ToStart = 0.f;
			Event::JerkyLoader->ProcessEvent(StartEventFunc, &ToStart);
		}
		
		return OriginalOnAircraftExitedDropZone(GameMode, FortAthenaAircraft);
	}

	__int64 (*OnAircraftEnteredDropZoneOG)(AFortGameModeAthena*);
	__int64 OnAircraftEnteredDropZone(AFortGameModeAthena* a1)
	{
		Log("OnAircraftEnteredDropZone Called!");

		for (auto FactionBot : FactionBots)
		{
			static auto Name1 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhaseStep"));
			static auto Name2 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhase"));
			FactionBot->PC->Blackboard->SetValueAsEnum(Name1, (uint8)EAthenaGamePhaseStep::BusFlying);
			FactionBot->PC->Blackboard->SetValueAsEnum(Name2, (uint8)EAthenaGamePhase::Aircraft);
		}

		if (!Globals::LateGame) {
			for (auto PlayerBot : PlayerBotArray)
			{
				static auto Name1 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhaseStep"));
				static auto Name2 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhase"));
				PlayerBot->PC->Blackboard->SetValueAsEnum(Name1, (uint8)EAthenaGamePhaseStep::BusFlying);
				PlayerBot->PC->Blackboard->SetValueAsEnum(Name2, (uint8)EAthenaGamePhase::Aircraft);

				static auto Name9 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_IsInBus"));

				PlayerBot->PC->Blackboard->SetValueAsBool(Name9, true);

				PlayerBot->BotState = EBotState::Bus;
			}
		}

		return OnAircraftEnteredDropZoneOG(a1);
	}

	void (*StormOG)(AFortGameModeAthena* GameMode, int32 ZoneIndex);
	void __fastcall Storm(AFortGameModeAthena* GameMode, int32 ZoneIndex)
	{
		auto GameState = (AFortGameStateAthena*)GameMode->GameState;

		static bool First = true;

		if (Globals::LateGame && !First) //fixes crashing
		{
			for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
			{
				Quests::GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeID_SurviveStormCircle.AccoladeID_SurviveStormCircle"));
			}
		}
		else if (!Globals::LateGame)
		{
			for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
			{
				Quests::GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeID_SurviveStormCircle.AccoladeID_SurviveStormCircle"));
			}
		}

		static bool initstorm = false;

		if (Globals::LateGame && !initstorm)
		{
			initstorm = true;
			GameMode->SafeZonePhase = 4;
			GameState->SafeZonePhase = 4;
			GameState->SafeZonesStartTime = 0;
			ZoneIndex = 4;
			First = false;

			if (Globals::bBotsEnabled) {
				for (size_t i = 0; i < PlayerBotArray.size(); i++)
				{
					PlayerBotArray[i]->BotState = EBotState::Bus;
				}
			}
		}

		if (Globals::LateGame && initstorm)
		{
			int newPhase = GameState->SafeZonePhase + 1;

			GameMode->SafeZonePhase = newPhase;
			GameState->SafeZonePhase = newPhase;
		}

		if (Globals::bBotsEnabled && !Globals::LateGame) {
			for (size_t i = 0; i < PlayerBotArray.size(); i++)
			{
				// Lets actually make sure the bot landed first before moving to zone
				if (PlayerBotArray[i]->BotState < EBotState::Landed)
					continue;
				PlayerBotArray[i]->BotState = EBotState::MovingToSafeZone; // i dont know the best way to get the bots to move to zone tbh
			}
		}

		return StormOG(GameMode, ZoneIndex);
	}

	void Hook() {
		MH_CreateHook((LPVOID)(ImageBase + 0x4640A30), ReadyToStartMatch, (LPVOID*)&ReadyToStartMatchOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x18F6250), SpawnDefaultPawnFor, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x18F9BB0), StartAircraftPhase, (LPVOID*)&StartAircraftPhaseOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x18E07D0), OnAircraftExitedDropZone, (LPVOID*)&OriginalOnAircraftExitedDropZone);

		MH_CreateHook((LPVOID)(ImageBase + 0x18E0730), OnAircraftEnteredDropZone, (LPVOID*)&OnAircraftEnteredDropZoneOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x18FD350), Storm, (LPVOID*)&StormOG);

		Log("Gamemode Hooked!");
	}
}
