#pragma once
#include <SOP/SOP_Node.h>

namespace houdini::tools::interface {
    class SOP_SegmentsIntersection : public SOP_Node
    {
    public:
        static PRM_Template myTemplateList[];
        static OP_Node      *myConstructor(OP_Network *net, const char *name, OP_Operator *op);

        virtual const char *inputLabel(unsigned in) const;

    protected:
        SOP_SegmentsIntersection(OP_Network *net, const char *name, OP_Operator *op);
        ~SOP_SegmentsIntersection() override;

        OP_ERROR cookMySop(OP_Context &context) override;

    protected:
        const GA_PointGroup *myGroup = nullptr;
    };
}
