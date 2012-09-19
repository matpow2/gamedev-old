# Copyright (c) Mathias Kaerlev
# See LICENSE for details.

from chowdren.object import ObjectBase
from chowdren.image import default_image
from PySide.QtGui import QColor

class Sprite(ObjectBase):
    def initialize(self):
        self.image = default_image

    def read(self, data):
        self.image = self.get_image(data['image'])

    def write(self, data):
        data['image'] = self.save_image(self.image)

    def get_bounding_box(self):
        return self.image.get_bounding_box()

    def draw(self, painter):
        self.image.draw(painter)

    # for runtime

    def get_parameters(self):
        return [self.image]

    def write_init(self, writer):
        pass

    def get_init_list(self):
        return []

def get_object():
    return Sprite