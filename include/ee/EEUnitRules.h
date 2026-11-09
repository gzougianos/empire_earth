// patched: the random-map unit rules. Not part of the original game: the setup screen's Units option lets the host
// disable units, buildings and heroes for every player of a new random-map game (or re-enable the ones the tech tree
// disables by default), with the scenario editor's mechanism (the tech tree nodes' mScenEditorObsoleted flag, which
// production, the computer players and saved games already respect). The rules live in their own globals, not in
// EEGameOptions: that class is embedded in the preferences and the game state, whose members other units address at
// fixed offsets (link/exe/fulllink_members.tsv), so it must keep its layout. Implemented in EEGameOptions.cpp.
// Requires ee/EEPlayer.h for the tech tree classes when calling Apply.
#ifndef EE_EEUNITRULES_H
#define EE_EEUNITRULES_H

#include <map>
#include <vector>

class EETechTree;
class UWideString;

// the categories the setup screen's dialog shows (the scenario editor's first three unit-type groups)
enum EEUnitRulesCategory {
    kUnitRulesUnits = 0,
    kUnitRulesBuildings = 1,
    kUnitRulesHeroes = 2,
    kUnitRulesCategoryCount = 3
};

class EEUnitRules {
public:
    // the game's own defaults: no change
    void Reset() { this->mChanges.clear(); }
    bool IsDefault() const { return this->mChanges.empty(); }
    // the text form the registry and the lobby messages carry: comma-separated "-id" (disabled although the tech
    // tree enables it by default) and "+id" (enabled although disabled by default); "" for the defaults
    void ToString(UWideString& outText) const;
    // reads the text form; malformed entries are skipped, unknown ids are kept (Apply ignores them)
    void FromString(const char* inText);
    // whether a tech is disabled under the rules (the tech tree's default when the rules leave it alone)
    bool IsDisabled(long inID) const;
    // disables or enables a tech as the scenario editor does: disabling also disables every tech that has it as a
    // prerequisite, enabling also enables its prerequisites (EETechTreeNode::SetScenEditorObsoleted's recursion)
    void SetDisabled(long inID, bool inDisabled);
    // how many of the dialog's units, buildings and heroes the rules change
    unsigned long GetNumChanged() const;
    // sets a player's tree to the rules (a new random-map game, before its tree is applied); the stored changes
    // already hold the recursion, so each node is set on its own
    void Apply(EETechTree& ioTree) const;
    // sends the rules to the other players in the lobby (inSlot: one player's slot, kSlotNone: everyone) as lobby
    // option 18 messages small enough for a communications tick (defined in EEMessage.cpp, which says why); the
    // caller holds gGameOptions' lock
    void Send(unsigned char inSlot) const;
    // takes one option 18 piece; the last piece of a sequence sets the rules
    void ReceivePiece(unsigned long inValue, const char* inText);
    // the techs the dialog lists for a category: shown in the editor, building something of that category
    static void GetTechs(long inCategory, std::vector<long>& outIDs);
    // the setup screen's summary: "Game defaults" or "Custom (N changed)"
    void GetSummary(UWideString& outText) const;
    // a string of the language resource, or inFallback when Language.dll lacks it (one built before these strings)
    static void GetText(long inStringID, const char* inFallback, UWideString& outText);

    std::map<long, bool> mChanges;          // tech id -> disabled
private:
    void SetOne(long inID, bool inDisabled);
};

extern EEUnitRules gUnitRules;              // the rules of the game being set up or played (the lobby syncs them)
extern EEUnitRules gLastUnitRules;          // the last new random-map game's rules (registry "Game Options" key)

#endif
