from distutils.core import setup
# import py2exe, os

setup(
    data_files = [('', ['DEFAULT.PNG'])],
    # options = {'py2exe': {'bundle_files': 1, 'compressed': True}},
    windows = [{'script': "wifit.py"}],
    zipfile = None,
)
