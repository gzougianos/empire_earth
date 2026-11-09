// C:\EEX\Empire Earth\User Interface Thread\EEUIFError.cpp (__FILE__ 0x4cd490). The game's message box form and its two
// button classes: 0x260c27..0x2624a6 (the init chains of the seven colours, EEUIFEBOK, EEUIFEBCancel, EEUIFError;
// EEUIFileComboBox's ctor at 0x2624a6). Unit flags: the exe module set. The colours, constants and button members
// have provisional names.
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
#include "ee/EEMessage.h"
#include "ee/EECommunications.h"
#include "lle/UResourceFile.h"
#include "lle/GEFont.h"
#include "lle/GEText.h"
#include "lle/UIScrollBar.h"
#include "lle/UITextLabel.h"
#include "lle/UIEditBox.h"
#include "ee/EmpireEarth.h"
#include "lle/UIBitmapBackground.h"
#include "ee/EEUserInterface.h"
#include "ee/EEUIManager.h"
#include "ee/EEUIBGenericBackground.h"
#include "ee/EEUIFError.h"
#include "ee/EEStringIDs.h"
static const float kMessageScrollBarOpacity = 0.3f;             // 0x44a634
static const float kMessageButtonOpacity = 0.4f;                // 0x44a638
static const GEColor kMessageScrollBarColor(1.0f, 1.0f, 1.0f);  // 0x5645f8
static const GEColor kMessageButtonColor(1.0f, 1.0f, 1.0f);     // 0x5645e8
static const GEColor kMessageTextColor(0.0f, 0.0f, 0.0f);       // 0x5645d8
static const GEColor kMessageFocusedColor(0.61f, 0.46f, 0.0f);  // 0x5645c8
static const GEColor kMessageEditBoxColor(1.0f, 1.0f, 1.0f);    // 0x5645b8
static const GEColor kMessageSelectedColor(0.61f, 0.46f, 0.0f); // 0x5645a8
static const GEColor kMessageBackgroundColor(0.91f, 0.85f, 0.73f); // 0x564598

// The OK button (RTTI EEUIFEBOK > UIButton; vtable 0x44a640; sizeof 0x110). The in-class virtuals follow the ctor
// with the ??_G in declaration order.
class EEUIFEBOK : public UIButton {
    friend class EEUIFError;
public:
    EEUIFEBOK(UIForm* inForm, unsigned long inId);
    virtual long DrawPolygons()
    {
        GERhombus theRhombus;
        theRhombus.SetColor(kMessageButtonColor);
        theRhombus.SetRectangle(this->mCopiedDrawRectangle);
        theRhombus.SetOpacity(kMessageButtonOpacity);
        this->GetRasterizer()->Draw(&theRhombus, this->GetScreenIndex());
        return UIButton::DrawPolygons();
    }
    virtual ~EEUIFEBOK() {}
    void SetCallback(EEUIFErrorCallback inCallback, void* inData)
    {
        this->mCallback = inCallback;
        this->mData = inData;
    }
protected:
    virtual void ButtonUp();

    EEUIFErrorCallback mCallback;           // +0x104
    void* mData;                            // +0x108
    bool mPressed;                          // +0x10c
};

// The No and Cancel buttons (RTTI EEUIFEBCancel > UIButton; vtable 0x44a6d8; sizeof 0x114).
class EEUIFEBCancel : public UIButton {
    friend class EEUIFError;
public:
    EEUIFEBCancel(UIForm* inForm, unsigned long inId);
    virtual long DrawPolygons()
    {
        GERhombus theRhombus;
        theRhombus.SetColor(kMessageButtonColor);
        theRhombus.SetRectangle(this->mCopiedDrawRectangle);
        theRhombus.SetOpacity(kMessageButtonOpacity);
        this->GetRasterizer()->Draw(&theRhombus, this->GetScreenIndex());
        return UIButton::DrawPolygons();
    }
    virtual ~EEUIFEBCancel() {}
    void SetCallback(EEUIFErrorCallback inCallback, void* inData)
    {
        this->mCallback = inCallback;
        this->mData = inData;
    }
protected:
    virtual void ButtonUp();

    EEUIFErrorCallback mCallback;           // +0x104
    void* mData;                            // +0x108
    UIForm* mCloseForm;                     // +0x10c (DisplayCancel)
    bool mPressed;                          // +0x110
};

// FUNCTION: EXE 0x660d85
// Creates the message box's OK/Yes button (50x20) with its callback unset
EEUIFEBOK::EEUIFEBOK(UIForm* inForm, unsigned long inId)
    : UIButton(inId, NULL, NULL, NULL, GERectangle(GE2DPoint(0.0f, 0.0f), 50.0f, 20.0f), inForm, inId, 1),
      mCallback(NULL), mData(NULL)
{
    this->SetFocusedTextColor(kMessageFocusedColor);
    this->mPressed = false;
}

// FUNCTION: EXE 0x660e01
// Draws the button over a light translucent panel
// (EEUIFEBOK::DrawPolygons, in the class)

// SYNTHETIC: EXE 0x660e7b
// (the scalar deleting destructor, emitted with the vtable; slot EEUIFEBOK+0x0)

// FUNCTION: EXE 0x660e9e
// Marks the OK/Yes button pressed and closes the message box
void EEUIFEBOK::ButtonUp()
{
    this->mPressed = true;
    UIButton::ButtonUp();
    gUserInterface->CloseForm(NULL);
}

// FUNCTION: EXE 0x660eb9
// Creates a No or Cancel button (50x20) with its callback and close form unset
EEUIFEBCancel::EEUIFEBCancel(UIForm* inForm, unsigned long inId)
    : UIButton(inId, NULL, NULL, NULL, GERectangle(GE2DPoint(0.0f, 0.0f), 50.0f, 20.0f), inForm, inId, 1),
      mCallback(NULL), mData(NULL), mCloseForm(NULL)
{
    this->SetFocusedTextColor(kMessageFocusedColor);
    this->mPressed = false;
}

// FUNCTION: EXE 0x660f3b
// Draws the button over a light translucent panel
// (EEUIFEBCancel::DrawPolygons, in the class)

// SYNTHETIC: EXE 0x660fb5
// (the scalar deleting destructor, emitted with the vtable; slot EEUIFEBCancel+0x0)

// FUNCTION: EXE 0x660fd8
// Marks the No/Cancel button pressed and closes the message box (or its close form)
void EEUIFEBCancel::ButtonUp()
{
    this->mPressed = true;
    UIButton::ButtonUp();
    gUserInterface->CloseForm(this->mCloseForm);
}

// FUNCTION: EXE 0x660ffb
// Creates the game's message box form: message label, OK, No and Cancel buttons, an edit box and the gold
// framed background, laid out for a 1024x768 design
EEUIFError::EEUIFError(UIManager* inManager)
    : UIForm(7, 1, inManager, -1, true)
{
    GERectangle theScreen(0.0f, 0.0f, (float)gGameWindow->GetWidth(), (float)gGameWindow->GetHeight());
    UILayoutUtility theLayout;
    GETextureDescription theDescription;
    theDescription.mIs3DRenderable = false;
    theDescription.mIsTransparencyEnabled = false;
    theDescription.mPixelDepth = 16;
    theDescription.mNumMipLevelsToDrop = 0;
    theDescription.mSurfaceVolatility = 1;
    theDescription.mFrameFormat = 0;
    theDescription.mHasAlpha = true;
    theDescription.mCreateMIPMaps = true;
    GEFont* theFont = UIFontTable::CreateClosestFont(0x1c, UMFloatToLong((gFontTahoma16->GetHeight() * (1.0f / 768.0f)) *
                                                                         theScreen.GetHeight()));
    this->mError = new UITextLabel(UWideString(""), theFont, 0, GERectangle(GE2DPoint(0.0f, 0.0f), 280.0f, 135.0f), this, 1,
                             0, 0);
    ESVERIFY_AT(this->mError, 220);
    this->mError->SetScrollBarBgColor(kMessageScrollBarColor);
    this->mError->SetScrollBarBgOpacity(kMessageScrollBarOpacity);
    this->mError->SetAlignment(kTextAlignCenter);
    GEText* theGEText = this->mError->GetGEText();
    theGEText->SetIsTransparent(true);
    theGEText->SetFgColor(kMessageTextColor);
    this->AddControl(this->mError, 0);

    this->mOk = new EEUIFEBOK(this, 1);
    ESVERIFY_AT(this->mOk, 236);
    this->AddControl(this->mOk, 0);

    this->mNo = new EEUIFEBCancel(this, 2);
    ESVERIFY_AT(this->mNo, 244);
    this->AddControl(this->mNo, 0);

    this->mCancel = new EEUIFEBCancel(this, 3);
    ESVERIFY_AT(this->mCancel, 252);
    this->AddControl(this->mCancel, 0);

    this->mEditBox = new UIEditBox(4, theFont, 0x32, false, GERectangle(GE2DPoint(0.0f, 0.0f), 220.0f, 24.0f), this, 4, 2,
                             false, false);
    ESVERIFY_AT(this->mEditBox, 260);
    this->mEditBox->GetGEText()->SetIsTransparent(true);
    this->mEditBox->GetGEText()->SetFgColor(kMessageTextColor);
    this->mEditBox->SetSelectedBgColor(kMessageSelectedColor);
    this->mEditBox->SetBgColor(kMessageEditBoxColor);
    this->mEditBox->SetCursorColor(kMessageTextColor);
    this->AddControl(this->mEditBox, 0);
    this->GetEntityGroup(0)->Enable();
    this->mMode = 0;
    theLayout.SetLeft(4, 295.0f / 1024.0f);
    theLayout.SetRight(4, 730.0f / 1024.0f);
    theLayout.SetTop(4, 262.0f / 768.0f);
    theLayout.SetBottom(4, 506.0f / 768.0f);
    this->SetLayout(theLayout);
    theDescription.mPixelDepth = this->GetManager()->GetUITextureBitDepth();
    theDescription.mHasAlpha = true;
    this->mBackground = new EEUIBGenericBackground(0, "textures\\EEGenericBack_Gold", theDescription, theLayout, this, 0,
                                             kMessageBackgroundColor, true, 1.0f, 1.0f, 1.0f, 3);
    theFont->Dereference();
}

// SYNTHETIC: EXE 0x66149b
// (the scalar deleting destructor, emitted with the vtable; slot EEUIFError+0x0)

// FUNCTION: EXE 0x6614b7
// Destroys the message box and its controls
EEUIFError::~EEUIFError()
{
    delete this->mError;
    delete this->mOk;
    delete this->mNo;
    delete this->mCancel;
    delete this->mEditBox;
}

// FUNCTION: EXE 0x66151a
// Shows the message box asking for a text or password, which OK passes to a callback
void EEUIFError::DisplayPassword(const UWideString& inMessage, unsigned long inAlignment,
                                 EEUIFPasswordCallback inCallback, bool inPassword)
{
    UWideString theText;
    GEText* theGEText;
    gResource->GetString(IDS_LOBBY_OK, theText);
    this->mOk->SetText(theText, gFontTahoma12);
    this->mOk->SetCallback((EEUIFErrorCallback)inCallback, (void*)this->mEditBox->GetText(0));
    this->mOk->SetDisplayMode(kDisplayModeShown);
    theGEText = this->mOk->GetGEText();
    theGEText->SetFgColor(kMessageTextColor);
    theGEText->SetIsTransparent(true);
    this->mError->SetAlignment(inAlignment);
    this->mError->SetText(inMessage);
    this->mError->SetRectangle(GERectangle(GE2DPoint(0.0f, 0.0f), 280.0f, 50.0f));
    this->mNo->SetDisplayMode(kDisplayModeHidden);
    this->mCancel->SetDisplayMode(kDisplayModeHidden);
    this->mEditBox->SetDisplayMode(kDisplayModeShown);
    this->SetTabbing(true);
    this->SetTabStop(this->mEditBox->GetIndex());
    this->mEditBox->SetIsPasswordBox(inPassword);
    this->mMode = 3;
    gUserInterface->SwitchToForm(this);
}

// FUNCTION: EXE 0x66167f
// Shows a message with only a Cancel button, with its callback and the form Cancel closes
void EEUIFError::DisplayCancel(const UWideString& inMessage, unsigned long inAlignment, void* inData,
                               EEUIFErrorCallback inCallback, UIForm* inForm)
{
    UWideString theText;
    GEText* theGEText;
    gResource->GetString(IDS_LOBBY_CANCEL, theText);
    this->mCancel->SetText(theText, gFontTahoma12);
    this->mCancel->SetCallback(inCallback, inData);
    this->mCancel->SetDisplayMode(kDisplayModeShown);
    this->mCancel->mCloseForm = inForm;
    theGEText = this->mCancel->GetGEText();
    theGEText->SetFgColor(kMessageTextColor);
    theGEText->SetIsTransparent(true);
    this->mError->SetAlignment(inAlignment);
    this->mError->SetText(inMessage);
    this->mNo->SetDisplayMode(kDisplayModeHidden);
    this->mOk->SetDisplayMode(kDisplayModeHidden);
    this->mEditBox->SetDisplayMode(kDisplayModeHidden);
    this->mMode = 4;
    gUserInterface->SwitchToForm(this);
}

// FUNCTION: EXE 0x661788
// Shows a message with an optional OK button and its callback
void EEUIFError::DisplayMessage(const UWideString& inMessage, unsigned long inAlignment, void* inData,
                                EEUIFErrorCallback inCallback, bool inShowOk)
{
    UWideString theText;
    GEText* theGEText;
    gResource->GetString(IDS_LOBBY_OK, theText);
    this->mOk->SetText(theText, gFontTahoma12);
    this->mOk->SetCallback(inCallback, inData);
    this->mOk->SetDisplayMode(kDisplayModeShown);
    theGEText = this->mOk->GetGEText();
    theGEText->SetFgColor(kMessageTextColor);
    theGEText->SetIsTransparent(true);
    this->mError->SetAlignment(inAlignment);
    this->mError->SetText(inMessage);
    if (inShowOk)
        this->mOk->SetDisplayMode(kDisplayModeShown);
    else
        this->mOk->SetDisplayMode(kDisplayModeHidden);
    this->mNo->SetDisplayMode(kDisplayModeHidden);
    this->mCancel->SetDisplayMode(kDisplayModeHidden);
    this->mEditBox->SetDisplayMode(kDisplayModeHidden);
    this->mMode = 0;
    gUserInterface->SwitchToForm(this);
}

// FUNCTION: EXE 0x661894
// Shows a Yes/No question with a callback for each answer
void EEUIFError::DisplayQuestion(const UWideString& inMessage, unsigned long inAlignment, void* inData,
                                 EEUIFErrorCallback inYes, EEUIFErrorCallback inNo)
{
    UWideString theText;
    GEText* theGEText;
    gResource->GetString(IDS_YES, theText);
    this->mOk->SetText(theText, gFontTahoma12);
    this->mOk->SetCallback(inYes, inData);
    this->mOk->SetDisplayMode(kDisplayModeShown);
    theGEText = this->mOk->GetGEText();
    theGEText->SetFgColor(kMessageTextColor);
    theGEText->SetIsTransparent(true);
    gResource->GetString(IDS_NO, theText);
    this->mNo->SetText(theText, gFontTahoma12);
    this->mNo->SetCallback(inNo, inData);
    this->mNo->SetDisplayMode(kDisplayModeShown);
    theGEText = this->mNo->GetGEText();
    theGEText->SetFgColor(kMessageTextColor);
    theGEText->SetIsTransparent(true);
    this->mError->SetAlignment(inAlignment);
    this->mError->SetText(inMessage);
    this->mCancel->SetDisplayMode(kDisplayModeHidden);
    this->mEditBox->SetDisplayMode(kDisplayModeHidden);
    this->mMode = 1;
    gUserInterface->SwitchToForm(this);
}

// FUNCTION: EXE 0x6619f9
// Shows a Yes/No/Cancel question with Yes and No callbacks (Cancel has none)
void EEUIFError::DisplayYesNoCancel(const UWideString& inMessage, unsigned long inAlignment, void* inData,
                                    EEUIFErrorCallback inYes, EEUIFErrorCallback inNo)
{
    UWideString theText;
    GEText* theGEText;
    gResource->GetString(IDS_YES, theText);
    this->mOk->SetText(theText, gFontTahoma12);
    this->mOk->SetCallback(inYes, inData);
    this->mOk->SetDisplayMode(kDisplayModeShown);
    theGEText = this->mOk->GetGEText();
    theGEText->SetFgColor(kMessageTextColor);
    theGEText->SetIsTransparent(true);
    gResource->GetString(IDS_NO, theText);
    this->mNo->SetText(theText, gFontTahoma12);
    this->mNo->SetCallback(inNo, inData);
    this->mNo->SetDisplayMode(kDisplayModeShown);
    theGEText = this->mNo->GetGEText();
    theGEText->SetFgColor(kMessageTextColor);
    theGEText->SetIsTransparent(true);
    gResource->GetString(IDS_LOBBY_CANCEL, theText);
    this->mCancel->SetText(theText, gFontTahoma12);
    this->mCancel->SetDisplayMode(kDisplayModeShown);
    theGEText = this->mCancel->GetGEText();
    theGEText->SetFgColor(kMessageTextColor);
    theGEText->SetIsTransparent(true);
    this->mEditBox->SetDisplayMode(kDisplayModeHidden);
    this->mError->SetAlignment(inAlignment);
    this->mError->SetText(inMessage);
    this->mMode = 2;
    gUserInterface->SwitchToForm(this);
}

// FUNCTION: EXE 0x661baf
// Draws the message box background
long EEUIFError::DrawBackground(const GERectangle& inRectangle)
{
    return UIForm::DrawBackground(inRectangle);
}

// FUNCTION: EXE 0x661bbc
// Opens the message box, scales its background to the window and lays out the buttons for its mode
void EEUIFError::Open()
{
    UIForm::Open();
    this->mBackground->SetScale((float)gGameWindow->GetWidth() * (1.0f / 1024.0f),
                          (float)gGameWindow->GetHeight() * (1.0f / 768.0f));
    switch (this->mMode) {
    case 4:
        this->LayoutCancel();
        break;
    case 3:
        this->LayoutPassword();
        break;
    case 2:
        this->LayoutYesNoCancel();
        break;
    case 1:
        this->LayoutQuestion();
        break;
    case 0:
        this->LayoutMessage();
        break;
    }
}

// FUNCTION: EXE 0x661c4c
// Closes the message box and posts the pressed button's callback and data (OK first, then No, then Cancel)
void EEUIFError::Close()
{
    EEUIFErrorCallbackData* theData = new EEUIFErrorCallbackData;
    theData->mCallback = NULL;
    theData->mData = NULL;
    if (this->mOk != NULL && this->mOk->mPressed) {
        theData->mCallback = this->mOk->mCallback;
        theData->mData = this->mOk->mData;
    } else if (this->mNo != NULL && this->mNo->mPressed) {
        theData->mCallback = this->mNo->mCallback;
        theData->mData = this->mNo->mData;
    } else if (this->mCancel != NULL && this->mCancel->mPressed) {
        theData->mCallback = this->mCancel->mCallback;
        theData->mData = this->mCancel->mData;
    }
    gUserInterface->PostErrorClosed(this, theData);
    UIForm::Close();
}

// FUNCTION: EXE 0x661cda
// Lays out the Cancel-only box: the button and the message area
void EEUIFError::LayoutCancel()
{
    UILayoutUtility theLayout;
    GERectangle theScreen(0.0f, 0.0f, (float)gGameWindow->GetWidth(), (float)gGameWindow->GetHeight());
    GERectangle theRectangle;
    theLayout.SetLeft(4, 451.0f / 1024.0f);
    theLayout.SetRight(4, 573.0f / 1024.0f);
    theLayout.SetTop(4, 465.0f / 768.0f);
    theLayout.SetBottom(4, 491.0f / 768.0f);
    theLayout.CalculateRectangle(theScreen, theRectangle, true);
    this->mCancel->SetRectangle(theRectangle);
    GEFont* theFont = UIFontTable::CreateClosestFont(0x1c, UMFloatToLong((gFontTahoma16->GetHeight() * (1.0f / 768.0f)) *
                                                                         theScreen.GetHeight()));
    this->mCancel->GetGEText()->SetFont(theFont);
    theFont->Dereference();
    theLayout.SetLeft(4, 325.0f / 1024.0f);
    theLayout.SetRight(4, 699.0f / 1024.0f);
    theLayout.SetTop(4, 300.0f / 768.0f);
    theLayout.SetBottom(4, 460.0f / 768.0f);
    theLayout.CalculateRectangle(theScreen, theRectangle, true);
    this->mError->SetRectangle(theRectangle);
}

// FUNCTION: EXE 0x661e18
// Lays out the text-entry box: OK, the message and the edit box
void EEUIFError::LayoutPassword()
{
    UILayoutUtility theLayout;
    GERectangle theScreen(0.0f, 0.0f, (float)gGameWindow->GetWidth(), (float)gGameWindow->GetHeight());
    GERectangle theRectangle;
    theLayout.SetLeft(4, 451.0f / 1024.0f);
    theLayout.SetRight(4, 573.0f / 1024.0f);
    theLayout.SetTop(4, 465.0f / 768.0f);
    theLayout.SetBottom(4, 491.0f / 768.0f);
    theLayout.CalculateRectangle(theScreen, theRectangle, true);
    this->mOk->SetRectangle(theRectangle);
    GEFont* theFont = UIFontTable::CreateClosestFont(0x1c, UMFloatToLong((gFontTahoma16->GetHeight() * (1.0f / 768.0f)) *
                                                                         theScreen.GetHeight()));
    this->mOk->GetGEText()->SetFont(theFont);
    theFont->Dereference();
    theLayout.SetLeft(4, 325.0f / 1024.0f);
    theLayout.SetRight(4, 699.0f / 1024.0f);
    theLayout.SetTop(4, 300.0f / 768.0f);
    theLayout.SetBottom(4, 420.0f / 768.0f);
    theLayout.CalculateRectangle(theScreen, theRectangle, true);
    this->mError->SetRectangle(theRectangle);
    theLayout.SetLeft(4, 325.0f / 1024.0f);
    theLayout.SetRight(4, 699.0f / 1024.0f);
    theLayout.SetTop(4, 430.0f / 768.0f);
    theLayout.SetBottom(4, 460.0f / 768.0f);
    theLayout.CalculateRectangle(theScreen, theRectangle, true);
    this->mEditBox->SetRectangle(theRectangle);
}

// FUNCTION: EXE 0x661f9c
// Lays out the question box: Yes, No and the message area
void EEUIFError::LayoutQuestion()
{
    UILayoutUtility theLayout;
    GERectangle theScreen(0.0f, 0.0f, (float)gGameWindow->GetWidth(), (float)gGameWindow->GetHeight());
    GERectangle theRectangle;
    theLayout.SetLeft(4, 388.0f / 1024.0f);
    theLayout.SetRight(4, 510.0f / 1024.0f);
    theLayout.SetTop(4, 465.0f / 768.0f);
    theLayout.SetBottom(4, 491.0f / 768.0f);
    theLayout.CalculateRectangle(theScreen, theRectangle, true);
    this->mOk->SetRectangle(theRectangle);
    GEFont* theFont = UIFontTable::CreateClosestFont(0x1c, UMFloatToLong((gFontTahoma16->GetHeight() * (1.0f / 768.0f)) *
                                                                         theScreen.GetHeight()));
    this->mOk->GetGEText()->SetFont(theFont);
    theLayout.SetLeft(4, 514.0f / 1024.0f);
    theLayout.SetRight(4, 636.0f / 1024.0f);
    theLayout.CalculateRectangle(theScreen, theRectangle, true);
    this->mNo->SetRectangle(theRectangle);
    this->mNo->GetGEText()->SetFont(theFont);
    theFont->Dereference();
    theLayout.SetLeft(4, 325.0f / 1024.0f);
    theLayout.SetRight(4, 699.0f / 1024.0f);
    theLayout.SetTop(4, 300.0f / 768.0f);
    theLayout.SetBottom(4, 460.0f / 768.0f);
    theLayout.CalculateRectangle(theScreen, theRectangle, true);
    this->mError->SetRectangle(theRectangle);
}

// FUNCTION: EXE 0x662121
// Lays out the Yes/No/Cancel box: the three buttons and the message area
void EEUIFError::LayoutYesNoCancel()
{
    UILayoutUtility theLayout;
    GERectangle theScreen(0.0f, 0.0f, (float)gGameWindow->GetWidth(), (float)gGameWindow->GetHeight());
    GERectangle theRectangle;
    theLayout.SetLeft(4, 325.0f / 1024.0f);
    theLayout.SetRight(4, 447.0f / 1024.0f);
    theLayout.SetTop(4, 465.0f / 768.0f);
    theLayout.SetBottom(4, 491.0f / 768.0f);
    theLayout.CalculateRectangle(theScreen, theRectangle, true);
    this->mOk->SetRectangle(theRectangle);
    GEFont* theFont = UIFontTable::CreateClosestFont(0x1c, UMFloatToLong((gFontTahoma16->GetHeight() * (1.0f / 768.0f)) *
                                                                         theScreen.GetHeight()));
    this->mOk->GetGEText()->SetFont(theFont);
    theLayout.SetLeft(4, 451.0f / 1024.0f);
    theLayout.SetRight(4, 573.0f / 1024.0f);
    theLayout.CalculateRectangle(theScreen, theRectangle, true);
    this->mNo->SetRectangle(theRectangle);
    this->mNo->GetGEText()->SetFont(theFont);
    theLayout.SetLeft(4, 577.0f / 1024.0f);
    theLayout.SetRight(4, 699.0f / 1024.0f);
    theLayout.CalculateRectangle(theScreen, theRectangle, true);
    this->mCancel->SetRectangle(theRectangle);
    this->mCancel->GetGEText()->SetFont(theFont);
    theFont->Dereference();
    theLayout.SetLeft(4, 325.0f / 1024.0f);
    theLayout.SetRight(4, 699.0f / 1024.0f);
    theLayout.SetTop(4, 300.0f / 768.0f);
    theLayout.SetBottom(4, 460.0f / 768.0f);
    theLayout.CalculateRectangle(theScreen, theRectangle, true);
    this->mError->SetRectangle(theRectangle);
}

// FUNCTION: EXE 0x6622ed
// Lays out the message box: OK, and the message area centred vertically for short messages
void EEUIFError::LayoutMessage()
{
    UILayoutUtility theLayout;
    GERectangle theScreen(0.0f, 0.0f, (float)gGameWindow->GetWidth(), (float)gGameWindow->GetHeight());
    GERectangle theRectangle;
    theLayout.SetLeft(4, 451.0f / 1024.0f);
    theLayout.SetRight(4, 573.0f / 1024.0f);
    theLayout.SetTop(4, 465.0f / 768.0f);
    theLayout.SetBottom(4, 491.0f / 768.0f);
    theLayout.CalculateRectangle(theScreen, theRectangle, true);
    this->mOk->SetRectangle(theRectangle);
    GEFont* theFont = UIFontTable::CreateClosestFont(0x1c, UMFloatToLong((gFontTahoma16->GetHeight() * (1.0f / 768.0f)) *
                                                                         theScreen.GetHeight()));
    this->mOk->GetGEText()->SetFont(theFont);
    theFont->Dereference();
    theLayout.SetLeft(4, 325.0f / 1024.0f);
    theLayout.SetRight(4, 699.0f / 1024.0f);
    theLayout.SetTop(4, 300.0f / 768.0f);
    theLayout.SetBottom(4, 460.0f / 768.0f);
    theLayout.CalculateRectangle(theScreen, theRectangle, true);
    unsigned long theLines = UMFloor(theRectangle.GetHeight() / gFontTahoma12->GetHeight()) - 2;
    float theOffset;
    if (theLines > this->mError->GetLineCount())
        theOffset = (float)(theLines - this->mError->GetLineCount()) * gFontTahoma12->GetHeight() * 0.5f;
    else
        theOffset = 0.0f;
    theRectangle.SetTop(theRectangle.GetTop() + theOffset);
    theRectangle.SetBottom(theRectangle.GetBottom() - theOffset);
    this->mError->SetRectangle(theRectangle);
}

// SYNTHETIC: EXE 0x660c27
// SYNTHETIC: EXE 0x660c31
// SYNTHETIC: EXE 0x660c4a
// SYNTHETIC: EXE 0x660c56
// SYNTHETIC: EXE 0x660c57
// SYNTHETIC: EXE 0x660c61
// SYNTHETIC: EXE 0x660c7a
// SYNTHETIC: EXE 0x660c86
// SYNTHETIC: EXE 0x660c87
// SYNTHETIC: EXE 0x660c91
// SYNTHETIC: EXE 0x660caa
// SYNTHETIC: EXE 0x660cb6
// SYNTHETIC: EXE 0x660cb7
// SYNTHETIC: EXE 0x660cc1
// SYNTHETIC: EXE 0x660cde
// SYNTHETIC: EXE 0x660cea
// SYNTHETIC: EXE 0x660ceb
// SYNTHETIC: EXE 0x660cf5
// SYNTHETIC: EXE 0x660d0e
// SYNTHETIC: EXE 0x660d1a
// SYNTHETIC: EXE 0x660d1b
// SYNTHETIC: EXE 0x660d25
// SYNTHETIC: EXE 0x660d42
// SYNTHETIC: EXE 0x660d4e
// SYNTHETIC: EXE 0x660d4f
// SYNTHETIC: EXE 0x660d59
// SYNTHETIC: EXE 0x660d78
// SYNTHETIC: EXE 0x660d84

// ---- patched: the setup screen's unit rules dialog (ee/EEUIFUnitRules.h; not in the original game) ----
#include <algorithm>
#include <vector>
#include "lle/UIListBox.h"
#include "lle/UILBString.h"
#include "lle/UILayoutUtility.h"
#include "ee/EEGameOptions.h"
#include "ee/EEUnitRules.h"
#include "ee/EEUIFUnitRules.h"

// what the dialog's buttons and lists do (the event's action)
enum EEURAction {
    kURActionUnits = kUnitRulesUnits,
    kURActionBuildings = kUnitRulesBuildings,
    kURActionHeroes = kUnitRulesHeroes,
    kURActionDisable = 10,                  // the available list's selection goes to the disabled list
    kURActionEnable,                        // and back
    kURActionDefaults,
    kURActionOk,
    kURActionCancel
};

// the dialog's controls, by form index (also their ids)
enum EEURControl {
    kURTitle = 0,
    kURHint,
    kURAvailableLabel,
    kURDisabledLabel,
    kURCategoryFirst,                       // the three category buttons
    kURAvailableList = kURCategoryFirst + kUnitRulesCategoryCount,
    kURDisabledList,
    kURDisableButton,
    kUREnableButton,
    kURDefaultsButton,
    kUROkButton,
    kURCancelButton,
    kURControlCount
};

class EEUIFUnitRules;

// runs one of the dialog's actions (a button's button-up, a list's double-click)
class EEUIFUREvent : public UIEvent {
public:
    EEUIFUREvent(long inAction) : mAction(inAction) {}
    virtual long ProcessEvent(UIForm* inForm, UIEntity* inEntity);
private:
    long mAction;
};

// a text button over a light translucent panel, like the message box's (EEUIFEBOK), stronger when highlighted
class EEUIFURButton : public UIButton {
public:
    EEUIFURButton(UIForm* inForm, unsigned long inIndex, const GERectangle& inRectangle, const UWideString& inText,
                  GEFont* inFont, long inAction)
        : UIButton(inIndex, NULL, NULL, NULL, inRectangle, inForm, inIndex, 1), mHighlighted(false)
    {
        this->SetFocusedTextColor(kMessageFocusedColor);
        this->SetText(inText, inFont);
        GEText* theGEText = this->GetGEText();
        theGEText->SetFgColor(kMessageTextColor);
        theGEText->SetIsTransparent(true);
        UIEvent* theEvent = new EEUIFUREvent(inAction);
        this->SetTriggerEvent(kControlTriggerButtonUp, theEvent);
        theEvent->Dereference();
    }
    virtual ~EEUIFURButton() {}
    virtual long DrawPolygons()
    {
        GERhombus theRhombus;
        theRhombus.SetColor(this->mHighlighted ? kMessageSelectedColor : kMessageButtonColor);
        theRhombus.SetRectangle(this->mCopiedDrawRectangle);
        theRhombus.SetOpacity(this->mHighlighted ? 0.6f : kMessageButtonOpacity);
        this->GetRasterizer()->Draw(&theRhombus, this->GetScreenIndex());
        return UIButton::DrawPolygons();
    }
    void SetHighlighted(bool inHighlighted)
    {
        if (this->mHighlighted != inHighlighted) {
            this->mHighlighted = inHighlighted;
            this->SetDirty(true);
        }
    }
private:
    bool mHighlighted;
};

// one line of a list: the tech's epoch (1-based) and unit name, sorted by epoch, then name
struct EEURLine {
    long mEpoch;
    UWideString mName;
    long mID;
};
struct EEURLineLess {
    bool operator()(const EEURLine& inLeft, const EEURLine& inRight) const
    {
        if (inLeft.mEpoch != inRight.mEpoch)
            return inLeft.mEpoch < inRight.mEpoch;
        return _mbsicmp(reinterpret_cast<const unsigned char*>((const char*)inLeft.mName),
                        reinterpret_cast<const unsigned char*>((const char*)inRight.mName)) < 0;
    }
};

class EEUIFUnitRules : public UIForm {
public:
    EEUIFUnitRules(UIManager* inManager);
    virtual ~EEUIFUnitRules();
    void OnAction(long inAction);
protected:
    virtual void Open();
    virtual void Close();
private:
    GERectangle GetRectangle(float inLeft, float inTop, float inRight, float inBottom) const;
    UITextLabel* CreateLabel(unsigned long inIndex, const UWideString& inText, float inLeft, float inTop,
                             float inRight, float inBottom, unsigned long inAlignment, unsigned long inLabelType);
    UILBString* CreateList(unsigned long inIndex, float inLeft, float inRight, long inDoubleClickAction);
    void Refill();
    void Move(bool inToDisabled);

    EEUnitRules mRules;                     // the working copy (OK stores it)
    long mCategory;
    GERectangle mScreen;
    GEFont* mFont;
    UITextLabel* mLabels[4];                // title, hint, the two list headers
    EEUIFURButton* mCategoryButtons[kUnitRulesCategoryCount];
    EEUIFURButton* mButtons[5];             // disable, enable, defaults, OK, cancel
    UILBString* mAvailable;
    UILBString* mDisabled;
    std::vector<long> mAvailableIDs;        // the lists' tech ids, line by line
    std::vector<long> mDisabledIDs;
    EEUIBGenericBackground* mBackground;
};

long EEUIFUREvent::ProcessEvent(UIForm* inForm, UIEntity* inEntity)
{
    static_cast<EEUIFUnitRules*>(inForm)->OnAction(this->mAction);
    return kErrorOK;
}

// a rectangle in the 1024x768 design, scaled to the window
GERectangle EEUIFUnitRules::GetRectangle(float inLeft, float inTop, float inRight, float inBottom) const
{
    UILayoutUtility theLayout;
    GERectangle theRectangle;
    theLayout.SetLeft(4, inLeft / 1024.0f);
    theLayout.SetRight(4, inRight / 1024.0f);
    theLayout.SetTop(4, inTop / 768.0f);
    theLayout.SetBottom(4, inBottom / 768.0f);
    theLayout.CalculateRectangle(this->mScreen, theRectangle, true);
    return theRectangle;
}

UITextLabel* EEUIFUnitRules::CreateLabel(unsigned long inIndex, const UWideString& inText, float inLeft, float inTop,
                                         float inRight, float inBottom, unsigned long inAlignment,
                                         unsigned long inLabelType)
{
    UITextLabel* theLabel = new UITextLabel(inText, this->mFont, inIndex, this->GetRectangle(inLeft, inTop, inRight, inBottom),
                                            this, inLabelType, inIndex, 0);
    theLabel->SetAlignment(inAlignment);
    GEText* theGEText = theLabel->GetGEText();
    theGEText->SetIsTransparent(true);
    theGEText->SetFgColor(kMessageTextColor);
    this->AddControl(theLabel, 0);
    return theLabel;
}

UILBString* EEUIFUnitRules::CreateList(unsigned long inIndex, float inLeft, float inRight, long inDoubleClickAction)
{
    std::vector<UWideString> theLines;
    UILBString* theList = new UILBString(inIndex, theLines, this->mFont, false, this->GetRectangle(inLeft, 206.0f, inRight, 566.0f),
                                         this, inIndex, 1);
    GEText* theGEText = theList->GetGEText();
    theGEText->SetIsTransparent(true);
    theGEText->SetFgColor(kMessageTextColor);
    theList->SetBgColor(kMessageEditBoxColor);
    theList->SetBgOpacity(0.5f);
    theList->SetScrollBarBgOpacity(kMessageScrollBarOpacity);
    theList->GetSelectedBackground()->SetColor(kMessageSelectedColor);
    UIEvent* theEvent = new EEUIFUREvent(inDoubleClickAction);
    theList->SetTriggerEvent(kControlTriggerDoubleClicked, theEvent);
    theEvent->Dereference();
    this->AddControl(theList, 0);
    return theList;
}

// Creates the dialog on the current unit rules, laid out for a 1024x768 design like the message box
EEUIFUnitRules::EEUIFUnitRules(UIManager* inManager)
    : UIForm(kURControlCount, 1, inManager, -1, true), mCategory(kUnitRulesUnits)
{
    UWideString theText;
    unsigned long i;
    this->mScreen = GERectangle(0.0f, 0.0f, (float)gGameWindow->GetWidth(), (float)gGameWindow->GetHeight());
    gGameOptions.mLock->ReadLock();
    this->mRules = gUnitRules;
    gGameOptions.mLock->ReadUnlock();
    this->mFont = UIFontTable::CreateClosestFont(0x1c, UMFloatToLong((gFontTahoma12->GetHeight() * (1.0f / 768.0f)) *
                                                                     this->mScreen.GetHeight()));
    EEUnitRules::GetText(IDS_UNIT_RULES_TITLE, "Units and buildings every player can make", theText);
    this->mLabels[0] = this->CreateLabel(kURTitle, theText, 172.0f, 110.0f, 852.0f, 136.0f, kTextAlignCenter, 0);
    EEUnitRules::GetText(IDS_UNIT_RULES_HINT, "Double-click a line to move it.", theText);
    this->mLabels[1] = this->CreateLabel(kURHint, theText, 172.0f, 572.0f, 852.0f, 618.0f, kTextAlignCenter, 1);
    EEUnitRules::GetText(IDS_UNIT_RULES_AVAILABLE, "Available", theText);
    this->mLabels[2] = this->CreateLabel(kURAvailableLabel, theText, 172.0f, 182.0f, 482.0f, 202.0f, kTextAlignCenter, 0);
    EEUnitRules::GetText(IDS_UNIT_RULES_DISABLED, "Disabled", theText);
    this->mLabels[3] = this->CreateLabel(kURDisabledLabel, theText, 542.0f, 182.0f, 852.0f, 202.0f, kTextAlignCenter, 0);
    for (i = 0; i < kUnitRulesCategoryCount; i++) {
        // the scenario editor's names: Units, Buildings, Heroes
        gResource->GetString(IDS_SE_UNIT_GROUP_FIRST + i, theText);
        float theLeft = 172.0f + 130.0f * i;
        this->mCategoryButtons[i] = new EEUIFURButton(this, kURCategoryFirst + i,
                                                      this->GetRectangle(theLeft, 146.0f, theLeft + 120.0f, 170.0f),
                                                      theText, this->mFont, kURActionUnits + i);
        this->AddControl(this->mCategoryButtons[i], 0);
    }
    this->mAvailable = this->CreateList(kURAvailableList, 172.0f, 482.0f, kURActionDisable);
    this->mDisabled = this->CreateList(kURDisabledList, 542.0f, 852.0f, kURActionEnable);
    this->mButtons[0] = new EEUIFURButton(this, kURDisableButton, this->GetRectangle(492.0f, 330.0f, 532.0f, 356.0f),
                                          UWideString(">>"), this->mFont, kURActionDisable);
    this->mButtons[1] = new EEUIFURButton(this, kUREnableButton, this->GetRectangle(492.0f, 366.0f, 532.0f, 392.0f),
                                          UWideString("<<"), this->mFont, kURActionEnable);
    EEUnitRules::GetText(IDS_UNIT_RULES_DEFAULT, "Game defaults", theText);
    this->mButtons[2] = new EEUIFURButton(this, kURDefaultsButton, this->GetRectangle(172.0f, 626.0f, 332.0f, 652.0f),
                                          theText, this->mFont, kURActionDefaults);
    gResource->GetString(IDS_LOBBY_OK, theText);
    this->mButtons[3] = new EEUIFURButton(this, kUROkButton, this->GetRectangle(612.0f, 626.0f, 722.0f, 652.0f),
                                          theText, this->mFont, kURActionOk);
    gResource->GetString(IDS_LOBBY_CANCEL, theText);
    this->mButtons[4] = new EEUIFURButton(this, kURCancelButton, this->GetRectangle(742.0f, 626.0f, 852.0f, 652.0f),
                                          theText, this->mFont, kURActionCancel);
    for (i = 0; i < 5; i++)
        this->AddControl(this->mButtons[i], 0);
    this->GetEntityGroup(0)->Enable();
    UILayoutUtility theLayout;
    theLayout.SetLeft(4, 152.0f / 1024.0f);
    theLayout.SetRight(4, 872.0f / 1024.0f);
    theLayout.SetTop(4, 94.0f / 768.0f);
    theLayout.SetBottom(4, 674.0f / 768.0f);
    this->SetLayout(theLayout);
    GETextureDescription theDescription;
    theDescription.mIs3DRenderable = false;
    theDescription.mIsTransparencyEnabled = false;
    theDescription.mNumMipLevelsToDrop = 0;
    theDescription.mSurfaceVolatility = 1;
    theDescription.mFrameFormat = 0;
    theDescription.mCreateMIPMaps = true;
    theDescription.mPixelDepth = this->GetManager()->GetUITextureBitDepth();
    theDescription.mHasAlpha = true;
    this->mBackground = new EEUIBGenericBackground(0, "textures\\EEGenericBack_Gold", theDescription, theLayout, this, 0,
                                                   kMessageBackgroundColor, true, 1.0f, 1.0f, 1.0f, 3);
    this->Refill();
}

// Destroys the dialog's controls and its font (the form owns the background, as for the message box)
EEUIFUnitRules::~EEUIFUnitRules()
{
    unsigned long i;
    for (i = 0; i < 4; i++)
        delete this->mLabels[i];
    for (i = 0; i < kUnitRulesCategoryCount; i++)
        delete this->mCategoryButtons[i];
    for (i = 0; i < 5; i++)
        delete this->mButtons[i];
    delete this->mAvailable;
    delete this->mDisabled;
    this->mFont->Dereference();
}

// Opens the dialog, scaling the background to the window as the message box does
void EEUIFUnitRules::Open()
{
    UIForm::Open();
    this->mBackground->SetScale((float)gGameWindow->GetWidth() * (1.0f / 1024.0f),
                                (float)gGameWindow->GetHeight() * (1.0f / 768.0f));
}

// Closes the dialog: the UI thread deletes it once it is off the form stack (EEUserInterface's message 0x417, as for
// the message box), and the lobby redraws its Units option
void EEUIFUnitRules::Close()
{
    EEUIFErrorCallbackData* theData = new EEUIFErrorCallbackData;
    theData->mCallback = NULL;
    theData->mData = NULL;
    gUserInterface->PostErrorClosed(this, theData);
    UIForm::Close();
    gUserInterface->PostLobbyUpdateGameOptions();
}

// Refills both lists with the current category's techs, by the working copy
void EEUIFUnitRules::Refill()
{
    std::vector<long> theIDs;
    std::vector<EEURLine> theLines;
    EEURLine theLine;
    UWideString theText;
    unsigned long i;
    EEUnitRules::GetTechs(this->mCategory, theIDs);
    for (i = 0; i < theIDs.size(); i++) {
        EETechTreeNode* theNode = gTechTree.Get(theIDs[i]);
        if (theNode == NULL || theNode->mData == NULL)
            continue;
        theLine.mID = theIDs[i];
        theLine.mEpoch = theNode->mEpoch + 1;
        theLine.mName = "";
        long theClass = theNode->mData->mUnitClass;
        if (theClass > 0 && theClass < (long)(sizeof(gUnitClassTable) / sizeof(gUnitClassTable[0])) &&
            gUnitClassTable[theClass] != NULL)
            gResource->GetString(gUnitClassTable[theClass]->mNameStringID, theLine.mName);
        if (theLine.mName.GetLength() == 0)
            gResource->GetString(theNode->mData->mNameID, theLine.mName);
        theLines.push_back(theLine);
    }
    std::sort(theLines.begin(), theLines.end(), EEURLineLess());
    this->mAvailable->DeleteAllEntries();
    this->mDisabled->DeleteAllEntries();
    this->mAvailableIDs.clear();
    this->mDisabledIDs.clear();
    for (i = 0; i < theLines.size(); i++) {
        sprintf(theText, "%d: %s", (int)theLines[i].mEpoch, (const char*)theLines[i].mName);
        if (this->mRules.IsDisabled(theLines[i].mID)) {
            this->mDisabled->AppendLine(theText);
            this->mDisabledIDs.push_back(theLines[i].mID);
        } else {
            this->mAvailable->AppendLine(theText);
            this->mAvailableIDs.push_back(theLines[i].mID);
        }
    }
    for (i = 0; i < kUnitRulesCategoryCount; i++)
        this->mCategoryButtons[i]->SetHighlighted((long)i == this->mCategory);
}

// Moves a list's selected tech to the other list (with what depends on it or what it needs), keeping a nearby line
// selected in the list it left and selecting it in the list it went to
void EEUIFUnitRules::Move(bool inToDisabled)
{
    UILBString* theFrom = inToDisabled ? this->mAvailable : this->mDisabled;
    UILBString* theTo = inToDisabled ? this->mDisabled : this->mAvailable;
    unsigned long theSelection;
    if (theFrom->GetCurrentSelection(theSelection) != kErrorOK)
        return;
    const std::vector<long>& theFromIDs = inToDisabled ? this->mAvailableIDs : this->mDisabledIDs;
    if (theSelection >= theFromIDs.size())
        return;
    long theID = theFromIDs[theSelection];
    this->mRules.SetDisabled(theID, inToDisabled);
    this->Refill();
    unsigned long theCount = theFrom->GetLineCount();
    if (theCount > 0)
        theFrom->ChangeSelectionIndex(theSelection < theCount ? theSelection : theCount - 1, false, true);
    const std::vector<long>& theToIDs = inToDisabled ? this->mDisabledIDs : this->mAvailableIDs;
    for (unsigned long j = 0; j < theToIDs.size(); j++) {
        if (theToIDs[j] == theID) {
            theTo->ChangeSelectionIndex(j, false, true);
            break;
        }
    }
}

void EEUIFUnitRules::OnAction(long inAction)
{
    switch (inAction) {
    case kURActionUnits:
    case kURActionBuildings:
    case kURActionHeroes:
        this->mCategory = inAction;
        this->Refill();
        break;
    case kURActionDisable:
        this->Move(true);
        break;
    case kURActionEnable:
        this->Move(false);
        break;
    case kURActionDefaults:
        this->mRules.Reset();
        this->Refill();
        break;
    case kURActionOk: {
        // store the working copy, and send it to the other players when it changed
        UWideString theOld;
        UWideString theNew;
        gGameOptions.mLock->WriteLock();
        gUnitRules.ToString(theOld);
        this->mRules.ToString(theNew);
        if (theOld != theNew) {
            gUnitRules = this->mRules;
            gUnitRules.Send(kSlotNone);
        }
        gGameOptions.mLock->WriteUnlock();
        gUserInterface->CloseForm(NULL);
        break;
    }
    case kURActionCancel:
        gUserInterface->CloseForm(NULL);
        break;
    }
}

void EEShowUnitRulesDialog()
{
    gUserInterface->SwitchToForm(new EEUIFUnitRules(gUIManager));
}
