# Copyright (c) Mathias Kaerlev
# See LICENSE for details.

class BaseSerializer(object):
    def __init__(self, data = None):
        if data is None:
            self.initialize()
        else:
            self.read(data)

    def initialize(self):
        pass

    @classmethod
    def load(cls, fp):
        return cls(eval(fp.read()))

    def save(self, fp):
        fp.write(repr(self.get_dict()))

    def read(self, data):
        raise NotImplementedError()

    def write(self, data):
        raise NotImplementedError()

    def get_dict(self):
        data = {}
        self.write(data)
        return data

class ObjectType(BaseSerializer):
    def initialize(self):
        self.name = 'Undefined'
        self.type_id = -1
        self.data = {}

    def read(self, data):
        self.name = data['name']
        self.type_id = data['id']
        self.data = data.get('data', {})

    def write(self, data):
        data['name'] = self.name
        data['id'] = self.type_id
        data['data'] = self.data

class ObjectInstance(BaseSerializer):
    def read(self, data):
        self.x = data['x']
        self.y = data['y']
        self.object_type = data['object_type']

    def write(self, data):
        data['x'] = self.x
        data['y'] = self.y
        data['object_type'] = self.object_type

class Scene(BaseSerializer):
    def initialize(self):
        self.instances = []

    def read(self, data):
        self.name = data['name']
        self.width = data['width']
        self.height = data['height']
        self.background = data['background']
        self.instances = []
        for item in data['instances']:
            self.instances.append(ObjectInstance(item))

    def write(self, data):
        data['name'] = self.name
        data['width'] = self.width
        data['height'] = self.height
        data['instances'] = instances = []
        data['background'] = self.background
        for item in self.instances:
            instances.append(item.get_dict())

class CodeData(BaseSerializer):
    def initialize(self):
        self.name = 'Application'
        self.scenes = []
        self.object_types = {}

    def read(self, data):
        self.name = data.get('name', 'Application')
        self.object_types = {}
        for k, v in data.get('object_types', {}).iteritems():
            self.object_types[k] = ObjectType(v)
        self.scenes = []
        for item in data.get('scenes', []):
            self.scenes.append(Scene(item))

    def write(self, data):
        data['name'] = self.name
        data['scenes'] = scenes = []
        for scene in self.scenes:
            scenes.append(scene.get_dict())
        data['object_types'] = object_types = {}
        for k, v in self.object_types.iteritems():
            object_types[k] = v.get_dict()