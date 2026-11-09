// C:\EEX\Empire Earth\Formations\EEFormation.cpp (file and folder names inferred: no __FILE__). The formation base: 0xddcc6..0xdde43 (Create, Arrange).
// Unit flags: the exe's module set.
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

#include "ee/EEFormation.h"
#include "ee/EEFCircle.h"
#include "ee/EEFInwardCircle.h"
#include "ee/EEFOutwardCircle.h"
#include "ee/EEFLine.h"
#include "ee/EEFLongLine.h"
#include "ee/EEFLooseLine.h"
#include "ee/EEFShortLine.h"
#include "ee/EEFStaggeredLine.h"
#include "ee/EEFWedge.h"
#include "ee/EEFDeepWedge.h"
#include "ee/EEFShallowWedge.h"
#include "ee/EEUnit.h"                      // patched: the point formation's unit radii
#include "ee/EEFPoint.h"                    // patched: the point formation

// FUNCTION: EXE 0x4ddcc6
// Creates the formation of a type: 0-3 short, long, loose, staggered line; 4-5 deep, shallow wedge;
// 6-7 inward, outward circle; NULL for any other type (patched: 9 the point formation)
EEFormation* EEFormation::Create(long inType)
{
    EEFormation* theFormation = NULL;
    switch (inType) {
    case 0:
        theFormation = new EEFShortLine;
        break;
    case 1:
        theFormation = new EEFLongLine;
        break;
    case 2:
        theFormation = new EEFLooseLine;
        break;
    case 3:
        theFormation = new EEFStaggeredLine;
        break;
    case 4:
        theFormation = new EEFDeepWedge;
        break;
    case 5:
        theFormation = new EEFShallowWedge;
        break;
    case 6:
        theFormation = new EEFInwardCircle;
        break;
    case 7:
        theFormation = new EEFOutwardCircle;
        break;
    case kFormationPoint:
        theFormation = new EEFPoint;    // patched
        break;
    }
    return theFormation;
}

// FUNCTION: EXE 0x4dddd5
// Collects the group's units and places them into formation slots
void EEFormation::Arrange(EEGroup* inGroup, std::vector<EEFormationSlot>& outSlots)
{
    std::vector<EEComplexUnit*> theUnits;
    theUnits.resize(inGroup->GetNumUnits());
    for (long i = inGroup->GetNumUnits() - 1; i >= 0; i--)
        theUnits[i] = inGroup->GetUnit(i);
    this->Place(theUnits, outSlots);
}

// ---- patched: the point formation (ee/EEFPoint.h; not in the original game) ----

static const float kPointSpacing = 0.5f;           // the clump's spacing, in the largest unit's collision size
static const float kPointRowHeight = 0.8660254f;   // sqrt(3) / 2: a lattice row's height, in spacings
static const long kPointTurnOrder[6] = { 0, 2, 4, 1, 3, 5 };   // a sixth of a turn's place: opposite sides alternate

// A point of the clump's triangular lattice. Point (a, b) lies a + b / 2 spacings to the side of the centre and
// b sqrt(3) / 2 ahead of it, a^2 + ab + b^2 spacings squared away (an integer, so the order below is exact and the
// same on every machine). The points of a ring of equal distance form orbits of six, a sixth of a turn apart; the
// clump takes the points ring by ring, orbit by orbit, and an orbit's points in pairs of opposite ones, so a ring
// that the units do not fill stays balanced around the centre.
struct EEFPointSpot {
    long mNorm;                             // the distance squared, in spacings squared
    long mOrbit;                            // the b of the orbit's point in the first sixth (a > 0, b >= 0)
    long mTurn;                             // the point's place in its orbit (kPointTurnOrder)
    long mA;
    long mB;
};

static bool EEFPointSpotLess(const EEFPointSpot& inLeft, const EEFPointSpot& inRight)
{
    if (inLeft.mNorm != inRight.mNorm)
        return inLeft.mNorm < inRight.mNorm;
    if (inLeft.mOrbit != inRight.mOrbit)
        return inLeft.mOrbit < inRight.mOrbit;
    return inLeft.mTurn < inRight.mTurn;
}

// Lays the units' slots on the lattice points nearest the centre, kPointSpacing times the largest collision sphere
// apart (the line formations stand one apart), turned with the formation and all facing its heading
void EEFPoint::Place(std::vector<EEComplexUnit*>& inUnits, std::vector<EEFormationSlot>& outSlots)
{
    std::vector<EEFPointSpot> theSpots;
    EEFPointSpot theSpot;
    EEFormationSlot theSlot;
    GEVector theOffset;
    GEVector theTransformedOffset;
    GE3DPoint theCenter(this->mCenter);
    float theRadius = 0.0f;
    long theCount = inUnits.size();
    long i;
    outSlots.clear();
    if (theCount == 0)
        return;
    for (i = 0; i < theCount; i++) {
        EEUnitClass* theClass = inUnits[i]->mUnitClass;
        theRadius = max(theRadius, max(theClass->mOuterCollisionSphere, theClass->GetInnerCollisionSphere()));
    }
    float theSpacing = theRadius * kPointSpacing;
    // every lattice point up to theSide steps out, more than the theCount nearest need (a point k steps out along
    // a or b is at least 3 k^2 / 4 spacings squared away)
    long theSide = 1;
    while (theSide * theSide < theCount)
        theSide++;
    theSide++;
    theSpots.reserve((2 * theSide + 1) * (2 * theSide + 1));
    for (long a = -theSide; a <= theSide; a++) {
        for (long b = -theSide; b <= theSide; b++) {
            theSpot.mA = a;
            theSpot.mB = b;
            theSpot.mNorm = a * a + a * b + b * b;
            // turn the point back a sixth of a turn at a time into the first sixth: (a, b) -> (a + b, -a)
            long theX = a;
            long theY = b;
            long theTurns = 0;
            while (theSpot.mNorm != 0 && !(theX > 0 && theY >= 0)) {
                long theNext = theX + theY;
                theY = -theX;
                theX = theNext;
                theTurns++;
            }
            theSpot.mOrbit = theY;
            theSpot.mTurn = kPointTurnOrder[theTurns];
            theSpots.push_back(theSpot);
        }
    }
    std::sort(theSpots.begin(), theSpots.end(), EEFPointSpotLess);
    for (i = 0; i < theCount; i++) {
        theOffset.SetX(((float)theSpots[i].mA + (float)theSpots[i].mB * 0.5f) * theSpacing);
        theOffset.SetY((float)theSpots[i].mB * kPointRowHeight * theSpacing);
        theOffset.SetZ(0.0f);
        this->mOrientation.Apply(theOffset, theTransformedOffset);
        theSlot.mPosition = GE3DPoint(theCenter.GetX() + theTransformedOffset.GetX(),
                                      theCenter.GetY() + theTransformedOffset.GetY(),
                                      theCenter.GetZ() + theTransformedOffset.GetZ());
        theSlot.mUnit = inUnits[i];
        theSlot.mHeading = this->mHeading;
        outSlots.push_back(theSlot);
    }
}
