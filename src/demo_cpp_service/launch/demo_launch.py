import launch
import launch_ros

def generate_launch_description():
    action_run_node_turtle_control = launch_ros.actions.Node(
        package = 'demo_cpp_service',
        executable = 'turtle_control',
        output = 'screen',
    )

    action_run_node_patrol_client = launch_ros.actions.Node(
        package = 'demo_cpp_service',
        executable = 'patrol_client',
        output = 'log',       
    )

    action_run_node_turtlesim = launch_ros.actions.Node(
        package = 'turtlesim',
        executable = 'turtlesim_node',
        output = 'both',       
    )

    #合成启动描述并返回
    launch_description = launch.LaunchDescription(
        [
            action_run_node_turtle_control,
            action_run_node_patrol_client,
            action_run_node_turtlesim
        ]
    )
    return launch_description