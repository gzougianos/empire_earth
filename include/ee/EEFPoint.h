// patched: the point formation (formation 9, kFormationPoint; not in the original game). The units pack into a tight
// round clump centred on the formation's centre: each takes one of the points of a triangular lattice nearest the
// centre, half the largest unit's collision size apart (the line formations stand a full size apart). The units of
// the clump overlap, which works because units of a player pass through each other (EEPassesThrough,
// ee/EEPhysicsModel.h). Defined in EEFormation.cpp (a unit of its own would need an address range in
// link/exe/units.txt). Requires ee/EEFormation.h.
#ifndef EE_EEFPOINT_H
#define EE_EEFPOINT_H

class EEFPoint : public EEFormation {
public:
    EEFPoint() : EEFormation(kFormationPoint) {}
    virtual ~EEFPoint() {}
    // one slot per unit in the clump, all facing the formation heading
    virtual void Place(std::vector<EEComplexUnit*>& inUnits, std::vector<EEFormationSlot>& outSlots);
};

#endif
