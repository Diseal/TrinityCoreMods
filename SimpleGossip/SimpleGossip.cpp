// This code is licensed under MIT license

#include "SimpleGossip.h"
#include "ScriptedGossip.h"
#include "Creature.h"
#include "Player.h"
#include <iostream>
#include <unordered_map>
#include <vector>

///////////////////////////////////////////////////////////////////////////////////////
// ****************************** Simple Gossip Option ***************************** //
///////////////////////////////////////////////////////////////////////////////////////

bool SimpleGossipOption::ShowOption(Player* player) { return true; }

SimpleGossipOptionIconText::SimpleGossipOptionIconText(
    GossipOptionIcon icon,
    std::string text,
    SGIconTextCallback callback)
{
    Icon = icon;
    Text = text;
    IconTextCallback = callback;
}

SimpleGossipOptionIconTextPopup::SimpleGossipOptionIconTextPopup(
    GossipOptionIcon icon,
    std::string text,
    std::string popupText,
    uint32 popupCopper,
    SGIconTextPopupCallback callback)
{
    Icon = icon;
    Text = text;
    PopupText = popupText;
    PopupCopper = popupCopper;
    IconTextPopupCallback = callback;
}

SimpleGossipOptionIconTextPopup::SimpleGossipOptionIconTextPopup(
    GossipOptionIcon icon,
    std::string text,
    std::string popupText,
    uint32 popupGold,
    uint8 popupSilver,
    uint8 popupCopper,
    SGIconTextPopupCallback callback)
{
    Icon = icon;
    Text = text;
    PopupText = popupText;
    PopupCopper = popupGold * 10000 + popupSilver * 100 + popupCopper;
    IconTextPopupCallback = callback;
}

SimpleGossipOptionDatabaseMenu::SimpleGossipOptionDatabaseMenu(
    uint32 menuId,
    uint32 menuItemId,
    SGDatabaseMenuCallback callback)
{
    MenuId = menuId;
    MenuItemId = menuItemId;
    DatabaseMenuCallback = callback;
}

bool SimpleGossipOption::SelectOption(Player* player)
{
    if (CloseDialogOnSelect)
    {
        ClearGossipMenuFor(player);
        CloseGossipMenuFor(player);
    }

    // Next parts > restarting
    if (NextParts.size() > 0)
    {
        if (NextTextId != 0)
        {
            Gossip->ShowParts(player, NextTextId, NextParts);
        }
        else
        {
            Gossip->ShowParts(player, NextParts);
        }
    }
    else if (NextTextId != 0)
    {
        Gossip->ShowParts(player, NextTextId, Gossip->StartingPartIds);
    }
    else if (RestartOnSelect)
    {
        Gossip->ShowStartingParts(player);
    }

    if (BaseCallback != nullptr)
    {
        BaseCallback(player, this);
    }

    return true;
}

bool SimpleGossipOptionIconText::SelectOption(Player* player)
{
    if (IconTextCallback == nullptr)
    {
        return false;
    }

    IconTextCallback(player, this);
    SimpleGossipOption::SelectOption(player);

    return true;
}
bool SimpleGossipOptionIconText::ShowOption(Player* player)
{
    if (ConditionallyShow != nullptr && !ConditionallyShow(player, this))
    {
        return false;
    }
    AddGossipItemFor(player, Icon, Text, GOSSIP_SENDER_MAIN, OptionId);
    return true;
}

bool SimpleGossipOptionIconTextPopup::SelectOption(Player* player)
{
    if (IconTextPopupCallback == nullptr)
    {
        return false;
    }

    auto money = player->GetMoney();
    bool success = false;
    if (money >= PopupCopper && player->ModifyMoney(PopupCopper * -1))
    {
        success = true;
    }

    IconTextPopupCallback(player, success, this);
    SimpleGossipOption::SelectOption(player);

    return true;
}
bool SimpleGossipOptionIconTextPopup::ShowOption(Player* player)
{
    if (ConditionallyShow != nullptr && !ConditionallyShow(player, this))
    {
        return false;
    }
    AddGossipItemFor(player, Icon, Text, GOSSIP_SENDER_MAIN, OptionId, PopupText, PopupCopper, IsCoded);
    return true;
}

bool SimpleGossipOptionDatabaseMenu::SelectOption(Player* player)
{
    if (DatabaseMenuCallback == nullptr)
    {
        return false;
    }

    DatabaseMenuCallback(player, this);
    SimpleGossipOption::SelectOption(player);

    return true;
}
bool SimpleGossipOptionDatabaseMenu::ShowOption(Player* player)
{
    if (ConditionallyShow != nullptr && !ConditionallyShow(player, this))
    {
        return false;
    }
    AddGossipItemFor(player, MenuId, MenuItemId, GOSSIP_SENDER_MAIN, OptionId);
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////
// ****************************** Simple Gossip Part ******************************* //
///////////////////////////////////////////////////////////////////////////////////////

bool SimpleGossipPart::ShowPart(Player* player)
{
    if (ConditionallyShow != nullptr && !ConditionallyShow(player, this))
    {
        return false;
    }

    for (std::vector<uint32>::iterator itr = OptionIds.begin(); itr != OptionIds.end(); ++itr)
    {
        SimpleGossipOption* option = Gossip->GetOptionById(*itr);
        if (option == nullptr)
        {
            return false;
        }
        option->ShowOption(player);
    }

    return true;
}

void SimpleGossipPart::AddOption(SimpleGossipOption* option)
{
    Gossip->AddOption(option);
    AddOptionId(option->OptionId);
}

void SimpleGossipPart::AddOptionId(uint32 optionId)
{
    OptionIds.push_back(optionId);
}

bool SimpleGossipPart::RemoveOptionId(uint32 optionId)
{
    std::vector<uint32> optionIdToRemove{ optionId };
    std::vector<uint32>::iterator it = optionIdToRemove.begin();
    uint32 size = OptionIds.size();
    OptionIds.erase(it);
    return size != OptionIds.size();
}

bool SimpleGossipPart::Clear()
{
    if (OptionIds.size() == 0)
    {
        return false;
    }
    OptionIds.clear();
    return OptionIds.size() == 0;
}

///////////////////////////////////////////////////////////////////////////////////////
// ********************************* Simple Gossip ********************************* //
///////////////////////////////////////////////////////////////////////////////////////

SimpleGossip::~SimpleGossip()
{
    for (std::unordered_map<uint32, SimpleGossipPart*>::iterator itr = Parts.begin(); itr != Parts.end(); ++itr)
    {
        delete& itr->second;
    }
    for (std::unordered_map<uint32, SimpleGossipOption*>::iterator itr = Options.begin(); itr != Options.end(); ++itr)
    {
        delete& itr->second;
    }
}

bool SimpleGossip::StartGossip(Player* player, ObjectGuid sender)
{
    bool show = ShowParts(player, StartingPartIds);
    return show;
}
bool SimpleGossip::StartGossip(Player* player, Creature* sender)
{
    bool show = ShowParts(player, sender, StartingPartIds);
    return show;
}
bool SimpleGossip::ShowParts(Player* player, ObjectGuid sender, std::vector<uint32> parts)
{
    return ShowParts(player, sender, StartingTextId, parts);
}
bool SimpleGossip::ShowParts(Player* player, ObjectGuid sender, uint32 textId, std::vector<uint32> parts)
{
    ClearGossipMenuFor(player);
    CloseGossipMenuFor(player);

    for (std::vector<uint32>::iterator itr = parts.begin(); itr != parts.end(); ++itr)
    {
        SimpleGossipPart* part = GetPartById(*itr);
        if (part == nullptr) {
            return false;
        }
        part->ShowPart(player);
    }

    SendGossipMenuFor(player, StartingTextId, sender);

    return true;
}
bool SimpleGossip::ShowParts(Player* player, Creature* sender, std::vector<uint32> parts)
{
    return ShowParts(player, sender, StartingTextId, parts);
}
bool SimpleGossip::ShowParts(Player* player, Creature* sender, uint32 textId, std::vector<uint32> parts)
{
    ClearGossipMenuFor(player);
    CloseGossipMenuFor(player);

    for (std::vector<uint32>::iterator itr = parts.begin(); itr != parts.end(); ++itr)
    {
        SimpleGossipPart* part = GetPartById(*itr);
        if (part == nullptr) {
            return false;
        }
        part->ShowPart(player);
    }

    SendGossipMenuFor(player, StartingTextId, sender);

    return true;
}
bool SimpleGossip::ShowParts(Player* player, std::vector<uint32> parts)
{
    return ShowParts(player, StartingTextId, parts);
}
bool SimpleGossip::ShowParts(Player* player, uint32 textId, std::vector<uint32> parts)
{
    auto menu = player->PlayerTalkClass->GetGossipMenu();
    ObjectGuid sender = menu.GetSenderGUID();
    return ShowParts(player, sender, textId, parts);
}
bool SimpleGossip::ShowStartingParts(Player* player)
{
    auto menu = player->PlayerTalkClass->GetGossipMenu();
    ObjectGuid sender = menu.GetSenderGUID();
    return ShowParts(player, sender, StartingTextId, StartingPartIds);
}

bool SimpleGossip::SelectGossipOption(Player* player, uint32 option)
{
    ClearGossipMenuFor(player);
    SimpleGossipOption* gossipOption = GetOptionById(option);
    return gossipOption != nullptr && gossipOption->SelectOption(player);
}

bool SimpleGossip::CloseGossip(Player* player)
{
    CloseGossipMenuFor(player);
    return true;
}

SimpleGossipPart* SimpleGossip::GetPartById(uint32 partId)
{
    auto search = Parts.find(partId);
    if (search == Parts.end())
    {
        return nullptr;
    }
    return search->second;
}

SimpleGossipOption* SimpleGossip::GetOptionById(uint32 partId)
{
    auto search = Options.find(partId);
    if (search == Options.end())
    {
        return nullptr;
    }
    return search->second;
}

SimpleGossipPart* SimpleGossip::AddPart()
{
    SimpleGossipPart* part = new SimpleGossipPart();
    part->PartId = GetNextPartId();
    part->Gossip = this;
    Parts[part->PartId] = part;
    IncremenetNextPartId();

    return part;
}

void SimpleGossip::AddPart(SimpleGossipPart* part)
{
    if (part == nullptr || part->Gossip == this)
    {
        return;
    }
    if (part->Gossip != nullptr)
    {
        part->Gossip->RemoveOption(part->PartId);
    }
    part->PartId = GetNextPartId();
    part->Gossip = this;
    Parts[part->PartId] = part;
    IncremenetNextPartId();
}

bool SimpleGossip::RemovePart(uint32 partId)
{
    auto result = Parts.erase(partId);

    return result > 0;
}

bool SimpleGossip::RemoveAndDeletePart(uint32 partId)
{
    SimpleGossipPart* part = GetPartById(partId);
    if (part == nullptr)
    {
        return false;
    }

    auto result = Parts.erase(partId);

    delete part;

    return true;
}

void SimpleGossip::AddOption(SimpleGossipOption* option)
{
    if (option == nullptr || option->Gossip == this)
    {
        return;
    }
    if (option->Gossip != nullptr)
    {
        option->Gossip->RemoveOption(option->OptionId);
    }
    option->OptionId = GetNextOptionId();
    option->Gossip = this;
    Options[option->OptionId] = option;
    IncremenetNextOptionId();
}

bool SimpleGossip::RemoveOption(uint32 optionId)
{
    auto result = Options.erase(optionId);

    for (std::unordered_map<uint32, SimpleGossipPart*>::iterator itr = Parts.begin(); itr != Parts.end(); ++itr)
    {
        itr->second->RemoveOptionId(optionId);
    }

    return result > 0;
}

bool SimpleGossip::RemoveAndDeleteOption(uint32 optionId)
{
    SimpleGossipOption* option = GetOptionById(optionId);
    if (option == nullptr)
    {
        return false;
    }

    auto result = Options.erase(optionId);

    for (std::unordered_map<uint32, SimpleGossipPart*>::iterator itr = Parts.begin(); itr != Parts.end(); ++itr)
    {
        itr->second->RemoveOptionId(optionId);
    }

    delete option;

    return true;
}

bool SimpleGossip::Clear()
{
    if (Parts.size() == 0 && Options.size() == 0)
    {
        return false;
    }

    Parts.clear();
    Options.clear();

    return Parts.size() == 0 && Options.size() == 0;
}

bool SimpleGossip::ClearAndDelete()
{
    if (Parts.size() == 0 && Options.size() == 0)
    {
        return false;
    }

    for (std::unordered_map<uint32, SimpleGossipPart*>::iterator itr = Parts.begin(); itr != Parts.end(); ++itr)
    {
        delete itr->second;
    }
    for (std::unordered_map<uint32, SimpleGossipOption*>::iterator itr = Options.begin(); itr != Options.end(); ++itr)
    {
        delete itr->second;
    }
    Parts.clear();
    Options.clear();

    return Parts.size() == 0 && Options.size() == 0;
}

bool CONDITIONALLY_SHOW_TRUE(Player* player, SimpleGossipOption* option)
{
    return true;
};
bool CONDITIONALLY_SHOW_FALSE(Player* player, SimpleGossipOption* option)
{
    return true;
};
void DONOTHING_BASECALLBACK(Player* player, SimpleGossipOption* option) {};
void DONOTHING_ICONTEXT(Player* player, SimpleGossipOptionIconText* option) {};
void DONOTHING_ICONTEXTPOPUP(Player* player, bool success, SimpleGossipOptionIconTextPopup* option) {};
void DONOTHING_DATABASEMENU(Player* player, SimpleGossipOptionDatabaseMenu* option) {};
