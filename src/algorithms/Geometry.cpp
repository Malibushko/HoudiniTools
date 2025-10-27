#include "Geometry.h"

#include <GU/GU_Detail.h>

using namespace houdini::tools::algorithms::geometry;

namespace {
    constexpr size_t MIN_CONVEX_POINTS = 3;
}

float houdini::tools::algorithms::geometry::polarAngle(float x1, float y1, float x2, float y2) {
    return atan2(y2 - y1, x2 - x1);
}

EOrientation houdini::tools::algorithms::geometry::orientation(float x1, float y1, float x2, float y2, float x3, float y3) {
    // 2d cross product
    const float result =  (x2 - x1) * (y3 - y1) - (y2 - y1) * (x3 - x1);
    if (result < 0) {
        return EOrientation::Clockwise;
    }
    if (result > 0) {
        return EOrientation::Counterclockwise;
    }
    return EOrientation::Colinear;
}

UT_Vector2 houdini::tools::algorithms::geometry::map(UT_Vector3 point, EPlane plane) {
    switch (plane) {
        case EPlane::XY:
            return {point.x(), point.y()};
        case EPlane::XZ:
            return {point.x(), point.z()};
        case EPlane::YZ:
            return {point.y(), point.z()};
        default:
            assert(false);
            return {};
    }
}

std::vector<houdini::tools::utils::IndexedPoint3D> houdini::tools::algorithms::geometry::getGeometryPoints(const GU_Detail *detail, const UT_StringRef& group) {
    std::vector<utils::IndexedPoint3D> points;

    if (!detail) {
        return points;
    }

    if (group) {
        if (const auto pointGroup = detail->findPointGroup(group)) {
            points.reserve(pointGroup->entries());

            detail->forEachPoint([&](const GA_Offset offset) {
                if (pointGroup->contains(offset)) {
                    points.push_back({offset, detail->getPos3(offset)});
                }
            });
        }
    } else {
        detail->forEachPoint([&](const GA_Offset offset) {
            points.push_back({offset, detail->getPos3(offset)});
        });
    }

    return points;
}

std::vector<GA_Offset> houdini::tools::algorithms::geometry::convexHull2D(const std::vector<utils::IndexedPoint3D>& points, EPlane plane) {
    std::vector<utils::IndexedPoint2D> mappedPoints;
    mappedPoints.reserve(points.size());

    for (const auto& [index, point3D] : points) {
        mappedPoints.push_back({index, map(point3D, plane)});
    }

    return convexHull2D(std::move(mappedPoints));
}

std::vector<GA_Offset> houdini::tools::algorithms::geometry::convexHull2D(std::vector<utils::IndexedPoint2D> points) {
    if (points.size() < MIN_CONVEX_POINTS) {
        return {};
    }

    const utils::IndexedPoint minPoint = *std::min_element(points.begin(), points.end(), [](const auto& left, const auto& right) {
        return std::tuple(left.point.y(), left.point.x()) < std::tuple(right.point.y(), right.point.x());
    });

    std::sort(points.begin(), points.end(), [&](const utils::IndexedPoint2D & p1, const utils::IndexedPoint2D & p2) {
        const auto leftAngle = polarAngle(minPoint.point.x(), minPoint.point.y(), p1.point.x(), p1.point.y());
        const auto rightAngle = polarAngle(minPoint.point.x(), minPoint.point.y(), p2.point.x(), p2.point.y());

        if (leftAngle == rightAngle) {
            return distance2(minPoint.point, p1.point) > distance2(minPoint.point, p2.point);
        }

        return leftAngle < rightAngle;
    });

    std::vector<utils::IndexedPoint2D> hullPoints;
    hullPoints.reserve(points.size());

    for (const auto& p : points) {
        while (hullPoints.size() >= MIN_CONVEX_POINTS - 1) {
            const auto prevPoint = hullPoints[hullPoints.size() - 1];
            const auto prevPrevPoint = hullPoints[hullPoints.size() - 2];
            const auto currentPoint = p;

            const EOrientation direction = orientation(
                    prevPrevPoint.point.x(),
                    prevPrevPoint.point.y(),
                    prevPoint.point.x(),
                    prevPoint.point.y(),
                    currentPoint.point.x(),
                    currentPoint.point.y()
                );

            if (direction == EOrientation::Clockwise) {
                hullPoints.pop_back();
                continue;
            }

            break;
        }

        hullPoints.push_back({p.index, p.point});
    }

    std::vector<GA_Offset> hullOffsets;
    for (const auto& p : hullPoints)
        hullOffsets.push_back(p.index);

    return hullOffsets;
}
