import os.path
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    package_path = get_package_share_directory('faster_lio')

    config_path_cmd = DeclareLaunchArgument(
            'config_path',
            default_value=os.path.join(package_path, 'config', 'mid360.yaml'),
            description='Path to lidar configuration'
            )
    config_path = LaunchConfiguration('config_path')

    use_rviz_cmd = DeclareLaunchArgument(
            'rviz',
            default_value='true',
            description='Launch Rviz'
            )
    use_rviz = LaunchConfiguration('rviz')

    rviz_config_path_cmd = DeclareLaunchArgument(
            'rviz_config',
            default_value=os.path.join(package_path, 'rviz', 'rviz.rviz'),
            description='Path to rviz configuration'
            )
    rviz_config = LaunchConfiguration('rviz_config')

    bag_file_cmd = DeclareLaunchArgument(
            'bag_file',
            description='Path to the bag to use'
            )
    bag_file = LaunchConfiguration('bag_file')

    rviz_node = Node(
            package='rviz2',
            executable='rviz2',
            arguments=['-d', rviz_config],
            condition=IfCondition(use_rviz)
            )

    faster_lio_node = Node(
            package='faster_lio',
            executable='run_mapping_offline',
            name='faster_lio',
            output='screen',
            arguments=[
                '--bag_file', bag_file,
                '--config_file', config_path
            ],
            parameters=[config_path]
            )

    return LaunchDescription([
        config_path_cmd,
        use_rviz_cmd,
        rviz_config_path_cmd,
        bag_file_cmd,
        faster_lio_node,
        rviz_node
        ])
