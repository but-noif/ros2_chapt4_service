#include "rclcpp/rclcpp.hpp"
#include <memory>
#include "chapt4_interfaces/srv/patrol.hpp"
#include "turtlesim/msg/pose.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include <chrono>
#include <cmath>


#include "rcl_interfaces/msg/set_parameters_result.hpp"


using namespace std;
using namespace std::chrono_literals;
using Patrol = chapt4_interfaces::srv::Patrol;

using SetParametersResult = rcl_interfaces::msg::SetParametersResult;



class TurtleController : public rclcpp::Node
{
    private:
        //创建订阅者智能指针
        rclcpp::Subscription<turtlesim::msg::Pose> :: SharedPtr subscriber_;
        //创建发布者智能指针
        rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr publisher_;
        //添加Patrol类型服务共享指针 patrol_server_ 为成员变量
        std::shared_ptr<rclcpp::Service<Patrol>> patrol_server_;
        
        double target_x_{1.0};
        double target_y_{8.0};
        double target_theta_{0.0}; //目标朝向角度（弧度）
        double k_{2.0}; //比例系数
        double max_speed_{1.0}; //最大速度
        double k_angle_{5.6};//角度比例系数
        bool reached_target_{false}; //是否已到达目标点（进入转向阶段）

        //参数回调句柄（必须作为成员变量保存，否则回调会被注销）
        rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr parameters_callback_handle_;

        


    public:
        //构造函数
        TurtleController() : Node("turtle_controller")
        {
            //初始化发布者指针
            publisher_ = this->create_publisher<geometry_msgs::msg::Twist>("/turtle1/cmd_vel",10);
            //初始化订阅者指针
            subscriber_ = this->create_subscription<turtlesim::msg::Pose>(
            "/turtle1/pose",
            10,
            std::bind(&TurtleController::on_pose_received_,this,std::placeholders::_1));  
            
            //添加参数声明
            //第一个参数为参数名，
            this->declare_parameter("k",2.0);
            this->declare_parameter("max_speed",1.0);

            this->get_parameter("k",k_);
            this->get_parameter("max_speed",max_speed_);



            //添加参数设置回调
            parameters_callback_handle_ = this->add_on_set_parameters_callback(
                                            [&](const std::vector<rclcpp::Parameter> &params)->
                                            SetParametersResult
                                                {
                                                    //遍历参数
                                                    for(const auto &param : params)
                                                    {
                                                        //只接受 double 类型，避免传整数时 as_double() 抛异常
                                                        if(param.get_type() != rclcpp::ParameterType::PARAMETER_DOUBLE)
                                                        {
                                                            RCLCPP_WARN(this->get_logger(),"参数%s不是double类型,已忽略",param.get_name().c_str());
                                                            continue;
                                                        }
                                                        if(param.get_name() == "k")
                                                        {
                                                            k_ = param.as_double();
                                                        }
                                                        else if(param.get_name() == "max_speed")
                                                        {
                                                            max_speed_ = param.as_double();
                                                        }
                                                        RCLCPP_INFO(this->get_logger(),"更新参数%s值为:%s",param.get_name().c_str(),param.value_to_string().c_str());
                                                    }
                                                    auto result = SetParametersResult();
                                                    result.successful = true;
                                                    return result;
                                                }
                                            );


            //创建服务指针
            patrol_server_ = this->create_service<Patrol>
            (
            "patrol",
            [&](std::shared_ptr<Patrol::Request> request,std::shared_ptr<Patrol::Response> response)->
                void
                { 
                    
                    if(
                        (0 < request->target_x) && (request->target_x < 12.0f)
                        &&
                        (0 < request->target_y) && (request->target_y < 12.0f)
                      )
                    {
                        target_x_ = request->target_x;
                        target_y_ = request->target_y;   
                        response->result = Patrol::Response::SUCCESS;
                    }  
                    else
                    {

                        response->result = Patrol::Response::FAIL;
                    }
                }

            );    

        }


    private:
        //订阅回调函数,函数参数等价于std:shared_Ptr<turtlesim::msg::Pose> pose
        //这里的参数类型不是你随便写的，它的参数由你的订阅类型决定
        //它告诉ros2，我这订阅的是/turtle/pose这个话题的，你传进来的参数都应该是turtlesim::msg::Pose这个类型的
        void on_pose_received_(const turtlesim::msg::Pose::SharedPtr pose)//参数：收到数据的共享指针
        {
            //获取当前位置
            auto current_x = pose->x;
            auto current_y = pose->y;

            

            auto msg = geometry_msgs::msg::Twist();

            //计算当前误差
            double dx = target_x_ - pose->x;
            double dy = target_y_ - pose->y;

            double distance_error = std::hypot(dx, dy);
            double desired_theta = std::atan2(dy, dx);

            double angle_error = std::atan2(
                std::sin(desired_theta - pose->theta),
                std::cos(desired_theta - pose->theta)
            );

            if (distance_error > 0.1)
            {
                // 限制角速度，避免转得过猛
                double angular_speed = k_angle_ * angle_error;

                if (angular_speed > 2.0)
                    angular_speed = 2.0;
                else if (angular_speed < -2.0)
                    angular_speed = -2.0;

                msg.angular.z = angular_speed;

                // 朝向误差较大时，先原地转向
                if (std::fabs(angle_error) > 0.1)
                {
                    msg.linear.x = 0.0;
                }
                else
                {
                    // 对准后再前进，并限制最大线速度
                    msg.linear.x = k_ * distance_error;

                    if (msg.linear.x > max_speed_)
                        msg.linear.x = max_speed_;
                    else if(msg.linear.x < -max_speed_)
                        msg.linear.x = -max_speed_;
                }
            }
            else
            {
                // 到达目标位置，进入最终朝向调整阶段
                reached_target_ = true;

                msg.linear.x = 0.0;
                msg.angular.z = 0.0;
            }
             RCLCPP_INFO(get_logger(),"当前:x=%f,y=%f,theta= %f,dis_err=%f,ang_err=%f,",current_x,current_y,pose->theta,distance_error,angle_error);   
            //发布话题
            publisher_->publish(msg);

        }        

};



int main(int argc,char*argv[])
{
    rclcpp::init(argc,argv);
    auto node = std::make_shared<TurtleController>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}

