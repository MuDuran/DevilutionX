#include "panels/spell_list.hpp"

#include <cstdint>

#include <fmt/format.h>

#include "control.h"
#include "controls/plrctrls.h"
#include "engine.h"
#include "engine/backbuffer_state.hpp"
#include "engine/palette.h"
#include "engine/render/text_render.hpp"
#include "inv_iterators.hpp"
#include "options.h"
#include "panels/spell_icons.hpp"
#include "player.h"
#include "spells.h"
#include "utils/language.h"
#include "utils/str_cat.hpp"
#include "utils/utf8.hpp"

#define SPLROWICONLS 10
#define SPLSMALICONLENGTH 37

namespace devilution {

namespace {

int GetSpeedBookIconSize()
{
	if (ControlMode == ControlTypes::KeyboardAndMouse)
		return SPLSMALICONLENGTH;
	return SPLICONLENGTH;
}


void PrintSBookSpellType(const Surface &out, Point position, string_view text, uint8_t rectColorIndex, int iconSize)
{
	if (iconSize == SPLICONLENGTH)
		DrawLargeSpellIconBorder(out, position, rectColorIndex);
	else
		DrawSmallSpellIconBorder(out, position, rectColorIndex);

	position += Displacement { iconSize / 2 - GetLineWidth(text) / 2, (IsSmallFontTall() ? -19 : -15) };

	DrawString(out, text, position, { UiFlags::ColorWhite | UiFlags::Outlined });
}

void PrintSBookHotkey(const Surface &out, Point position, const string_view text, int iconSize)
{
	position += Displacement { iconSize - (GetLineWidth(text.data()) + 5), 5 - iconSize };

	DrawString(out, text, position, { UiFlags::ColorWhite | UiFlags::Outlined });
}

bool GetSpellListSelection(SpellID &pSpell, SpellType &pSplType)
{
	pSpell = SpellID::Invalid;
	pSplType = SpellType::Invalid;
	Player &myPlayer = *MyPlayer;

	for (auto &spellListItem : GetSpellListItems()) {
		if (spellListItem.isSelected) {
			pSpell = spellListItem.id;
			pSplType = spellListItem.type;
			if (myPlayer._pClass == HeroClass::Monk && spellListItem.id == SpellID::Search)
				pSplType = SpellType::Skill;
			return true;
		}
	}

	return false;
}

std::optional<string_view> GetHotkeyName(SpellID spellId, SpellType spellType, bool useShortName = false)
{
	Player &myPlayer = *MyPlayer;
	for (size_t t = 0; t < NumHotkeys; t++) {
		if (myPlayer._pSplHotKey[t] != spellId || myPlayer._pSplTHotKey[t] != spellType)
			continue;
		auto quickSpellActionKey = StrCat("QuickSpell", t + 1);
		if (ControlMode == ControlTypes::Gamepad)
			return sgOptions.Padmapper.InputNameForAction(quickSpellActionKey, useShortName);
		return sgOptions.Keymapper.KeyNameForAction(quickSpellActionKey);
	}
	return {};
}

} // namespace

void DrawSpell(const Surface &out)
{
	Player &myPlayer = *MyPlayer;
	SpellID spl = myPlayer._pRSpell;
	SpellType st = myPlayer._pRSplType;

	if (!IsValidSpell(spl)) {
		st = SpellType::Invalid;
		spl = SpellID::Null;
	}

	if (st == SpellType::Spell) {
		int tlvl = myPlayer.GetSpellLevel(spl);
		if (CheckSpell(*MyPlayer, spl, st, true) != SpellCheckResult::Success)
			st = SpellType::Invalid;
		if (tlvl <= 0)
			st = SpellType::Invalid;
	}

	if (leveltype == DTYPE_TOWN && st != SpellType::Invalid && !GetSpellData(spl).isAllowedInTown())
		st = SpellType::Invalid;

	SetSpellTrans(st);
	const Point position = GetMainPanel().position + Displacement { 565, 119 };
	DrawLargeSpellIcon(out, position, spl);

	std::optional<string_view> hotkeyName = GetHotkeyName(spl, myPlayer._pRSplType, true);
	if (hotkeyName)
		PrintSBookHotkey(out, position, *hotkeyName, SPLICONLENGTH);
}

void DrawSpellList(const Surface &out)
{
	Player &myPlayer = *MyPlayer;
	const int iconSize = GetSpeedBookIconSize();
	const bool useSmallIcons = (iconSize == SPLSMALICONLENGTH);
	const bool isKBM = ControlMode == ControlTypes::KeyboardAndMouse;

	if (!isKBM || IsMouseOverSpellList())
		InfoString = {};

	for (auto &spellListItem : GetSpellListItems()) {
		const SpellID spellId = spellListItem.id;
		SpellType transType = spellListItem.type;
		int spellLevel = 0;
		const SpellData &spellDataItem = GetSpellData(spellListItem.id);
		if (leveltype == DTYPE_TOWN && !spellDataItem.isAllowedInTown()) {
			transType = SpellType::Invalid;
		}
		if (spellListItem.type == SpellType::Spell) {
			spellLevel = myPlayer.GetSpellLevel(spellListItem.id);
			if (spellLevel == 0)
				transType = SpellType::Invalid;
		}

		SetSpellTrans(transType);
		if (useSmallIcons)
			DrawSmallSpellIcon(out, spellListItem.location, spellId);
		else
			DrawLargeSpellIcon(out, spellListItem.location, spellId);

		std::optional<string_view> shortHotkeyName = GetHotkeyName(spellId, spellListItem.type, true);

		if (shortHotkeyName)
			PrintSBookHotkey(out, spellListItem.location, *shortHotkeyName, iconSize);

		if (!spellListItem.isSelected)
			continue;

		uint8_t spellColor = PAL16_GRAY + 5;

		switch (spellListItem.type) {
		case SpellType::Skill:
			spellColor = PAL16_YELLOW - 46;
			PrintSBookSpellType(out, spellListItem.location, _("Skill"), spellColor, iconSize);
			InfoString = fmt::format(fmt::runtime(_("{:s} Skill")), pgettext("spell", spellDataItem.sNameText));
			break;
		case SpellType::Spell:
			if (!myPlayer.isOnLevel(0)) {
				spellColor = PAL16_BLUE + 5;
			}
			PrintSBookSpellType(out, spellListItem.location, _("Spell"), spellColor, iconSize);
			InfoString = fmt::format(fmt::runtime(_("{:s} Spell")), pgettext("spell", spellDataItem.sNameText));
			if (spellId == SpellID::HolyBolt) {
				AddPanelString(_("Damages undead only"));
			}
			if (spellLevel == 0)
				AddPanelString(_("Spell Level 0 - Unusable"));
			else
				AddPanelString(fmt::format(fmt::runtime(_("Spell Level {:d}")), spellLevel));
			break;
		case SpellType::Scroll: {
			if (!myPlayer.isOnLevel(0)) {
				spellColor = PAL16_RED - 59;
			}
			PrintSBookSpellType(out, spellListItem.location, _("Scroll"), spellColor, iconSize);
			InfoString = fmt::format(fmt::runtime(_("Scroll of {:s}")), pgettext("spell", spellDataItem.sNameText));
			const InventoryAndBeltPlayerItemsRange items { myPlayer };
			const int scrollCount = std::count_if(items.begin(), items.end(), [spellId](const Item &item) {
				return item.isScrollOf(spellId);
			});
			AddPanelString(fmt::format(fmt::runtime(ngettext("{:d} Scroll", "{:d} Scrolls", scrollCount)), scrollCount));
		} break;
		case SpellType::Charges: {
			if (!myPlayer.isOnLevel(0)) {
				spellColor = PAL16_ORANGE + 5;
			}
			PrintSBookSpellType(out, spellListItem.location, _("Staff"), spellColor, iconSize);
			InfoString = fmt::format(fmt::runtime(_("Staff of {:s}")), pgettext("spell", spellDataItem.sNameText));
			int charges = myPlayer.InvBody[INVLOC_HAND_LEFT]._iCharges;
			AddPanelString(fmt::format(fmt::runtime(ngettext("{:d} Charge", "{:d} Charges", charges)), charges));
		} break;
		case SpellType::Invalid:
			break;
		}
		std::optional<string_view> fullHotkeyName = GetHotkeyName(spellId, spellListItem.type);
		if (fullHotkeyName) {
			AddPanelString(fmt::format(fmt::runtime(_("Spell Hotkey {:s}")), *fullHotkeyName));
		}
	}
}

std::vector<SpellListItem> GetSpellListItems()
{
	std::vector<SpellListItem> spellListItems;

	uint64_t mask;
	const Point mainPanelPosition = GetMainPanel().position;
	const int iconSize = GetSpeedBookIconSize();

	int x = mainPanelPosition.x + 12 + iconSize * SPLROWICONLS;
	int y = mainPanelPosition.y - 17;

	for (auto i : enum_values<SpellType>()) {
		Player &myPlayer = *MyPlayer;
		switch (static_cast<SpellType>(i)) {
		case SpellType::Skill:
			mask = myPlayer._pAblSpells;
			break;
		case SpellType::Spell:
			mask = myPlayer._pMemSpells;
			break;
		case SpellType::Scroll:
			mask = myPlayer._pScrlSpells;
			break;
		case SpellType::Charges:
			mask = myPlayer._pISpells;
			break;
		default:
			continue;
		}
		int8_t j = static_cast<int8_t>(SpellID::Firebolt);
		for (uint64_t spl = 1; j < MAX_SPELLS; spl <<= 1, j++) {
			if ((mask & spl) == 0)
				continue;
			int lx = x;
			int ly = y - iconSize;
			bool isSelected = (MousePosition.x >= lx && MousePosition.x < lx + iconSize && MousePosition.y >= ly && MousePosition.y < ly + iconSize);
			spellListItems.emplace_back(SpellListItem { { x, y }, static_cast<SpellType>(i), static_cast<SpellID>(j), isSelected });
			x -= iconSize;
			if (x == mainPanelPosition.x + 12 - iconSize) {
				x = mainPanelPosition.x + 12 + iconSize * SPLROWICONLS;
				y -= iconSize;
			}
		}
		if (mask != 0 && x != mainPanelPosition.x + 12 + iconSize * SPLROWICONLS)
			x -= iconSize;
		if (x == mainPanelPosition.x + 12 - iconSize) {
			x = mainPanelPosition.x + 12 + iconSize * SPLROWICONLS;
			y -= iconSize;
		}
	}

	return spellListItems;
}

void SetSpell()
{
	SpellID pSpell;
	SpellType pSplType;

	if (ControlMode != ControlTypes::KeyboardAndMouse)
		spselflag = false;

	if (!GetSpellListSelection(pSpell, pSplType)) {
		return;
	}

	Player &myPlayer = *MyPlayer;
	myPlayer._pRSpell = pSpell;
	myPlayer._pRSplType = pSplType;

	RedrawEverything();
}

bool IsMouseOverSpellList()
{
	for (auto &spellListItem : GetSpellListItems()) {
		if (spellListItem.isSelected)
			return true;
	}
	return false;
}

void SetSpeedSpell(size_t slot)
{
	SpellID pSpell;
	SpellType pSplType;

	if (!GetSpellListSelection(pSpell, pSplType)) {
		return;
	}
	Player &myPlayer = *MyPlayer;

	if (myPlayer._pSplHotKey[slot] == pSpell && myPlayer._pSplTHotKey[slot] == pSplType) {
		// Unset spell hotkey
		myPlayer._pSplHotKey[slot] = SpellID::Invalid;
		return;
	}

	for (size_t i = 0; i < NumHotkeys; ++i) {
		if (myPlayer._pSplHotKey[i] == pSpell && myPlayer._pSplTHotKey[i] == pSplType)
			myPlayer._pSplHotKey[i] = SpellID::Invalid;
	}
	myPlayer._pSplHotKey[slot] = pSpell;
	myPlayer._pSplTHotKey[slot] = pSplType;
}

void ToggleSpell(size_t slot)
{
	uint64_t spells;

	Player &myPlayer = *MyPlayer;

	const SpellID spellId = myPlayer._pSplHotKey[slot];
	if (!IsValidSpell(spellId)) {
		return;
	}

	switch (myPlayer._pSplTHotKey[slot]) {
	case SpellType::Skill:
		spells = myPlayer._pAblSpells;
		break;
	case SpellType::Spell:
		spells = myPlayer._pMemSpells;
		break;
	case SpellType::Scroll:
		spells = myPlayer._pScrlSpells;
		break;
	case SpellType::Charges:
		spells = myPlayer._pISpells;
		break;
	case SpellType::Invalid:
		return;
	}

	if ((spells & GetSpellBitmask(spellId)) != 0) {
		myPlayer._pRSpell = spellId;
		myPlayer._pRSplType = myPlayer._pSplTHotKey[slot];
		RedrawEverything();
	}
}

void DoSpeedBook()
{
	spselflag = true;
	const int iconSize = GetSpeedBookIconSize();
	const Point mainPanelPosition = GetMainPanel().position;
	int xo = mainPanelPosition.x + 12 + iconSize * 10;
	int yo = mainPanelPosition.y - 17;
	int x = xo + iconSize / 2;
	int y = yo - iconSize / 2;

	Player &myPlayer = *MyPlayer;

	if (IsValidSpell(myPlayer._pRSpell)) {
		for (auto i : enum_values<SpellType>()) {
			uint64_t spells;
			switch (static_cast<SpellType>(i)) {
			case SpellType::Skill:
				spells = myPlayer._pAblSpells;
				break;
			case SpellType::Spell:
				spells = myPlayer._pMemSpells;
				break;
			case SpellType::Scroll:
				spells = myPlayer._pScrlSpells;
				break;
			case SpellType::Charges:
				spells = myPlayer._pISpells;
				break;
			default:
				continue;
			}
			uint64_t spell = 1;
			for (int j = 1; j < MAX_SPELLS; j++) {
				if ((spell & spells) != 0) {
					if (j == static_cast<int8_t>(myPlayer._pRSpell) && static_cast<SpellType>(i) == myPlayer._pRSplType) {
						x = xo + iconSize / 2;
						y = yo - iconSize / 2;
					}
					xo -= iconSize;
					if (xo == mainPanelPosition.x + 12 - iconSize) {
						xo = mainPanelPosition.x + 12 + iconSize * SPLROWICONLS;
						yo -= iconSize;
					}
				}
				spell <<= 1ULL;
			}
			if (spells != 0 && xo != mainPanelPosition.x + 12 + iconSize * SPLROWICONLS)
				xo -= iconSize;
			if (xo == mainPanelPosition.x + 12 - iconSize) {
				xo = mainPanelPosition.x + 12 + iconSize * SPLROWICONLS;
				yo -= iconSize;
			}
		}
	}

	if (ControlMode != ControlTypes::KeyboardAndMouse)
		SetCursorPos({ x, y });
}

} // namespace devilution
