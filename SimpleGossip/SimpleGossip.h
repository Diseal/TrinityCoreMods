// This code is licensed under MIT license

#ifndef _SIMPLEGOSSIP_H
#define _SIMPLEGOSSIP_H

#include "GossipDef.h"
#include "Player.h"
#include <unordered_map>
#include <string>

class SimpleGossip;
class SimpleGossipPart;
class SimpleGossipOption;
class SimpleGossipOptionIconText;
class SimpleGossipOptionIconTextPopup;
class SimpleGossipOptionDatabaseMenu;

typedef std::function<void(Player* player, SimpleGossipOption* option)> SGBaseCallback;
typedef std::function<bool(Player* player, SimpleGossipOption* option)> SGConditionallyShow;
typedef std::function<void(Player* player, SimpleGossipOptionIconText* option)> SGIconTextCallback;
typedef std::function<void(Player* player, bool success, SimpleGossipOptionIconTextPopup* option)> SGIconTextPopupCallback;
typedef std::function<void(Player* player, SimpleGossipOptionDatabaseMenu* option)> SGDatabaseMenuCallback;

bool CONDITIONALLY_SHOW_TRUE(Player* player, SimpleGossipOption* option);
bool CONDITIONALLY_SHOW_FALSE(Player* player, SimpleGossipOption* option);
void DONOTHING_BASECALLBACK(Player* player, SimpleGossipOption* option);
void DONOTHING_ICONTEXT(Player* player, SimpleGossipOptionIconText* option);
void DONOTHING_ICONTEXTPOPUP(Player* player, bool success, SimpleGossipOptionIconTextPopup* option);
void DONOTHING_DATABASEMENU(Player* player, SimpleGossipOptionDatabaseMenu* option);

//typedef SimpleGossipOption::SGConditionallyShow SGConditionallyShow;
//typedef SimpleGossipOption::SGBaseCallback SGBaseCallback;
//typedef SimpleGossipOptionNewParts::SGNewPartsCallback SGNewPartsCallback;
//typedef SimpleGossipOptionIconText::SGIconTextCallback SGIconTextCallback;
//typedef SimpleGossipOptionIconTextPopup::SGIconTextPopupCallback SGIconTextPopupCallback;
//typedef SimpleGossipOptionDatabaseMenu::SGDatabaseMenuCallback SGDatabaseMenuCallback;

class SimpleGossipOption
{
public:
    SimpleGossipOption() = default;

    SimpleGossip* Gossip = nullptr;
	uint32 OptionId = 0;
    uint32 NextTextId = 0;
    std::vector<uint32> NextParts;
    bool RestartOnSelect = true;
    bool CloseDialogOnSelect = false;

    SGConditionallyShow ConditionallyShow = nullptr;
    SGBaseCallback BaseCallback = nullptr;

	virtual bool ShowOption(Player* player);
	virtual bool SelectOption(Player* player);
};

class SimpleGossipOptionIconText : public SimpleGossipOption
{
public:
    SGIconTextCallback IconTextCallback = nullptr;

	GossipOptionIcon Icon;
	std::string Text;

    SimpleGossipOptionIconText(
        GossipOptionIcon icon,
        std::string text,
        SGIconTextCallback callback);

	bool ShowOption(Player* player);
	bool SelectOption(Player* player);
};

class SimpleGossipOptionIconTextPopup : public SimpleGossipOption
{
public:
    SGIconTextPopupCallback IconTextPopupCallback = nullptr;

	GossipOptionIcon Icon;
	std::string Text;
	std::string PopupText;
	uint32 PopupCopper;
	bool IsCoded = false;

	SimpleGossipOptionIconTextPopup(
		GossipOptionIcon icon,
		std::string text,
		std::string popupText,
		uint32 popupCopper,
        SGIconTextPopupCallback callback);
	SimpleGossipOptionIconTextPopup(
		GossipOptionIcon icon,
		std::string text,
		std::string popupText,
		uint32 popupGold,
		uint8 popupSilver,
		uint8 popupCopper,
        SGIconTextPopupCallback callback);

	bool ShowOption(Player* player);
	bool SelectOption(Player* player);
};

class SimpleGossipOptionDatabaseMenu : public SimpleGossipOption
{
public:
    SGDatabaseMenuCallback DatabaseMenuCallback;

	uint32 MenuId;
	uint32 MenuItemId;

    SimpleGossipOptionDatabaseMenu(
        uint32 menuId,
        uint32 menuItemId,
        SGDatabaseMenuCallback callback);

	bool ShowOption(Player* player);
	bool SelectOption(Player* player);
};

////////////////////////////////////////////////////////////////////////////////////////////
// A single part of a gossip menu.
// It contains references to options which it will display.
// This allows parts to operate independantly.
////////////////////////////////////////////////////////////////////////////////////////////

class SimpleGossipPart
{
public:
    SimpleGossipPart() = default;

	SimpleGossip* Gossip = nullptr;
	uint32 PartId = 0;

	std::vector<uint32> OptionIds;

	bool (*ConditionallyShow)(Player* player, SimpleGossipPart* option);

	bool ShowPart(Player* player);
	void AddOption(SimpleGossipOption* option);
	void AddOptionId(uint32 optionId);
	bool RemoveOptionId(uint32 optionId);
	bool Clear();
};

////////////////////////////////////////////////////////////////////////////////////////////
// An easy way to set up coded gossip menus.
// Each gossip contains a set of parts and options to use.
// A single menu page can have multiple parts which each have their own options within them.
////////////////////////////////////////////////////////////////////////////////////////////
class SimpleGossip
{
protected:
	uint32 NextPartId = 1;
	uint32 NextOptionId = 1;
	std::unordered_map<uint32, SimpleGossipPart*> Parts;
	std::unordered_map<uint32, SimpleGossipOption*> Options;
public:
	std::vector<uint32> StartingPartIds;
    uint32 StartingTextId = 2; // The "Hello <name>, how can I help you?" text an NPC has above their options, stored in db

	SimpleGossip() = default;
	~SimpleGossip();

    bool StartGossip(Player* player, ObjectGuid sender);
    bool StartGossip(Player* player, Creature* sender);
    bool ShowParts(Player* player, ObjectGuid sender, std::vector<uint32> parts);
    bool ShowParts(Player* player, Creature* sender, std::vector<uint32> parts);
    bool ShowParts(Player* player, ObjectGuid sender, uint32 textId, std::vector<uint32> parts);
    bool ShowParts(Player* player, Creature* sender, uint32 textId, std::vector<uint32> parts);
    bool ShowParts(Player* player, uint32 textId, std::vector<uint32> parts);
    bool ShowParts(Player* player, std::vector<uint32> parts);
    bool ShowStartingParts(Player* player);

	bool SelectGossipOption(Player* player, uint32 action);
	bool CloseGossip(Player* player);

	SimpleGossipPart* AddPart();
	void AddPart(SimpleGossipPart* part);
	bool RemovePart(uint32 partId);
	bool RemoveAndDeletePart(uint32 partId);
	void AddOption(SimpleGossipOption* option);
	bool RemoveOption(uint32 optionId);
	bool RemoveAndDeleteOption(uint32 optionId);
	bool Clear();
	bool ClearAndDelete();

	SimpleGossipPart* GetPartById(uint32 partId);
	SimpleGossipOption* GetOptionById(uint32 partId);

	std::unordered_map<uint32, SimpleGossipPart*> GetParts() { return Parts; }
	uint32 GetNextPartId() { return NextPartId; }
	void IncremenetNextPartId() { NextPartId += 1; }

	std::unordered_map<uint32, SimpleGossipOption*> GetOptions() { return Options; }
	uint32 GetNextOptionId() { return NextOptionId; }
	void IncremenetNextOptionId() { NextOptionId += 1; }
};

#endif
