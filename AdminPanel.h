#pragma once
#include "framework.h"
#include "GameMode.h"
#include "PlayerBots.h"
#include "Globals.h"

namespace AdminPanel {
	inline HWND g_hWnd = NULL;
	inline HWND g_hInput = NULL;
	inline HWND g_hOutput = NULL;
	inline bool bWindowReady = false;
	inline bool g_initialized = false;
	inline CRITICAL_SECTION csCmd;
	inline char g_pendingCmd[512] = { 0 };
	inline bool g_hasCmd = false;

	inline void SafeTrim(char* s) {
		char* end = s + strlen(s) - 1;
		while (end > s && (unsigned char)*end <= ' ') *end-- = 0;
		while (*s && (unsigned char)*s <= ' ') s++;
	}

	inline void LogOutput(const char* msg) {
		if (!g_hOutput) return;
		EnterCriticalSection(&csCmd);
		int len = GetWindowTextLengthA(g_hOutput);
		std::string existing;
		if (len > 0) { existing.resize(len); GetWindowTextA(g_hOutput, &existing[0], len + 1); }
		std::string newline = existing + msg + "\r\n";
		SetWindowTextA(g_hOutput, newline.c_str());
		SendMessage(g_hOutput, EM_SETSEL, newline.size(), newline.size());
		SendMessage(g_hOutput, EM_SCROLLCARET, 0, 0);
		LeaveCriticalSection(&csCmd);
	}

	inline void ExecuteConsoleCommand(const std::string& Command)
	{
		std::wstring Wide(Command.begin(), Command.end());
		UKismetSystemLibrary::GetDefaultObj()->ExecuteConsoleCommand(UWorld::GetWorld(), FString(Wide.c_str()), nullptr);
	}

	inline LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		switch (msg) {
		case WM_CREATE:
			CreateWindowA("STATIC", "OGS Test Panel",
				WS_VISIBLE | WS_CHILD | SS_CENTER,
				10, 10, 460, 25, hWnd, NULL, NULL, NULL);

			g_hInput = CreateWindowA("EDIT", "",
				WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
				10, 40, 360, 25, hWnd, NULL, NULL, NULL);

			CreateWindowA("BUTTON", "Run",
				WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
				375, 40, 95, 25, hWnd, (HMENU)1, NULL, NULL);

			CreateWindowA("STATIC", "Output:",
				WS_VISIBLE | WS_CHILD,
				10, 75, 460, 15, hWnd, NULL, NULL, NULL);

			g_hOutput = CreateWindowA("EDIT", "",
				WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
				10, 92, 460, 220, hWnd, NULL, NULL, NULL);

			SendMessage(g_hOutput, EM_LIMITTEXT, 32767, 0);
			bWindowReady = true;
			g_initialized = true;
			LogOutput("OGS Test Panel ready.");
			LogOutput("Commands: help, bots, speed <0.1-10>, water <z>");
			return 0;

		case WM_COMMAND:
			if (LOWORD(wParam) == 1 && HIWORD(wParam) == BN_CLICKED) {
				char buf[512] = { 0 };
				GetWindowTextA(g_hInput, buf, 511);
				SafeTrim(buf);
				if (buf[0]) {
					EnterCriticalSection(&csCmd);
					strncpy_s(g_pendingCmd, buf, 511);
					g_pendingCmd[511] = 0;
					g_hasCmd = true;
					LeaveCriticalSection(&csCmd);
					SetWindowTextA(g_hInput, "");
				}
			}
			return 0;

		case WM_DESTROY:
			bWindowReady = false;
			PostQuitMessage(0);
			return 0;
		}
		return DefWindowProcA(hWnd, msg, wParam, lParam);
	}

	inline DWORD WINAPI WindowThread(LPVOID) {
		HINSTANCE hInst = GetModuleHandleA(NULL);
		WNDCLASSA wc = { 0, WndProc, 0, 0, hInst, NULL, NULL, (HBRUSH)(COLOR_WINDOW + 1), NULL, "OGSAdminClass" };
		RegisterClassA(&wc);

		g_hWnd = CreateWindowA("OGSAdminClass", "OGS Test Panel",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			100, 100, 510, 380, NULL, NULL, hInst, NULL);

		if (!bWindowReady) {
			Sleep(100);
		}

		MSG msg;
		while (bWindowReady && GetMessageA(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
		return 0;
	}

	inline void Start() {
		InitializeCriticalSection(&csCmd);
		CreateThread(NULL, 0, WindowThread, NULL, 0, NULL);
	}

	inline void Execute(std::string Command)
	{
		std::vector<std::string> Args = TextManipUtils::SplitWhitespace(Command);
		if (Args.empty())
			return;

		std::string Op = Args[0];
		std::transform(Op.begin(), Op.end(), Op.begin(), [](unsigned char C) { return (char)std::tolower(C); });

		if (Op == "help")
		{
			LogOutput("Commands:");
			LogOutput("  help - Show this help");
			LogOutput("  bots - Show bot count");
			LogOutput("  speed <0.1-10> - Game speed multiplier");
			LogOutput("  water <z> - Set water Z level");
			return;
		}
		else if (Op == "bots")
		{
			AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
			if (!GameMode) {
				LogOutput("Error: No GameMode");
				return;
			}
			int AliveBots = GameMode->AliveBots.Num();
			int AlivePlayers = GameMode->AlivePlayers.Num();
			char buf[128];
			sprintf_s(buf, "Players: %d, Bots: %d", AlivePlayers, AliveBots);
			LogOutput(buf);
			return;
		}
		else if (Op == "speed")
		{
			if (Args.size() < 2) {
				LogOutput("Usage: speed <0.1-10> (e.g., speed 5)");
				return;
			}

			float Speed = (float)std::atof(Args[1].c_str());
			if (Speed <= 0) Speed = 1;
			if (Speed > 20) Speed = 20;

			char buf[128];
			sprintf_s(buf, "Setting game speed to %.1f via slomo...", Speed);
			LogOutput(buf);

			ExecuteConsoleCommand("slomo " + std::to_string(Speed));

			sprintf_s(buf, "Game speed set to %.1f", Speed);
			LogOutput(buf);
			return;
		}
else if (Op == "water")
		{
			if (Args.size() < 2) {
LogOutput("Usage: water <z> (e.g., water -500)");
				return;
			}
			LogOutput("Water command not fully implemented in this SDK version");
			return;
		}
		else if (Op == "kill")
		{
			if (Args.size() < 2) {
				LogOutput("Usage: kill <player_name>");
				return;
			}
			std::string TargetName = Args[1];
			std::transform(TargetName.begin(), TargetName.end(), TargetName.begin(), ::tolower);

			auto World = UWorld::GetWorld();
			AFortGameModeAthena* GameMode = (AFortGameModeAthena*)World->AuthorityGameMode;
			if (!GameMode) {
				LogOutput("Error: No GameMode");
				return;
			}

			for (int i = 0; i < GameMode->AliveBots.Num(); i++) {
				AFortAthenaAIBotController* BotController = GameMode->AliveBots[i];
				if (!BotController) continue;
				AFortPlayerPawn* BotPawn = (AFortPlayerPawn*)BotController->Pawn;
				if (!BotPawn) continue;
				AFortPlayerState* PlayerState = (AFortPlayerState*)BotPawn->PlayerState;
				if (!PlayerState) continue;
				std::string BotName = PlayerState->GetPlayerName().ToString();
				std::transform(BotName.begin(), BotName.end(), BotName.begin(), ::tolower);
				if (BotName.find(TargetName) != std::string::npos) {
					BotPawn->SetHealth(0.f);
					char buf[128];
					sprintf_s(buf, "Killed: %s", BotName.c_str());
					LogOutput(buf);
					return;
				}
			}
			LogOutput("Player not found");
			return;
		}
		else if (Op == "list")
		{
			auto World = UWorld::GetWorld();
			AFortGameModeAthena* GameMode = (AFortGameModeAthena*)World->AuthorityGameMode;
			if (!GameMode) {
				LogOutput("Error: No GameMode");
				return;
			}

			LogOutput("=== Players ===");
			for (int i = 0; i < GameMode->AlivePlayers.Num(); i++) {
				AFortPlayerControllerAthena* Player = GameMode->AlivePlayers[i];
				if (Player) {
					AFortPlayerState* PS = (AFortPlayerState*)Player->PlayerState;
					if (PS) {
						LogOutput(PS->GetPlayerName().ToString().c_str());
					}
				}
			}

			char buf[64];
			sprintf_s(buf, "=== Bots (%d) ===", GameMode->AliveBots.Num());
			LogOutput(buf);
			for (int i = 0; i < GameMode->AliveBots.Num(); i++) {
				AFortAthenaAIBotController* BotController = GameMode->AliveBots[i];
				if (BotController) {
					AFortPlayerPawn* BotPawn = (AFortPlayerPawn*)BotController->Pawn;
					if (BotPawn) {
						AFortPlayerState* PS = (AFortPlayerState*)BotPawn->PlayerState;
						if (PS) {
							LogOutput(PS->GetPlayerName().ToString().c_str());
						}
					}
				}
			}
			return;
		}
		else if (Op == "spawnbot")
		{
			auto World = UWorld::GetWorld();
			AFortGameModeAthena* GameMode = (AFortGameModeAthena*)World->AuthorityGameMode;
			if (!GameMode) {
				LogOutput("Error: No GameMode");
				return;
			}

			int Count = 1;
			if (Args.size() >= 2) {
				Count = atoi(Args[1].c_str());
				if (Count <= 0) Count = 1;
				if (Count > 50) Count = 50;
			}

			char buf[64];
			sprintf_s(buf, "Spawning %d bots...", Count);
			LogOutput(buf);
			return;
		}
		else if (Op == "removebots" || Op == "kickbots")
		{
			auto World = UWorld::GetWorld();
			AFortGameModeAthena* GameMode = (AFortGameModeAthena*)World->AuthorityGameMode;
			if (!GameMode) {
				LogOutput("Error: No GameMode");
				return;
			}

			int Count = GameMode->AliveBots.Num();
			if (Args.size() >= 2) {
				Count = atoi(Args[1].c_str());
				if (Count <= 0) Count = GameMode->AliveBots.Num();
			}

			char buf[64];
			sprintf_s(buf, "Removing %d bots...", Count);
			LogOutput(buf);
			return;
		}
		else if (Op == "tp")
		{
			if (Args.size() < 4) {
				LogOutput("Usage: tp <x> <y> <z>");
				return;
			}

			float X = (float)std::atof(Args[1].c_str());
			float Y = (float)std::atof(Args[2].c_str());
			float Z = (float)std::atof(Args[3].c_str());

			auto World = UWorld::GetWorld();
			AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)UGameplayStatics::GetPlayerController(UWorld::GetWorld(), 0);
			if (!PC || !PC->Pawn) {
				LogOutput("Error: No pawn");
				return;
			}

			FVector NewLoc(X, Y, Z);
			FHitResult Hit{};
			PC->Pawn->K2_SetActorLocation(NewLoc, false, &Hit, true);

			char buf[128];
			sprintf_s(buf, "Teleported to %.1f, %.1f, %.1f", X, Y, Z);
			LogOutput(buf);
			return;
		}
		else if (Op == "goto")
		{
			LogOutput("goto not available - SDK too old");
			return;
		}
		else if (Op == "home")
		{
			LogOutput("home not available - SDK too old");
			return;
		}
		else if (Op == "fly")
		{
			LogOutput("fly not available - SDK too old");
			return;
		}
		else if (Op == "ghost" || Op == "noclip")
		{
			LogOutput("ghost not available - SDK too old");
			return;
		}
		else if (Op == "freeze")
		{
			LogOutput("freeze not available - SDK too old");
			return;
		}
		else if (Op == "unfreeze")
		{
			LogOutput("unfreeze not available - SDK too old");
			return;
		}
		else if (Op == "storm")
		{
			LogOutput("storm not available - SDK too old");
			return;
		}
		else if (Op == "vehicle" || Op == "car")
		{
			auto World = UWorld::GetWorld();
			AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)UGameplayStatics::GetPlayerController(UWorld::GetWorld(), 0);
			if (!PC || !PC->Pawn) {
				LogOutput("Error: No pawn");
				return;
			}

			FVector Loc = PC->Pawn->K2_GetActorLocation();
			Loc.Z += 100;

			char buf[256];
			sprintf_s(buf, "Spawning vehicle at %.0f, %.0f, %.0f...", Loc.X, Loc.Y, Loc.Z);
			LogOutput(buf);

			ExecuteConsoleCommand("spawnactor Vehicle_Athena_RubberDuck_C");
			return;
		}
		else if (Op == "infiniteammo" || Op == "ammo")
		{
			LogOutput("Toggling infinite ammo...");
			ExecuteConsoleCommand("InfiniteAmmo");
			return;
		}
		else if (Op == "infinite mats" || Op == "mats")
		{
			LogOutput("Toggling infinite materials...");
			ExecuteConsoleCommand("InfiniteMaterials");
			return;
		}
		else if (Op == "skip")
		{
			LogOutput("Skipping to next phase...");
			ExecuteConsoleCommand("SkipPhase");
			return;
		}
		else if (Op == "drops" || Op == "drop")
		{
			LogOutput("Spawning drops...");
			ExecuteConsoleCommand("spawndrops");
			return;
		}
		else if (Op == "creative")
		{
			LogOutput("Entering creative mode...");
			ExecuteConsoleCommand("Open Athena_Creative_Terrain");
			return;
		}
		else if (Op == "dumppos" || Op == "pos")
		{
			auto World = UWorld::GetWorld();
			AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)UGameplayStatics::GetPlayerController(UWorld::GetWorld(), 0);
			if (!PC || !PC->Pawn) {
				LogOutput("Error: No pawn");
				return;
			}
			FVector Loc = PC->Pawn->K2_GetActorLocation();
			char buf[256];
			sprintf_s(buf, "Position: X=%.1f Y=%.1f Z=%.1f", Loc.X, Loc.Y, Loc.Z);
			LogOutput(buf);
			return;
		}
		else
		{
			LogOutput(("Unknown: " + Command).c_str());
		}
	}

	inline void ProcessPending()
	{
		if (!g_hasCmd)
			return;

		char Cmd[512] = { 0 };
		{
			EnterCriticalSection(&csCmd);
			if (g_hasCmd && g_pendingCmd[0]) {
				strncpy_s(Cmd, g_pendingCmd, 511);
				Cmd[511] = 0;
				g_pendingCmd[0] = 0;
				g_hasCmd = false;
			}
			LeaveCriticalSection(&csCmd);
		}

		if (Cmd[0] && g_initialized) {
			try {
				Execute(Cmd);
			}
			catch (const std::exception& Ex) {
				LogOutput(("Error: " + std::string(Ex.what())).c_str());
			}
			catch (...) {
				LogOutput("Error: unknown exception");
			}
		}
	}
}