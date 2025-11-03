#pragma  once
#include <UT/UT_Vector2.h>
#include <UT/UT_Vector3.h>
#include <UT/UT_VectorTypes.h>

namespace houdini::tools::utils {
    template <typename PointType>
    struct Segment {
        PointType startPoint;
        PointType endPoint;
    };

    using Segment2D = Segment<UT_Vector2>;
    using Segment3D = Segment<UT_Vector3>;

    template <typename PointType>
    bool operator<(const Segment<PointType>& left, const Segment<PointType>& right) {
        return left.startPoint < right.startPoint;
    }

    template <typename PointType>
    bool operator==(const Segment<PointType>& left, const Segment<PointType>& right) {
        return left.startPoint == right.startPoint;
    }
}