// C:\EEX\Empire Earth\UI Events\EEUIEFormationChange.cpp (__FILE__ 0x4c633c, the ESVERIFY(false) of an unknown
// formation). One function: 0x1d6886..0x1d6b94 (the previous unit's last function is EEUIEEEECivBuilder's at
// 0x1d6845; the next unit starts with its init chain at 0x1d6b94). Unit flags: the exe's module set.
#include <windows.h>
#include <mbstring.h>
#include <algorithm>

#include "lle/LowLevelEngine.h"
#include "lle/GEColor.h"
#include "lle/GESurface.h"
#include "lle/GETextureCoordinate.h"
#include "lle/GETextureManager.h"
#include "lle/GETexture.h"
#include "GEWindow.h"
#include "lle/GEScreen.h"
#include "lle/GEViewport.h"
#include "lle/GEVertexArray.h"

#include "lle/GERasterizer.h"
#include "lle/GERhombus.h"
#include "lle/GEBitmap.h"
#include "lle/FSFileSpec.h"
#include "lle/FSArchive.h"
#include "lle/FSPath.h"
#include "lle/TMPoll.h"
#include "lle/TSCriticalSection.h"
#include "lle/TSScheduler.h"
#include "lle/SESample.h"
#include "lle/SESMInstanceNode.h"
#include "lle/SESoundManager.h"
#include "lle/Input System.h"
#include "lle/ISKeyboard.h"
#include "lle/ISMouse.h"
#include "lle/UIControl.h"
#include "lle/UIEvent.h"
#include "lle/UIEntityGroup.h"
#include "lle/UIBackground.h"
#include "lle/UIFontTable.h"
#include "lle/UIHotKey.h"
#include "lle/UIIMECandidate.h"
#include "lle/UIForm.h"
#include "lle/UIManager.h"
#include "lle/UIButton.h"
#include "lle/NEMessage.h"
#include "lle/NESynchronousCommunications.h"

#include "ee/RTSWorld.h"
#include "ee/EEWorld.h"
#include "ee/EEPlayer.h"
#include "ee/EEUnit.h"
#include "ee/EEFormation.h"
#include "ee/EEMessage.h"
#include "ee/EECommunications.h"
#include "ee/EEUIManager.h"
#include "ee/EEUIFMainGame.h"
#include "ee/EEUIFMGMainGroup.h"
#include "ee/EEUIEFormationChange.h"

// FUNCTION: EXE 0x5d6886
// Gives the selected group the chosen formation and posts the change; closes the formation menu and shows the
// formation's icon on the selected unit's formation command button
long EEUIEFormationChange::ProcessEvent(UIForm* inForm, UIEntity* inEntity)
{
    EEUnit* theUnit = static_cast<EEUIFMGMainGroup*>(gUIManager->mMainGameForm->mGroups[0])->GetSelectedUnit();
    EEComplexUnit* theUnitU30 = theUnit ? theUnit->mComplexUnit : 0;
    EEUnitClass* theUnitClass = theUnit ? theUnit->mUnitClass : 0;
    EEFormation* theFormation = EEFormation::Create(this->mFormationType);
    gCommunications->ProcessLocalUserInput(new EEMChangeFormation(this->mFormationType), kSlotNone);
    gUIManager->mMainGameForm->GetEntityGroup(8)->Disable();
    gUIManager->mMainGameForm->GetBackgroundByID(0x98)->SetVisibility(false);
    theFormation->mB5C = true;
    ::gWorld.mPlayers[::gWorld.mCurrentPlayer]->mSelectedGroup->SetFormation(theFormation);
    theFormation->Dereference();
    if (theUnitU30 == NULL || theUnitClass == NULL)
        return kErrorOK;
    for (unsigned long i = 0; i < 9; i++) {
        unsigned long theCommand = theUnitClass->mCommands[i];
        if (!(theCommand >= 2) || !(theCommand < 50) || theCommand != 30)
            continue;
        UIButton* theButton = static_cast<UIButton*>(gUIManager->mMainGameForm->GetControl(0x4e + i));
        if (this->mFormationType == 0) {
            theButton->SetNormalBitmap(static_cast<UIButton*>(gUIManager->mMainGameForm->GetControl(kMainGameFormationShortLine))->GetNormalBitmap());
            theButton->SetPressedBitmap(static_cast<UIButton*>(gUIManager->mMainGameForm->GetControl(kMainGameFormationShortLine))->GetPressedBitmap());
        } else if (this->mFormationType == 1) {
            theButton->SetNormalBitmap(static_cast<UIButton*>(gUIManager->mMainGameForm->GetControl(kMainGameFormationLongLine))->GetNormalBitmap());
            theButton->SetPressedBitmap(static_cast<UIButton*>(gUIManager->mMainGameForm->GetControl(kMainGameFormationLongLine))->GetPressedBitmap());
        } else if (this->mFormationType == 2) {
            theButton->SetNormalBitmap(static_cast<UIButton*>(gUIManager->mMainGameForm->GetControl(kMainGameFormationLooseLine))->GetNormalBitmap());
            theButton->SetPressedBitmap(static_cast<UIButton*>(gUIManager->mMainGameForm->GetControl(kMainGameFormationLooseLine))->GetPressedBitmap());
        } else if (this->mFormationType == 3) {
            theButton->SetNormalBitmap(static_cast<UIButton*>(gUIManager->mMainGameForm->GetControl(kMainGameFormationStaggeredLine))->GetNormalBitmap());
            theButton->SetPressedBitmap(static_cast<UIButton*>(gUIManager->mMainGameForm->GetControl(kMainGameFormationStaggeredLine))->GetPressedBitmap());
        } else if (this->mFormationType == 4) {
            theButton->SetNormalBitmap(static_cast<UIButton*>(gUIManager->mMainGameForm->GetControl(kMainGameFormationDeepWedge))->GetNormalBitmap());
            theButton->SetPressedBitmap(static_cast<UIButton*>(gUIManager->mMainGameForm->GetControl(kMainGameFormationDeepWedge))->GetPressedBitmap());
        } else if (this->mFormationType == 5) {
            theButton->SetNormalBitmap(static_cast<UIButton*>(gUIManager->mMainGameForm->GetControl(kMainGameFormationShallowWedge))->GetNormalBitmap());
            theButton->SetPressedBitmap(static_cast<UIButton*>(gUIManager->mMainGameForm->GetControl(kMainGameFormationShallowWedge))->GetPressedBitmap());
        } else if (this->mFormationType == 6) {
            theButton->SetNormalBitmap(static_cast<UIButton*>(gUIManager->mMainGameForm->GetControl(kMainGameFormationInwardCircle))->GetNormalBitmap());
            theButton->SetPressedBitmap(static_cast<UIButton*>(gUIManager->mMainGameForm->GetControl(kMainGameFormationInwardCircle))->GetPressedBitmap());
        } else if (this->mFormationType == 7) {
            theButton->SetNormalBitmap(static_cast<UIButton*>(gUIManager->mMainGameForm->GetControl(kMainGameFormationOutwardCircle))->GetNormalBitmap());
            theButton->SetPressedBitmap(static_cast<UIButton*>(gUIManager->mMainGameForm->GetControl(kMainGameFormationOutwardCircle))->GetPressedBitmap());
        } else if (this->mFormationType == kFormationPoint) {
            // patched: the point formation's button (made in code, EEUIFMainGame.cpp; no icon: the old one stays)
            UIButton* thePoint = static_cast<UIButton*>(gUIManager->mMainGameForm->GetControl(kMainGameFormationPoint));
            if (thePoint != NULL && thePoint->GetNormalBitmap() != NULL) {
                theButton->SetNormalBitmap(thePoint->GetNormalBitmap());
                theButton->SetPressedBitmap(thePoint->GetPressedBitmap());
            }
        } else
            ESVERIFY_AT(false, 121);
        break;
    }
    return kErrorOK;
}
