#include "SOP_Convex2D.h"

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

namespace {
    const size_t MIN_HULL_POINTS = 3;
}

static PRM_Name planeChoices[] = {
    PRM_Name("xy", "XY"),
    PRM_Name("xz", "XZ"),
    PRM_Name("zy", "ZY"),
    PRM_Name()
};
static PRM_ChoiceList planeChoiceList(PRM_CHOICELIST_SINGLE, &planeChoices[0]);
static PRM_Default planeDefault(static_cast<int>(houdini::tools::algorithms::geometry::EPlane::XZ));

static PRM_Name planeParameterName("plane", "Projection Plane");
static PRM_Name preserveGroupsParameterName("preserveGroups", "Preserve Groups");
static PRM_Name preserveAttributesParameterName("preserveAttributes", "Preserve Attributes");

PRM_Template SOP_Convex2D::myTemplateList[] = {
    PRM_Template(PRM_STRING, 1, &PRMgroupName, nullptr,
        &SOP_Node::pointGroupMenu,nullptr, 0,
        SOP_Node::getGroupSelectButton(GA_GROUP_POINT)),
    PRM_Template(PRM_ORD, 1, &planeParameterName, &planeDefault, &planeChoiceList),
    PRM_Template(PRM_SEPARATOR),
    PRM_Template(PRM_TOGGLE,  1, &preserveGroupsParameterName, PRMzeroDefaults),
    PRM_Template(PRM_TOGGLE,  1, &preserveAttributesParameterName, PRMzeroDefaults),
    PRM_Template()
};

OP_Node * SOP_Convex2D::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_Convex2D(net, name, op);
}

SOP_Convex2D::SOP_Convex2D(OP_Network *net, const char *name, OP_Operator *op)
    : SOP_Node(net, name, op)
{

}

SOP_Convex2D::~SOP_Convex2D() = default;


const char * SOP_Convex2D::inputLabel(unsigned in) const
{
    return "Geometry to make 2D convex hull from a set";
}

OP_ERROR SOP_Convex2D::cookMySop(OP_Context &context)
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

    const auto plane = static_cast<algorithms::geometry::EPlane>(evalInt(planeParameterName, 0, t));
    const auto convexHullPoints = algorithms::geometry::convexHull2D(algorithms::geometry::getGeometryPoints(geo, groupName), plane);

    if (convexHullPoints.size() < MIN_HULL_POINTS) {
        return error();
    }

    GU_Detail* outGdp = myGdpHandle.gdpNC();
    outGdp->clearAndDestroy();

    std::vector<GA_Offset> newPoints;
    newPoints.reserve(convexHullPoints.size());

    const bool preserveGroups = evalInt(preserveGroupsParameterName, 0, t);
    const bool preserveAttributes = evalInt(preserveAttributesParameterName, 0, t);

    for (const auto & srcPtOff : convexHullPoints) {
        const auto dstPtOff = outGdp->appendPoint();
        outGdp->setPos3(dstPtOff, geo->getPos3(srcPtOff));

        bool groupsPreserved = false;
        if (preserveAttributes) {
            GA_AttributeFilter filter;
            if (!preserveGroups) {
                filter = GA_AttributeFilter::selectAnd(filter, GA_AttributeFilter::selectNot(GA_AttributeFilter::selectGroup()));
            }

            groupsPreserved = true;
            outGdp->copyAttributes(GA_ATTRIB_POINT, dstPtOff, *geo, srcPtOff, &filter);
        }

        if (preserveGroups && !groupsPreserved) {
            const auto filter = GA_AttributeFilter::selectGroup();
            outGdp->copyAttributes(GA_ATTRIB_POINT, dstPtOff, *geo, srcPtOff, &filter);
        }

        newPoints.push_back(dstPtOff);
    }

    GEO_PrimPoly* poly = reinterpret_cast<GEO_PrimPoly*>(outGdp->appendPrimitive(GA_PRIMPOLY));
    poly->setClosed(true);

    for (auto ptOff : newPoints) {
        poly->appendVertex(ptOff);
    }

    return error(context);
}