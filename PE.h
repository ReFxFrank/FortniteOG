#pragma once
#include "framework.h"

namespace PE {
    static void* (*ProcessEventOG)(UObject*, UFunction*, void*);
    void* ProcessEvent(UObject* Obj, UFunction* Function, void* Params)
    {
        bool bClientFinishedLoading = false;
        AFortPlayerControllerAthena* PC = nullptr;
        const char* EarlyLoadingReason = nullptr;

        if (Obj && Function)
        {
            std::string FunctionName = Function->GetName();
            if (FunctionName == "ServerSetClientHasFinishedLoading" && Params)
            {
                bClientFinishedLoading = *(bool*)Params;
                PC = Cast<AFortPlayerControllerAthena>(Obj);
                if (PC && bClientFinishedLoading)
                    PC::MarkServerFinishedLoading(PC, "pre ServerSetClientHasFinishedLoading");
            }
            else if (FunctionName == "ServerNotifyLoadedWorld")
            {
                EarlyLoadingReason = "pre ServerNotifyLoadedWorld";
            }
            else if (FunctionName == "ServerUpdateLevelVisibility")
            {
                EarlyLoadingReason = "pre ServerUpdateLevelVisibility";
            }
            else if (FunctionName == "ServerUpdateMultipleLevelsVisibility")
            {
                EarlyLoadingReason = "pre ServerUpdateMultipleLevelsVisibility";
            }
            else if (FunctionName == "ServerCheckClientPossession" || FunctionName == "ServerCheckClientPossessionReliable")
            {
                EarlyLoadingReason = "pre ServerCheckClientPossession";
            }

            if (EarlyLoadingReason)
            {
                PC = Cast<AFortPlayerControllerAthena>(Obj);
                if (PC && !PC->bHasServerFinishedLoading)
                    PC::MarkServerFinishedLoading(PC, EarlyLoadingReason);
            }
        }

        void* Result = ProcessEventOG(Obj, Function, Params);

        if (PC && bClientFinishedLoading)
        {
            Log("ServerSetClientHasFinishedLoading intercepted.");
            PC::MarkServerFinishedLoading(PC, "ServerSetClientHasFinishedLoading");
            // In native aircraft mode the engine spawns the player on the warmup island
            // and boards them onto the bus itself (matching the upstream reference);
            // a manual pawn here collides with that and makes the player insta-jump.
            if (GameMode::ShouldForceNoAircraftStartup())
                PC::TryManualWarmupSpawn(PC, "ServerSetClientHasFinishedLoading");
        }

        return Result;
    }

    void Hook() {
        MH_CreateHook((LPVOID)(ImageBase + 0x02E13BF0), ProcessEvent, (LPVOID*)&ProcessEventOG);
        Log("PE Hooked!");
    }
}
