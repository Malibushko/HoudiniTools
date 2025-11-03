#include "Geometry.h"

#include <deque>
#include <filesystem>
#include <flicks.h>
#include <queue>
#include <set>
#include <GU/GU_Detail.h>

#include "AVLTree.h"

struct EventComp;
struct Event;
using namespace houdini::tools::algorithms::geometry;
using namespace houdini::tools;

namespace {
    constexpr size_t MIN_CONVEX_POINTS = 3;
}

float houdini::tools::algorithms::geometry::polarAngle(float x1, float y1, float x2, float y2) {
    return atan2(y2 - y1, x2 - x1);
}

bool houdini::tools::algorithms::geometry::in_range(float value, float min, float max) {
    return value >= min && value <= max;
}

bool houdini::tools::algorithms::geometry::in_range01(float value) {
    return in_range(value, 0.0f, 1.0f);
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

houdini::tools::utils::Segment2D houdini::tools::algorithms::geometry::map(utils::Segment3D segment, EPlane plane) {
    return utils::Segment2D{map(segment.startPoint, plane), map(segment.endPoint, plane)};
}

std::vector<houdini::tools::utils::IndexedPoint3D> houdini::tools::algorithms::geometry::getGeometryPoints(const GU_Detail *detail, const UT_StringRef& group) {
    std::vector<utils::IndexedPoint3D> points;

    if (!detail) {
        return points;
    }

    const auto pointGroup = detail->findPointGroup(group);

    detail->forEachPoint([&](const GA_Offset offset) {
        if (pointGroup != nullptr && !pointGroup->contains(offset)) {
            return;
        }

        points.emplace_back(detail->getPos3(offset), offset);
    });

    return points;
}

std::vector<houdini::tools::utils::Segment3D> houdini::tools::algorithms::geometry::getGeometrySegments(const GU_Detail *detail, const UT_StringRef &group) {
    std::vector<utils::Segment3D> segments;

    if (!detail) {
        return segments;
    }

    const auto primGroup = detail->findPrimitiveGroup(group);

    detail->forEachPrimitive([&](const GA_Offset offset) {
        if (primGroup != nullptr && !primGroup->contains(offset))
            return;

        const GA_Primitive *primitive = detail->getPrimitive(offset);
        if (!primitive || primitive->getTypeId() != GA_PRIMPOLY)
            return;

        const GA_Size vertexCount = primitive->getVertexCount();

        if (vertexCount < 2) {
            return;
        }

        const GA_Offset startPtOffset = primitive->getPointOffset(0);
        const GA_Offset endPtOffset = primitive->getPointOffset(vertexCount - 1);

        const UT_Vector3D startPos = detail->getPos3(startPtOffset);
        const UT_Vector3D endPos = detail->getPos3(endPtOffset);

        segments.emplace_back(utils::Segment3D{startPos, endPos});

    });

    return segments;
}

std::optional<UT_Vector2> houdini::tools::algorithms::geometry::computeIntersection(const utils::Segment2D &left, const utils::Segment2D &right) {
    const UT_Vector2 &a = left.startPoint;
    const UT_Vector2 &b = left.endPoint;
    const UT_Vector2 &c = right.startPoint;
    const UT_Vector2 &d = right.endPoint;

    const UT_Vector2 ab = b - a;
    const UT_Vector2 cd = d - c;
    const UT_Vector2 ac = c - a;

    // Check if angle is zero by cross product 2d
    if (cross(ab, cd) == 0) {
        return {};
    }

    // derived formula for intersection
    const float ab_cross_cd = cross(ab, cd);
    const float t1 = cross(ac, cd) / ab_cross_cd;
    const float t2 = -cross(ab, ac) / ab_cross_cd;

    // lies outside segments
    if (!in_range01(t1) || !in_range01(t2)) {
        return {};
    }

    return a + (b - a) * t1;
}

std::vector<UT_Vector2> houdini::tools::algorithms::geometry::computeIntersectionsSimple(const std::vector<utils::Segment2D> &segments) {
    std::vector<UT_Vector2> intersections;

    std::set<std::pair<size_t, size_t>> seenIntersections;

    for (size_t i = 0; i < segments.size(); ++i) {
        for (size_t j = 0; j < segments.size(); ++j) {
            if (i == j) {
                continue;
            }

            std::pair<size_t, size_t> intersection = std::minmax(i, j);
            if (seenIntersections.count(intersection)) {
                continue;
            }

            seenIntersections.insert(intersection);

            if (const auto intersectionPoint = computeIntersection(segments[i], segments[j])) {
                intersections.push_back(intersectionPoint.value());
            }
        }
    }

    return intersections;
}

std::vector<UT_Vector2> houdini::tools::algorithms::geometry::computeIntersectionsQuick(const std::vector<utils::Segment2D>& segments) {
    using IndexedSegment = utils::Indexed<utils::Segment2D, size_t>;

    std::vector<UT_Vector2> intersections;
    // Bentley-Ottmann algorithm https://en.wikipedia.org/wiki/Bentley%E2%80%93Ottmann_algorithm

    enum class EventType { IntersectionPoint = 0, EndPoint = 1, StartPoint = 2 };  // Reordered for correct priority: Start > End > Intersection

    struct SweepEvent {
        EventType type;
        UT_Vector2 point;
        size_t segA;
        size_t segB;

        bool operator<(const SweepEvent &other) const {
            return std::make_tuple(point.y(), point.x(), type) < std::make_tuple(other.point.y(), other.point.x(), other.type);
        }
    };

    std::priority_queue<SweepEvent> eventQueue;

    for (size_t i = 0; i < segments.size(); ++i) {
        const auto &s = segments[i];
        const bool isStart = s.startPoint.y() > s.endPoint.y();
        const UT_Vector2 &pStart = isStart ? s.startPoint : s.endPoint;
        const UT_Vector2 &pEnd = isStart ? s.endPoint : s.startPoint;

        eventQueue.push({EventType::StartPoint, pStart, i});
        eventQueue.push({EventType::EndPoint, pEnd, i});
    }

    float sweepY = 0;

    auto xAtY = [&](size_t idx) -> double {
        const auto &s = segments[idx];
        if (std::abs(s.startPoint.y() - s.endPoint.y()) < 1e-6)
            return s.startPoint.x();

        return s.startPoint.x() + (s.endPoint.x() - s.startPoint.x()) * (sweepY - s.startPoint.y()) / (s.endPoint.y() - s.startPoint.y());
    };

    auto statusComparator = [&](const IndexedSegment& a, const IndexedSegment& b) {
        double xa = xAtY(a.index);
        double xb = xAtY(b.index);
        if (std::abs(xa - xb) < 1e-6) {
            return a.index < b.index;
        }
        return xa < xb;
    };

    std::set<std::pair<size_t, size_t>> seenIntersections;
    auto tryAddIntersection = [&](size_t idx1, size_t idx2) {
        if (idx1 == idx2)
            return;

        const auto index = std::minmax(idx1, idx2);
        if (seenIntersections.count(index)) {
            return;
        }

        double x1 = xAtY(idx1);
        double x2 = xAtY(idx2);

        size_t left = idx1;
        size_t right = idx2;
        if (x1 > x2 || (std::abs(x1 - x2) < 1e-6)) {
            left = idx2;
            right = idx1;
        }

        if (auto inter = computeIntersection(segments[left], segments[right])) {
            if (inter->y() <= sweepY) {
                eventQueue.push({EventType::IntersectionPoint, *inter, left, right});
                seenIntersections.insert(index);
            }
        }
    };

    AVLTree<utils::Indexed<utils::Segment2D, size_t>, decltype(statusComparator)> sweepTree(statusComparator);

    while (!eventQueue.empty()) {
        SweepEvent e = eventQueue.top();
        eventQueue.pop();

        sweepY = e.point.y();

        switch (e.type) {
            case EventType::StartPoint: {
                const auto segment = IndexedSegment(segments.at(e.segA), e.segA);

                sweepTree.insert(segment);

                if (auto prev = sweepTree.find_predecessor(segment))
                    tryAddIntersection(prev->index, e.segA);

                if (auto next = sweepTree.find_successor(segment))
                    tryAddIntersection(next->index, e.segA);

                break;
            }

            case EventType::EndPoint: {
                const auto segment = IndexedSegment(segments.at(e.segA), e.segA);

                auto prev = sweepTree.find_predecessor(segment);
                auto next = sweepTree.find_successor(segment);

                if (prev && next) {
                    tryAddIntersection(prev->index, next->index);
                }

                sweepTree.remove(segment);

                break;
            }

            case EventType::IntersectionPoint: {
                intersections.push_back(e.point);

                auto seg1 = IndexedSegment(segments.at(e.segA), e.segA);
                auto seg2 = IndexedSegment(segments.at(e.segB), e.segB);

                IndexedSegment left_seg, right_seg;
                if (statusComparator(seg1, seg2)) {
                    left_seg = seg1;
                    right_seg = seg2;
                } else {
                    left_seg = seg2;
                    right_seg = seg1;
                }

                sweepTree.remove(left_seg);
                sweepTree.remove(right_seg);

                sweepY = e.point.y() - 1e-6f;

                sweepTree.insert(left_seg);
                sweepTree.insert(right_seg);

                // $NOTE: this is wrong core, leave it here for now
                for (const auto & segment : sweepTree) {
                    tryAddIntersection(segment.index, left_seg.index);
                    tryAddIntersection(segment.index, right_seg.index);
                }
            }
        }
    }
    return intersections;
}

std::vector<GA_Offset> houdini::tools::algorithms::geometry::convexHull2D(const std::vector<utils::IndexedPoint3D>& points, EPlane plane) {
    std::vector<utils::IndexedPoint2D> mappedPoints;
    mappedPoints.reserve(points.size());

    for (const auto& [point3D, index] : points) {
        mappedPoints.push_back({map(point3D, plane), index});
    }

    return convexHull2D(std::move(mappedPoints));
}

std::vector<GA_Offset> houdini::tools::algorithms::geometry::convexHull2D(std::vector<utils::IndexedPoint2D> points) {
    if (points.size() < MIN_CONVEX_POINTS) {
        return {};
    }

    const utils::IndexedPoint2D minPoint = *std::min_element(points.begin(), points.end(), [](const auto& left, const auto& right) {
        return std::tuple(left.value.y(), left.value.x()) < std::tuple(right.value.y(), right.value.x());
    });

    std::sort(points.begin(), points.end(), [&](const utils::IndexedPoint2D & p1, const utils::IndexedPoint2D & p2) {
        const auto leftAngle = polarAngle(minPoint.value.x(), minPoint.value.y(), p1.value.x(), p1.value.y());
        const auto rightAngle = polarAngle(minPoint.value.x(), minPoint.value.y(), p2.value.x(), p2.value.y());

        if (leftAngle == rightAngle) {
            return distance2(minPoint.value, p1.value) > distance2(minPoint.value, p2.value);
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
                    prevPrevPoint.value.x(),
                    prevPrevPoint.value.y(),
                    prevPoint.value.x(),
                    prevPoint.value.y(),
                    currentPoint.value.x(),
                    currentPoint.value.y()
                );

            if (direction == EOrientation::Clockwise) {
                hullPoints.pop_back();
                continue;
            }

            break;
        }

        hullPoints.push_back({p.value, p.index});
    }

    std::vector<GA_Offset> hullOffsets;
    for (const auto& p : hullPoints)
        hullOffsets.push_back(p.index);

    return hullOffsets;
}
