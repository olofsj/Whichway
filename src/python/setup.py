from distutils.core import setup, Extension

setup (name = 'Whichway',
       version = '0.1',
       description = 'Routing module',
       ext_modules = [Extension('whichway', 
           sources = ['whichway_python.c'], 
           include_dirs=['../lib/'],
           extra_objects=['/usr/local/lib/libwhichway.so'],
           )])

