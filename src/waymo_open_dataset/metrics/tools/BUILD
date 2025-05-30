load("@rules_python//python:py_binary.bzl", "py_binary")

package(
    default_applicable_licenses = ["//waymo_open_dataset:license"],
    default_visibility = [
        "//visibility:public",
    ],
)

licenses(["notice"])

filegroup(
    name = "fakedata",
    srcs = [
        "fake_ground_truths.bin",
        "fake_predictions.bin",
        "fake_segmentation_groundtruths.bin",
        "fake_segmentation_predictions.bin",
    ],
)

cc_binary(
    name = "compute_detection_metrics_main",
    srcs = ["compute_detection_metrics_main.cc"],
    deps = [
        "//waymo_open_dataset:label_cc_proto",
        "//waymo_open_dataset/metrics:config_util",
        "//waymo_open_dataset/metrics:detection_metrics",
        "//waymo_open_dataset/protos:breakdown_cc_proto",
        "//waymo_open_dataset/protos:metrics_cc_proto",
        "@com_google_absl//absl/strings",
    ],
)

cc_binary(
    name = "compute_segmentation_metrics_main",
    srcs = ["compute_segmentation_metrics_main.cc"],
    deps = [
        "//waymo_open_dataset:dataset_cc_proto",
        "//waymo_open_dataset/metrics:segmentation_metrics",
        "//waymo_open_dataset/protos:segmentation_cc_proto",
        "//waymo_open_dataset/protos:segmentation_metrics_cc_proto",
        "@com_google_glog//:glog",
        "@zlib",
    ],
)

cc_binary(
    name = "compute_tracking_metrics_main",
    srcs = ["compute_tracking_metrics_main.cc"],
    deps = [
        "//waymo_open_dataset:label_cc_proto",
        "//waymo_open_dataset/metrics:config_util",
        "//waymo_open_dataset/metrics:tracking_metrics",
        "//waymo_open_dataset/protos:breakdown_cc_proto",
        "//waymo_open_dataset/protos:metrics_cc_proto",
    ],
)

cc_binary(
    name = "create_submission",
    srcs = ["create_submission.cc"],
    deps = [
        "//waymo_open_dataset:dataset_cc_proto",
        "//waymo_open_dataset:label_cc_proto",
        "//waymo_open_dataset/protos:metrics_cc_proto",
        "//waymo_open_dataset/protos:submission_cc_proto",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_glog//:glog",
    ],
)

cc_binary(
    name = "compute_detection_let_metrics_main",
    srcs = ["compute_detection_let_metrics_main.cc"],
    deps = [
        "//waymo_open_dataset:dataset_cc_proto",
        "//waymo_open_dataset:label_cc_proto",
        "//waymo_open_dataset/metrics:config_util",
        "//waymo_open_dataset/metrics:detection_metrics",
        "//waymo_open_dataset/protos:breakdown_cc_proto",
        "//waymo_open_dataset/protos:metrics_cc_proto",
        "@com_google_absl//absl/strings",
    ],
)

py_binary(
    name = "create_prediction_file_example",
    srcs = ["create_prediction_file_example.py"],
    deps = [
        "//waymo_open_dataset:dataset_proto_py_pb2",
        "//waymo_open_dataset:label_proto_py_pb2",
        "//waymo_open_dataset/protos:metrics_proto_py_pb2",
    ],
)

py_binary(
    name = "create_segmentation_prediction_file_example",
    srcs = ["create_segmentation_prediction_file_example.py"],
    deps = [
        "//waymo_open_dataset:dataset_proto_py_pb2",
        "//waymo_open_dataset/protos:segmentation_metrics_proto_py_pb2",
        "//waymo_open_dataset/protos:segmentation_proto_py_pb2",
        "//waymo_open_dataset/protos:segmentation_submission_proto_py_pb2",
    ],
)
