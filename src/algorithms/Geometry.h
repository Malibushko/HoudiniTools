#pragma once
#include <optional>
#include <vector>
#include <GA/GA_Types.h>
#include <UT/UT_String.h>
#include <UT/UT_VectorTypes.h>

#include "IndexedPoint.h"
#include "Segment.h"

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
    bool in_range(float value, float min, float max);
    bool in_range01(float value);

    EOrientation orientation(float x1, float y1, float x2, float y2, float x3, float y3);
    UT_Vector2 map(UT_Vector3 point, EPlane plane);
    utils::Segment2D map(utils::Segment3D segment, EPlane plane);

    std::vector<utils::IndexedPoint3D> getGeometryPoints(const GU_Detail* detail, const UT_StringRef& group);
    std::vector<utils::Segment3D> getGeometrySegments(const GU_Detail* detail, const UT_StringRef& group);

    std::optional<UT_Vector2> computeIntersection(const utils::Segment2D & left, const utils::Segment2D & right);
    std::vector<UT_Vector2> computeIntersectionsSimple(const std::vector<utils::Segment2D> & segments);
    std::vector<UT_Vector2> computeIntersectionsQuick(const std::vector<utils::Segment2D> & segments);

    std::vector<GA_Offset> convexHull2D(std::vector<utils::IndexedPoint2D> points);
    std::vector<GA_Offset> convexHull2D(const std::vector<utils::IndexedPoint3D>& points, EPlane plane);
}
