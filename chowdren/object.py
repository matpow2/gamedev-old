# Copyright (c) Mathias Kaerlev
# See LICENSE for details.

import os
import glob
import imp
from chowdren.data import ObjectType

OBJECTS_DIRECTORY = os.path.join(os.getcwd(), 'objects')

class state:
    objects = None

def get_objects():
    if state.objects is None:
        state.objects = {}
        for name in os.listdir(OBJECTS_DIRECTORY):
            object_dir = os.path.join(OBJECTS_DIRECTORY, name)
            if not os.path.isdir(object_dir):
                continue
            try:
                module = imp.load_source(name, os.path.join(object_dir, 
                    'edittime.py'))
                object_type = module.get_object()
                state.objects[object_type.__name__] = object_type
            except AttributeError:
                continue

    return state.objects

class ObjectBase(object):
    def __init__(self, project, data = None):
        self.project = project
        self.get_image = project.get_image
        self.save_image = project.save_image
        if data is None:
            self.initialize()
        else:
            self.read(data)

    def initialize(self):
        pass

    def read(self, data):
        pass

    def write(self, data):
        pass

    @classmethod
    def get_class_name(cls):
        return cls.__name__

    def get_data(self):
        object_type = ObjectType()
        object_type.name = self.get_class_name()
        object_type.type_id = self.id
        object_type.data = {}
        self.write(object_type.data)
        return object_type

    # for runtime

    def get_parameters(self):
        return []

    @classmethod
    def get_runtime(cls):
        name = cls.__module__
        return (name, os.path.join(OBJECTS_DIRECTORY, name, 'runtime.cpp'))