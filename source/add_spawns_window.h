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

#ifndef RME_ADD_SPAWNS_WINDOW_H_
#define RME_ADD_SPAWNS_WINDOW_H_

#include "main.h"
#include "common_windows.h"
#include "editor.h"

// Fills the current selection with monster spawns.
//
// The dialog gathers a set of creature names plus the placement parameters and
// then walks the selected tiles once, dropping spawn centres on walkable ground
// while honouring a minimum distance between them.
class AddSpawnsDialog : public wxDialog {
public:
	AddSpawnsDialog(wxWindow* parent);
	~AddSpawnsDialog();

	void OnFilterText(wxCommandEvent& event);
	void OnAddClicked(wxCommandEvent& event);
	void OnRemoveClicked(wxCommandEvent& event);
	void OnAutoSizeToggled(wxCommandEvent& event);
	void OnAutoDistanceToggled(wxCommandEvent& event);
	void OnGroupModeChanged(wxCommandEvent& event);
	void OnExecuteClicked(wxCommandEvent& event);
	void OnCancelClicked(wxCommandEvent& event);

private:
	// One spawn per creature, or one spawn holding every chosen creature.
	bool IsGrouped() const;

	// Radius actually used, taking the "auto" checkbox into account.
	int GetEffectiveSize() const;
	// Minimum gap between two spawn centres, ditto.
	int GetEffectiveDistance() const;

	// A tile can host a spawn centre if it exists, has ground, is walkable and
	// is not already occupied by a creature or covered by another spawn.
	bool IsValidSpawnTile(const Tile* tile) const;

	void RefreshCreatureList();
	void UpdateWidgets();

	void Execute();

	wxTextCtrl* filter_field;
	wxListBox* available_list;
	wxListBox* chosen_list;
	wxButton* add_button;
	wxButton* remove_button;

	wxRadioBox* mode_box;
	wxSpinCtrl* size_spin;
	wxCheckBox* auto_size_check;
	wxSpinCtrl* distance_spin;
	wxCheckBox* auto_distance_check;
	wxSpinCtrl* interval_spin;
	wxCheckBox* skip_pz_check;
	wxCheckBox* skip_houses_check;

	wxStaticText* summary_text;
	wxButton* execute_button;
	wxButton* close_button;

	std::vector<std::string> chosen_creatures;
};

#endif
