// patched: the setup screen's unit rules dialog (not in the original game; ee/EEUnitRules.h). A form built in code
// like the message box (EEUIFError), defined in EEUIFError.cpp (a unit of its own would need an address range in
// link/exe/units.txt): two lists, the available and the disabled units, buildings or heroes of the database's tech
// tree, moved between them by double-click or the arrow buttons. OK stores the working copy in gUnitRules and sends
// it to the other players (lobby option 18).
#ifndef EE_EEUIFUNITRULES_H
#define EE_EEUIFUNITRULES_H

// opens the dialog over the current form on the rules of the game being set up (the host's lobby)
void EEShowUnitRulesDialog();

#endif
