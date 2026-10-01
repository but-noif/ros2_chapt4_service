import rclpy
from rclpy.node import Node
from chapt4_interfaces.srv import FaceDetector
from ament_index_python.packages import get_package_share_directory
from cv_bridge import CvBridge #用于转换格式
import cv2
import face_recognition
import time



from rcl_interfaces.msg import SetParametersResult


class FaceDetectorionNode(Node):
    def __init__(self):
        super().__init__('face_detection_node')
        self.bridge = CvBridge()
        self.service = self.create_service(FaceDetector,'face_detect',self.service_callback)
        self.defaut_image_path = get_package_share_directory('demo_python_service')+'/resource/default.jpg'


        #声明参数
        self.declare_parameter('face_locations_upsample_times',1)
        self.declare_parameter('model',"hog")
        #获取参数
        self.upsample_times = self.get_parameter("face_locations_upsample_times").value
        self.model = self.get_parameter("model").value


        #添加参数更新回调
        #这个方法（函数）你就记得是专门用来添加参数更新回调的，然后注意是在init构造函数里面调用的
        self.add_on_set_parameters_callback(self.parameter_callback)

    #定义参数回调函数
    def parameter_callback(self,parameters):
            for parameter in parameters:
                self.get_logger().info(
                    f'参数{parameter.name}设置为：{parameter.value}')
                if parameter.name == 'face_locations_upsample_times':
                    self.upsample_times = parameter.value
                if parameter.name == 'model':
                    self.model = parameter.value
                    
            return SetParametersResult(successful=True)




    def service_callback(self,request,response):
        if request.image.data:
            cv_image = self.bridge.imgmsg_to_cv2(request.image)
        else:
            cv_image = cv2.imread(self.defaut_image_path)

        start_time = time.time()
        self.get_logger().info('加载完图像，开始检测')

        face_locations = face_recognition.face_locations(
            cv_image,
            number_of_times_to_upsample=self.upsample_times,
            model=self.model
        )

        end_time = time.time()
        self.get_logger().info(
            f'检测完成，耗时 {end_time - start_time}'
        )

        response.number = len(face_locations)
        response.use_time = end_time - start_time

        for top, right, bottom, left in face_locations:
            response.top.append(top)
            response.right.append(right)
            response.bottom.append(bottom)
            response.left.append(left)
        return response


def main(args= None):
    rclpy.init()
    node = FaceDetectorionNode()
    rclpy.spin(node)
    rc;py.shutdown()
