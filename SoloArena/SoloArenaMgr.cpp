// This code is licensed under MIT license

#include "SimpleGossip.h"
#include "SoloArenaMgr.h"
#include "ArenaTeam.h"
#include "ArenaTeamMgr.h"
#include "DBCStores.h" 
#include "Chat.h"
#include "Config.h"
#include "Log.h"
#include <string>
#include "BattlegroundMgr.h"
#include "DisableMgr.h"
#include "Player.h"
#include "WorldSession.h"
#include <Globals\ObjectMgr.h>

/// <summary>
/// Checks a player's talents and spells to see if they're allowed to play solo arena.
/// </summary>
/// <param name="player">The player to check for forbidden spells.</param>
/// <returns value="true">The player is allowed to play Solo Arena with their current talents and spells.</returns>
/// <returns value="false">The player is not allowed to play Solo Arena because their talents or their spells..</returns>
bool SoloArenaMgr::CheckIfPlayerTalentsAndSpellsAreAllowed(Player* player)
{
    if (!player)
        return false;

    // Check talents
    if (EnableForbiddenTalentTreeBlocking)
    {
        std::map<uint32, uint8> talentCount;

        for (uint8 talentId = 0; talentId < sTalentStore.GetNumRows(); ++talentId)
        {
            TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentId);

            if (!talentInfo)
                continue;

            for (uint8 rank = MAX_TALENT_RANK - 1; rank >= 0; --rank)
            {
                if (talentInfo->SpellRank[rank] == 0)
                    continue;

                if (player->HasTalent(talentInfo->SpellRank[rank], player->GetActiveSpec()))
                {
                    for (uint8 i = 0; ForbiddenTalentTrees[i] != 0; i++)
                    {
                        if (ForbiddenTalentTrees[i] == talentInfo->TabID)
                        {
                            if (talentCount.find(ForbiddenTalentTrees[i]) == talentCount.end())
                            {
                                // No entry yet, start one
                                talentCount[ForbiddenTalentTrees[i]] = rank + 1;
                            }
                            else
                            {
                                // There's an entry, add to it
                                talentCount[ForbiddenTalentTrees[i]] = talentCount[ForbiddenTalentTrees[i]] + rank + 1;
                            }
                            // Check the talent tree limits to see if there's too many
                            if (talentCount[ForbiddenTalentTrees[i]] > ForbiddenTalentTreeLimits[talentCount[ForbiddenTalentTrees[i]]])
                            {
                                ChatHandler(player->GetSession()).SendSysMessage("You can't join because you have invested too many points in a forbidden talent tree. Please edit your talents.");
                                return false;
                            }
                        }
                    }
                }
            }
        }
    }

    if (EnableForbiddenSpellBlocking)
    {
        uint32 size = ForbiddenSpells.size();
        for (uint32 index = 0; index < size; index++)
        {
            if (player->HasSpell(ForbiddenSpells[index]))
            {
                ChatHandler(player->GetSession()).SendSysMessage("You can't join because you have a forbidden spell.");
                return false;
            }
        }
    }

    return true;
}

// Initializes the configuration settings for Solo Arena.
void SoloArenaMgr::InitializeSoloArenaMgr()
{
    Enable = sConfigMgr->GetBoolDefault("Arena.1v1.Enable", true);
    MinLevel = sConfigMgr->GetIntDefault("Arena.1v1.MinLevel", 0);
    int val = sConfigMgr->GetIntDefault("Arena.1v1.CharterCostOverride", -1);
    if (val < 0)
    {
        CharterCost = sConfigMgr->GetIntDefault("ArenaTeam.CharterCost.1v1", 0);
    }
    else
    {
        CharterCost = val;
    }
    EnableForbiddenTalentTreeBlocking = sConfigMgr->GetBoolDefault("Arena.1v1.EnableForbiddenTalentTreeBlocking", false);
    DisableForbiddenTalentTreesCompletely = sConfigMgr->GetBoolDefault("Arena.1v1.DisableForbiddenTalentTreesCompletely", false);
    std::string strForbiddenTalentTrees = sConfigMgr->GetStringDefault("Arena.1v1.ForbiddenTalentTrees", "");
    std::string strForbiddenTalentTreeLimits = sConfigMgr->GetStringDefault("Arena.1v1.ForbiddenTalentTreeLimits", "");

    EnableForbiddenSpellBlocking = sConfigMgr->GetBoolDefault("Arena.1v1.EnableForbiddenSpellBlocking", false);
    std::string strForbiddenSpells = sConfigMgr->GetStringDefault("Arena.1v1.ForbiddenSpells", "");

    ForbiddenTalentTrees = ParseConfigStringIntoUInt32Array(strForbiddenTalentTrees);
    std::vector<uint32> forbiddenTalentTreeLimitsUnmapped = ParseConfigStringIntoUInt32Array(strForbiddenTalentTreeLimits);
    ForbiddenTalentTreeLimits = MapForbiddenTalentTreeLimits(ForbiddenTalentTrees, forbiddenTalentTreeLimitsUnmapped);
    ForbiddenSpells = ParseConfigStringIntoUInt32Array(strForbiddenSpells);

    if (Gossip)
    {
        delete Gossip;
    }
    Gossip = new SimpleGossip();

    SetupGossip(Gossip);
}

const std::string COLOR_BLOODRED = "|cff9F0000";

const std::string COLOR_GOLD = "|cffFFD700";
const std::string COLOR_SILVER = "|cffC0C0C0";
const std::string COLOR_COPPER = "|cffB87333";
const std::string COLOR_WOOD = "|cffDEB887";

const std::string COLOR_BLACK = "|cff000000";
const std::string COLOR_RED = "|cffFF0000";
const std::string COLOR_YELLOW = "|cffFFFF00";
const std::string COLOR_VIOLET = "|cffFF00FF";
const std::string COLOR_GREEN = "|cff00FF00";
const std::string COLOR_TEAL = "|cff00FFFF";
const std::string COLOR_BLUE = "|cff0000FF";
const std::string COLOR_WHITE = "|cffFFFFFF";

// NOTE: Solo arena teams are found by their names. If the name has no special characters in it like |, then it won't be recognized as a solo team.
const std::string COLOR_SOLOARENATEAMNAME = COLOR_BLOODRED;
const std::string COLOR_END = "|r";

const uint32 BANNER_1 = 4288676352;
const uint32 BANNER_2 = 82;
const uint32 BANNER_3 = 4294962904;
const uint32 BANNER_4 = 2;
const uint32 BANNER_5 = 4278190080;

std::string COLOR(std::string color, std::string text)
{
    return color + text + COLOR_END;
}

bool ocdIsPlayerRegisteredP(Player* player, SimpleGossipPart* part)
{
    return sSoloArenaMgr->IsPlayerRegistered(player);
}
bool ocdIsPlayerRegisteredO(Player* player, SimpleGossipOption* option)
{
    return sSoloArenaMgr->IsPlayerRegistered(player);
}
bool ocdIsntPlayerRegisteredP(Player* player, SimpleGossipPart* part)
{
    return !sSoloArenaMgr->IsPlayerRegistered(player);
}
bool ocdIsntPlayerRegisteredO(Player* player, SimpleGossipOption* option)
{
    return !sSoloArenaMgr->IsPlayerRegistered(player);
}
bool SoloArenaMgr::IsPlayerRegistered(Player* player)
{
    return GetSoloArenaTeam(player) != nullptr;
}

ArenaTeam* SoloArenaMgr::GetSoloArenaTeam(Player* player)
{
    auto ats = sArenaTeamMgr->GetArenaTeamsByPlayer(player->GetGUID());

    for (ArenaTeam* at : ats)
    {
        std::string atname = at->GetName();
        // check that the name has something funky
        if (sObjectMgr->IsReservedName(atname) || !ObjectMgr::IsValidCharterName(atname))
        {
            return at;
        }
    }

    return nullptr;
}

std::string SoloArenaMgr::GetSoloArenaTeamNameForPlayer(std::string playerName)
{
    return COLOR(COLOR_SOLOARENATEAMNAME, playerName);
}

bool ocdIsInQueueForSoloArenaO(Player* player, SimpleGossipOption* option)
{
    return player->InBattlegroundQueueForBattlegroundQueueType(BATTLEGROUND_QUEUE_1v1);
}
bool ocdIsNotInQueueForSoloArenaO(Player* player, SimpleGossipOption* option)
{
    bool inQueue = ocdIsInQueueForSoloArenaO(player, option);
    return !inQueue;
}
bool ocdIsInQueueForSoloArenaP(Player* player, SimpleGossipPart* part)
{
    return player->InBattlegroundQueueForBattlegroundQueueType(BATTLEGROUND_QUEUE_1v1);
}
bool ocdIsNotInQueueForSoloArenaP(Player* player, SimpleGossipPart* part)
{
    bool inQueue = ocdIsInQueueForSoloArenaP(player, part);
    return !inQueue;
}

void ocQueueForSkrimish(Player* player, SimpleGossipOptionIconText* option) { sSoloArenaMgr->QueueForSkrimish(player); }
bool SoloArenaMgr::QueueForSkrimish(Player* player)
{
    if (CheckIfPlayerTalentsAndSpellsAreAllowed(player) && !JoinArenaQueue(player, false))
    {
        ChatHandler(player->GetSession()).SendSysMessage("Something went wrong while joining Skrimish Solo Arena queue.");
        return false;
    }
    return true;
}

void ocQueueForRated(Player* player, SimpleGossipOptionIconText* option) { sSoloArenaMgr->QueueForRated(player); }
bool SoloArenaMgr::QueueForRated(Player* player)
{
    if (CheckIfPlayerTalentsAndSpellsAreAllowed(player) && !JoinArenaQueue(player, true))
    {
        ChatHandler(player->GetSession()).SendSysMessage("Something went wrong while joining Rated Solo Arena queue.");
        return false;
    }
    return true;
}

bool SoloArenaMgr::JoinArenaQueue(Player* player, bool rated)
{
    if (!player)
    {
        return false;
    }

    if (player->GetLevel() < sSoloArenaMgr->MinLevel)
    {
        return false;
    }

    Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(BATTLEGROUND_AA);
    if (!bg)
    {
        TC_LOG_ERROR("Arena", "Battleground: template bg (all arenas) not found");
        return false;
    }

    if (DisableMgr::IsDisabledFor(DISABLE_TYPE_BATTLEGROUND, BATTLEGROUND_AA, NULL))
    {
        ChatHandler(player->GetSession()).PSendSysMessage(LANG_ARENA_DISABLED);
        return false;
    }

    BattlegroundTypeId bgTypeId = bg->GetTypeID();
    BattlegroundQueueTypeId bgQueueTypeId = BattlegroundMgr::BGQueueTypeId(bgTypeId, ARENA_TYPE_1v1);
    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(), player->GetLevel());
    if (!bracketEntry)
    {
        return false;
    }

    GroupJoinBattlegroundResult err = ERR_GROUP_JOIN_BATTLEGROUND_FAIL;

    // check if already in queue
    if (player->GetBattlegroundQueueIndex(bgQueueTypeId) < PLAYER_MAX_BATTLEGROUND_QUEUES)
    {
        //player is already in this queue
        return false;
    }
    // check if has free queue slots
    if (!player->HasFreeBattlegroundQueueId())
    {
        return false;
    }

    ArenaTeam* arenaTeam = GetSoloArenaTeam(player);

    uint32 arenaRating = 0;
    uint32 matchmakerRating = 0;
    uint32 arenaTeamId = 0;

    if (rated)
    {
        if (!arenaTeam)
        {
            player->GetSession()->SendNotInArenaTeamPacket(ARENA_TYPE_1v1);
            return false;
        }

        arenaTeamId = arenaTeam->GetId();
        arenaRating = arenaTeam->GetRating();

        matchmakerRating = arenaRating;

        if (arenaRating <= 0)
        {
            arenaRating = 1;
        }
    }

    BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);
    bg->SetRated(rated);

    GroupQueueInfo* ginfo = bgQueue.AddGroup(player, NULL, bgTypeId, bracketEntry, ARENA_TYPE_1v1, rated, false, arenaRating, matchmakerRating, arenaTeamId);
    uint32 avgTime = bgQueue.GetAverageQueueWaitTime(ginfo, bracketEntry->GetBracketId());
    uint32 queueSlot = player->AddBattlegroundQueueId(bgQueueTypeId);

    WorldPacket data;
    // send status packet (in queue)
    sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, queueSlot, STATUS_WAIT_QUEUE, avgTime, 0, ARENA_TYPE_1v1, 0);
    player->GetSession()->SendPacket(&data);

    sBattlegroundMgr->ScheduleQueueUpdate(matchmakerRating, ARENA_TYPE_1v1, bgQueueTypeId, bgTypeId, bracketEntry->GetBracketId());

    return true;
}

void ocLeaveQueue(Player* player, SimpleGossipOptionIconText* option) { sSoloArenaMgr->LeaveQueue(player); }
bool SoloArenaMgr::LeaveQueue(Player* player)
{
    WorldPacket Data;
    Data << (uint8)0x1 << (uint8)0x0 << (uint32)BATTLEGROUND_AA << (uint16)0x0 << (uint8)0x0;
    player->GetSession()->HandleBattleFieldPortOpcode(Data);
    return true;
}

void ocRegisterForRated_Replace2v2(Player* player, bool success, SimpleGossipOptionIconTextPopup* option) { if (success) sSoloArenaMgr->RegisterForRated(player, FIRST_SLOT, true); }
void ocRegisterForRated_Replace3v3(Player* player, bool success, SimpleGossipOptionIconTextPopup* option) { if (success) sSoloArenaMgr->RegisterForRated(player, SECOND_SLOT, true); }
void ocRegisterForRated_Replace5v5(Player* player, bool success, SimpleGossipOptionIconTextPopup* option) { if (success) sSoloArenaMgr->RegisterForRated(player, THIRD_SLOT, true); }
bool SoloArenaMgr::RegisterForRated(Player* player, ArenaTeamSlot replace, bool chatWarnings)
{
    ArenaTeam* currentSoloTeam = GetSoloArenaTeam(player);

    if (currentSoloTeam != nullptr)
    {
        if (chatWarnings) ChatHandler(player->GetSession()).SendSysMessage("You are already registered for a Solo Arena Team.");
        return false;
    }

    ArenaTeam* slot = sArenaTeamMgr->GetArenaTeamById(player->GetArenaTeamId(replace));
    if (slot != nullptr)
    {
        if (chatWarnings) ChatHandler(player->GetSession()).SendSysMessage("You cannot register for Solo Arena Rated in that slot because you are already registered for an Arena Team in it.");
        return false;
    }

    ArenaTeam* arenaTeam = new ArenaTeam();
    auto arenaTeamName = GetSoloArenaTeamNameForPlayer(player->GetName());

    ArenaTeamType type = FIVE_VS_FIVE;
    switch (replace)
    {
    case FIRST_SLOT: type = TWO_VS_TWO; break;
    case SECOND_SLOT: type = THREE_VS_THREE; break;
    case THIRD_SLOT: type = FIVE_VS_FIVE; break;
    }

    if (!arenaTeam->Create(player->GetGUID(), type, arenaTeamName, BANNER_1, BANNER_2, BANNER_3, BANNER_4, BANNER_5))
    {
        if (chatWarnings) ChatHandler(player->GetSession()).SendSysMessage("Failed to create the Solo Arena Rated Team for some reason.");
        delete arenaTeam;
        return false;
    }

    // Register arena team
    sArenaTeamMgr->AddArenaTeam(arenaTeam);

    arenaTeam->AddMember(player->GetGUID());
    arenaTeam->SaveToDB();

    if (chatWarnings) ChatHandler(player->GetSession()).SendSysMessage("You are now registered for Solo Arena Rated.");

    return true;
}

void ocUnregisterFromRated(Player* player, bool success, SimpleGossipOptionIconTextPopup* option) { if (success) sSoloArenaMgr->UnregisterFromRated(player, true); }
bool SoloArenaMgr::UnregisterFromRated(Player* player, bool chatWarnings)
{
    ArenaTeam* arenaTeam = GetSoloArenaTeam(player);

    if (!arenaTeam)
    {
        if (chatWarnings) ChatHandler(player->GetSession()).SendSysMessage("You are not registered for Solo Arena Rated, so you cannot unregister.");
        return false;
    }

    arenaTeam->Disband();

    if (chatWarnings) ChatHandler(player->GetSession()).SendSysMessage("Unregistered from Solo Arena Rated.");

    return true;
}

bool ocdRegisterdAndNotInQueueP(Player* player, SimpleGossipPart* part)
{
    return ocdIsPlayerRegisteredP(player, part) && ocdIsNotInQueueForSoloArenaP(player, part);
}
bool ocdRegisterdAndNotInQueueO(Player* player, SimpleGossipOption* option)
{
    return ocdIsPlayerRegisteredO(player, option) && ocdIsNotInQueueForSoloArenaO(player, option);
}
void ocSwapRatedReplacement_Replace2v2(Player* player, SimpleGossipOptionIconText* option) { sSoloArenaMgr->SwapRatedReplacement(player, FIRST_SLOT, true); }
void ocSwapRatedReplacement_Replace3v3(Player* player, SimpleGossipOptionIconText* option) { sSoloArenaMgr->SwapRatedReplacement(player, SECOND_SLOT, true); }
void ocSwapRatedReplacement_Replace5v5(Player* player, SimpleGossipOptionIconText* option) { sSoloArenaMgr->SwapRatedReplacement(player, THIRD_SLOT, true); }
bool SoloArenaMgr::SwapRatedReplacement(Player* player, ArenaTeamSlot replace, bool chatWarnings)
{
    ArenaTeam* arenaTeam;
    arenaTeam = GetSoloArenaTeam(player);

    if (!arenaTeam)
    {
        if (chatWarnings) ChatHandler(player->GetSession()).SendSysMessage("You are not registered for Solo Arena Rated, so you cannot swap your Arena Team's position.");
        return false;
    }

    ArenaTeam* slot;
    slot = sArenaTeamMgr->GetArenaTeamById(player->GetArenaTeamId(replace));
    if (slot != nullptr)
    {
        if (chatWarnings) ChatHandler(player->GetSession()).SendSysMessage("You cannot swap your Solo Arena Rated Team to a position that's already occupied with an Arena Team.");
        return false;
    }

    ArenaTeamType type = FIVE_VS_FIVE;
    switch (replace)
    {
    case FIRST_SLOT: type = TWO_VS_TWO; break;
    case SECOND_SLOT: type = THREE_VS_THREE; break;
    case THIRD_SLOT: type = FIVE_VS_FIVE; break;
    }

    std::string playerName = player->GetName();
    std::string teamName = arenaTeam->GetName();
    ObjectGuid playerGuid = player->GetGUID();
    ArenaTeamMember* playerMemberPtr;
    playerMemberPtr = arenaTeam->GetMember(playerGuid);

    if (!playerMemberPtr)
    {
        if (chatWarnings) ChatHandler(player->GetSession()).SendSysMessage("Something went wrong trying to find you in your own team! Tell the devs.");
        return false;
    }

    ArenaTeamMember playerMember = *playerMemberPtr;
    ArenaTeamStats arenaTeamStats = arenaTeam->GetStats();

    UnregisterFromRated(player, false);

    ArenaTeam* arenaTeam2;
    arenaTeam2 = new ArenaTeam();

    uint8 c = 0;
    do {
        if (!arenaTeam2->Create(playerGuid, type, teamName, 4283124816, 45, 4294242303, 5, 4294705149))
        {
            c++;
        }
        else {
            break;
        }
    } while (c < 10);

    if (!arenaTeam2)
    {
        if (chatWarnings) ChatHandler(player->GetSession()).SendSysMessage("Failed to swap your Solo Arena Rated Team, also it was deleted, get fucked.");
        delete arenaTeam2;
        return false;
    }

    // Register arena team
    sArenaTeamMgr->AddArenaTeam(arenaTeam2);

    arenaTeam2->AddMember(playerGuid);
    arenaTeam2->SetStats(arenaTeamStats);
    ArenaTeamMember* playerMember2;
    playerMember2 = arenaTeam2->GetMember(playerName);
    playerMember2->PersonalRating = playerMember.PersonalRating;
    playerMember2->SeasonGames = playerMember.SeasonGames;
    playerMember2->SeasonWins = playerMember.SeasonWins;
    playerMember2->WeekGames = playerMember.WeekGames;
    playerMember2->WeekWins = playerMember.WeekWins;

    arenaTeam->SaveToDB();

    if (chatWarnings) ChatHandler(player->GetSession()).SendSysMessage("Your Solo Arena Rated Team has had its position swapped.");

    return true;
}

std::string getCaptainName(ArenaTeam* at)
{
    if (!at)
    {
        return "";
    }
    return at->GetMember(at->GetCaptain())->Name;
}
bool sortRatedArenaTeams(ArenaTeam* a, ArenaTeam* b)
{
    return a->GetRating() > b->GetRating();
}
bool sortRatedArenaTeamsReverse(ArenaTeam* a, ArenaTeam* b)
{
    return a->GetRating() < b->GetRating();
}
void ocDisplayRatedStatistics(Player* player, SimpleGossipOptionIconText* option) { sSoloArenaMgr->DisplayRatedStatistics(player); }
bool SoloArenaMgr::DisplayRatedStatistics(Player* player)
{
    ArenaTeam* at;
    at = GetSoloArenaTeam(player);

    if (!at)
    {
        ChatHandler(player->GetSession()).SendSysMessage(COLOR(COLOR_SOLOARENATEAMNAME, "You are currently not in a Solo Arena Team, and thus you cannot see your stats."));
        return true;
    }

    auto stats = at->GetStats();
    auto color = COLOR_WHITE;
    switch (stats.Rank)
    {
    case 0:
    case 1:
        color = COLOR_GOLD; break;
    case 2:
        color = COLOR_SILVER; break;
    case 3:
        color = COLOR_COPPER; break;
    case 4: case 5:
        color = COLOR_WOOD; break;
    }

    ChatHandler(player->GetSession()).SendSysMessage(COLOR(COLOR_SOLOARENATEAMNAME, "Your Solo Arena Stats:"));
    ChatHandler(player->GetSession()).SendSysMessage(
        COLOR(color, getCaptainName(at))
        + " Rating: " + COLOR(color, std::to_string(stats.Rating))
        + " Wins: " + COLOR(color, std::to_string(stats.SeasonWins))
        + " Loses: " + COLOR(color, std::to_string(stats.SeasonGames - stats.SeasonWins))
        + " Weekly Wins: " + COLOR(color, std::to_string(stats.WeekWins))
        + " Weekly Loses: " + COLOR(color, std::to_string(stats.WeekGames - stats.WeekWins)));

    return true;
}

void ocDisplayServerStatistics(Player* player, SimpleGossipOptionIconText* option) { sSoloArenaMgr->DisplayServerStatistics(player); }
bool SoloArenaMgr::DisplayServerStatistics(Player* player)
{
    ArenaTeamMgr::ArenaTeamContainer atc = sArenaTeamMgr->GetArenaTeams();

    uint32 size = atc.size();

    if (size == 0)
    {
        ChatHandler(player->GetSession()).SendSysMessage(COLOR(COLOR_SOLOARENATEAMNAME, "There are currently no Solo Arena Teams."));
        return true;
    }

    std::vector<ArenaTeam*> ats;

    for (auto kv : atc) {
        ArenaTeam* kvat;
        kvat = kv.second;
        std::string teamName = kvat->GetName();
        std::string captainName = getCaptainName(kvat);

        // We check that the team is a 1v1 based on the special name it uses.
        if (teamName == GetSoloArenaTeamNameForPlayer(captainName))
        {
            ats.push_back(kv.second);
        }
    }

    sort(ats.begin(), ats.end(), sortRatedArenaTeams);

    size = ats.size();

    uint32 range = 10;
    if (size < range)
    {
        range = size;
    }
    if (range <= 0)
    {
        ChatHandler(player->GetSession()).SendSysMessage(COLOR(COLOR_SOLOARENATEAMNAME, "There are currently no Solo Arena Teams."));
        return true;
    }

    std::vector<ArenaTeam*> firstTen(ats.cbegin(), ats.cbegin() + range + 1);

    ChatHandler(player->GetSession()).SendSysMessage(COLOR(COLOR_SOLOARENATEAMNAME, "Solo Arena Rankings for the Server:"));
    for (uint32 i = 0; i < range; i++)
    {
        ArenaTeam* at = firstTen[i];
        std::string color = COLOR_WHITE;
        switch (i)
        {
        case 0: color = COLOR_GOLD; break;
        case 1: color = COLOR_SILVER; break;
        case 2: color = COLOR_COPPER; break;
        case 3:
        case 4:
        case 5:
            color = COLOR_WOOD; break;
        }

        std::string captainName = getCaptainName(at);
        if (captainName.size() < 12)
        {
            captainName.append(12 - captainName.size(), ' ');
        }

        auto stats = at->GetStats();

        ChatHandler(player->GetSession()).SendSysMessage(
            COLOR(color, getCaptainName(at))
            + " Rating: " + COLOR(color, std::to_string(stats.Rating))
            + " Wins: " + COLOR(color, std::to_string(stats.SeasonWins))
            + " Loses: " + COLOR(color, std::to_string(stats.SeasonGames - stats.SeasonWins))
            + " Weekly Wins: " + COLOR(color, std::to_string(stats.WeekWins))
            + " Weekly Loses: " + COLOR(color, std::to_string(stats.WeekGames - stats.WeekWins)));
    }

    return true;
}

// Sets up the gossip for the solo arena manager.
void SoloArenaMgr::SetupGossip(SimpleGossip* gossip)
{
    if (!gossip)
    {
        return;
    }

    // Conditionals
    //SGConditionallyShow sgocdIsNotInQueueForSoloArena; sgocdIsNotInQueueForSoloArena = ocdIsNotInQueueForSoloArena;

    // Queueing Dialog
    SimpleGossipPart* pQueuing;
    pQueuing = gossip->AddPart();

    SimpleGossipOptionIconText* oGoBackToStart;
    oGoBackToStart = new SimpleGossipOptionIconText(GOSSIP_ICON_INTERACT_1, "Go Back", DONOTHING_ICONTEXT);

    SimpleGossipOptionIconText* oQueueForSkrimish;
    oQueueForSkrimish = new SimpleGossipOptionIconText(GOSSIP_ICON_BATTLE, "Queue for Solo Arena Skrimish.", ocQueueForSkrimish);
    oQueueForSkrimish->ConditionallyShow = ocdIsNotInQueueForSoloArenaO;
    SimpleGossipOptionIconText* oQueueForRated;
    oQueueForRated = new SimpleGossipOptionIconText(GOSSIP_ICON_BATTLE, "Queue for Solo Arena Rated.", ocQueueForRated);
    oQueueForRated->ConditionallyShow = ocdRegisterdAndNotInQueueO;
    SimpleGossipOptionIconText* oLeaveQueue;
    oLeaveQueue = new SimpleGossipOptionIconText(GOSSIP_ICON_TAXI, "Leave Arena Queue.", ocLeaveQueue);
    oLeaveQueue->ConditionallyShow = ocdIsInQueueForSoloArenaO;

    pQueuing->AddOption(oQueueForSkrimish);
    pQueuing->AddOption(oQueueForRated);
    pQueuing->AddOption(oLeaveQueue);

    SimpleGossipPart* pGoToRegisterPage;
    pGoToRegisterPage = gossip->AddPart();
    pGoToRegisterPage->ConditionallyShow = ocdIsntPlayerRegisteredP;
    SimpleGossipPart* pRegister;
    pRegister = gossip->AddPart();

    SimpleGossipOptionIconText* oGoToRegisterPage;
    oGoToRegisterPage = new SimpleGossipOptionIconText(GOSSIP_ICON_TALK, "Register for Solo Arena Rated.", DONOTHING_ICONTEXT);
    oGoToRegisterPage->NextParts = std::vector<uint32>{ pRegister->PartId };
    SimpleGossipOptionIconTextPopup* oRegisterForRated_Replace2v2;
    oRegisterForRated_Replace2v2 = new SimpleGossipOptionIconTextPopup(GOSSIP_ICON_TABARD, "Register, Replace 2v2.", "Are you sure you want to Register?", CharterCost, ocRegisterForRated_Replace2v2);
    SimpleGossipOptionIconTextPopup* oRegisterForRated_Replace3v3;
    oRegisterForRated_Replace3v3 = new SimpleGossipOptionIconTextPopup(GOSSIP_ICON_TABARD, "Register, Replace 3v3.", "Are you sure you want to Register?", CharterCost, ocRegisterForRated_Replace3v3);
    SimpleGossipOptionIconTextPopup* oRegisterForRated_Replace5v5;
    oRegisterForRated_Replace5v5 = new SimpleGossipOptionIconTextPopup(GOSSIP_ICON_TABARD, "Register, Replace 5v5.", "Are you sure you want to Register?", CharterCost, ocRegisterForRated_Replace5v5);

    pGoToRegisterPage->AddOption(oGoToRegisterPage);
    pRegister->AddOption(oRegisterForRated_Replace2v2);
    pRegister->AddOption(oRegisterForRated_Replace3v3);
    pRegister->AddOption(oRegisterForRated_Replace5v5);
    pRegister->AddOption(oGoBackToStart);

    SimpleGossipPart* pGoToSwapPage;
    pGoToSwapPage = gossip->AddPart();
    pGoToSwapPage->ConditionallyShow = ocdRegisterdAndNotInQueueP;
    SimpleGossipPart* pSwap;
    pSwap = gossip->AddPart();

    SimpleGossipOptionIconText* oGoToSwapRatedPage;
    oGoToSwapRatedPage = new SimpleGossipOptionIconText(GOSSIP_ICON_CHAT, "Swap Solo Arena Rated Position.", DONOTHING_ICONTEXT);
    oGoToSwapRatedPage->NextParts = std::vector<uint32>{ pSwap->PartId };
    SimpleGossipOptionIconText* oSwapRated_Replace2v2;
    oSwapRated_Replace2v2 = new SimpleGossipOptionIconText(GOSSIP_ICON_CHAT, "Swap Solo Arena Team with 2v2.", ocSwapRatedReplacement_Replace2v2);
    SimpleGossipOptionIconText* oSwapRated_Replace3v3;
    oSwapRated_Replace3v3 = new SimpleGossipOptionIconText(GOSSIP_ICON_CHAT, "Swap Solo Arena Team with 3v3.", ocSwapRatedReplacement_Replace3v3);
    SimpleGossipOptionIconText* oSwapRated_Replace5v5;
    oSwapRated_Replace5v5 = new SimpleGossipOptionIconText(GOSSIP_ICON_CHAT, "Swap Solo Arena Team with 5v5.", ocSwapRatedReplacement_Replace5v5);

    pGoToSwapPage->AddOption(oGoToSwapRatedPage);
    pSwap->AddOption(oSwapRated_Replace2v2);
    pSwap->AddOption(oSwapRated_Replace3v3);
    pSwap->AddOption(oSwapRated_Replace5v5);
    pSwap->AddOption(oGoBackToStart);

    SimpleGossipPart* pUnregister;
    pUnregister = gossip->AddPart();
    pUnregister->ConditionallyShow = ocdIsPlayerRegisteredP;

    SimpleGossipOptionIconTextPopup* oUnregisterFromRated;
    oUnregisterFromRated = new SimpleGossipOptionIconTextPopup(GOSSIP_ICON_TALK, "Unregister from Solo Arena Rated.", "Are you sure you want to Unregister?", 0, ocUnregisterFromRated);

    pUnregister->AddOption(oUnregisterFromRated);

    SimpleGossipPart* pStats = gossip->AddPart();

    SimpleGossipOptionIconText* oDisplayRatedStatistics;
    oDisplayRatedStatistics = new SimpleGossipOptionIconText(GOSSIP_ICON_INTERACT_1, "Show My Solo Arena Rated Statistics.", ocDisplayRatedStatistics);
    oDisplayRatedStatistics->ConditionallyShow = ocdIsPlayerRegisteredO;
    SimpleGossipOptionIconText* oDisplayServerStatistics;
    oDisplayServerStatistics = new SimpleGossipOptionIconText(GOSSIP_ICON_INTERACT_1, "Show Server Solo Arena Rated Statistics.", ocDisplayServerStatistics);

    pStats->AddOption(oDisplayRatedStatistics);
    pStats->AddOption(oDisplayServerStatistics);

    SimpleGossipPart* pGoodbye = gossip->AddPart();

    SimpleGossipOptionIconText* oGoodbye;
    oGoodbye = new SimpleGossipOptionIconText(GOSSIP_ICON_INTERACT_1, "Goodbye", DONOTHING_ICONTEXT);
    oGoodbye->CloseDialogOnSelect = true;

    pGoodbye->AddOption(oGoodbye);

    gossip->StartingPartIds = std::vector<uint32>{
        pQueuing->PartId,
        pGoToRegisterPage->PartId,
        pGoToSwapPage->PartId,
        pUnregister->PartId,
        pStats->PartId,
        pGoodbye->PartId
    };
}

// Parses the config string seperated by commas and spaces into an array of parsed integers
std::vector<uint32> SoloArenaMgr::ParseConfigStringIntoUInt32Array(std::string str)
{
    // Empty string, return immediately
    int strSize = str.size();
    if (strSize == 0) {
        return {};
    }

    // Get rid of all spaces, we don't need them where we're going
    str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end());

    // Empty string after removing spaces, return immediately
    strSize = str.size();
    if (strSize == 0) {
        return {};
    }

    // Split string by commas
    std::stringstream ss(str);
    std::vector<std::string> result;
    while (ss.good())
    {
        std::string substr;
        getline(ss, substr, ',');
        result.push_back(substr);
    }

    std::vector<uint32> arr;

    // Parse strings to integers and add them to the object
    std::string tmp;
    while (ss >> tmp) {
        try {
            int value = std::stoi(tmp);
            arr.push_back(value);
        }
        catch (...) {
            TC_LOG_ERROR("bg.arena", "Unable to parse config value '%u' from string to integer. Make sure comma delimited settings are correct.", tmp);
        }
    }

    return arr;
}

// Creates a map of talent tree int to how many its limited to.
std::unordered_map<uint32, uint32> SoloArenaMgr::MapForbiddenTalentTreeLimits(std::vector<uint32> trees, std::vector<uint32> treeLimits)
{
    std::unordered_map<uint32, uint32> map;

    if (trees.size() != treeLimits.size())
    {
        TC_LOG_ERROR("arena.loading", "Unable to map forbidden tree talents with their limits. Make sure the configuration is correct.");
        return map;
    }

    uint8 treeSize = trees.size();
    for (uint8 i = 0; i < treeSize; i++)
    {
        map.insert(std::pair<uint32, uint32>(trees[i], treeLimits[i]));
    }

    return map;
}

SoloArenaMgr* SoloArenaMgr::instance()
{
    static SoloArenaMgr instance;
    return &instance;
}
