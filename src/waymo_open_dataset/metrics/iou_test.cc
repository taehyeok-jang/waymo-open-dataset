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

#include "waymo_open_dataset/metrics/iou.h"

#include <vector>

#include <gtest/gtest.h>
#include "waymo_open_dataset/label.pb.h"
#include "waymo_open_dataset/metrics/test_utils.h"
#include "waymo_open_dataset/protos/metrics.pb.h"

namespace waymo {
namespace open_dataset {
namespace {

constexpr double kError = 1e-5;

using TolerantConfig = Config::LongitudinalErrorTolerantConfig;

TEST(ComputeIoU, AABox2d) {
  const Label::Box b1 = BuildAA2dBox(0.0, 0.0, 1.0, 2.0);
  const Label::Box b2 = BuildAA2dBox(0.0, 0.0, 1.0, 2.0);
  const Label::Box b3 = BuildAA2dBox(10.0, 10.0, 1.0, 2.0);
  const Label::Box b4 = BuildAA2dBox(0.5, 0.0, 1.0, 4.0);
  // Zero-sized box.
  const Label::Box b5 = BuildAA2dBox(0.0, 0.0, 0.0, 0.0);

  EXPECT_NEAR(1.0, ComputeIoU(b1, b2, Label::Box::TYPE_AA_2D), kError);
  EXPECT_NEAR(0.0, ComputeIoU(b1, b3, Label::Box::TYPE_AA_2D), kError);
  EXPECT_NEAR(0.5 * 2.0 / (2.0 + 4.0 - 0.5 * 2.0),
              ComputeIoU(b1, b4, Label::Box::TYPE_AA_2D), kError);
  EXPECT_NEAR(0.0, ComputeIoU(b1, b5, Label::Box::TYPE_AA_2D), kError);
}

TEST(ComputeIoU, Box2d) {
  const Label::Box b1 = BuildBox2d(0.0, 0.0, 1.0, 2.0, 0.0);
  const Label::Box b2 = BuildBox2d(0.0, 0.0, 1.0, 2.0, 0.0);
  const Label::Box b3 = BuildBox2d(10.0, 10.0, 1.0, 2.0, 1.0);
  const Label::Box b4 = BuildBox2d(0.5, 0.0, 1.0, 4.0, 0.0);
  const Label::Box b5 = BuildBox2d(0.5, 0.0, 1.0, 4.0, M_PI * 0.5);
  const Label::Box b6 = BuildBox2d(0.5, 0.0, 1.0, 4.0, M_PI * 0.25);
  // Zero-sized box.
  const Label::Box b7 = BuildBox2d(0.0, 0.0, 0.0, 0.0, 0.0);

  EXPECT_NEAR(1.0, ComputeIoU(b1, b2, Label::Box::TYPE_2D), kError);
  EXPECT_NEAR(0.0, ComputeIoU(b1, b3, Label::Box::TYPE_2D), kError);
  EXPECT_NEAR(0.5 * 2.0 / (2.0 + 4.0 - 0.5 * 2.0),
              ComputeIoU(b1, b4, Label::Box::TYPE_2D), kError);
  EXPECT_NEAR(1.0 / (2.0 + 4.0 - 1.0), ComputeIoU(b1, b5, Label::Box::TYPE_2D),
              kError);
  EXPECT_NEAR(0.24074958176697656, ComputeIoU(b1, b6, Label::Box::TYPE_2D),
              kError);
  EXPECT_NEAR(0.0, ComputeIoU(b1, b7, Label::Box::TYPE_2D), kError);
}

TEST(ComputeIoU, Box3d) {
  const Label::Box b1 = BuildBox3d(0.0, 0.0, 0.0, 1.0, 2.0, 2.0, 0.0);
  // Same as b1.
  const Label::Box b2 = BuildBox3d(0.0, 0.0, 0.0, 1.0, 2.0, 2.0, 0.0);
  // Zero overlap.
  const Label::Box b3 = BuildBox3d(10.0, 10.0, 0.0, 1.0, 2.0, 2.0, 1.0);
  const Label::Box b4 = BuildBox3d(0.5, 0.0, 0.0, 1.0, 4.0, 4.0, 0.0);
  const Label::Box b5 = BuildBox3d(0.5, 0.0, 0.0, 1.0, 4.0, 4.0, M_PI * 0.5);
  const Label::Box b6 = BuildBox3d(0.5, 0.0, 0.0, 1.0, 4.0, 2.0, M_PI * 0.25);
  // Zero-sized box.
  const Label::Box b7 = BuildBox3d(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  const Label::Box b8 = BuildBox3d(0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
  // Illegal box.
  const Label::Box b9 = BuildBox3d(0.0, 0.0, 0.0, 1.0, -1.0, 1.0, 0.0);
  // Tiny box
  const Label::Box b10 = BuildBox3d(0.0, 0.0, 0.0, 1.0, 1e-12, 1.0, 0.0);
  // Small box
  const Label::Box b11 = BuildBox3d(0.0, 0.0, 0.0, 1.0, 1.01e-2, 2.0, 0.0);

  EXPECT_NEAR(1.0, ComputeIoU(b1, b2, Label::Box::TYPE_3D), kError);
  EXPECT_NEAR(0.0, ComputeIoU(b1, b3, Label::Box::TYPE_3D), kError);
  EXPECT_NEAR(0.5 * 2.0 * 2.0 / (4.0 + 16.0 - 0.5 * 2.0 * 2.0),
              ComputeIoU(b1, b4, Label::Box::TYPE_3D), kError);
  EXPECT_NEAR(0.5 * 2.0 * 2.0 / (4.0 + 16.0 - 0.5 * 2.0 * 2.0),
              ComputeIoU(b1, b5, Label::Box::TYPE_3D), kError);
  EXPECT_NEAR(0.24074958176697656, ComputeIoU(b1, b6, Label::Box::TYPE_3D),
              kError);
  EXPECT_NEAR(0.0, ComputeIoU(b1, b7, Label::Box::TYPE_3D), kError);
  EXPECT_NEAR(0.0, ComputeIoU(b1, b8, Label::Box::TYPE_3D), kError);
  EXPECT_NEAR(0.0, ComputeIoU(b1, b10, Label::Box::TYPE_3D), kError);
  EXPECT_NEAR(0.0, ComputeIoU(b10, b10, Label::Box::TYPE_3D), kError);
  EXPECT_NEAR(0.00505, ComputeIoU(b1, b11, Label::Box::TYPE_3D), kError);
  EXPECT_NEAR(1.0, ComputeIoU(b11, b11, Label::Box::TYPE_3D), kError);
}

TEST(ComputeIoU, OverlappingBox3d) {
  const Label::Box b1 =
      BuildBox3d(70.970001220703125, 10.090000152587891, -0.37999999523162842,
                 4.2699999809265137, 2.0199999809265137, 1.6599999666213989,
                 -3.119999885559082);
  // Same as b1 with a small difference on length.
  const Label::Box b2 = BuildBox3d(
      70.970001220703125, 10.090000152587891, -0.37999999523162842, 4.25,
      2.0199999809265137, 1.6599999666213989, -3.119999885559082);
  EXPECT_NEAR(ComputeIoU(b1, b2, Label::Box::TYPE_3D),
      1 - (4.2699999809265137 - 4.25) * 2.0199999809265137 *
              1.6599999666213989 /
              (4.2699999809265137 * 2.0199999809265137 * 1.6599999666213989),
      kError);
}

TEST(BatchComputeIoU, EmptyB2s) {
  const Label::Box b1 = BuildBox3d(0.0, 0.0, 0.0, 1.0, 2.0, 2.0, 0.0);
  ASSERT_EQ(BatchComputeIoU(b1, {}, Label::Box::TYPE_3D).size(), 0);
}

TEST(BatchComputeIoU, Box3d) {
  // Boxes and expected values are the same as the non-batched version above.
  const Label::Box b1 = BuildBox3d(0.0, 0.0, 0.0, 1.0, 2.0, 2.0, 0.0);
  // Same as b1.
  const Label::Box b2 = BuildBox3d(0.0, 0.0, 0.0, 1.0, 2.0, 2.0, 0.0);
  // Zero overlap.
  const Label::Box b3 = BuildBox3d(10.0, 10.0, 0.0, 1.0, 2.0, 2.0, 1.0);
  const Label::Box b4 = BuildBox3d(0.5, 0.0, 0.0, 1.0, 4.0, 4.0, 0.0);
  const Label::Box b5 = BuildBox3d(0.5, 0.0, 0.0, 1.0, 4.0, 4.0, M_PI * 0.5);
  const Label::Box b6 = BuildBox3d(0.5, 0.0, 0.0, 1.0, 4.0, 2.0, M_PI * 0.25);
  // Zero-sized box.
  const Label::Box b7 = BuildBox3d(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  const Label::Box b8 = BuildBox3d(0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
  // Tiny box
  const Label::Box b9 = BuildBox3d(0.0, 0.0, 0.0, 1.0, 1e-12, 1.0, 0.0);
  // Small box
  const Label::Box b10 = BuildBox3d(0.0, 0.0, 0.0, 1.0, 1.01e-2, 2.0, 0.0);

  const std::vector<Label::Box> boxes = {b2, b3, b4, b5,  b6,
                                         b7, b8, b9, b10, b1};
  const std::vector<double>& ious =
      BatchComputeIoU(b1, boxes, Label::Box::TYPE_3D);
  ASSERT_EQ(ious.size(), 10);
  EXPECT_NEAR(1.0, ious[0], kError);
  EXPECT_NEAR(0.0, ious[1], kError);
  EXPECT_NEAR(0.5 * 2.0 * 2.0 / (4.0 + 16.0 - 0.5 * 2.0 * 2.0), ious[2],
              kError);
  EXPECT_NEAR(0.5 * 2.0 * 2.0 / (4.0 + 16.0 - 0.5 * 2.0 * 2.0), ious[3],
              kError);
  EXPECT_NEAR(0.24074958176697656, ious[4], kError);
  EXPECT_NEAR(0.0, ious[5], kError);
  EXPECT_NEAR(0.0, ious[6], kError);
  EXPECT_NEAR(0.0, ious[7], kError);
  EXPECT_NEAR(0.00505, ious[8], kError);
  EXPECT_NEAR(1.0, ious[9], kError);
}

TEST(ComputeLongitudinalAffinity, Box3d) {
  // Constructs a ground truth at near range.
  const Label::Box gt1 = BuildBox3d(1.0, 0.0, 0.0, 1.0, 2.0, 2.0, 0.0);
  // Same as gt1.
  const Label::Box pd1_1 = BuildBox3d(1.0, 0.0, 0.0, 1.0, 2.0, 2.0, 0.0);
  // Moves center along line of sight.
  const Label::Box pd1_2 = BuildBox3d(1.5, 0.0, 0.0, 1.0, 2.0, 2.0, 0.0);
  // Move center along line of sight and add some lateral error.
  const Label::Box pd1_3 = BuildBox3d(1.5, 1.0, 0.0, 1.0, 2.0, 2.0, 0.0);
  // Range-aligned pd3 to gt1.
  const Label::Box pd1_4 = BuildBox3d(4.0, 0.0, 0.0, 1.0, 2.0, 2.0, 0.0);

  Config::LongitudinalErrorTolerantConfig let_config = BuildDefaultLetConfig();

  EXPECT_NEAR(1.0, ComputeLongitudinalAffinity(pd1_1, gt1, let_config), kError);
  EXPECT_NEAR(0.75, ComputeLongitudinalAffinity(pd1_2, gt1, let_config),
              kError);
  EXPECT_NEAR(0.75, ComputeLongitudinalAffinity(pd1_3, gt1, let_config),
              kError);
  EXPECT_NEAR(0.0, ComputeLongitudinalAffinity(pd1_4, gt1, let_config), kError);

  // Constructs a ground truth at long range.
  const Label::Box gt2 = BuildBox3d(30.0, 40.0, 0.0, 1.0, 2.0, 2.0, 0.0);
  const Label::Box pd2_1 = BuildBox3d(30.0, 40.0, 0.0, 1.0, 2.0, 2.0, 0.0);
  const Label::Box pd2_2 = BuildBox3d(33.0, 44.0, 0.0, 1.0, 2.0, 2.0, 0.0);
  const Label::Box pd2_3 =
      BuildBox3d(33.0 + 0.4, 44.0 - 0.3, 0.0, 1.0, 2.0, 2.0, 0.0);
  const Label::Box pd2_4 =
      BuildBox3d(33.0 + 1.2, 44.0 - 0.9, 0.0, 1.0, 2.0, 2.0, 0.0);

  EXPECT_NEAR(1.0, ComputeLongitudinalAffinity(pd2_1, gt2, let_config), kError);
  EXPECT_NEAR(1.0 - 5.0 / 7.5,
              ComputeLongitudinalAffinity(pd2_2, gt2, let_config), kError);
  EXPECT_NEAR(1.0 - 5.0 / 7.5,
              ComputeLongitudinalAffinity(pd2_3, gt2, let_config), kError);
  EXPECT_NEAR(1.0 - 5.0 / 7.5,
              ComputeLongitudinalAffinity(pd2_4, gt2, let_config), kError);
}

TEST(ComputeLetIoU, Box3d) {
  // Constructs a ground truth at near range.
  const Label::Box gt1 = BuildBox3d(1.0, 0.0, 0.0, 1.0, 2.0, 2.0, 0.0);
  // Same as gt1.
  const Label::Box pd1 = BuildBox3d(1.0, 0.0, 0.0, 1.0, 2.0, 2.0, 0.0);
  // Moves center along the line of sight but keep the shape.
  const Label::Box pd2 = BuildBox3d(1.5, 0.0, 0.0, 1.0, 2.0, 2.0, 0.0);
  // Move center along the line of sight and add some lateral error.
  const Label::Box pd3 = BuildBox3d(1.5, 1.0, 0.0, 1.0, 2.0, 2.0, 0.0);
  // Range-aligned pd3 to gt1.
  const Label::Box aligned_pd3 =
      BuildBox3d(9.0 / 13.0, 18.0 / 39.0, 0.0, 1.0, 2.0, 2.0, 0.0);

  Config::LongitudinalErrorTolerantConfig::AlignType iou_type_not_aligned =
      Config::LongitudinalErrorTolerantConfig::TYPE_NOT_ALIGNED;
  Config::LongitudinalErrorTolerantConfig::AlignType iou_type_center =
      Config::LongitudinalErrorTolerantConfig::TYPE_CENTER_ALIGNED;
  Config::LongitudinalErrorTolerantConfig::AlignType iou_type_range =
      Config::LongitudinalErrorTolerantConfig::TYPE_RANGE_ALIGNED;

  Config::LongitudinalErrorTolerantConfig::Location3D sensor_location;
  sensor_location.set_x(0.0);
  sensor_location.set_y(0.0);
  sensor_location.set_z(0.0);

  EXPECT_NEAR(1.0,
              ComputeLetIoU(pd1, gt1, sensor_location, iou_type_not_aligned,
                            Label::Box::TYPE_3D),
              kError);
  EXPECT_NEAR(1.0,
              ComputeLetIoU(pd1, gt1, sensor_location, iou_type_center,
                            Label::Box::TYPE_3D),
              kError);
  EXPECT_NEAR(1.0,
              ComputeLetIoU(pd1, gt1, sensor_location, iou_type_range,
                            Label::Box::TYPE_3D),
              kError);

  EXPECT_NEAR(1.0 / 3.0,
              ComputeLetIoU(pd2, gt1, sensor_location, iou_type_not_aligned,
                            Label::Box::TYPE_3D),
              kError);
  EXPECT_NEAR(1.0,
              ComputeLetIoU(pd2, gt1, sensor_location, iou_type_center,
                            Label::Box::TYPE_3D),
              kError);
  EXPECT_NEAR(1.0,
              ComputeLetIoU(pd2, gt1, sensor_location, iou_type_range,
                            Label::Box::TYPE_3D),
              kError);

  EXPECT_NEAR(1.0 / 7.0,
              ComputeLetIoU(pd3, gt1, sensor_location, iou_type_not_aligned,
                            Label::Box::TYPE_3D),
              kError);
  EXPECT_NEAR(1.0,
              ComputeLetIoU(pd3, gt1, sensor_location, iou_type_center,
                            Label::Box::TYPE_3D),
              kError);
  EXPECT_NEAR(ComputeIoU(aligned_pd3, gt1, Label::Box::TYPE_3D),
              ComputeLetIoU(pd3, gt1, sensor_location, iou_type_range,
                            Label::Box::TYPE_3D),
              kError);

  // Constructs an ground truth at near range.
  const Label::Box shifted_gt1 =
      BuildBox3d(1.0 + 1.0, 0.0, 0.0 + 0.5, 1.0, 2.0, 2.0, 0.0);
  // Same as gt1.
  const Label::Box shifted_pd1 =
      BuildBox3d(1.0 + 1.0, 0.0, 0.0 + 0.5, 1.0, 2.0, 2.0, 0.0);
  // Move center along line of sight and add some bearing error.
  const Label::Box shifted_pd3 =
      BuildBox3d(1.5 + 1.0, 1.0, 0.0 + 0.5, 1.0, 2.0, 2.0, 0.0);

  sensor_location.set_x(1.0);
  sensor_location.set_y(0.0);
  sensor_location.set_z(0.5);

  EXPECT_NEAR(1.0,
              ComputeLetIoU(shifted_pd1, shifted_gt1, sensor_location,
                            iou_type_not_aligned, Label::Box::TYPE_3D),
              kError);
  EXPECT_NEAR(1.0,
              ComputeLetIoU(shifted_pd1, shifted_gt1, sensor_location,
                            iou_type_center, Label::Box::TYPE_3D),
              kError);
  EXPECT_NEAR(1.0,
              ComputeLetIoU(shifted_pd1, shifted_gt1, sensor_location,
                            iou_type_range, Label::Box::TYPE_3D),
              kError);
  EXPECT_NEAR(1.0 / 7.0,
              ComputeLetIoU(shifted_pd3, shifted_gt1, sensor_location,
                            iou_type_not_aligned, Label::Box::TYPE_3D),
              kError);
  EXPECT_NEAR(1.0,
              ComputeLetIoU(shifted_pd3, shifted_gt1, sensor_location,
                            iou_type_center, Label::Box::TYPE_3D),
              kError);
  EXPECT_NEAR(ComputeIoU(aligned_pd3, gt1, Label::Box::TYPE_3D),
              ComputeLetIoU(shifted_pd3, shifted_gt1, sensor_location,
                            iou_type_range, Label::Box::TYPE_3D),
              kError);
}

TEST(ComputeLongitudinalAffnity, Box2d) {
  // Constructs a ground truth at near range.
  const Label::Box gt1 = BuildBox2d(1.0, 0.0, 1.0, 2.0, 0.0);
  // Same as gt1.
  const Label::Box pd1_1 = BuildBox2d(1.0, 0.0, 1.0, 2.0, 0.0);
  // Moves center along line of sight.
  const Label::Box pd1_2 = BuildBox2d(1.5, 0.0, 1.0, 2.0, 0.0);
  // Moves center along line of sight and add some lateral error.
  const Label::Box pd1_3 = BuildBox2d(1.5, 1.0, 1.0, 2.0, 0.0);
  // Range-aligned pd3 to gt1.
  const Label::Box pd1_4 = BuildBox2d(4.0, 0.0, 1.0, 2.0, 0.0);

  Config::LongitudinalErrorTolerantConfig let_config = BuildDefaultLetConfig();

  EXPECT_NEAR(1.0, ComputeLongitudinalAffinity(pd1_1, gt1, let_config), kError);
  EXPECT_NEAR(0.75, ComputeLongitudinalAffinity(pd1_2, gt1, let_config),
              kError);
  EXPECT_NEAR(0.75, ComputeLongitudinalAffinity(pd1_3, gt1, let_config),
              kError);
  EXPECT_NEAR(0.0, ComputeLongitudinalAffinity(pd1_4, gt1, let_config), kError);

  // Constructs a ground truth at long range.
  const Label::Box gt2 = BuildBox2d(30.0, 40.0, 1.0, 2.0, 0.0);
  const Label::Box pd2_1 = BuildBox2d(30.0, 40.0, 1.0, 2.0, 0.0);
  const Label::Box pd2_2 = BuildBox2d(33.0, 44.0, 1.0, 2.0, 0.0);
  const Label::Box pd2_3 = BuildBox2d(33.0 + 0.4, 44.0 - 0.3, 1.0, 2.0, 0.0);
  const Label::Box pd2_4 = BuildBox2d(33.0 + 1.2, 44.0 - 0.9, 1.0, 2.0, 0.0);

  EXPECT_NEAR(1.0, ComputeLongitudinalAffinity(pd2_1, gt2, let_config), kError);
  EXPECT_NEAR(1.0 - 5.0 / 7.5,
              ComputeLongitudinalAffinity(pd2_2, gt2, let_config), kError);
  EXPECT_NEAR(1.0 - 5.0 / 7.5,
              ComputeLongitudinalAffinity(pd2_3, gt2, let_config), kError);
  EXPECT_NEAR(1.0 - 5.0 / 7.5,
              ComputeLongitudinalAffinity(pd2_4, gt2, let_config), kError);
}

TEST(ComputeLetIoU, SuccessCenterAligned) {
  // Constructs a ground truth at near range.
  const Label::Box gt1 = BuildBox2d(1.0, 0.0, 1.0, 2.0, 0.0);
  // Same as gt1.
  const Label::Box pd1 = BuildBox2d(1.0, 0.0, 1.0, 2.0, 0.0);
  // Moves center along the line of sight but keep the shape.
  const Label::Box pd2 = BuildBox2d(1.5, 0.0, 1.0, 2.0, 0.0);
  // Move center along the line of sight and add some lateral error.
  const Label::Box pd3 = BuildBox2d(1.5, 1.0, 1.0, 2.0, 0.0);

  Config::LongitudinalErrorTolerantConfig::Location3D sensor_location;
  sensor_location.set_x(0.0);
  sensor_location.set_y(0.0);

  EXPECT_NEAR(
      1.0,
      ComputeLetIoU(pd1, gt1, sensor_location,
                    TolerantConfig::TYPE_CENTER_ALIGNED, Label::Box::TYPE_2D),
      kError);
  EXPECT_NEAR(
      1.0,
      ComputeLetIoU(pd2, gt1, sensor_location,
                    TolerantConfig::TYPE_CENTER_ALIGNED, Label::Box::TYPE_2D),
      kError);
  EXPECT_NEAR(
      1.0,
      ComputeLetIoU(pd3, gt1, sensor_location,
                    TolerantConfig::TYPE_CENTER_ALIGNED, Label::Box::TYPE_2D),
      kError);

  // Constructs an ground truth at near range.
  const Label::Box shifted_gt1 = BuildBox2d(1.0 + 1.0, 0.0, 1.0, 2.0, 0.0);
  // Same as gt1.
  const Label::Box shifted_pd1 = BuildBox2d(1.0 + 1.0, 0.0, 1.0, 2.0, 0.0);
  // Move center along line of sight and add some bearing error.
  const Label::Box shifted_pd3 = BuildBox2d(1.5 + 1.0, 1.0, 1.0, 2.0, 0.0);

  sensor_location.set_x(1.0);
  sensor_location.set_y(0.0);
  EXPECT_NEAR(
      1.0,
      ComputeLetIoU(shifted_pd1, shifted_gt1, sensor_location,
                    TolerantConfig::TYPE_CENTER_ALIGNED, Label::Box::TYPE_2D),
      kError);
  EXPECT_NEAR(
      1.0,
      ComputeLetIoU(shifted_pd3, shifted_gt1, sensor_location,
                    TolerantConfig::TYPE_CENTER_ALIGNED, Label::Box::TYPE_2D),
      kError);
}
TEST(ComputeLetIoU, RangeAligned) {
  // Constructs a ground truth at near range.
  const Label::Box gt1 = BuildBox2d(1.0, 0.0, 1.0, 2.0, 0.0);
  // Same as gt1.
  const Label::Box pd1 = BuildBox2d(1.0, 0.0, 1.0, 2.0, 0.0);
  // Moves center along the line of sight but keep the shape.
  const Label::Box pd2 = BuildBox2d(1.5, 0.0, 1.0, 2.0, 0.0);
  // Move center along the line of sight and add some lateral error.
  const Label::Box pd3 = BuildBox2d(1.5, 1.0, 1.0, 2.0, 0.0);
  // Move closer along the line of sight but keep the shape.
  const Label::Box pd4 = BuildBox2d(0.5, 0.0, 1.0, 2.0, 0.0);
  // Move closer along the line of sight but keep the shape.
  const Label::Box pd5 = BuildBox2d(-0.5, 0.0, 1.0, 2.0, 0.0);
  // Range-aligned pd3 to gt1.
  const Label::Box aligned_pd3 =
      BuildBox2d(9.0 / 13.0, 18.0 / 39.0, 1.0, 2.0, 0.0);

  Config::LongitudinalErrorTolerantConfig::Location3D sensor_location;
  sensor_location.set_x(0.0);
  sensor_location.set_y(0.0);

  EXPECT_NEAR(
      1.0,
      ComputeLetIoU(pd1, gt1, sensor_location,
                    TolerantConfig::TYPE_RANGE_ALIGNED, Label::Box::TYPE_2D),
      kError);
  EXPECT_NEAR(
      1.0,
      ComputeLetIoU(pd2, gt1, sensor_location,
                    TolerantConfig::TYPE_RANGE_ALIGNED, Label::Box::TYPE_2D),
      kError);
  EXPECT_NEAR(
      ComputeIoU(aligned_pd3, gt1, Label::Box::TYPE_2D),
      ComputeLetIoU(pd3, gt1, sensor_location,
                    TolerantConfig::TYPE_RANGE_ALIGNED, Label::Box::TYPE_2D),
      kError);

  // Constructs an ground truth at near range.
  const Label::Box shifted_gt1 = BuildBox2d(1.0 + 1.0, 0.0, 1.0, 2.0, 0.0);
  // Same as gt1.
  const Label::Box shifted_pd1 = BuildBox2d(1.0 + 1.0, 0.0, 1.0, 2.0, 0.0);
  // Move center along line of sight and add some bearing error.
  const Label::Box shifted_pd3 = BuildBox2d(1.5 + 1.0, 1.0, 1.0, 2.0, 0.0);

  sensor_location.set_x(1.0);
  sensor_location.set_y(0.0);

  EXPECT_NEAR(
      1.0,
      ComputeLetIoU(shifted_pd1, shifted_gt1, sensor_location,
                    TolerantConfig::TYPE_RANGE_ALIGNED, Label::Box::TYPE_2D),
      kError);
  EXPECT_NEAR(
      ComputeIoU(aligned_pd3, gt1, Label::Box::TYPE_2D),
      ComputeLetIoU(shifted_pd3, shifted_gt1, sensor_location,
                    TolerantConfig::TYPE_RANGE_ALIGNED, Label::Box::TYPE_2D),
      kError);
}

TEST(ComputeLetIoU, BetweenOriginAndGtOnlyRangeAligned) {
  // Constructs a ground truth at near range.
  const Label::Box gt1 = BuildBox2d(2.0, 0.0, 1.0, 2.0, 0.0);
  // Moves center along the line of sight but keep the shape.
  const Label::Box p1 = BuildBox2d(5, 0.0, 1.0, 2.0, 0.0);
  // Move closer along the line of sight but keep the shape.
  const Label::Box p2 = BuildBox2d(0.5, 0.0, 1.0, 2.0, 0.0);
  // Move closer along the line of sight but keep the shape.
  const Label::Box p3 = BuildBox2d(-0.5, 0.0, 1.0, 2.0, 0.0);

  Config::LongitudinalErrorTolerantConfig::Location3D sensor_location;
  sensor_location.set_x(0.0);
  sensor_location.set_y(0.0);

  // Prediction is further than ground truth compared to sensor origin.
  EXPECT_NEAR(0.0,
              ComputeLetIoU(
                  p1, gt1, sensor_location,
                  TolerantConfig::TYPE_BETWEEN_ORIGIN_AND_GT_ONLY_RANGE_ALIGNED,
                  Label::Box::TYPE_2D),
              kError);
  // Prediction is between sensor origin and gt.
  EXPECT_NEAR(1.0,
              ComputeLetIoU(
                  p2, gt1, sensor_location,
                  TolerantConfig::TYPE_BETWEEN_ORIGIN_AND_GT_ONLY_RANGE_ALIGNED,
                  Label::Box::TYPE_2D),
              kError);
  // Prediction is behind sensor origin in comparison to ground truth.
  EXPECT_NEAR(0.0,
              ComputeLetIoU(
                  p3, gt1, sensor_location,
                  TolerantConfig::TYPE_BETWEEN_ORIGIN_AND_GT_ONLY_RANGE_ALIGNED,
                  Label::Box::TYPE_2D),
              kError);

  // Constructs a ground truth at near range.
  const Label::Box gt2 = BuildBox2d(-2.0, 0.0, 1.0, 2.0, 0.0);
  // Same as gt1.
  const Label::Box pd8 = BuildBox2d(-5, 0.0, 1.0, 2.0, 0.0);
  // Moves center along the line of sight but keep the shape.
  const Label::Box pd9 = BuildBox2d(-0.5, 0.0, 1.0, 2.0, 0.0);
  // Moves center along the line of sight but keep the shape.
  const Label::Box pd10 = BuildBox2d(1.5, 0.0, 1.0, 2.0, 0.0);

  // Prediction is further than ground truth compared to sensor origin.
  EXPECT_NEAR(0.0,
              ComputeLetIoU(
                  pd8, gt2, sensor_location,
                  TolerantConfig::TYPE_BETWEEN_ORIGIN_AND_GT_ONLY_RANGE_ALIGNED,
                  Label::Box::TYPE_2D),
              kError);
  // Prediction is between sensor origin and gt.
  EXPECT_NEAR(1.0,
              ComputeLetIoU(
                  pd9, gt2, sensor_location,
                  TolerantConfig::TYPE_BETWEEN_ORIGIN_AND_GT_ONLY_RANGE_ALIGNED,
                  Label::Box::TYPE_2D),
              kError);
  // Prediction is behind sensor origin in comparison to ground truth.
  EXPECT_NEAR(0.0,
              ComputeLetIoU(
                  pd10, gt2, sensor_location,
                  TolerantConfig::TYPE_BETWEEN_ORIGIN_AND_GT_ONLY_RANGE_ALIGNED,
                  Label::Box::TYPE_2D),
              kError);
}

TEST(ComputeLetIoU, FurtherOnlyRangeAligned) {
  // Constructs a ground truth at near range.
  const Label::Box gt1 = BuildBox2d(2.0, 0.0, 1.0, 2.0, 0.0);
  // Moves center along the line of sight but keep the shape.
  const Label::Box p1 = BuildBox2d(5, 0.0, 1.0, 2.0, 0.0);
  // Move closer along the line of sight but keep the shape.
  const Label::Box p2 = BuildBox2d(0.5, 0.0, 1.0, 2.0, 0.0);
  // Move closer along the line of sight but keep the shape.
  const Label::Box p3 = BuildBox2d(-0.5, 0.0, 1.0, 2.0, 0.0);

  Config::LongitudinalErrorTolerantConfig::Location3D sensor_location;
  sensor_location.set_x(0.0);
  sensor_location.set_y(0.0);

  // Prediction is further than ground truth compared to sensor origin.
  EXPECT_NEAR(1.0,
              ComputeLetIoU(p1, gt1, sensor_location,
                            TolerantConfig::TYPE_FURTHER_ONLY_RANGE_ALIGNED,
                            Label::Box::TYPE_2D),
              kError);
  // Prediction is between sensor origin and gt.
  EXPECT_NEAR(0.0,
              ComputeLetIoU(p2, gt1, sensor_location,
                            TolerantConfig::TYPE_FURTHER_ONLY_RANGE_ALIGNED,
                            Label::Box::TYPE_2D),
              kError);
  // Prediction is behind sensor origin in comparison to ground truth.
  EXPECT_NEAR(0.0,
              ComputeLetIoU(p3, gt1, sensor_location,
                            TolerantConfig::TYPE_FURTHER_ONLY_RANGE_ALIGNED,
                            Label::Box::TYPE_2D),
              kError);

  // Constructs a ground truth at near range.
  const Label::Box gt2 = BuildBox2d(-2.0, 0.0, 1.0, 2.0, 0.0);
  // Same as gt1.
  const Label::Box pd8 = BuildBox2d(-5, 0.0, 1.0, 2.0, 0.0);
  // Moves center along the line of sight but keep the shape.
  const Label::Box pd9 = BuildBox2d(-0.5, 0.0, 1.0, 2.0, 0.0);
  // Moves center along the line of sight but keep the shape.
  const Label::Box pd10 = BuildBox2d(1.5, 0.0, 1.0, 2.0, 0.0);

  // Prediction is further than ground truth compared to sensor origin.
  EXPECT_NEAR(1.0,
              ComputeLetIoU(pd8, gt2, sensor_location,
                            TolerantConfig::TYPE_FURTHER_ONLY_RANGE_ALIGNED,
                            Label::Box::TYPE_2D),
              kError);
  // Prediction is between sensor origin and gt.
  EXPECT_NEAR(0.0,
              ComputeLetIoU(pd9, gt2, sensor_location,
                            TolerantConfig::TYPE_FURTHER_ONLY_RANGE_ALIGNED,
                            Label::Box::TYPE_2D),
              kError);
  // Prediction is behind sensor origin in comparison to ground truth.
  EXPECT_NEAR(0.0,
              ComputeLetIoU(pd10, gt2, sensor_location,
                            TolerantConfig::TYPE_FURTHER_ONLY_RANGE_ALIGNED,
                            Label::Box::TYPE_2D),
              kError);
}

TEST(ComputeLetIoU, AnyCloserOnlyRangeAligned) {
  // Constructs a ground truth at near range.
  const Label::Box gt1 = BuildBox2d(2.0, 0.0, 1.0, 2.0, 0.0);
  // Moves center along the line of sight but keep the shape.
  const Label::Box p1 = BuildBox2d(5, 0.0, 1.0, 2.0, 0.0);
  // Move closer along the line of sight but keep the shape.
  const Label::Box p2 = BuildBox2d(0.5, 0.0, 1.0, 2.0, 0.0);
  // Move closer along the line of sight but keep the shape.
  const Label::Box p3 = BuildBox2d(-0.5, 0.0, 1.0, 2.0, 0.0);

  Config::LongitudinalErrorTolerantConfig::Location3D sensor_location;
  sensor_location.set_x(0.0);
  sensor_location.set_y(0.0);

  // Prediction is further than ground truth compared to sensor origin.
  EXPECT_NEAR(0.0,
              ComputeLetIoU(p1, gt1, sensor_location,
                            TolerantConfig::TYPE_ANY_CLOSER_ONLY_RANGE_ALIGNED,
                            Label::Box::TYPE_2D),
              kError);
  // Prediction is between sensor origin and gt.
  EXPECT_NEAR(1.0,
              ComputeLetIoU(p2, gt1, sensor_location,
                            TolerantConfig::TYPE_ANY_CLOSER_ONLY_RANGE_ALIGNED,
                            Label::Box::TYPE_2D),
              kError);
  // Prediction is behind sensor origin in comparison to ground truth.
  EXPECT_NEAR(1.0,
              ComputeLetIoU(p3, gt1, sensor_location,
                            TolerantConfig::TYPE_ANY_CLOSER_ONLY_RANGE_ALIGNED,
                            Label::Box::TYPE_2D),
              kError);

  // Constructs a ground truth at near range.
  const Label::Box gt2 = BuildBox2d(-2.0, 0.0, 1.0, 2.0, 0.0);
  // Same as gt1.
  const Label::Box pd8 = BuildBox2d(-5, 0.0, 1.0, 2.0, 0.0);
  // Moves center along the line of sight but keep the shape.
  const Label::Box pd9 = BuildBox2d(-0.5, 0.0, 1.0, 2.0, 0.0);
  // Moves center along the line of sight but keep the shape.
  const Label::Box pd10 = BuildBox2d(1.5, 0.0, 1.0, 2.0, 0.0);

  // Prediction is further than ground truth compared to sensor origin.
  EXPECT_NEAR(0.0,
              ComputeLetIoU(pd8, gt2, sensor_location,
                            TolerantConfig::TYPE_ANY_CLOSER_ONLY_RANGE_ALIGNED,
                            Label::Box::TYPE_2D),
              kError);
  // Prediction is between sensor origin and gt.
  EXPECT_NEAR(1.0,
              ComputeLetIoU(pd9, gt2, sensor_location,
                            TolerantConfig::TYPE_ANY_CLOSER_ONLY_RANGE_ALIGNED,
                            Label::Box::TYPE_2D),
              kError);
  // Prediction is behind sensor origin in comparison to ground truth.
  EXPECT_NEAR(1.0,
              ComputeLetIoU(pd10, gt2, sensor_location,
                            TolerantConfig::TYPE_ANY_CLOSER_ONLY_RANGE_ALIGNED,
                            Label::Box::TYPE_2D),
              kError);
}

}  // namespace
}  // namespace open_dataset
}  // namespace waymo
