#pragma once
#include "framework.h"

struct LocRot {
	FVector Location;
	FRotator Rotation;
};

TArray<LocRot> CameraSpawners;
TArray<AActor*> CameraActors;

namespace Net {
	enum class ENetMode
	{
		Standalone,
		DedicatedServer,
		ListenServer,
		Client,

		MAX,
	};

	ENetMode (*WorldGetNetModeOG)(UWorld*);
	ENetMode WorldGetNetMode(UWorld* a1)
	{
		// This hook unconditionally reports a dedicated server. Do NOT touch a1
		// here: GetNetMode() is called on the replication hot path (inside
		// ServerReplicateActors / the ReplicationGraph) for actors that may be
		// mid-construction or pending-kill, so dereferencing the argument (the
		// old code did a1->GetName()) is a needless crash/perf surface.
		return ENetMode::DedicatedServer;
	}

	ENetMode (*AActorGetNetModeOG)(AActor*);
	ENetMode AActorGetNetMode(AActor* a1)
	{
		// Hot path: AActor::GetNetMode() is invoked for every actor many times
		// per frame during ServerReplicateActors. The hook always returns
		// DedicatedServer, so never dereference a1 (the old code called
		// a1->GetName()) -- a transient/pending-kill actor would fault here,
		// which matches the server dying on the first replication after a
		// player is possessed.
		return ENetMode::DedicatedServer;
	}

	void Hook() {
		MH_CreateHook((LPVOID)(ImageBase + 0x45C9D90), WorldGetNetMode, (LPVOID*)&WorldGetNetModeOG);
		MH_CreateHook((LPVOID)(ImageBase + 0x3EB6780), AActorGetNetMode, (LPVOID*)&AActorGetNetModeOG);

		Log("Hooked Net!");
	}
}