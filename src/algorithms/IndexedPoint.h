#pragma once
#include <GA/GA_Types.h>
#include <UT/UT_Vector3.h>
#include <UT/UT_VectorTypes.h>

namespace houdini::tools::utils {
    template <typename PointType>
    struct IndexedPoint {
        GA_Offset index{};
        PointType point{};
    };

    template <typename PointType>
    inline bool operator<(const IndexedPoint<PointType>& left, const IndexedPoint<PointType>& right) {
        return left.point < right.point;
    }

    using IndexedPoint2D = IndexedPoint<UT_Vector2>;
    using IndexedPoint3D = IndexedPoint<UT_Vector3>;
}