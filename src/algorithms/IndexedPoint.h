#pragma once
#include <GA/GA_Types.h>
#include <UT/UT_Vector3.h>
#include <UT/UT_VectorTypes.h>

#include "IndexedValue.h"

namespace houdini::tools::utils {
    template<class T>
    using IndexedPoint = Indexed<T, GA_Offset>;

    using IndexedPoint2D = IndexedPoint<UT_Vector2>;
    using IndexedPoint3D = IndexedPoint<UT_Vector3>;

    template <typename PointType>
    inline bool operator<(const IndexedPoint<PointType> & left, const IndexedPoint<PointType>& right) {
        return left.point < right.point;
    }

    template <typename PointType>
    inline bool operator==(const IndexedPoint<PointType>& left, const IndexedPoint<PointType>& right) {
        return left.index == right.index;
    }
}
