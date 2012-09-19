# Copyright (c) Mathias Kaerlev
# See LICENSE for details.

import os
from PySide.QtGui import QPixmap

class Image(object):
    id = None
    hotspot_x = hotspot_y = 0.0

    def __init__(self, filename, hotspot_x = 0, hotspot_y = 0):
        self.pixmap = QPixmap(filename)
        self.hotspot_x, self.hotspot_y = hotspot_x, hotspot_y

    def draw(self, painter, x = 0.0, y = 0.0):
        painter.drawPixmap(x - self.hotspot_x, y - self.hotspot_y, self.pixmap)

    def save(self, filename):
        self.pixmap.save(filename)

    def get_bounding_box(self):
        img = self.pixmap
        return (-self.hotspot_x, -self.hotspot_y, img.width(), img.height())

default_image = Image(
    os.path.join(os.path.dirname(__file__), 'res', 'default.png'))