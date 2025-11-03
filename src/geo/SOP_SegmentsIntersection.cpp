#include "SOP_SegmentsIntersection.h"

#include <map>
#include <map>
#include <GU/GU_Detail.h>
#include <GU/GU_Detail.h>
#include <GEO/GEO_PrimPoly.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <OP/OP_AutoLockInputs.h>
#include <PRM/PRM_Include.h>
#include <Ga/GA_AttributeFilter.h>
#include <UT/UT_Interrupt.h>
#include "algorithms/Geometry.h"

using namespace houdini::tools::interface;

enum class AlgorithmChoice {
    BruteForce,
    BentleyOttmann
};

static PRM_Name algoChoices[] = {
    PRM_Name("simple", "Brute-Force"),
    PRM_Name("bentleyOttmann", "Bentley-Ottman Algorithm"),
    PRM_Name()
};
static PRM_ChoiceList algoChoiceList(PRM_CHOICELIST_SINGLE, &algoChoices[0]);
static PRM_Default algoDefault(0);

static PRM_Name algoParameterName("algo", "Method");

PRM_Template SOP_SegmentsIntersection::myTemplateList[] = {
    PRM_Template(PRM_STRING, 1, &PRMgroupName, nullptr,
        &SOP_Node::pointGroupMenu,nullptr, 0,
        SOP_Node::getGroupSelectButton(GA_GROUP_POINT)),
    PRM_Template(PRM_ORD, 1, &algoParameterName, &algoDefault, &algoChoiceList),
    PRM_Template(PRM_SEPARATOR),
    PRM_Template()
};

OP_Node * SOP_SegmentsIntersection::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_SegmentsIntersection(net, name, op);
}

SOP_SegmentsIntersection::SOP_SegmentsIntersection(OP_Network *net, const char *name, OP_Operator *op)
    : SOP_Node(net, name, op)
{

}

SOP_SegmentsIntersection::~SOP_SegmentsIntersection() = default;


const char * SOP_SegmentsIntersection::inputLabel(unsigned in) const
{
    return "Find intersection points from a set of segments on a plane";
}

OP_ERROR SOP_SegmentsIntersection::cookMySop(OP_Context &context)
{
    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT)
        return error();

    const GU_Detail* geo = inputGeo(0);
    if (geo == nullptr) {
        return error();
    }

    cookInputPointGroups(context, myGroup);

    UT_StringRef groupName;
    if (myGroup) {
        groupName = myGroup->getName();
    }

    const fpreal t = context.getTime();

    GU_Detail* outGdp = myGdpHandle.gdpNC();
    outGdp->clearAndDestroy();

    std::vector<utils::Segment2D> geoSegments;
    for (const auto & segment3D :  algorithms::geometry::getGeometrySegments(geo, groupName)) {
        geoSegments.push_back(map(segment3D, algorithms::geometry::EPlane::XZ));
    }

    std::vector<UT_Vector2> intersectionPoints;
    switch (static_cast<AlgorithmChoice>(evalInt(algoParameterName, 0, t))) {
        case AlgorithmChoice::BruteForce: {
            intersectionPoints = algorithms::geometry::computeIntersectionsSimple(geoSegments);
            break;
        }
        case AlgorithmChoice::BentleyOttmann: {
            intersectionPoints = algorithms::geometry::computeIntersectionsQuick(geoSegments);
            break;
        }
        default: {
            addError(SOP_MESSAGE, "Invalid algorithm choice");
            return error();
        }
    };

    for (const UT_Vector2 & srcPos2: intersectionPoints) {
        const GA_Offset dstPtOff = outGdp->appendPoint();
        outGdp->setPos3(dstPtOff, UT_Vector3(srcPos2.x(), 0, srcPos2.y()));
    }

    return error(context);
}