#include "SOP_RandomLines.h"

#include <random>
#include <GU/GU_Detail.h>
#include <GU/GU_Detail.h>
#include <GEO/GEO_PrimPoly.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <OP/OP_AutoLockInputs.h>
#include <PRM/PRM_Include.h>
#include <Ga/GA_AttributeFilter.h>
#include "algorithms/Geometry.h"
#include <CH/CH_Manager.h>

#include <RU/RU_PixelFunctions.h>
using namespace houdini::tools::interface;

static PRM_Name maxAmountToggleParamterName("force_count", "Force Total Count");
static PRM_Default maxAmountDefault(100);

static PRM_Name maxAmountParameterName("maxAmount", "Force total amount");
static PRM_Name seedParameterName("seed", "Random seed");

PRM_Template SOP_RandomLines::myTemplateList[] = {
    PRM_Template(PRM_STRING, 1, &PRMgroupName, nullptr,
        &SOP_Node::pointGroupMenu,nullptr, 0,
        SOP_Node::getGroupSelectButton(GA_GROUP_POINT)),
    PRM_Template(PRM_TOGGLE, PRM_TYPE_TOGGLE_JOIN, 1, &maxAmountToggleParamterName, &maxAmountDefault),
    PRM_Template(PRM_INT, 1, &maxAmountParameterName, nullptr),
    PRM_Template(PRM_INT_E, 1, &seedParameterName, nullptr),
    PRM_Template()
};

OP_Node * SOP_RandomLines::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_RandomLines(net, name, op);
}

SOP_RandomLines::SOP_RandomLines(OP_Network *net, const char *name, OP_Operator *op)
    : SOP_Node(net, name, op)
{

}

SOP_RandomLines::~SOP_RandomLines() = default;


const char * SOP_RandomLines::inputLabel(unsigned in) const
{
    return "Generate a set of random lines from a set of points";
}

OP_ERROR SOP_RandomLines::cookMySop(OP_Context &context)
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

    std::vector<utils::IndexedPoint3D> startPoints = algorithms::geometry::getGeometryPoints(geo, groupName);
    std::vector<utils::IndexedPoint3D> endPoints = startPoints;

    const fpreal t = context.getTime();
    const int randomSeed = evalInt(seedParameterName, 0, t);
    const size_t maxTotalAmount = std::min(getMaxAmount(t), startPoints.size());

    std::shuffle(startPoints.begin(), startPoints.end(), std::mt19937(randomSeed));
    assert(startPoints.size() == endPoints.size());

    GU_Detail* outGdp = myGdpHandle.gdpNC();
    outGdp->clearAndDestroy();

    for (int index = 0; index < maxTotalAmount; ++index) {
        if (startPoints[index] == endPoints[index]) {
            continue;
        }

        const auto startPtOff = outGdp->appendPoint();
        const auto endPtOff = outGdp->appendPoint();

        outGdp->setPos3(startPtOff, geo->getPos3(startPoints[index].index));
        outGdp->setPos3(endPtOff, geo->getPos3(endPoints[index].index));

        GEO_PrimPoly* poly = reinterpret_cast<GEO_PrimPoly*>(outGdp->appendPrimitive(GA_PRIMPOLY));

        poly->appendVertex(startPtOff);
        poly->appendVertex(endPtOff);
    }

    return error();
}

bool SOP_RandomLines::updateParmsFlags() {
    bool changed = SOP_Node::updateParmsFlags();

    fpreal now = CHgetEvalTime();

    bool is_enabled = (evalInt(maxAmountToggleParamterName, 0, now) != 0);
    changed |= enableParm(maxAmountParameterName.getToken(), is_enabled);
    return changed;
}

size_t SOP_RandomLines::getMaxAmount(fpreal t) const {
    if (evalInt(maxAmountToggleParamterName, 0, t) == 0) {
        return -1;
    }

    return evalInt(maxAmountParameterName, 0, t);
}
