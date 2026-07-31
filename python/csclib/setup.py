from setuptools import setup, Extension, find_packages

import shutil, glob, os

setup(name = 'csclib',
      ext_modules = [
          Extension(
              name = 'ccsclib',
              sources = ['src/csclib/pycsclib.c'],
              extra_compile_args=['-O3', '-Wno-unused-function', '-Wno-unused-variable'],
              #extra_compile_args=['-O0', '-g', '-Wno-unused-function', '-Wno-unused-variable'],
           )],
      package_data={"csclib": ["*.h"]}
)
