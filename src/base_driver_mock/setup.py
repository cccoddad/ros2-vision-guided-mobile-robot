from glob import glob
from setuptools import find_packages, setup

package_name = 'base_driver_mock'

setup(
    name=package_name,
    version='0.1.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages', ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        ('share/' + package_name + '/launch', glob('launch/*.launch.py')),
        ('share/' + package_name + '/config', glob('config/*.yaml')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Robot Project',
    maintainer_email='maintainer@example.com',
    description='Software-in-the-loop differential-drive base simulation.',
    license='Apache-2.0',
    entry_points={
        'console_scripts': [
            'mock_base = base_driver_mock.mock_base:main',
        ],
    },
)
