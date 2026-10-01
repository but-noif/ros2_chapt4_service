from setuptools import find_packages, setup

package_name = 'demo_python_service'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        ('share/' + package_name+"/resource", ['resource/default.jpg']),
        #目标路径(一般在install，也就是供ros查找的地方) ， 源文件路径（一般填的就是当前你加进来的文件相对你这个setup文件的相对路径）

        ('share/' + package_name+"/resource", ['resource/three.jpg']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='rw',
    maintainer_email='2192704798@qq.com',
    description='TODO: Package description',
    license='Apache-2.0',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
        'console_scripts': [
           # 'learn_face_detect = demo_python_service.learn_python_detect:main',
            'face_detect_node = demo_python_service.face_detect_node:main',
            'face_detect_client_node = demo_python_service.face_detect_client_node:main',
        ],
    },
)
