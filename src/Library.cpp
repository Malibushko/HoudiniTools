#include <OP/OP_OperatorTable.h>

#include "geo/SOP_Convex2D.h"
#include <UT/UT_DSOVersion.h>
#include <PRM/PRM_Include.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>

#include "geo/SOP_RandomLines.h"

using namespace houdini::tools::interface;

void newSopOperator(OP_OperatorTable *table)
{
    {
        auto* convex2D = new OP_Operator("convex2d", "Convex2D", SOP_Convex2D::myConstructor, SOP_Convex2D::myTemplateList, 1, 1, nullptr);
        convex2D->setIconName("hicon:/SVGIcons.index?VIEW_hiddenline.svg");
        table->addOperator(convex2D);
    }
    {
        auto* randomLines = new OP_Operator("random_lines", "Random Lines", SOP_RandomLines::myConstructor, SOP_RandomLines::myTemplateList, 1, 1, nullptr);
        randomLines->setIconName("hicon:/SVGIcons.index?CHANNELS_linear.svg");
        table->addOperator(randomLines);
    }
}

