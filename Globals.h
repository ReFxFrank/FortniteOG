#pragma once
#include <xkeycheck.h>

namespace Globals {
	bool bIsProdServer = false;

	bool bCreativeEnabled = false;
	bool bSTWEnabled = false;
	bool bEventEnabled = false;

	bool bBossesEnabled = false;
	bool bBotsEnabled = true;
	// The "OGS Test Panel" debug window. Off by default -- it only exposes dev test
	// commands (bots/speed/water) and isn't needed for normal play. Flip to true to bring
	// it back for debugging.
	bool bAdminPanelEnabled = false;

	bool bUseLegacyAI_MANG = true;
	// false = run the real native aircraft/battle-bus phase (like the upstream OGS
	// reference). When false we stop stripping the flight/nav streaming levels and stop
	// skipping ReadyToStartMatchOG, so the engine builds a real flight path and the bus
	// flies. Set back to true to fall straight back to the no-aircraft instant drop.
	bool bSkipUnsafeAircraftPhase = false;

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
	int MaxBotsToSpawn = 95;
	int MinPlayersForEarlyStart = 100;
}
