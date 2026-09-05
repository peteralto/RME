//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "add_spawns_window.h"
#include "creatures.h"
#include "creature.h"
#include "spawn.h"
#include "gui.h"

#include <algorithm>

// Radius used when the user lets the editor pick one. A single creature only
// needs to stand on its own tile; a group needs room for everyone around the
// centre, and sqrt(n) grows slowly enough that 4-6 creatures land on 3.
static int AutoSpawnSize(size_t creature_count, bool grouped) {
	if (!grouped || creature_count <= 1) {
		return 1;
	}
	int size = (int)std::ceil(std::sqrt((double)creature_count));
	return std::max(2, std::min(size, 10));
}

// Two spawns of radius r stop overlapping once their centres are 2r + 1 apart.
static int AutoSpawnDistance(int size) {
	return (2 * size) + 1;
}

AddSpawnsDialog::AddSpawnsDialog(wxWindow* parent) :
	wxDialog(parent, wxID_ANY, "Add Monster Spawns", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE) {

	wxSizer* root = newd wxBoxSizer(wxVERTICAL);

	// ---- creature pickers -------------------------------------------------
	wxSizer* pickers = newd wxBoxSizer(wxHORIZONTAL);

	wxSizer* left = newd wxBoxSizer(wxVERTICAL);
	left->Add(newd wxStaticText(this, wxID_ANY, "Available creatures"), 0, wxBOTTOM, 2);
	filter_field = newd wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, -1));
	filter_field->SetHint("Filter by name...");
	left->Add(filter_field, 0, wxEXPAND | wxBOTTOM, 4);
	available_list = newd wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(200, 240), 0, nullptr, wxLB_EXTENDED);
	left->Add(available_list, 1, wxEXPAND);
	pickers->Add(left, 1, wxEXPAND | wxALL, 5);

	wxSizer* middle = newd wxBoxSizer(wxVERTICAL);
	middle->AddStretchSpacer();
	add_button = newd wxButton(this, wxID_ANY, "Add >", wxDefaultPosition, wxSize(70, -1));
	middle->Add(add_button, 0, wxBOTTOM, 4);
	remove_button = newd wxButton(this, wxID_ANY, "< Remove", wxDefaultPosition, wxSize(70, -1));
	middle->Add(remove_button, 0);
	middle->AddStretchSpacer();
	pickers->Add(middle, 0, wxALIGN_CENTER_VERTICAL);

	wxSizer* right = newd wxBoxSizer(wxVERTICAL);
	right->Add(newd wxStaticText(this, wxID_ANY, "Creatures to spawn"), 0, wxBOTTOM, 2);
	right->AddSpacer(25);
	chosen_list = newd wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(200, 240), 0, nullptr, wxLB_EXTENDED);
	right->Add(chosen_list, 1, wxEXPAND);
	pickers->Add(right, 1, wxEXPAND | wxALL, 5);

	root->Add(pickers, 1, wxEXPAND);

	// ---- placement mode ---------------------------------------------------
	wxString modes[] = {
		"One spawn per creature (individual spawns)",
		"One shared spawn holding every chosen creature"
	};
	mode_box = newd wxRadioBox(this, wxID_ANY, "Spawn mode", wxDefaultPosition, wxDefaultSize, 2, modes, 1, wxRA_SPECIFY_COLS);
	root->Add(mode_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

	// ---- numeric parameters ----------------------------------------------
	wxSizer* params = newd wxStaticBoxSizer(newd wxStaticBox(this, wxID_ANY, "Parameters"), wxVERTICAL);
	wxFlexGridSizer* grid = newd wxFlexGridSizer(3, 3, 4, 8);
	grid->AddGrowableCol(1);

	grid->Add(newd wxStaticText(this, wxID_ANY, "Spawn size (radius):"), 0, wxALIGN_CENTER_VERTICAL);
	size_spin = newd wxSpinCtrl(this, wxID_ANY, "3", wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 1, 50, 3);
	grid->Add(size_spin, 0);
	auto_size_check = newd wxCheckBox(this, wxID_ANY, "Automatic (from creature count)");
	grid->Add(auto_size_check, 0, wxALIGN_CENTER_VERTICAL);

	grid->Add(newd wxStaticText(this, wxID_ANY, "Distance between spawns:"), 0, wxALIGN_CENTER_VERTICAL);
	distance_spin = newd wxSpinCtrl(this, wxID_ANY, "7", wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 1, 100, 7);
	grid->Add(distance_spin, 0);
	auto_distance_check = newd wxCheckBox(this, wxID_ANY, "Automatic (no overlap)");
	grid->Add(auto_distance_check, 0, wxALIGN_CENTER_VERTICAL);

	grid->Add(newd wxStaticText(this, wxID_ANY, "Spawn interval (seconds):"), 0, wxALIGN_CENTER_VERTICAL);
	interval_spin = newd wxSpinCtrl(this, wxID_ANY, "60", wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 1, 86400, g_gui.GetSpawnTime());
	grid->Add(interval_spin, 0);
	grid->AddSpacer(0);

	params->Add(grid, 0, wxEXPAND | wxALL, 4);

	skip_pz_check = newd wxCheckBox(this, wxID_ANY, "Skip protection zone tiles");
	skip_pz_check->SetValue(true);
	params->Add(skip_pz_check, 0, wxLEFT | wxBOTTOM, 4);

	skip_houses_check = newd wxCheckBox(this, wxID_ANY, "Skip house tiles");
	skip_houses_check->SetValue(true);
	params->Add(skip_houses_check, 0, wxLEFT | wxBOTTOM, 4);

	root->Add(params, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

	summary_text = newd wxStaticText(this, wxID_ANY, wxEmptyString);
	root->Add(summary_text, 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);

	// ---- buttons ----------------------------------------------------------
	wxSizer* buttons = newd wxBoxSizer(wxHORIZONTAL);
	execute_button = newd wxButton(this, wxID_ANY, "Execute");
	buttons->Add(execute_button, 0, wxRIGHT, 5);
	close_button = newd wxButton(this, wxID_ANY, "Close");
	buttons->Add(close_button, 0);
	root->Add(buttons, 0, wxALIGN_RIGHT | wxALL, 5);

	SetSizerAndFit(root);
	Centre(wxBOTH);

	filter_field->Bind(wxEVT_TEXT, &AddSpawnsDialog::OnFilterText, this);
	add_button->Bind(wxEVT_BUTTON, &AddSpawnsDialog::OnAddClicked, this);
	remove_button->Bind(wxEVT_BUTTON, &AddSpawnsDialog::OnRemoveClicked, this);
	auto_size_check->Bind(wxEVT_CHECKBOX, &AddSpawnsDialog::OnAutoSizeToggled, this);
	auto_distance_check->Bind(wxEVT_CHECKBOX, &AddSpawnsDialog::OnAutoDistanceToggled, this);
	mode_box->Bind(wxEVT_RADIOBOX, &AddSpawnsDialog::OnGroupModeChanged, this);
	execute_button->Bind(wxEVT_BUTTON, &AddSpawnsDialog::OnExecuteClicked, this);
	close_button->Bind(wxEVT_BUTTON, &AddSpawnsDialog::OnCancelClicked, this);

	RefreshCreatureList();
	UpdateWidgets();
}

AddSpawnsDialog::~AddSpawnsDialog() {
	////
}

bool AddSpawnsDialog::IsGrouped() const {
	return mode_box->GetSelection() == 1;
}

int AddSpawnsDialog::GetEffectiveSize() const {
	if (auto_size_check->GetValue()) {
		return AutoSpawnSize(chosen_creatures.size(), IsGrouped());
	}
	return size_spin->GetValue();
}

int AddSpawnsDialog::GetEffectiveDistance() const {
	if (auto_distance_check->GetValue()) {
		return AutoSpawnDistance(GetEffectiveSize());
	}
	return distance_spin->GetValue();
}

void AddSpawnsDialog::RefreshCreatureList() {
	const std::string filter = as_lower_str(nstr(filter_field->GetValue()));

	available_list->Freeze();
	available_list->Clear();
	for (CreatureDatabase::iterator it = g_creatures.begin(); it != g_creatures.end(); ++it) {
		CreatureType* type = it->second;
		if (!type || type->isNpc) {
			// NPCs are placed by hand, they do not belong in a hunt spawn.
			continue;
		}

		if (!filter.empty() && as_lower_str(type->name).find(filter) == std::string::npos) {
			continue;
		}

		available_list->Append(wxstr(type->name));
	}
	available_list->Thaw();
}

void AddSpawnsDialog::UpdateWidgets() {
	size_spin->Enable(!auto_size_check->GetValue());
	distance_spin->Enable(!auto_distance_check->GetValue());
	execute_button->Enable(!chosen_creatures.empty());

	wxString summary;
	summary << (int)chosen_creatures.size() << " creature type(s) selected. "
			<< "Using size " << GetEffectiveSize()
			<< ", distance " << GetEffectiveDistance() << ".";
	summary_text->SetLabel(summary);
}

void AddSpawnsDialog::OnFilterText(wxCommandEvent& WXUNUSED(event)) {
	RefreshCreatureList();
}

void AddSpawnsDialog::OnAddClicked(wxCommandEvent& WXUNUSED(event)) {
	wxArrayInt selections;
	available_list->GetSelections(selections);

	for (size_t i = 0; i < selections.GetCount(); ++i) {
		const std::string name = nstr(available_list->GetString(selections[i]));
		if (std::find(chosen_creatures.begin(), chosen_creatures.end(), name) != chosen_creatures.end()) {
			continue;
		}

		chosen_creatures.push_back(name);
		chosen_list->Append(wxstr(name));
	}

	UpdateWidgets();
}

void AddSpawnsDialog::OnRemoveClicked(wxCommandEvent& WXUNUSED(event)) {
	wxArrayInt selections;
	chosen_list->GetSelections(selections);

	std::vector<int> indices;
	indices.reserve(selections.GetCount());
	for (size_t i = 0; i < selections.GetCount(); ++i) {
		indices.push_back(selections[i]);
	}

	// Backwards, so the earlier indices stay valid as entries are erased.
	std::sort(indices.begin(), indices.end());
	for (size_t i = indices.size(); i > 0; --i) {
		const int index = indices[i - 1];
		chosen_creatures.erase(chosen_creatures.begin() + index);
		chosen_list->Delete(index);
	}

	UpdateWidgets();
}

void AddSpawnsDialog::OnAutoSizeToggled(wxCommandEvent& WXUNUSED(event)) {
	UpdateWidgets();
}

void AddSpawnsDialog::OnAutoDistanceToggled(wxCommandEvent& WXUNUSED(event)) {
	UpdateWidgets();
}

void AddSpawnsDialog::OnGroupModeChanged(wxCommandEvent& WXUNUSED(event)) {
	UpdateWidgets();
}

void AddSpawnsDialog::OnCancelClicked(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}

void AddSpawnsDialog::OnExecuteClicked(wxCommandEvent& WXUNUSED(event)) {
	Execute();
	EndModal(wxID_OK);
}

bool AddSpawnsDialog::IsValidSpawnTile(const Tile* tile) const {
	if (!tile || !tile->ground) {
		// No ground at all -- nothing to stand on.
		return false;
	}

	// Covers walls, closed doors, big rocks and anything else flagged
	// unpassable in the OTB; kept up to date by Tile::update().
	if (tile->isBlocking()) {
		return false;
	}

	if (tile->creature) {
		return false;
	}

	// Already inside somebody else's spawn radius.
	if (tile->spawn || tile->getLocation()->getSpawnCount() > 0) {
		return false;
	}

	if (skip_pz_check->GetValue() && tile->isPZ()) {
		return false;
	}

	if (skip_houses_check->GetValue() && tile->isHouseTile()) {
		return false;
	}

	return true;
}

void AddSpawnsDialog::Execute() {
	if (chosen_creatures.empty() || !g_gui.IsEditorOpen()) {
		return;
	}

	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor || editor->selection.size() == 0) {
		return;
	}

	const bool grouped = IsGrouped();
	const int size = GetEffectiveSize();
	const int distance = GetEffectiveDistance();
	const int interval = interval_spin->GetValue();

	Map& map = editor->map;

	// Work on a sorted copy: the selection is an unordered_set, and iterating it
	// directly would scatter the spawns differently on every run for the same
	// input, which makes the result impossible to reason about or reproduce.
	std::vector<Tile*> candidates;
	candidates.reserve(editor->selection.size());
	for (TileSet::iterator it = editor->selection.begin(); it != editor->selection.end(); ++it) {
		Tile* tile = (*it);
		if (IsValidSpawnTile(tile)) {
			candidates.push_back(tile);
		}
	}

	std::sort(candidates.begin(), candidates.end(), [](const Tile* a, const Tile* b) {
		const Position& pa = a->getPosition();
		const Position& pb = b->getPosition();
		if (pa.z != pb.z) {
			return pa.z < pb.z;
		}
		if (pa.y != pb.y) {
			return pa.y < pb.y;
		}
		return pa.x < pb.x;
	});

	if (candidates.empty()) {
		g_gui.PopupDialog("Add Monster Spawns", "No walkable tile in the selection could host a spawn.", wxOK);
		return;
	}

	g_gui.CreateLoadBar("Placing spawns...");

	BatchAction* batch = editor->actionQueue->createBatch(ACTION_DRAW);
	Action* action = editor->actionQueue->createAction(batch);

	std::vector<Position> placed_centres;
	int spawns_created = 0;
	int creatures_created = 0;
	size_t next_creature = 0;
	size_t done = 0;

	for (std::vector<Tile*>::iterator it = candidates.begin(); it != candidates.end(); ++it) {
		if (++done % 0x400 == 0) {
			g_gui.SetLoadDone((unsigned int)(100 * done / candidates.size()));
		}

		Tile* tile = (*it);
		const Position& position = tile->getPosition();

		// Chebyshev distance, because spawn radii are square in Tibia.
		bool too_close = false;
		for (std::vector<Position>::const_iterator pit = placed_centres.begin(); pit != placed_centres.end(); ++pit) {
			if (pit->z != position.z) {
				continue;
			}
			const int dx = std::abs(pit->x - position.x);
			const int dy = std::abs(pit->y - position.y);
			if (std::max(dx, dy) < distance) {
				too_close = true;
				break;
			}
		}

		if (too_close) {
			continue;
		}

		Tile* newtile = tile->deepCopy(map);
		newtile->spawn = newd Spawn(size);

		if (grouped) {
			// The centre gets the first creature; the rest are laid out on the
			// nearest valid tiles inside the radius. Anything that does not fit
			// is skipped rather than stacked, since a tile holds one creature.
			size_t placed_here = 0;

			CreatureType* centre_type = g_creatures[chosen_creatures[0]];
			if (centre_type) {
				newtile->creature = newd Creature(centre_type);
				newtile->creature->setSpawnTime(interval);
				++creatures_created;
				++placed_here;
			}

			action->addChange(newd Change(newtile));

			for (size_t c = 1; c < chosen_creatures.size(); ++c) {
				CreatureType* type = g_creatures[chosen_creatures[c]];
				if (!type) {
					continue;
				}

				bool seated = false;
				for (int r = 1; r <= size && !seated; ++r) {
					for (int dy = -r; dy <= r && !seated; ++dy) {
						for (int dx = -r; dx <= r && !seated; ++dx) {
							if (std::max(std::abs(dx), std::abs(dy)) != r) {
								continue; // only the ring at distance r
							}

							const Position member(position.x + dx, position.y + dy, position.z);
							Tile* member_tile = map.getTile(member);
							if (!member_tile || !member_tile->ground || member_tile->isBlocking() || member_tile->creature) {
								continue;
							}
							if (skip_pz_check->GetValue() && member_tile->isPZ()) {
								continue;
							}

							Tile* new_member = member_tile->deepCopy(map);
							new_member->creature = newd Creature(type);
							new_member->creature->setSpawnTime(interval);
							action->addChange(newd Change(new_member));

							++creatures_created;
							++placed_here;
							seated = true;
						}
					}
				}
			}

			if (placed_here == 0) {
				// Nothing could be seated -- do not leave an empty spawn behind.
				continue;
			}
		} else {
			// Round robin through the chosen types, so a mixed hunt ends up
			// evenly distributed instead of one species per area.
			CreatureType* type = g_creatures[chosen_creatures[next_creature % chosen_creatures.size()]];
			++next_creature;

			if (!type) {
				delete newtile;
				continue;
			}

			newtile->creature = newd Creature(type);
			newtile->creature->setSpawnTime(interval);
			++creatures_created;

			action->addChange(newd Change(newtile));
		}

		placed_centres.push_back(position);
		++spawns_created;
	}

	// Committed after the loop: commit mutates the selection set iterated above.
	batch->addAndCommitAction(action);
	editor->addBatch(batch);

	g_gui.DestroyLoadBar();

	wxString message;
	message << spawns_created << " spawn(s) created with " << creatures_created << " creature(s).";
	g_gui.PopupDialog("Add Monster Spawns", message, wxOK);

	map.doChange();
	g_gui.RefreshView();
}