//
// Created by xiang on 2021/10/9.
//

#include <gflags/gflags.h>
#include <unistd.h>
#include <csignal>

#include "rclcpp/rclcpp.hpp"

#include <rosbag2_cpp/reader.hpp>
#include <rosbag2_cpp/readers/sequential_reader.hpp>
#include <rosbag2_storage/serialized_bag_message.hpp>
#include <rosbag2_storage/storage_options.hpp>
#include <rclcpp/serialization.hpp>
#include <rclcpp/serialized_message.hpp>

#include <livox_ros_driver2/msg/custom_msg.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

#include "laser_mapping.h"
#include "utils.h"

/// run faster-LIO in offline mode

DEFINE_string(config_file, "./config/avia.yaml", "path to config file");
DEFINE_string(bag_file, "/home/xiang/Data/dataset/fast_lio2/avia/2020-09-16-quick-shack.bag", "path to the ros bag");
DEFINE_string(time_log_file, "./Log/time.log", "path to time log file");
DEFINE_string(traj_log_file, "./Log/traj.txt", "path to traj log file");

DEFINE_string(r, "", "");
DEFINE_string(ros_args, "", "");
DEFINE_string(params_file, "", "");

void SigHandle(int sig) {
    faster_lio::options::FLAG_EXIT = true;
    std::cout << "catch sig" << sig << std::endl;
}

int main(int argc, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    FLAGS_stderrthreshold = google::INFO;
    FLAGS_colorlogtostderr = true;
    google::InitGoogleLogging(argv[0]);

    rclcpp::init(argc, argv);

    const std::string bag_file = FLAGS_bag_file;
    const std::string config_file = FLAGS_config_file;

    auto laser_mapping = std::make_shared<faster_lio::LaserMapping>();
    if (!laser_mapping->InitWithoutROS(FLAGS_config_file)) {
        LOG(ERROR) << "laser mapping init failed.";
        return -1;
    }

    /// handle ctrl-c
    signal(SIGINT, SigHandle);

    // just read the bag and send the data
    LOG(INFO) << "Opening rosbag, be patient";

    // Configure and open the ROS 2 bag using rosbag2_cpp
    rosbag2_cpp::Reader reader(std::make_unique<rosbag2_cpp::readers::SequentialReader>());
    rosbag2_storage::StorageOptions storage_options;
    storage_options.uri = FLAGS_bag_file;   // path to your .db3 folder
    storage_options.storage_id = "sqlite3"; // or whatever storage plugin you use

    rosbag2_cpp::ConverterOptions converter_options{};
    // "cdr" is the default serialization format in most ROS 2 distros
    converter_options.input_serialization_format = "cdr";
    converter_options.output_serialization_format = "cdr";

    reader.open(storage_options, converter_options);

    rclcpp::Serialization<livox_ros_driver2::msg::CustomMsg> livox_serial;
    rclcpp::Serialization<sensor_msgs::msg::PointCloud2> pcl_serial;
    rclcpp::Serialization<sensor_msgs::msg::Imu> imu_serial;

    LOG(INFO) << "Go!";
    while (reader.has_next() && !faster_lio::options::FLAG_EXIT) {
        auto bag_message = reader.read_next();
        const auto &topic_name = bag_message->topic_name;

        rclcpp::SerializedMessage serialized_msg(*bag_message->serialized_data);

        if (topic_name == "/livox/lidar") {
            livox_ros_driver2::msg::CustomMsg livox_msg;
            livox_serial.deserialize_message(&serialized_msg, &livox_msg);

            faster_lio::Timer::Evaluate(
                [&]() {
                laser_mapping->LivoxPCLCallBack(std::make_shared<livox_ros_driver2::msg::CustomMsg>(livox_msg));
                laser_mapping->Run();
                },
                "Laser Mapping Single Run"
            );
        }
        else if (topic_name == "/imu/data") {
            sensor_msgs::msg::Imu imu_msg;
            imu_serial.deserialize_message(&serialized_msg, &imu_msg);
            laser_mapping->IMUCallBack(std::make_shared<sensor_msgs::msg::Imu>(imu_msg));
        }
    }

    /* for (const rosbag::MessageInstance &m : rosbag::View(bag)) { */
    /*     auto livox_msg = m.instantiate<livox_ros_driver::CustomMsg>(); */
    /*     if (livox_msg) { */
    /*         faster_lio::Timer::Evaluate( */
    /*             [&laser_mapping, &livox_msg]() { */
    /*                 laser_mapping->LivoxPCLCallBack(livox_msg); */
    /*                 laser_mapping->Run(); */
    /*             }, */
    /*             "Laser Mapping Single Run"); */
    /*         continue; */
    /*     } */

    /*     auto point_cloud_msg = m.instantiate<sensor_msgs::PointCloud2>(); */
    /*     if (point_cloud_msg) { */
    /*         faster_lio::Timer::Evaluate( */
    /*             [&laser_mapping, &point_cloud_msg]() { */
    /*                 laser_mapping->StandardPCLCallBack(point_cloud_msg); */
    /*                 laser_mapping->Run(); */
    /*             }, */
    /*             "Laser Mapping Single Run"); */
    /*         continue; */
    /*     } */

    /*     auto imu_msg = m.instantiate<sensor_msgs::Imu>(); */
    /*     if (imu_msg) { */
    /*         laser_mapping->IMUCallBack(imu_msg); */
    /*         continue; */
    /*     } */

    /*     if (faster_lio::options::FLAG_EXIT) { */
    /*         break; */
    /*     } */
    /* } */

    LOG(INFO) << "finishing mapping";
    laser_mapping->Finish();

    /// print the fps
    double fps = 1.0 / (faster_lio::Timer::GetMeanTime("Laser Mapping Single Run") / 1000.);
    LOG(INFO) << "Faster LIO average FPS: " << fps;

    LOG(INFO) << "save trajectory to: " << FLAGS_traj_log_file;
    laser_mapping->Savetrajectory(FLAGS_traj_log_file);

    faster_lio::Timer::PrintAll();
    faster_lio::Timer::DumpIntoFile(FLAGS_time_log_file);

    return 0;
}
