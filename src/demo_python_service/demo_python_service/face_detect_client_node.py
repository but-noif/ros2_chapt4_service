import rclpy
from rclpy.node import Node
from chapt4_interfaces.srv import FaceDetector
from sensor_msgs.msg import Image
import cv2
from cv_bridge import CvBridge
from ament_index_python.packages import get_package_share_directory



from rcl_interfaces.srv import SetParameters
from rcl_interfaces.msg import Parameter,ParameterValue,ParameterType


class FaceDetectorClient(Node):
    def __init__(self):
        super().__init__('face_detect_client')
        #创建客户端
        #只有前两个参数是必填，后两个有默认值了可以不用填
        self.client = self.create_client(FaceDetector,'face_detect')

        self.bridge = CvBridge()
        self.three_image_path = get_package_share_directory(
        'demo_python_service')+'/resource/three.jpg'
        self.image = cv2.imread(self.three_image_path)

    #发送请求并处理结果
    def send_request(self):
        # 1 判断服务是否上线
        while self.client.wait_for_service(1.0) is False:
            self.get_logger().info(f'等待服务器上线...')
        
        #到这一步就是服务器已经响应了，所以我们要开始发请求了
        # 2 构造request
        request = FaceDetector.Request()
        request.image = self.bridge.cv2_to_imgmsg(self.image)

        # 3 发送并spin等待服务处理完成
        #把请求异步发送给服务端，并返回一个保存未来结果的future对象，async表示异步的意思，异步就是不会一直在这里等待服务端处理完成
        future = self.client.call_async(request)

        #该方法内部会在执行spin的同时检测future是否完成，当请求完成后，该方法会自动退出
        rclpy.spin_until_future_complete(self,future)

        #4 根据处理结果
        response = future.result()
        #这里response.没有提示number和use_time是因为Pylance不知道你的具体response类型，正常现象
        self.get_logger().info(f'接收到响应：图像中共有：{response.number}张脸，耗时{response.use_time}')
        #self.show_face_locations(response)




    #下面这个函数是根据response绘制并显示
    def show_face_locations(self,response):
        for i in range(response.number):
            top = response.top[i]
            right = response.right[i]
            bottom = response.bottom[i]
            left = response.left[i]

            cv2.rectangle(
                self.image,
                (left, top),
                (right, bottom),
                (255, 0, 0),
                2
            )

        cv2.imshow('Face Detection Result', self.image)
        cv2.waitKey(0)



    def call_set_parameters(self,parameters):
        #创建一个客户端，等待服务端上线
        client = self.create_client(SetParameters,'/face_detection_node/set_parameters')
        while not client.wait_for_service(timeout_sec = 1.0):
            self.get_logger().info('等待服务端上线中...')

        #创建请求对象
        request =  SetParameters.Request();   
        request.parameters = parameters

        #异步调用
        future = client.call_async(request)
        rclpy.spin_until_future_complete(self,future)

        #处理响应
        response = future.result()
        return response

    

    def update_detect_model(self,model):
        #创建一个参数对象
        param = Parameter()
        param.name =  "face_locations_model"
        #创建参数值并赋值
        new_model_value = ParameterValue()
        new_model_value.type = ParameterType.PARAMETER_STRING
        new_model_value.string_value = model
        param.value = new_model_value
        #请求更新参数并处理
        response = self.call_set_parameters([param])
        for result in response.results:
            if result.successful:
                self.get_logger().info(f'参数{param.name}设置为{model}')
            else:
                self.get_logger().info(f'参数配置失败，原因为:{result.reason}')


def main(args=None):
    rclpy.init(args=args)

    face_detect_client = FaceDetectorClient()

    face_detect_client.update_detect_model('hog')
    face_detect_client.send_request()

    face_detect_client.update_detect_model('cnn')
    face_detect_client.send_request()

    rclpy.spin(face_detect_client)
    rclpy.shutdown()


