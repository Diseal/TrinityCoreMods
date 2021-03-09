// This code is licensed under MIT license

#include "SimpleGossip.h"
#include "SoloArenaMgr.h"
#include "Common.h"
#include "Player.h"
#include "CreatureAI.h"
#include "Log.h"
#include "ScriptMgr.h"

class custom_npc_SoloArena : public CreatureScript
{
public:
    custom_npc_SoloArena() : CreatureScript("custom_npc_SoloArena")
    {

    }

    struct custom_npc_SoloArenaAI : public CreatureAI
    {
        custom_npc_SoloArenaAI(Creature* creature) : CreatureAI(creature) { }

        bool OnGossipHello(Player* player) override
        {
            if (!player || !me)
            {
                return false;
            }

            return sSoloArenaMgr->Gossip->StartGossip(player, me);
        }

        bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
        {
            if (!player || !me)
            {
                return false;
            }

            uint32 const action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);

            sSoloArenaMgr->Gossip->SelectGossipOption(player, action);

            return true;
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
            {
                return;
            }
            DoMeleeAttackIfReady();
        }

    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new custom_npc_SoloArenaAI(creature);
    }
};

class custom_npc_SoloArena_world : public WorldScript
{
public:
    custom_npc_SoloArena_world() : WorldScript("custom_npc_SoloArena_world") {}

    void OnConfigLoad(bool reload) override
    {
        if (reload)
        {
            sSoloArenaMgr->InitializeSoloArenaMgr();
            TC_LOG_INFO("server.loading", "Reloaded custom_npc_SoloArena script...");
        }
        else // On worldserver Startup
        {
            sSoloArenaMgr->InitializeSoloArenaMgr();
            TC_LOG_INFO("server.loading", "Loaded custom_npc_SoloArena script...");
        }
    }
};

void Add_Custom_NPC_SoloArena()
{
    new custom_npc_SoloArena();
    new custom_npc_SoloArena_world();
}
