# Copyright (c) Mathias Kaerlev
# See LICENSE for details.

import os

from chowdren.image import Image
from chowdren.common import IDPool
from chowdren.object import get_objects
from chowdren.data import CodeData

class ProjectManager(object):
    base_dir = None
    def __init__(self, directory = None):
        self.base_dir = directory
        self.object_types = {}
        self.object_type_ids = IDPool()
        self.images = {}
        self.image_ids = IDPool()
        if directory is None:
            self.data = CodeData()
            return
        with open(self.get_application_file(), 'rb') as fp:
            self.data = CodeData.load(fp)

    def get_application_file(self):
        return os.path.join(self.base_dir, 'application.py')

    def save(self):
        self.data.object_types = {}
        for k, v in self.object_types.iteritems():
            self.data.object_types[k] = v.get_data()
        with open(self.get_application_file(), 'wb') as fp:
            self.data.save(fp)

    def set_directory(self, directory):
        self.base_dir = directory

    def get_image_file(self, ref):
        return os.path.join(self.base_dir, '%s.png' % ref)

    def get_image(self, ref):
        if ref in self.images:
            return self.images[ref]
        image = Image(self.get_image_file(ref))
        self.image_ids.pop(ref)
        image.id = ref
        self.images[ref] = image
        return image

    def save_image(self, image):
        if image.id is None:
            image.id = self.image_ids.pop()
            self.images[image.id] = image
        image.save(self.get_image_file(image.id))
        return image.id

    # object type management

    def create_object_type(self, klass):
        object_type = klass(self)
        object_type.id = self.object_type_ids.pop()
        self.object_types[object_type.id] = object_type
        return object_type

    def get_object_type(self, ref):
        if ref in self.object_types:
            return self.object_types[ref]
        type_data = self.data.object_types[ref]
        object_type = get_objects()[type_data.name](self, type_data.data)
        object_type.id = type_data.type_id
        self.object_type_ids.pop(object_type.id)
        self.object_types[ref] = object_type
        return object_type