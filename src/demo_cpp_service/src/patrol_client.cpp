#include "rclcpp/rclcpp.hpp"
#include "chapt4_interfaces/srv/patrol.hpp"
using Patrol = chapt4_interfaces::srv::Patrol;
#include <chrono> //引入时间相关头文件
#include <ctime>

using namespace std::chrono_literals;//这一句是能直接让你用时间字面量 10ms 20s 1min



class PatrolClient:public rclcpp::Node
{
    private:
        //创建客户端智能指针
        rclcpp::Client<Patrol>::SharedPtr client_;
        //ros2定时器类型智能指针
        rclcpp::TimerBase::SharedPtr timer_;
        


    private:
        void timer_callback()
        {
            //1等待服务端上线
            while (!client_->wait_for_service())
            {
                RCLCPP_INFO(this->get_logger(),"等待服务器上线中");
            }

            //服务器已上线

            //初始化指针
            auto request = std::make_shared<Patrol::Request>() ;
            request->target_x = rand() % 15;
            request->target_y = rand() % 15;
            RCLCPP_INFO(this->get_logger(),"请求巡逻：(%f,%f)",request->target_x,request->target_y);
            
            //发送异步请求
            this->client_->async_send_request
            (
                request,
                [&](rclcpp::Client<Patrol>::SharedFuture result_future) -> void
                {
                    auto response = result_future.get();
                    if(response->result == Patrol::Response::SUCCESS)
                    {
                        RCLCPP_INFO(this->get_logger(),"目标点处理成功");
                    }
                    else if(response->result == Patrol::Response::FAIL)
                    {
                        RCLCPP_INFO(this->get_logger(),"目标点处理失败") ;
                    }
                    
                }
        
            );
            
            //伪代码
        }

    public:
        //构造函数
        PatrolClient() : Node("patrol_client")
        {
            //初始化客户端智能指针
            client_ = this->create_client<Patrol>("patrol");  
            //初始化定时器智能指针

            //std::bind起到一个绑定的作用，把调用的对象和被调用的函数绑定在一起
            //也可以直接用lambda函数去替代它
            //写this->timer_callback是不行，因为它仍然没有生成一个以后可以调用的回调对象
            //写this->timer_callback()则是立即调用函数，并把它的返回值void传回去，也不行
            //std::bind(&PatrolClient::timer_callback, this)，则是把成员函数和当前对象绑定起来，生成稍后可以调用的回调函数
            timer_ = this->create_wall_timer(10s,std::bind(&PatrolClient::timer_callback,this));

            srand(time(NULL));//初始化随机数种子，使用当前时间作为种子
            RCLCPP_INFO(this->get_logger(),"客户端节点创建成功");
        }

  
};

int main(int argc, char **argv)
{
    rclcpp::init(argc,argv);
    auto node = std::make_shared<PatrolClient>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;

}