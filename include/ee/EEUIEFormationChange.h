// C:\EEX\Empire Earth\UI Events\EEUIEFormationChange.h (RECONSTRUCTION; file name inferred). The formation
// buttons' event (RTTI EEUIEFormationChange > UIEvent; vtable 0x44b4b8, created by the UI's event factory).
// Requires lle/UIEvent.h.
#ifndef EE_EEUIEFORMATIONCHANGE_H
#define EE_EEUIEFORMATIONCHANGE_H

class EEUIEFormationChange : public UIEvent {
public:
    virtual ~EEUIEFormationChange() {}   // user-declared: its ??_G stores the own vptr first (EEUIManager.cpp)
    // EEUIManager.cpp's main game factory (0xf1..0xf8) stores the type after the vptr: a body assignment
    EEUIEFormationChange(long inFormationType) { this->mFormationType = inFormationType; }
    virtual long ProcessEvent(UIForm* inForm, UIEntity* inEntity);

    long mFormationType;                    // +0x0c 0..7 (EEFormation::Create; patched: 9, the point formation)
};

#endif
