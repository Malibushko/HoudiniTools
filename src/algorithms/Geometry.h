#pragma once
#include <vector>
#include <GA/GA_Types.h>
#include <UT/UT_String.h>
#include <UT/UT_VectorTypes.h>

#include "IndexedPoint.h"

class GU_Detail;

namespace houdini::tools::algorithms::geometry {
    enum class EOrientation {
        Colinear = 0,
        Clockwise = -1,
        Counterclockwise = 1,
    };

    enum class EPlane {
        XY,
        XZ,
        YZ
    };

    float polarAngle(float x1, float y1, float x2, float y2);
    EOrientation orientation(float x1, float y1, float x2, float y2, float x3, float y3);
    UT_Vector2 map(UT_Vector3 point, EPlane plane);

    std::vector<utils::IndexedPoint3D> getGeometryPoints(const GU_Detail* detail, const UT_StringRef& group);

    std::vector<GA_Offset> convexHull2D(std::vector<utils::IndexedPoint2D> points);
    std::vector<GA_Offset> convexHull2D(const std::vector<utils::IndexedPoint3D>& points, EPlane plane);
}
