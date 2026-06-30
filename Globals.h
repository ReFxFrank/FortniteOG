#pragma once
#include <xkeycheck.h>

namespace Globals {
	bool bIsProdServer = false;

	bool bCreativeEnabled = false;
	bool bSTWEnabled = false;
	bool bEventEnabled = false;

	bool bBossesEnabled = false;
	bool bBotsEnabled = true;
	bool bAdminPanelEnabled = true;

	bool bUseLegacyAI_MANG = true;
	bool bSkipUnsafeAircraftPhase = true;

	bool LateGame = false;
	bool Automatics = true;
	bool BattleLab = false;
	bool Blitz = false;
	bool StormKing = false;
	bool Arsenal = false;
	bool TeamRumble = false;
	bool SolidGold = false;
	bool UnVaulted = false;
	bool Siphon = false;
	bool Arena = false;

	// Total session capacity (humans + bots). The engine's AGameSession::AtCapacity
	// check rejects joins once this many players are connected, so this MUST be > 1
	// for multiple users to play together. Previously MaxPlayers was derived from
	// MaxBotsToSpawn + 1, which made a bots-off server a 1-slot lobby (only the first
	// remote player could ever connect).
	int MaxPlayers = 100;
	int MaxBotsToSpawn = 25;
	int MinPlayersForEarlyStart = 100;
}
