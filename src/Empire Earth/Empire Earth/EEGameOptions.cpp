// C:\EEX\Empire Earth\Empire Earth\EEGameOptions.cpp (__FILE__ 0x4bf9c4, the ctors' CreateReadWriteLock asserts).
// 0x12f4b2..0x1304e4: the empty player slot's `$E` chain, EEGameOptionsPlayer, EEGameOptions and
// the two table-size helpers; EEGameState::Clear 0x1304e4 starts the next unit. Unit flags: the exe module set.
#include <windows.h>
#include "lle/LowLevelEngine.h"
#include "lle/UWideString.h"
#include "ee/EESMDialogMessage.h"
#include "lle/FSFile.h"
#include <vector>
#include "ee/EEAreaUtilities.h"
#include "ee/EEScenarioManager.h"
#include "ee/EESMCondition.h"
#include <algorithm>
#include <mbstring.h>
#include <stdlib.h>
#include "ee/EESMEffect.h"
#include "ee/EESMObject.h"
#include "ee/EESMArea.h"
#include "ee/EESMTrigger.h"
#include "ee/EEDeleteVectorContents.h"
#include "lle/UResourceFile.h"
#include "ee/EmpireEarth.h"
#include "ee/RTSWorld.h"
#include "ee/EEWorld.h"
#include "ee/EEPlayer.h"
#include "ee/EEComputerPlayer.h"
#include "ee/EEUnit.h"
#include "ee/EEUCAirplane.h"
#include "ee/EEUGLocation.h"
#include "ee/EEUCBuilding.h"
#include "ee/EEUCGate.h"
#include "ee/EEPhysics.h"
#include "ee/EEAIRandom.h"
#include "ee/EEDebugTrace.h"
#include "LostDebug.h"
#include "lle/SESoundManager.h"
#include "lle/FSFileSpec.h"
#include "GEWindow.h"
#include "ee/EECameraControl.h"
#include "ee/EEGraphics.h"
#include "ee/EEMouse.h"
#include "ee/EEUserInterface.h"
#include "lle/GEColor.h"
#include "lle/GETextureCoordinate.h"
#include "lle/GESurface.h"
#include "lle/GETextureManager.h"
#include "lle/GETexture.h"
#include "lle/GEBitmap.h"
#include "lle/GEVertexArray.h"
#include "lle/GERhombus.h"
#include "lle/ISEntity.h"
#include "lle/UIEntity.h"
#include "lle/UIControl.h"
#include "lle/UIEvent.h"
#include "lle/UIEntityGroup.h"
#include "lle/UIBackground.h"
#include "lle/UIFontTable.h"
#include "lle/Input System.h"
#include "lle/ISKeyboard.h"
#include "lle/UIHotKey.h"
#include "lle/UIIMECandidate.h"
#include "lle/UIForm.h"
#include "lle/GEScreen.h"
#include "lle/GEViewport.h"
#include "lle/GERasterizer.h"
#include "lle/UIManager.h"
#include "ee/EEUIManager.h"
#include "ee/EEUIFMainGame.h"
#include "ee/EEGameSound.h"
#include "lle/NEMessage.h"
#include "lle/NESynchronousCommunications.h"
#include "ee/EECommunications.h"
#include "lle/TSScheduler.h"
#include "ee/EEDatabaseTables.h"
#include "ee/EEGameVariant.h"
#include "ee/EERandomMapGenerators.h"
#include "ee/EEGameOptions.h"

extern char gEmptyString[];                 // 0x4e5b98 (pooled "")
extern EEGameOptions gLobbyGameOptions;     // 0x53030c (SetDefaults starts from it)
unsigned long EEGetNumberOfResourceLevels();

// 0x529b38: the empty slot every constructor and SetDefaults copies into the sixteen players
static EEGameOptionsPlayer sEmptyPlayer;
// SYNTHETIC: EXE 0x52f4b2
// SYNTHETIC: EXE 0x52f4bc
// SYNTHETIC: EXE 0x52f4c6
// SYNTHETIC: EXE 0x52f4d2

#include "ee/EEStartingResources.h"

// FUNCTION: EXE 0x52f4dd
// Creates an empty, closed player slot with no name, colour, team or starting citizens
EEGameOptionsPlayer::EEGameOptionsPlayer()
{
    this->mType = 3;
    this->mLoadedType = 3;
    this->mName = gEmptyString;
    this->mPing = 0.0f;
    this->mSlot = 0;
    this->mColor = 0;
    this->mTeam = 0;
    this->mStartingCitizens = 0;
    this->mIsLocal = false;
    this->mIsReady = false;
    this->mTransferID = 0;
    this->mB36 = true;
}

// FUNCTION: EXE 0x52f526
// Copies a player slot's settings (type, name, colour, team, citizens, ping, ready state and transfer id)
EEGameOptionsPlayer& EEGameOptionsPlayer::operator=(const EEGameOptionsPlayer& inOther)
{
    this->mType = inOther.mType;
    this->mName = inOther.mName;
    this->mColor = inOther.mColor;
    this->mTeam = inOther.mTeam;
    this->mStartingCitizens = inOther.mStartingCitizens;
    this->mPing = inOther.mPing;
    this->mIsLocal = inOther.mIsLocal;
    this->mIsReady = inOther.mIsReady;
    this->mSlot = inOther.mSlot;
    this->mB36 = inOther.mB36;
    this->mLoadedType = inOther.mLoadedType;
    this->mTransferID = inOther.mTransferID;
    return *this;
}

// FUNCTION: EXE 0x52f582
// Reads a player slot from a saved game and remembers its loaded type; returns the first read error
long EEGameOptionsPlayer::Load(unsigned long inMajorVersion, unsigned long inMinorVersion, FSFile& inFile)
{
    long theResult = inFile.Read(this->mType);
    if (theResult == kErrorOK)
        theResult = this->mName.Load(inFile);
    if (theResult == kErrorOK)
        theResult = inFile.Read(this->mPing);
    if (theResult == kErrorOK)
        theResult = inFile.Read(this->mSlot);
    if (theResult == kErrorOK)
        theResult = inFile.Read(this->mColor);
    if (theResult == kErrorOK)
        theResult = inFile.Read(this->mTeam);
    if (theResult == kErrorOK)
        theResult = inFile.Read(this->mStartingCitizens);
    if (theResult == kErrorOK)
        theResult = inFile.Read(this->mIsLocal);
    if (theResult == kErrorOK)
        theResult = inFile.Read(this->mIsReady);
    if (theResult == kErrorOK)
        theResult = inFile.Read(this->mB36);
    this->mLoadedType = this->mType;
    return theResult;
}

// FUNCTION: EXE 0x52f626
// Writes a player slot to a saved game; returns the first write error
long EEGameOptionsPlayer::Save(FSFile& outFile)
{
    long theResult = outFile.Write(this->mType);
    if (theResult == 0)
        theResult = this->mName.Save(outFile);
    if (theResult == 0)
        theResult = outFile.Write(this->mPing);
    if (theResult == 0)
        theResult = outFile.Write(this->mSlot);
    if (theResult == 0)
        theResult = outFile.Write(this->mColor);
    if (theResult == 0)
        theResult = outFile.Write(this->mTeam);
    if (theResult == 0)
        theResult = outFile.Write(this->mStartingCitizens);
    if (theResult == 0)
        theResult = outFile.Write(this->mIsLocal);
    if (theResult == 0)
        theResult = outFile.Write(this->mIsReady);
    if (theResult == 0)
        theResult = outFile.Write(this->mB36);
    return theResult;
}

// FUNCTION: EXE 0x52f6d8
// Creates default game options (random epochs, normal speed and map size, unit limit 50, three wonders to win)
// with sixteen empty player slots
EEGameOptions::EEGameOptions()
{
    ESVERIFY_AT(TSScheduler::CreateReadWriteLock(this->mLock) == keseOK, 304);
    this->mLoadType = 0;
    this->mResources = 0;
    this->mVariant = 0;
    this->mEpochStart = kTechEpochAll;
    this->mEpochEnd = kTechEpochSpace;
    this->mMapType = gEmptyString;
    this->mCampaignName = gEmptyString;
    this->mScenarioName = gEmptyString;
    this->mMapName = gEmptyString;
    this->mDifficulty = 0;
    this->mCampaignChecksum = 0;
    this->mLockTeams = false;
    this->mLockSpeed = false;
    this->mRevealMap = false;
    this->mCheatCodes = false;
    this->mRecordGames = false;
    this->mAllowCustomCivs = false;
    this->mIsMultiplayer = false;
    this->mMapSize = 1;
    this->mUnitLimit = 50;
    this->mWondersForVictory = 3;
    this->mGameSpeed = 1;
    this->mVictory = true;
    this->mB407 = true;
    for (unsigned long i = 0; i < 16; i++)
        this->mPlayers[i] = sEmptyPlayer;
}

// FUNCTION: EXE 0x52f818
// Creates a copy of other game options, with its own lock
EEGameOptions::EEGameOptions(const EEGameOptions& inOther)
{
    ESVERIFY_AT(TSScheduler::CreateReadWriteLock(this->mLock) == keseOK, 349);
    *this = inOther;
}

// FUNCTION: EXE 0x52f892
// Destroys the game options, releasing their lock
EEGameOptions::~EEGameOptions()
{
    TSScheduler::DestroyReadWriteLock(this->mLock);
}

// FUNCTION: EXE 0x52f8e0
// Copies other game options, replacing each out-of-range setting (load type, resources, variant, epochs, map,
// size, difficulty, speed, wonders, unit limit) with a default; copies names, flags and all sixteen slots as they are
EEGameOptions& EEGameOptions::operator=(const EEGameOptions& inOther)
{
    unsigned long theIndex;
    if (inOther.mLoadType < 6)
        this->mLoadType = inOther.mLoadType;
    else
        this->mLoadType = 0;
    if (inOther.mResources < EEGetNumberOfResourceLevels())
        this->mResources = inOther.mResources;
    else
        this->mResources = 0;
    if (inOther.mVariant < EEGetNumberOfGameVariants())
        this->mVariant = inOther.mVariant;
    else
        this->mVariant = 0;
    if (inOther.mEpochStart <= kTechEpochAll)
        this->mEpochStart = inOther.mEpochStart;
    else
        this->mEpochStart = 0;
    if (inOther.mEpochEnd < kTechEpochAll && (inOther.mEpochStart == kTechEpochAll || inOther.mEpochEnd >= inOther.mEpochStart))
        this->mEpochEnd = inOther.mEpochEnd;
    else
        this->mEpochEnd = this->mEpochStart;
    if (inOther.mLoadType == 0) {
        if (EERMFindGenerator(inOther.mMapType, theIndex) == 0)
            this->mMapType = inOther.mMapType;
        else
            this->mMapType = EERMGetNumberOfGenerators() >= 1 ? EERMGetGeneratorName(0) : UWideString(gEmptyString);
    } else {
        this->mMapType = inOther.mMapType;
    }
    if (inOther.mMapSize < 6)
        this->mMapSize = inOther.mMapSize;
    else
        this->mMapSize = 1;
    if (inOther.mDifficulty < 3)
        this->mDifficulty = inOther.mDifficulty;
    else
        this->mDifficulty = 0;
    if (inOther.mGameSpeed < 4)
        this->mGameSpeed = inOther.mGameSpeed;
    else
        this->mGameSpeed = 1;
    if (inOther.mWondersForVictory <= gWorldTable.GetLongValue(77))
        this->mWondersForVictory = inOther.mWondersForVictory;
    else
        this->mWondersForVictory = 0;
    if (inOther.mUnitLimit <= gWorldTable.GetLongValue2(26))
        this->mUnitLimit = inOther.mUnitLimit;
    else
        this->mUnitLimit = gWorldTable.GetLongValue(26);
    this->mCampaignName = inOther.mCampaignName;
    this->mScenarioName = inOther.mScenarioName;
    this->mCampaignChecksum = inOther.mCampaignChecksum;
    this->mVictory = inOther.mVictory;
    this->mLockTeams = inOther.mLockTeams;
    this->mLockSpeed = inOther.mLockSpeed;
    this->mRevealMap = inOther.mRevealMap;
    this->mCheatCodes = inOther.mCheatCodes;
    this->mRecordGames = inOther.mRecordGames;
    this->mAllowCustomCivs = inOther.mAllowCustomCivs;
    this->mIsMultiplayer = inOther.mIsMultiplayer;
    this->mIsScenarioGame = inOther.mIsScenarioGame;
    this->mMapName = inOther.mMapName;
    for (unsigned long i = 0; i < 16; i++)
        this->mPlayers[i] = inOther.mPlayers[i];
    return *this;
}

// FUNCTION: EXE 0x52fb70
// Returns the map's width and height in tiles for the map size setting (0 x 0 for an unknown size)
void EEGameOptions::GetMapDimensions(long& outWidth, long& outHeight)
{
    long theSize;
    switch (this->mMapSize) {
    case 0:
        theSize = 42;
        break;
    case 1:
        theSize = 58;
        break;
    case 2:
        theSize = 75;
        break;
    case 3:
        theSize = 105;
        break;
    case 4:
        theSize = 160;
        break;
    case 5:
        theSize = 220;
        break;
    default:
        outWidth = 0;
        outHeight = 0;
        return;
    }
    outWidth = theSize;
    outHeight = theSize;
}

// FUNCTION: EXE 0x52fbc5
// Returns the campaign scenario's map name when the options name a campaign with a checksum and a map, else ""
bool EEGameOptions::GetCampaignMapName(UWideString& outName)
{
    bool theResult = false;
    if (this->mCampaignName.GetLength() > 0 && this->mCampaignChecksum > 0 && this->mMapName.GetLength() > 0) {
        outName = this->mMapName;
        theResult = true;
    } else {
        outName = gEmptyString;
    }
    return theResult;
}

// FUNCTION: EXE 0x52fc0c
// Returns whether civilizations may be bought: not in the scenario editor and not in a scenario game
bool EEGameOptions::CanBuyCivilization()
{
    bool theResult = false;
    if (!gUIManager->mInScenarioEditor && !this->mIsScenarioGame)
        theResult = true;
    return theResult;
}

// FUNCTION: EXE 0x52fc27
// Resets the options to the lobby's defaults (all resources, random epochs, first map type, cheats on, victory off,
// the preferred difficulty) with sixteen empty player slots
void EEGameOptions::SetDefaults()
{
    *this = gLobbyGameOptions;
    this->mLoadType = 0;
    this->mResources = EEGetNumberOfResourceLevels() - 1;
    this->mVariant = 0;
    this->mEpochStart = kTechEpochAll;
    this->mEpochEnd = kTechEpochSpace;
    this->mMapType = EERMGetNumberOfGenerators() > 0 ? EERMGetGeneratorName(0) : UWideString(gEmptyString);
    this->mCampaignName = gEmptyString;
    this->mScenarioName = gEmptyString;
    this->mMapName = gEmptyString;
    this->mMapSize = 1;
    this->mUnitLimit = gWorldTable.GetLongValue(27);
    this->mWondersForVictory = 3;
    this->mGameSpeed = 1;
    this->mVictory = false;
    this->mLockTeams = false;
    this->mLockSpeed = false;
    this->mRevealMap = false;
    this->mCheatCodes = true;
    this->mRecordGames = false;
    this->mAllowCustomCivs = false;
    this->mIsMultiplayer = false;
    this->mB407 = true;
    this->mIsScenarioGame = false;
    this->mDifficulty = 0;
    unsigned long theDifficulty = gDifficultyLevel;
    if (theDifficulty < 3)
        this->mDifficulty = theDifficulty;
    for (unsigned long i = 0; i < 16; i++)
        this->mPlayers[i] = sEmptyPlayer;
}

// FUNCTION: EXE 0x52fd6b
// Reads the game options and the sixteen slots from a saved game (old saves lack the map name), falling back to
// the first resource level and variant when out of range; returns the first read error
long EEGameOptions::Load(unsigned long inMajorVersion, unsigned long inMinorVersion, FSFile& inFile)
{
    UWideString theUnused;
    long theResult = 0;
    unsigned long i;
    if (inMajorVersion == 0 && inMinorVersion < 200) {
        for (i = 0; i < 16 && theResult == 0; i++)
            theResult = this->mPlayers[i].Load(inMajorVersion, inMinorVersion, inFile);
        if (theResult == 0)
            theResult = inFile.Read(this->mGameSpeed);
        if (theResult == 0)
            theResult = inFile.Read(this->mLoadType);
        if (theResult == 0)
            theResult = inFile.Read(this->mVariant);
        if (theResult == 0)
            theResult = inFile.Read(this->mMapSize);
        if (theResult == 0)
            theResult = inFile.Read(this->mEpochStart);
        if (theResult == 0)
            theResult = inFile.Read(this->mEpochEnd);
        if (theResult == 0) {
            theResult = inFile.Read(this->mResources);
            if (this->mResources >= EEGetNumberOfResourceLevels())
                this->mResources = 0;
        }
        if (theResult == 0)
            theResult = inFile.Read(this->mUnitLimit);
        if (theResult == 0)
            theResult = inFile.Read(this->mWondersForVictory);
        if (theResult == 0)
            theResult = inFile.Read(this->mDifficulty);
        if (theResult == 0)
            theResult = inFile.Read(this->mVictory);
        if (theResult == 0)
            theResult = inFile.Read(this->mLockTeams);
        if (theResult == 0)
            theResult = inFile.Read(this->mLockSpeed);
        if (theResult == 0)
            theResult = inFile.Read(this->mRevealMap);
        if (theResult == 0)
            theResult = inFile.Read(this->mCheatCodes);
        if (theResult == 0)
            theResult = inFile.Read(this->mRecordGames);
        if (theResult == 0)
            theResult = inFile.Read(this->mAllowCustomCivs);
        if (theResult == 0)
            theResult = inFile.Read(this->mIsMultiplayer);
        if (theResult == 0)
            theResult = this->mCampaignName.Load(inFile);
        if (theResult == 0)
            theResult = this->mScenarioName.Load(inFile);
        if (theResult == 0)
            theResult = inFile.Read(this->mCampaignChecksum);
        if (theResult == 0)
            theResult = inFile.Read(this->mIsScenarioGame);
    } else {
        for (i = 0; i < 16 && theResult == 0; i++)
            theResult = this->mPlayers[i].Load(inMajorVersion, inMinorVersion, inFile);
        if (theResult == 0)
            theResult = inFile.Read(this->mGameSpeed);
        if (theResult == 0)
            theResult = inFile.Read(this->mLoadType);
        if (theResult == 0)
            theResult = inFile.Read(this->mVariant);
        if (theResult == 0)
            theResult = inFile.Read(this->mMapSize);
        if (theResult == 0)
            theResult = inFile.Read(this->mEpochStart);
        if (theResult == 0)
            theResult = inFile.Read(this->mEpochEnd);
        if (theResult == 0) {
            theResult = inFile.Read(this->mResources);
            if (this->mResources >= EEGetNumberOfResourceLevels())
                this->mResources = 0;
        }
        if (theResult == 0)
            theResult = inFile.Read(this->mUnitLimit);
        if (theResult == 0)
            theResult = inFile.Read(this->mWondersForVictory);
        if (theResult == 0)
            theResult = inFile.Read(this->mDifficulty);
        if (theResult == 0)
            theResult = inFile.Read(this->mVictory);
        if (theResult == 0)
            theResult = inFile.Read(this->mLockTeams);
        if (theResult == 0)
            theResult = inFile.Read(this->mLockSpeed);
        if (theResult == 0)
            theResult = inFile.Read(this->mRevealMap);
        if (theResult == 0)
            theResult = inFile.Read(this->mCheatCodes);
        if (theResult == 0)
            theResult = inFile.Read(this->mRecordGames);
        if (theResult == 0)
            theResult = inFile.Read(this->mAllowCustomCivs);
        if (theResult == 0)
            theResult = inFile.Read(this->mIsMultiplayer);
        if (theResult == 0)
            theResult = this->mCampaignName.Load(inFile);
        if (theResult == 0)
            theResult = this->mScenarioName.Load(inFile);
        if (theResult == 0)
            theResult = inFile.Read(this->mCampaignChecksum);
        if (theResult == 0)
            theResult = inFile.Read(this->mIsScenarioGame);
        if (theResult == 0)
            theResult = this->mMapName.Load(inFile);
    }
    if (this->mVariant >= EEGetNumberOfGameVariants() || gGameVariants[this->mVariant] == NULL)
        this->mVariant = 0;
    return theResult;
}

// FUNCTION: EXE 0x530272
// Calls each player slot's empty member, so does nothing (EEMStartLoading calls it under the read lock)
void EEGameOptions::Fn130272()
{
    for (long i = 0; i < 16; i++)
        this->mPlayers[i].FnE123D();
}

// (empty: ICF-folded into 0xe123d with the other empty members)
void EEGameOptionsPlayer::FnE123D()
{
}

// FUNCTION: EXE 0x530289
// Writes the sixteen player slots and the game options to a saved game; returns the first write error
long EEGameOptions::Save(FSFile& outFile)
{
    long theResult = 0;
    for (unsigned long i = 0; i < 16 && theResult == 0; i++)
        theResult = this->mPlayers[i].Save(outFile);
    if (theResult == 0)
        theResult = outFile.Write(this->mGameSpeed);
    if (theResult == 0)
        theResult = outFile.Write(this->mLoadType);
    if (theResult == 0)
        theResult = outFile.Write(this->mVariant);
    if (theResult == 0)
        theResult = outFile.Write(this->mMapSize);
    if (theResult == 0)
        theResult = outFile.Write(this->mEpochStart);
    if (theResult == 0)
        theResult = outFile.Write(this->mEpochEnd);
    if (theResult == 0)
        theResult = outFile.Write(this->mResources);
    if (theResult == 0)
        theResult = outFile.Write(this->mUnitLimit);
    if (theResult == 0)
        theResult = outFile.Write(this->mWondersForVictory);
    if (theResult == 0)
        theResult = outFile.Write(this->mDifficulty);
    if (theResult == 0)
        theResult = outFile.Write(this->mVictory);
    if (theResult == 0)
        theResult = outFile.Write(this->mLockTeams);
    if (theResult == 0)
        theResult = outFile.Write(this->mLockSpeed);
    if (theResult == 0)
        theResult = outFile.Write(this->mRevealMap);
    if (theResult == 0)
        theResult = outFile.Write(this->mCheatCodes);
    if (theResult == 0)
        theResult = outFile.Write(this->mRecordGames);
    if (theResult == 0)
        theResult = outFile.Write(this->mAllowCustomCivs);
    if (theResult == 0)
        theResult = outFile.Write(this->mIsMultiplayer);
    if (theResult == 0)
        theResult = this->mCampaignName.Save(outFile);
    if (theResult == 0)
        theResult = this->mScenarioName.Save(outFile);
    if (theResult == 0)
        theResult = outFile.Write(this->mCampaignChecksum);
    if (theResult == 0)
        theResult = outFile.Write(this->mIsScenarioGame);
    if (theResult == 0)
        theResult = this->mMapName.Save(outFile);
    return theResult;
}

// FUNCTION: EXE 0x5304b4
// Returns the number of game variants
unsigned long EEGetNumberOfGameVariants()
{
    return gGameVariants.size();
}

// FUNCTION: EXE 0x5304cc
// Returns the number of starting resource levels
unsigned long EEGetNumberOfResourceLevels()
{
    return EEStartingResources::sTable.size();
}

// patched: the random-map unit rules (ee/EEUnitRules.h). Not in the original game.
#include <stdio.h>
#include "ee/EEStringIDs.h"
#include "ee/EEUnitRules.h"

EEUnitRules gUnitRules;
EEUnitRules gLastUnitRules;

// whether the database's tech tree disables a tech by default (EETechTreeNode::Load sets mScenEditorObsoleted from
// it); an unknown id is not disabled
static bool EEUnitRulesDefault(long inID)
{
    EETechTreeNode* theNode = gTechTree.Get(inID);
    return theNode != NULL && theNode->mData != NULL && theNode->mData->mObsoletedByDefault;
}

// the dialog's category of a tech (kUnitRulesCategoryCount: not listed): the scenario editor's grouping
// (EESEPlayerScreen::BuildTechTypeLists), with every building family member counted as a building (the editor lists
// docks among the units: their class is not a land building)
static long EEUnitRulesCategory(long inID)
{
    EETechTreeNode* theNode = gTechTree.Get(inID);
    if (theNode == NULL || theNode->mData == NULL || !theNode->mData->mShowInEditor)
        return kUnitRulesCategoryCount;
    long theClassIndex = theNode->mData->mUnitClass;
    if (theClassIndex <= 0 || theClassIndex >= (long)(sizeof(gUnitClassTable) / sizeof(gUnitClassTable[0])))
        return kUnitRulesCategoryCount;
    EEUnitClass* theClass = gUnitClassTable[theClassIndex];
    if (theClass == NULL)
        return kUnitRulesCategoryCount;
    if (theClass->mFamily == kUnitFamilyBuilding || theClass->IsLandBuilding())
        return kUnitRulesBuildings;
    if (theClass->mAIKind == kAIKindHero || theClass->mFamily == kUnitFamilyHero)
        return kUnitRulesHeroes;
    return kUnitRulesUnits;
}

void EEUnitRules::ToString(UWideString& outText) const
{
    char theEntry[24];
    outText = "";
    for (std::map<long, bool>::const_iterator it = this->mChanges.begin(); it != this->mChanges.end(); ++it) {
        sprintf(theEntry, "%s%c%ld", outText.GetLength() > 0 ? "," : "", it->second ? '-' : '+', it->first);
        outText += theEntry;
    }
}

void EEUnitRules::FromString(const char* inText)
{
    this->mChanges.clear();
    const char* thePosition = inText;
    while (thePosition != NULL && *thePosition != '\0') {
        while (*thePosition == ',' || *thePosition == ' ')
            ++thePosition;
        char theSign = *thePosition;
        if (theSign != '-' && theSign != '+')
            break;
        char* theEnd;
        long theID = strtol(thePosition + 1, &theEnd, 10);
        if (theEnd == thePosition + 1)
            break;
        this->mChanges[theID] = theSign == '-';
        thePosition = theEnd;
    }
}

bool EEUnitRules::IsDisabled(long inID) const
{
    std::map<long, bool>::const_iterator it = this->mChanges.find(inID);
    if (it != this->mChanges.end())
        return it->second;
    return EEUnitRulesDefault(inID);
}

// records one tech's state: a change from the default, or no entry when it matches the default
void EEUnitRules::SetOne(long inID, bool inDisabled)
{
    if (inDisabled == EEUnitRulesDefault(inID))
        this->mChanges.erase(inID);
    else
        this->mChanges[inID] = inDisabled;
}

void EEUnitRules::SetDisabled(long inID, bool inDisabled)
{
    EETechTreeNode* theNode = gTechTree.Get(inID);
    if (theNode == NULL || theNode->mData == NULL || this->IsDisabled(inID) == inDisabled)
        return;
    // set before recursing (the original sets it after: the same on the database's acyclic prerequisites, and no
    // endless recursion on a cycle)
    this->SetOne(inID, inDisabled);
    if (inID == kTechEpochPrehistoric)
        return;
    std::vector<long> theIDs;
    gTechTree.GetNodeIDs(theIDs);
    RTSTechTreeNodeData* theData = theNode->mData;
    for (unsigned long i = 0; i < theIDs.size(); i++) {
        long theOtherID = theIDs[i];
        if (theOtherID == inID)
            continue;
        EETechTreeNode* theOther = gTechTree.Get(theOtherID);
        if (theOther == NULL || theOther->mData == NULL)
            continue;
        if (inDisabled) {
            // disabling: whatever needs this tech goes too
            RTSTechTreeNodeData* theOtherData = theOther->mData;
            if (theOtherData->mPrerequisite1 == inID || theOtherData->mPrerequisite2 == inID ||
                theOtherData->mPrerequisite3 == inID || theOtherData->mPrerequisite4 == inID)
                this->SetDisabled(theOtherID, true);
        } else {
            // enabling: whatever this tech needs comes back
            if (theData->mPrerequisite1 == theOtherID || theData->mPrerequisite2 == theOtherID ||
                theData->mPrerequisite3 == theOtherID || theData->mPrerequisite4 == theOtherID)
                this->SetDisabled(theOtherID, false);
        }
    }
}

unsigned long EEUnitRules::GetNumChanged() const
{
    unsigned long theCount = 0;
    for (std::map<long, bool>::const_iterator it = this->mChanges.begin(); it != this->mChanges.end(); ++it) {
        if (EEUnitRulesCategory(it->first) != kUnitRulesCategoryCount)
            theCount++;
    }
    return theCount;
}

void EEUnitRules::Apply(EETechTree& ioTree) const
{
    for (std::map<long, bool>::const_iterator it = this->mChanges.begin(); it != this->mChanges.end(); ++it) {
        EETechTreeNode* theNode = ioTree.Get(it->first);
        if (theNode != NULL)
            theNode->SetScenEditorObsoleted(it->second, false);
    }
}

void EEUnitRules::GetTechs(long inCategory, std::vector<long>& outIDs)
{
    std::vector<long> theIDs;
    gTechTree.GetNodeIDs(theIDs);
    outIDs.clear();
    for (unsigned long i = 0; i < theIDs.size(); i++) {
        if (EEUnitRulesCategory(theIDs[i]) == inCategory)
            outIDs.push_back(theIDs[i]);
    }
}

void EEUnitRules::GetSummary(UWideString& outText) const
{
    if (this->IsDefault()) {
        EEUnitRules::GetText(IDS_UNIT_RULES_DEFAULT, "Game defaults", outText);
    } else {
        UWideString theFormat;
        EEUnitRules::GetText(IDS_UNIT_RULES_CUSTOM, "Custom (%d changed)", theFormat);
        sprintf(outText, theFormat, (int)this->GetNumChanged());
    }
}

void EEUnitRules::GetText(long inStringID, const char* inFallback, UWideString& outText)
{
    outText = "";
    if (gResource == NULL || gResource->GetString(inStringID, outText) != kErrorOK || outText.GetLength() == 0)
        outText = inFallback;
}
