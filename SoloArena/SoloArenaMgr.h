// This code is licensed under MIT license

#ifndef _SOLOARENAMGR_H
#define _SOLOARENAMGR_H

#include "SimpleGossip.h"
#include "ArenaTeam.h"
#include <vector>
#include <unordered_map>
#include <string>

enum ArenaTeamType
{
	TWO_VS_TWO = 2,
	THREE_VS_THREE = 3,
	FIVE_VS_FIVE = 5
};

enum ArenaTeamSlot
{
	FIRST_SLOT = 0,
	SECOND_SLOT = 1,
	THIRD_SLOT = 2
};

class TC_GAME_API SoloArenaMgr
{
protected:
	std::vector<uint32> ParseConfigStringIntoUInt32Array(std::string str);
	std::unordered_map<uint32, uint32> MapForbiddenTalentTreeLimits(std::vector<uint32> trees, std::vector<uint32> treeLimits);
public:
	static SoloArenaMgr* instance();

	SimpleGossip* Gossip = nullptr;
	bool Enable;
	uint8 MinLevel;
	uint32 CharterCost;

	bool EnableForbiddenTalentTreeBlocking;
	bool DisableForbiddenTalentTreesCompletely;
	std::vector<uint32> ForbiddenTalentTrees;

	std::unordered_map<uint32, uint32> ForbiddenTalentTreeLimits;

	bool EnableForbiddenSpellBlocking;
	std::vector<uint32> ForbiddenSpells;

	bool CheckIfPlayerTalentsAndSpellsAreAllowed(Player* player);
	void InitializeSoloArenaMgr();
	void SetupGossip(SimpleGossip* gossip);

	bool QueueForSkrimish(Player* player);
	bool QueueForRated(Player* player);
	bool JoinArenaQueue(Player* player, bool rated);
	bool LeaveQueue(Player* player);

	bool IsPlayerRegistered(Player* player);
	ArenaTeam* GetSoloArenaTeam(Player* player);
	std::string GetSoloArenaTeamNameForPlayer(std::string playerName);

    bool RegisterForRated(Player* player, ArenaTeamSlot replace, bool chatWarnings = false);
	bool UnregisterFromRated(Player* player, bool chatWarnings = false);
	bool SwapRatedReplacement(Player* player, ArenaTeamSlot replace, bool chatWarnings = false);

	bool DisplayRatedStatistics(Player* player);
	bool DisplayServerStatistics(Player* player);
};

#define sSoloArenaMgr SoloArenaMgr::instance()

#endif
