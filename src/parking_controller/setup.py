from glob import glob
from setuptools import find_packages, setup


package_name = 'parking_controller'

setup(
    name=package_name,
    version='0.1.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages', ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        ('share/' + package_name + '/launch', glob('launch/*.launch.py')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Robot Project',
    maintainer_email='maintainer@example.com',
    description='Action-based AprilTag parking controller for software-in-the-loop simulation.',
    license='Apache-2.0',
    entry_points={
        'console_scripts': [
            'parking_controller = parking_controller.parking_controller:main',
        ],
    },
)
