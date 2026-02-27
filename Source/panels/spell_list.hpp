#pragma once

#include <cstddef>
#include <vector>

#include "engine/point.hpp"
#include "engine/surface.hpp"
#include "spelldat.h"

namespace devilution {

struct SpellListItem {
	Point location;
	SpellType type;
	SpellID id;
	bool isSelected;
};

void DrawSpell(const Surface &out);
void DrawSpellList(const Surface &out);
std::vector<SpellListItem> GetSpellListItems();
void SetSpell();
void SetSpeedSpell(size_t slot);
void ToggleSpell(size_t slot);

/**
 * @brief Returns true if the mouse cursor is currently over a spell icon in the speed book.
 */
bool IsMouseOverSpellList();

/**
 * Draws the "Speed Book": the rows of known spells for quick-setting a spell that
 * show up when you click the spell slot at the control panel.
 */
void DoSpeedBook();

} // namespace devilution
