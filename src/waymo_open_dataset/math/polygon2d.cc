/* Copyright 2019 The Waymo Open Dataset Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

// Copyright 2011 Google Inc. All Rights Reserved.

#include <float.h>
#include <stddef.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <limits>
#include <memory>

#include <glog/logging.h>
#include "absl/base/optimization.h"
#include "absl/strings/str_format.h"

#include "waymo_open_dataset/math/math_util.h"
#include "waymo_open_dataset/math/polygon2d.h"
#include "waymo_open_dataset/math/segment2d.h"

namespace waymo {
namespace open_dataset {

constexpr double Polygon2d::kEpsilon;

namespace {

// Returns the cross product of (v1 - v0) and (v2 - v0).
double Cross(const Vec2d v0, const Vec2d v1, const Vec2d v2) {
  return (v1 - v0).CrossProd(v2 - v0);
}

// Returns the cross product of (v1 - v0) and (v3 - v2).
double Cross(const Vec2d v0, const Vec2d v1, const Vec2d v2, const Vec2d v3) {
  return (v1 - v0).CrossProd(v3 - v2);
}

// Constant to check if the result of a Cross operation would need to be
// recomputed with exact precision. Checking that precisely would require
// dynamically computing the epsilon. Approximate by using 2^-50, which is two
// bits apart from 1.0. This is conservative as it will happen on all cases
// below 2^-50, which should be rare.
static constexpr double kCrossExactEpsilon = 0x1p-50;  // 2 ^ -50

// Accsum computes the accurate summation of 16 double precision floats.
// Algorithm from the following paper:
//
// Accurate floating-point summation part I: Faithful Rounding.
// Siegfried M. Rump, Takeshi Ogita And Shin’Ichi Oish
//
// https://www.tuhh.de/ti3/paper/rump/RuOgOi07I.pdf
double AccSum(std::array<double, 16>& p) {
  static constexpr double kAccSum = 0x1p-53;  // 2 ^ - 53

  auto NextPowerTwo = [](double d) {
    return std::pow(2, static_cast<int>(std::ceil(std::log2(d))));
  };

  const int n = p.size();
  double mu = std::abs(p[0]);
  for (const double pe : p) {
    mu = std::max(mu, std::abs(pe));
  }
  if (mu == 0.0) return 0.0;
  const double ms = 32.0;  // NextPowerTwo(n + 2);
  const double phi = kAccSum * ms;
  const double factor = kAccSum * ms * ms;
  double sigma = ms * NextPowerTwo(mu);

  double t = 0.0;
  for (;;) {
    double tau = 0.0;
    double sum_p = 0.0;
    for (int i = 0; i < n; ++i) {
      const double q = (sigma + p[i]) - sigma;
      tau += q;
      p[i] -= q;
      sum_p += p[i];
    }
    double tau1 = t + tau;
    if (std::abs(tau1) >= factor * sigma ||
        sigma <= std::numeric_limits<double>::min()) {
      const double tau2 = tau - (tau1 - t);
      return tau1 + (tau2 + sum_p);
    }
    t = tau1;
    if (t == 0.0) {
      return AccSum(p);
    }
    sigma = phi * sigma;
  }
}

// Compute (v1-v0).Cross(v3-v2). The sign of the result is computed with precise
// accuracy, by using precise floating point algorithms for results close to
// zero, at a significant performance cost. The intent of this function is to be
// used when computing intersections, which uses an algorithm that relies on
// exact sign computations (see Polygon2d::ComputeIntersectionVertices).
double CrossMaybeExact(const Vec2d v0, const Vec2d v1, const Vec2d v2,
                       const Vec2d v3) {
  if (v0 == v1 || v2 == v3) {
    return 0.0;
  }

  const double c = Cross(v0, v1, v2, v3);
  if (ABSL_PREDICT_TRUE(std::abs(c) > kCrossExactEpsilon)) {
    // More precision is needed if c is a single bit apart from 0.0.
    return c;
  }

  // Expand (v1-v0).Cross(v3-v2) as a sum of products.
  std::array<double, 16> term;

  // Compute each product.
  term[0] = v1.x() * v3.y();
  term[1] = -v0.x() * v3.y();
  term[2] = -v1.x() * v2.y();
  term[3] = v0.x() * v2.y();
  term[4] = -v1.y() * v3.x();
  term[5] = v0.y() * v3.x();
  term[6] = v1.y() * v2.x();
  term[7] = -v0.y() * v2.x();
  // Compute any loss due to rounding.
  // std::fma computes the intermediate result at infinite precision.
  term[8] = std::fma(v1.x(), v3.y(), -term[0]);
  term[9] = std::fma(-v0.x(), v3.y(), -term[1]);
  term[10] = std::fma(-v1.x(), v2.y(), -term[2]);
  term[11] = std::fma(v0.x(), v2.y(), -term[3]);
  term[12] = std::fma(-v1.y(), v3.x(), -term[4]);
  term[13] = std::fma(v0.y(), v3.x(), -term[5]);
  term[14] = std::fma(v1.y(), v2.x(), -term[6]);
  term[15] = std::fma(-v0.y(), v2.x(), -term[7]);

  // Compute the sum of the terms without precision loss.
  return AccSum(term);
}

// Compute the area of a non self-instersecting polygon, which is represented by
// all its vectices in counter-clockwise order.
double AreaInternal(const std::vector<Vec2d>& points) {
  // Convex polygons must be simple polygons (i.e. without self-intersections).
  // One idea to compute the area of a simple polygon with N sides is to sum the
  // areas of N triangles where each triangle is formed by one polygon side and
  // an arbitrary 2D point. After shifting the polygon to one of its points, if
  // all the points lie in counter-clock direction all the triangle areas should
  // be positive. We choose this arbitrary point to be the origin points[0] so
  // that the area of each triangle could be easily computed from the cross
  // product of two vectors. Please see this page for more detailed explanation.
  // http://en.wikipedia.org/wiki/Polygon
  double area = 0.0;
  // Shift points to x_0, y_0 for precision in calculating area.
  // Starting from i = 1 because cross_prod for (x_0, y_0) is zero.
  const int num_points = points.size();
  for (int i = 1; i < num_points - 1; ++i) {
    const int j = i + 1;
    area += Cross(points[0], points[i], points[j]);
  }
  if (std::abs(area) <= Polygon2d::kEpsilon) {
    area = 0.0;
  }
  return area * 0.5;
}

void RemoveConsecutiveDuplicatePoints(std::vector<Vec2d>* points) {
  // Not reusing the implementation above to make kEpsilonSqr a compile time
  // constant.
  if (points->empty()) return;
  constexpr double kEpsilonSqr = Sqr(Polygon2d::kEpsilon);
  auto to = points->begin();
  for (auto from = std::next(to); from != points->end(); ++from) {
    if (from->DistanceSqrToPoint(*to) >= kEpsilonSqr) {
      ++to;
      if (to != from) *to = *from;
    }
  }
  points->erase(std::next(to), points->end());
  // Handle the case that the first and last points are identical.
  if (points->size() >= 2 &&
      points->front().DistanceSqrToPoint(points->back()) < kEpsilonSqr) {
    points->pop_back();
  }
}

}  // namespace

Polygon2d::Polygon2d(std::vector<Vec2d> points) : points_(std::move(points)) {
  CHECK_GT(points_.size(), 2);
  bool counter_clockwise = true;
  DCHECK(IsConvexHull(points_, &counter_clockwise))
      << "Is counter clockwise? " << counter_clockwise
      << ", offending polygon:\n"
      << PrintPointsToString(points_, true);

  RemoveConsecutiveDuplicatePoints(&points_);
  BuildFromPoints();
}

void Polygon2d::BuildFromPoints() {
  // Note: This function assumes that the input points_ vector is non empty.
  CHECK_GT(points_.size(), 0);
  // In case of the polygon has zero area, create some fake points to make sure
  // num_points >= 3, but most member functions are not guaranteed to be working
  // as expected.
  if (points_.size() == 1) {
    Vec2d p0 = points_[0];
    points_.push_back(p0);
  }
  if (points_.size() == 2) {
    Vec2d p0 = points_[0];
    Vec2d p1 = points_[1];
    points_.push_back(p1);
    points_.push_back(p0);
  }

  num_points_ = points_.size();

  AxisAlignedBoundingBox(&bbox_bottom_left_, &bbox_top_right_);
}

bool Polygon2d::PointInside(const Vec2d p) const {
  if (num_points_ < 3) return false;
  if (p.x() < bbox_bottom_left_.x() || p.y() < bbox_bottom_left_.y() ||
      p.x() > bbox_top_right_.x() || p.y() > bbox_top_right_.y()) {
    return false;
  }

  int start_ix = 0;
  int end_ix = num_points_;
  // For small polygons / ranges, e.g. polygon with 10 vertices, the constants
  // of the O(log(n)) algorithm are sufficienly high that the O(n) algorithm
  // is faster.
  constexpr int kLinearSearchThreshold = 10;
  // Divide the polygon into two and decide which side we should continue
  // checking. As invariant, the polygon we consider has vertices
  // points_[0], points_[start_ix], ... points_[end_ix].
  while (end_ix - start_ix > kLinearSearchThreshold) {
    const int mid_ix = (end_ix + start_ix) / 2;
    const Vec2d bisector(points_[mid_ix] - points_[0]);
    if (bisector.CrossProd(p - points_[0]) >= 0.0) {
      start_ix = mid_ix;
    } else {
      end_ix = mid_ix;
    }
  }
  for (int ii = start_ix; ii < end_ix - 1; ++ii) {
    const Vec2d s = points_[ii + 1] - points_[ii];
    if (s.CrossProd(p - points_[ii]) < 0.0) {
      return false;
    }
  }
  if (start_ix < end_ix &&
      Segment(end_ix - 1).CrossProd(p - points_[end_ix - 1]) < 0.0) {
    return false;
  }
  return true;
}

double Polygon2d::AreaNoValidityCheck() const { return AreaInternal(points_); }

namespace {

// Given a segment and a colinear point, check if the point is in the segment,
// and if so, update *intersection to the intersection value (q).
bool ColinearSegmentPointIntersection(const Vec2d p0, const Vec2d p1,
                                      const Vec2d q, Vec2d* intersection) {
  // Compute the AABB for the segment and check that the point lands inside it.
  const Vec2d bottom_left = p0.ElemWiseMin(p1);
  const Vec2d top_right = p0.ElemWiseMax(p1);
  if (bottom_left.x() <= q.x() && q.x() <= top_right.x() &&
      bottom_left.y() <= q.y() && q.y() <= top_right.y()) {
    *intersection = q;
    return true;
  }
  return false;
}

// Compute the intersection of two segments. The algorithm in
// ComputeIntersectionVertices depends on exact computation, adjustment by
// epsilon is not acceptable.
bool ExactIntersection(const Vec2d p0, const Vec2d p1, const Vec2d q0,
                       const Vec2d q1, const double det, Vec2d* intersection) {
  if (det == 0.0) {
    // Segments are parallel.
    if (CrossMaybeExact(p0, p1, p0, q0) != 0.0) {
      // But not colinear.
      return false;
    }
    // If they are overlapping, return one of the intersection points.
    return ColinearSegmentPointIntersection(q0, q1, p0, intersection) ||
           ColinearSegmentPointIntersection(q0, q1, p1, intersection) ||
           ColinearSegmentPointIntersection(p0, p1, q0, intersection) ||
           ColinearSegmentPointIntersection(p0, p1, q1, intersection);
  }

  const double detsign = std::copysign(1.0, det);
  const double epsilon = std::max(std::abs(det), 1.0) * kCrossExactEpsilon;

  // t1/det is the intersection point projected to s1.
  // Must be in the range [0-1] if there is an intersection
  const double t1 = CrossMaybeExact(p0, q0, q1, q0);
  if (t1 * detsign < 0.0 || t1 * detsign > std::abs(det) + epsilon) {
    return false;
  }
  if (ABSL_PREDICT_FALSE(t1 * detsign > std::abs(det) - epsilon)) {
    // t1/det is close to 1, reverse both segments and recompute the
    // intersection, to use full precision if close to the segment start.
    const double rt1 = CrossMaybeExact(p1, q1, q0, q1);
    if (rt1 * detsign < 0.0) return false;
  }

  // t2/det is the intersection point projected to s2.
  // Must be in the range [0-1] if there is an intersection
  const double t2 = CrossMaybeExact(p0, p1, p0, q0);
  if (t2 * detsign < 0.0 || t2 * detsign > std::abs(det) + epsilon) {
    return false;
  }
  if (ABSL_PREDICT_FALSE(t2 * detsign > std::abs(det) - epsilon)) {
    // t2/det is close to 1, reverse both segments and recompute the
    // intersection, to use full precision if close to the segment start.
    const double rt2 = CrossMaybeExact(p1, p0, p1, q1);
    if (rt2 * detsign < 0.0) return false;
  }

  // BoundToRange to avoid precision errors from the division.
  *intersection = p0 + (p1 - p0) * BoundToRange(0.0, 1.0, t1 / det);
  return true;
}

}  // namespace

namespace internal {

bool ExactSegmentIntersection(const Segment2d& s1, const Segment2d& s2,
                              Vec2d* intersection) {
  const Vec2d s1s = s1.start(), s1e = s1.end();
  const Vec2d s2s = s2.start(), s2e = s2.end();

  const double det = CrossMaybeExact(s1s, s1e, s2e, s2s);
  return ExactIntersection(s1s, s1e, s2s, s2e, det, intersection);
}


}  // namespace internal

std::vector<Vec2d> Polygon2d::ComputeIntersectionVertices(
    const Polygon2d& other) const {
  if (!MaybeHasIntersectionWith(other)) {
    return {};
  }

  // This algorithm is from the following paper:
  //
  // O'Rourke, Joseph, et al. "A new linear algorithm for intersecting convex
  // polygons." Computer Graphics and Image Processing 19.4 (1982): 384-391.
  //
  // The algorithm can be viewed as a geometric generalization of merging two
  // sorted lists. It performs a counter-clockwise traversal of the boundaries
  // of the two polygons. The algorithm maintains a pair of edges, one from each
  // polygon. From a consideration of the relative positions of these edges the
  // algorithm advances one of them to the next edge in counterclockwise order
  // around its polygon. Intuitively, this is done in such a way that these two
  // edges effectively “chase” each other around the boundary of the
  // intersection polygon.

  // We use P and Q to represent this polygon and the other one.
  const std::vector<Vec2d>& other_points = other.points();
  bool p_inside = false, q_inside = false;
  const int total_num_points = NumPoints() + other.NumPoints();
  std::vector<Vec2d> convex_points;
  convex_points.reserve(total_num_points);

  // A lambda that is used to append a new convex point to the list. We ignore
  // the point that is identical to either the first or the last convex point in
  // the list to handle corner cases, in which two polygons may "intersect" with
  // either a point or an edge.
  const auto append_convex_point = [](Vec2d p, std::vector<Vec2d>* convex_pts) {
    if (convex_pts->empty() ||
        (p != convex_pts->front() && p != convex_pts->back())) {
      convex_pts->push_back(p);
    }
  };

  int p_idx = 0, q_idx = 0;
  int p_idx_next = 1, q_idx_next = 1;
  Vec2d p0 = points_[p_idx];
  Vec2d p1 = points_[p_idx_next];
  Vec2d q0 = other_points[q_idx];
  Vec2d q1 = other_points[q_idx_next];

  for (int i = 0; i < total_num_points * 2; ++i) {
    // If we found a intersection between <p0, p1> and <q0, q1>, put the
    // intersection into the convex points list, and check that after the
    // intersection whose points will be inside and hence recorded. If we reach
    // the first intersection point, we are done.
    Vec2d inter;
    const double det = CrossMaybeExact(p0, p1, q1, q0);
    if (ExactIntersection(p0, p1, q0, q1, det, &inter)) {
      if (convex_points.size() > 2 && inter == convex_points[0]) {
        return convex_points;
      }
      if (convex_points.empty()) {
        // When we found the first intersection, we only need to iterate this
        // loop at most total_num_points + 1 times to get back to this
        // intersection.
        i = total_num_points - 1;
      }
      append_convex_point(inter, &convex_points);
      // If p1 is on the left of <q0, q1>, mark P as inside; otherwise mark Q as
      // inside.
      if (CrossMaybeExact(q1, q0, p1, q0) >= 0.0) {
        p_inside = true;
        q_inside = false;
      } else {
        p_inside = false;
        q_inside = true;
      }
    }

    // Determine in which polygon we would like to advance according to the
    // "advance rule" in the algorithm.
    const bool advance_p = (det >= 0.0)
                               ? CrossMaybeExact(q1, q0, p1, q0) < 0.0
                               : CrossMaybeExact(p1, p0, q1, p0) >= 0.0;

    if (advance_p) {
      if (p_inside) {
        append_convex_point(p1, &convex_points);
      }
      p_idx = p_idx_next;
      p0 = p1;
      p_idx_next = Next(p_idx_next);
      p1 = points_[p_idx_next];
    } else {
      if (q_inside) {
        append_convex_point(q1, &convex_points);
      }
      q_idx = q_idx_next;
      q0 = q1;
      q_idx_next = other.Next(q_idx_next);
      q1 = other_points[q_idx_next];
    }
  }

  // Handle the case that those two polygons share one vertex or (part of) one
  // edge, in which case the intersection is empty.
  if (!convex_points.empty()) {
    return {};
  }

  // Now we have three cases:
  //   1. P is inside of Q.
  //   2. Q is inside of P.
  //   3. P and Q don't intersect.

  // Check the case 1.
  bool p_is_inside = true;
  for (const auto& point : points_) {
    if (!other.PointInside(point)) {
      p_is_inside = false;
      break;
    }
  }
  if (p_is_inside) {
    return points_;
  }

  // Check the case 2.
  bool q_is_inside = true;
  for (const auto& point : other_points) {
    if (!PointInside(point)) {
      q_is_inside = false;
      break;
    }
  }
  if (q_is_inside) {
    return other.points_;
  }

  // P and Q don't intersect.
  return {};
}

double Polygon2d::ComputeIntersectionArea(const Polygon2d& other) const {
  auto convex_points = ComputeIntersectionVertices(other);
  if (convex_points.size() < 3) {
    return 0.0;
  }
  return AreaInternal(convex_points);
}

bool Polygon2d::MaybeHasIntersectionWith(const Polygon2d& other) const {
  const Vec2d other_bottom_left = other.bbox_bottom_left();
  const Vec2d other_top_right = other.bbox_top_right();

  if (bbox_bottom_left_.x() > other_top_right.x() ||
      other_bottom_left.x() > bbox_top_right_.x() ||
      bbox_bottom_left_.y() > other_top_right.y() ||
      other_bottom_left.y() > bbox_top_right_.y()) {
    return false;
  }

  return true;
}

void Polygon2d::AxisAlignedBoundingBox(Vec2d* bottom_left,
                                       Vec2d* top_right) const {
  CHECK_GT(num_points_, 2);
  Vec2d min_point = points_[0];
  Vec2d max_point = points_[0];
  for (int i = 1, n = points_.size(); i < n; ++i) {
    min_point = min_point.ElemWiseMin(points_[i]);
    max_point = max_point.ElemWiseMax(points_[i]);
  }
  *bottom_left = min_point;
  *top_right = max_point;
}

void Polygon2d::ShiftCenter(const Vec2d offset) {
  for (auto& point : points_) {
    point += offset;
  }
  bbox_bottom_left_ += offset;
  bbox_top_right_ += offset;
}

void Polygon2d::Transform(double angle, const Vec2d center,
                          const Vec2d offset) {
  if (angle == 0.0) {
    ShiftCenter(offset);
    return;
  }
  const double sina = sin(angle);
  const double cosa = cos(angle);
  const Vec2d shift = center + offset;
  for (auto& point : points_) {
    const double x = point.x() - center.x();
    const double y = point.y() - center.y();
    point.set_x(x * cosa - y * sina + shift.x());
    point.set_y(x * sina + y * cosa + shift.y());
  }
  BuildFromPoints();
}

// static
std::string Polygon2d::PrintPointsToString(const std::vector<Vec2d>& points,
                                           bool full_precision) {
  std::string result;
  for (const Vec2d point : points) {
    if (full_precision) {
      ::absl::StrAppendFormat(&result, "{%.*e, %.*e},\n", 21, point.x(), 21,
                              point.y());
    } else {
      ::absl::StrAppendFormat(&result, "{%f, %f},\n", point.x(), point.y());
    }
  }
  return result;
}

std::string Polygon2d::DebugString() const {
  return PrintPointsToString(points_, false);
}

// static
bool Polygon2d::IsConvexHull(const std::vector<Vec2d>& points,
                             bool* counter_clockwise) {
  int n = points.size();
  bool has_ccw_direction = false;
  bool has_cw_direction = false;
  for (int ii = 0; ii < n; ++ii) {
    // For convex polygon, all adjacent segments are in either clockwise or
    // counter-clockwise direction.
    const double cross_prod =
        Cross(points[ii == 0 ? n - 1 : ii - 1], points[ii],
              points[ii == n - 1 ? 0 : ii + 1]);
    if (cross_prod > kEpsilon) {
      has_ccw_direction = true;
    } else if (cross_prod < -kEpsilon) {
      has_cw_direction = true;
    }
    if (has_ccw_direction && has_cw_direction) return false;
  }

  // Conclusive means every edge's cross product have a large enough absolute
  // value for us to confidently tell its sign.
  bool conclusive = false;
  for (int ref = 0; ref < n && !conclusive; ++ref) {
    conclusive = true;
    const Vec2d ref_point = points[ref];
    for (int ii = 0; ii < n; ++ii) {
      // Still need the following checks since convex polygons must be simple
      // polygons.
      // For convex polygon, after shifting the polygon to one of its points,
      // all the points should lie in either clockwise or counter-clockwise
      // direction.
      // We choose this arbitrary point to be the origin points_[0].
      if (ii >= 2) {
        const double shift_prod = Cross(ref_point, points[ii - 1], points[ii]);
        if (shift_prod > kEpsilon) {
          has_ccw_direction = true;
        } else if (shift_prod < -kEpsilon) {
          has_cw_direction = true;
        } else {
          // In this inconclusive case, we switch to the next vertex as the ref
          // point.
          conclusive = false;
          break;
        }
      }
      if (has_ccw_direction && has_cw_direction) return false;
    }
  }
  if (counter_clockwise != nullptr) {
    *counter_clockwise = !has_cw_direction;
  }
  return true;
}

}  // namespace open_dataset
}  // namespace waymo
