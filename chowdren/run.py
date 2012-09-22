# Copyright (c) Mathias Kaerlev
# See LICENSE for details.

import sys
import math
import os
import json
from PySide import QtCore, QtGui, QtOpenGL
global_application = QtGui.QApplication(sys.argv)
global_application.setStyle("cleanlooks")

from PySide.QtCore import Qt
from OpenGL import GL
from chowdren.build import build
from chowdren.project import ProjectManager
from chowdren.object import get_objects
from chowdren.image import Image
from chowdren.common import IDPool
from chowdren import data as sourcedata

class SceneTreeItem(QtGui.QTreeWidgetItem):
    def __init__(self, scene):
        super(SceneTreeItem, self).__init__([scene.name])
        self.scene = scene

class Workspace(QtGui.QTreeWidget):
    def __init__(self, parent):
        super(Workspace, self).__init__(parent)
        self.headerItem().setHidden(True)
        data = parent.data
        self.application = QtGui.QTreeWidgetItem([data.name])
        self.addTopLevelItem(self.application)

    def add_scene(self, scene):
        item = SceneTreeItem(scene)
        self.application.addChild(item)

class InsertObjectItem(QtGui.QListWidgetItem):
    def __init__(self, object_class):
        super(InsertObjectItem, self).__init__(object_class.__name__)
        self.object_class = object_class

class InsertDialog(QtGui.QDialog):
    def __init__(self):
        super(InsertDialog, self).__init__()
        self.setWindowTitle('Create new object')
        h_layout = self.h_layout = QtGui.QHBoxLayout()
        self.list_widget = QtGui.QListWidget()
        self.list_widget.itemDoubleClicked.connect(self.on_accept)
        h_layout.addWidget(self.list_widget)
        for item in get_objects().values():
            self.list_widget.addItem(InsertObjectItem(item))
        button_layout = self.button_layout = QtGui.QVBoxLayout()
        self.ok_button = QtGui.QPushButton('OK')
        self.ok_button.clicked.connect(self.on_accept)
        self.cancel_button = QtGui.QPushButton('Cancel')
        self.cancel_button.clicked.connect(self.on_cancel)
        for item in (self.ok_button, self.cancel_button):
            item.setSizePolicy(QtGui.QSizePolicy.Expanding, 
                               QtGui.QSizePolicy.Expanding)
        button_layout.addWidget(self.ok_button)
        button_layout.addWidget(self.cancel_button)
        h_layout.addLayout(button_layout)
        self.setLayout(h_layout)

    def on_accept(self, item = None):
        self.selected_object = self.list_widget.currentItem().object_class
        self.accept()

    def on_cancel(self):
        self.reject()

class SceneObject(QtGui.QGraphicsItem):
    def __init__(self, object_type, data):
        super(SceneObject, self).__init__()
        self.setFlag(QtGui.QGraphicsItem.ItemIsMovable, True)
        self.setFlag(QtGui.QGraphicsItem.ItemIsSelectable, True)
        self.setFlag(QtGui.QGraphicsItem.ItemSendsScenePositionChanges, True)
        self.data = data
        self.setPos(data.x, data.y)
        self.object_type = object_type

    def boundingRect(self):
        return QtCore.QRectF(*self.object_type.get_bounding_box())

    def itemChange(self, change, value):
        if change == QtGui.QGraphicsItem.ItemScenePositionHasChanged:
            data = self.data
            data.x, data.y = int(value.x()), int(value.y())
        return super(SceneObject, self).itemChange(change, value)

    def paint(self, painter, option, widget):
        self.object_type.draw(painter)

        if not self.isSelected():
            return

        # fgcolor = option.palette.windowText()
        brush1 = QtGui.QColor(0, 0, 0)
        brush2 = QtGui.QBrush(QtGui.QColor(128, 128, 255), Qt.Dense4Pattern)
        
        # draw resize border stuff
        
        x, y, width, height = self.object_type.get_bounding_box()
        x -= 1
        y -= 1
        width += 2
        height += 2
        
        # if self.resizable:
        #     for item in self.get_resize_rects():
        #         painter.fillRect(item, bgcolor)
        
        painter.setPen(QtGui.QPen(brush1, 2, Qt.SolidLine, j = Qt.MiterJoin))
        painter.drawRect(x, y, width, height)

        painter.setPen(QtGui.QPen(brush2, 2, j = Qt.MiterJoin))
        painter.drawRect(x, y, width, height)

class Scene(QtGui.QGraphicsScene):
    def __init__(self, main, data):
        super(Scene, self).__init__()
        self.main = main
        self.project = main.project
        self.data = data
        self.setSceneRect(QtCore.QRectF(0.0, 0.0, data.width, data.height))
        self.items = []
        # create actions
        self.insert_action = QtGui.QAction("Insert object", self, 
            triggered = self.on_insert)

        # load data
        for instance in data.instances:
            object_type = self.project.get_object_type(instance.object_type)
            self.add_instance(object_type, instance)

    def on_insert(self):
        dialog = InsertDialog()
        if dialog.exec_() != QtGui.QDialog.Accepted:
            return
        object_type = self.project.create_object_type(dialog.selected_object)
        self.create_instance(object_type, 0, 0)

    def create_instance(self, object_type, x, y):
        instance_data = sourcedata.ObjectInstance()
        instance_data.x = x
        instance_data.y = y
        self.data.instances.append(instance_data)
        self.add_instance(object_type, instance_data)

    def add_instance(self, object_type, data):
        data.object_type = object_type.id
        scene_object = SceneObject(object_type, data)
        self.addItem(scene_object)

    def drawBackground(self, painter, rect):
        painter.fillRect(rect, QtGui.QColor(192, 192, 192))
        data = self.data
        painter.fillRect(0, 0, data.width, data.height,
                         QtGui.QColor(*data.background))

    def contextMenuEvent(self, event):
        menu = QtGui.QMenu()
        menu.addAction(self.insert_action)
        menu.exec_(event.screenPos())

ITEM_HEIGHT = 40

def make_widget(layout):
    widget = QtGui.QWidget()
    widget.setLayout(layout)
    return widget

def make_horizontal_layout(*widgets):
    layout = QtGui.QHBoxLayout()
    # layout.setAlignment(Qt.AlignTop)
    for item in widgets:
        layout.addWidget(item, alignment = Qt.AlignTop)
    return layout

class ConditionDialog(QtGui.QDialog):
    def __init__(self):
        super(ConditionDialog, self).__init__()

class EventFrame(QtGui.QFrame):
    def __init__(self):
        super(EventFrame, self).__init__()

    def dragEnterEvent(self, event):
        pass

    def dragMoveEvent(self, event):
        pass

    def dropEvent(self, event):
        pass

    def mousePressEvent(self, event):
        pass

class EventGroup(object):
    def __init__(self, main, data):
        self.main = main
        self.data = data

        layout = self.layout = QtGui.QHBoxLayout()

        self.conditions = QtGui.QVBoxLayout()
        self.actions = QtGui.QVBoxLayout()

        self.conditions.addWidget(QtGui.QPushButton('Add condition'))
        self.actions.addWidget(QtGui.QPushButton('Add action'))

        tab_label = QtGui.QLabel('1')
        tab_label.setSizePolicy(QtGui.QSizePolicy.Fixed, 
                                QtGui.QSizePolicy.Fixed)

        layout.addWidget(tab_label)
        layout.addLayout(self.conditions)
        layout.addLayout(self.actions)
        margin = 2
        layout.setContentsMargins(margin, margin, margin, margin)

        self.widget = EventFrame()
        self.widget.setFrameStyle(QtGui.QFrame.StyledPanel | QtGui.QFrame.Plain)
        self.widget.setLayout(layout)

    def get_widget(self):
        return self.widget

def make_framed_label(text):
    label = QtGui.QLabel(text)
    label.setFrameStyle(QtGui.QFrame.StyledPanel | QtGui.QFrame.Plain)
    label.setAlignment(Qt.AlignLeft | Qt.AlignVCenter)
    return label

class EventEditor(QtGui.QWidget):
    def __init__(self, main, data):
        super(EventEditor, self).__init__()
        self.main = main
        self.data = data

        self.groups = []

        self.vertical_layout = QtGui.QVBoxLayout()
        margin = 2
        self.vertical_layout.setContentsMargins(margin, margin, margin, margin)
        self.vertical_layout.setAlignment(Qt.AlignTop)

        self.add_button = QtGui.QPushButton('Add event')
        self.add_button.clicked.connect(self.on_add_event)
        self.add_widgets([self.add_button])
        self.vertical_layout.insertSpacerItem(-1, QtGui.QSpacerItem(0, 0, 
            vData = QtGui.QSizePolicy.MinimumExpanding))
        self.setLayout(self.vertical_layout)

    def on_add_event(self):
        self.add_event()

    def add_event(self):
        group = EventGroup(self.main, self.data)
        self.groups.append(group)
        self.add_widget(group.get_widget(), self.vertical_layout.count() - 2)

    def add_layout(self, layout, index = None):
        if index is None:
            index = self.vertical_layout.count()
        self.vertical_layout.insertLayout(index, layout)

    def add_widget(self, widget, index = None):
        if index is None:
            index = self.vertical_layout.count()
        self.vertical_layout.insertWidget(index, widget)

    def add_widgets(self, widgets, index = None):
        layout = make_horizontal_layout(*widgets)
        self.add_layout(layout, index)

class MainWindow(QtGui.QMainWindow):
    build_timer = build_generator = None
    def __init__(self, app):
        super(MainWindow, self).__init__()

        self.app = app
        self.load(os.path.join(os.getcwd(), 'save'))

        name = 'Chowdren'
        self.setWindowTitle(self.tr(name))
        self.app.setApplicationName(name)
        self.clipboard = app.clipboard()

        self.workspace = self.create_dock(Workspace, 'Workspace', 
            Qt.LeftDockWidgetArea)

        for scene in self.data.scenes:
            self.workspace.add_scene(scene)

        scene_data = self.data.scenes[0]
        self.graphics_scene = Scene(self, scene_data)
        self.scenes = [self.graphics_scene]
        self.graphics_view = QtGui.QGraphicsView(self.graphics_scene)
        self.graphics_view.setDragMode(QtGui.QGraphicsView.RubberBandDrag)
        self.event_editor = EventEditor(self, scene_data)
        self.event_area = QtGui.QScrollArea()
        self.event_area.setWidget(self.event_editor)
        self.event_area.setWidgetResizable(True)

        # self.gl_widget = QtOpenGL.QGLWidget(
        #     QtOpenGL.QGLFormat(QtOpenGL.QGL.SampleBuffers))
        # self.graphics_view.setViewport(self.gl_widget)
        # self.graphics_view.setViewportUpdateMode(
        #     QtGui.QGraphicsView.FullViewportUpdate)

        self.setCentralWidget(self.event_area)

        status_bar = self.statusBar()
        self.status_label = QtGui.QLabel()
        status_bar.addWidget(self.status_label)
        menu = self.menuBar()
        self.file = menu.addMenu('&File')
        self.new_action = QtGui.QAction('&New', self,
            shortcut = QtGui.QKeySequence('Ctrl+N'), 
            triggered = self.new_selected)
        self.file.addAction(self.new_action)
        self.open_action = QtGui.QAction('&Open...', self,
            shortcut = QtGui.QKeySequence('Ctrl+O'), 
            triggered = self.open_selected)
        self.file.addAction(self.open_action)
        self.file.addSeparator()
        self.save_action = QtGui.QAction('&Save', self, 
            shortcut = QtGui.QKeySequence.Save, triggered = self.save_selected)
        # self.save_action.setEnabled(False)
        self.file.addAction(self.save_action)
        self.save_as_action = QtGui.QAction('Save &As...', self,
            shortcut = QtGui.QKeySequence('Ctrl+Shift+S'), 
            triggered = self.save_as_selected)
        self.file.addAction(self.save_as_action)
        self.file.addSeparator()
        self.build_action = QtGui.QAction('&Build', self,
            shortcut = QtGui.QKeySequence('Ctrl+B'), 
            triggered = self.build_selected)
        self.file.addAction(self.build_action)

        self.addToolBar('Main')

        self.set_status('Ready.')

    def reset(self):
        self.project = ProjectManager()
        self.data = data = self.project.data
        data.name = 'Application 1'
        self.create_scene()

    def load(self, directory):
        try:
            self.project = ProjectManager(directory)
            self.data = self.project.data
        except IOError:
            print 'could not load'
            self.reset()
            return

    def save(self):
        self.set_status('Saving...')
        self.project.save()
        self.set_status('Saved.')

    def create_scene(self):
        scene = sourcedata.Scene()
        scene.name = 'Scene %s' % (len(self.data.scenes) + 1)
        scene.width = 640
        scene.height = 480
        scene.background = (255, 255, 255)
        self.data.scenes.append(scene)

    def new_selected(self):
        self.base_dir = None
        self.reset()

    def open_selected(self):
        pass

    def save_selected(self):
        if self.project.base_dir is None:
            self.save_as_selected()
            return
        self.save()

    def save_as_selected(self):
        ret = QtGui.QFileDialog.getExistingDirectory(self, 'Save As')
        if not ret:
            return
        self.project.set_directory(ret)
        self.save()

    def set_status(self, text):
        self.status_label.setText(text)
        # it is important that we update the contents of the status label
        # if we are entering a longer, blocking call
        self.app.processEvents()

    def build_selected(self):
        self.save()
        self.set_status('Building...')
        self.build_generator = build(self.project, os.path.join(os.getcwd(), 
            'out'))
        self.build_timer = QtCore.QTimer(self)
        self.build_timer.timeout.connect(self.update_build)
        self.build_timer.start(0.1)

    def update_build(self):
        try:
            self.build_generator.next()
        except StopIteration:
            self.build_timer.stop()
            self.build_timer = self.build_generator = None
            self.set_status('Build finished.')

    def create_dock(self, widget_class, name, area):
        dock = QtGui.QDockWidget(name, self)
        widget = widget_class(self)
        dock.setWidget(widget)
        self.addDockWidget(area, dock)
        return widget

class GLWidget(QtOpenGL.QGLWidget):
    def __init__(self, parent=None):
        super(GLWidget, self).__init__(parent)

    def minimumSizeHint(self):
        return QtCore.QSize(50, 50)

    def sizeHint(self):
        return QtCore.QSize(400, 400)

    def initializeGL(self):
        pass

    def paintGL(self):
        pass

    def resizeGL(self, width, height):
        side = min(width, height)
        GL.glViewport((width - side) / 2, (height - side) / 2, side, side)

        GL.glMatrixMode(GL.GL_PROJECTION)
        GL.glLoadIdentity()
        GL.glOrtho(-0.5, +0.5, +0.5, -0.5, 4.0, 15.0)
        GL.glMatrixMode(GL.GL_MODELVIEW)

    def mousePressEvent(self, event):
        self.lastPos = QtCore.QPoint(event.pos())

    def mouseMoveEvent(self, event):
        pass

def main():
    window = MainWindow(global_application)
    window.show()
    # window.showMaximized()
    sys.exit(global_application.exec_())