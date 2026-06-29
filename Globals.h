#pragma once
#include <xkeycheck.h>

namespace Globals {
	bool bIsProdServer = false;

	bool bCreativeEnabled = false;
	bool bSTWEnabled = false;
	bool bEventEnabled = false;

	bool bBossesEnabled = false;
	bool bBotsEnabled = false;
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

	int MaxBotsToSpawn = 0;
	int MinPlayersForEarlyStart = 100;
}
