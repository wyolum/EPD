from distutils.core import setup
import py2exe, os

setup(
    name="wifit",
    version=0,
    data_files=[('', ['DEFAULT.PNG', "unifont.wff", "unifont.hex"])],
    # options = {'py2exe': {'bundle_files': 1, 'compressed': True}},
    windows = [{'script': "wifit.py"}],
    author="Justin Shaw",
    email="wyojustin@gmail.com",
    zipfile=None,
)
