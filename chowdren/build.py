import os
import shutil
from cStringIO import StringIO
from chowdren.common import to_c, repr_c, copy_tree, make_color, to_cap_words
from chowdren.image import Image
import subprocess

RUNTIME_DIR = os.path.join(os.getcwd(), 'runtime')

def get_cmake_path(path):
    return path.replace('\\', '/')

BASE_COMPILER_PATH = os.getcwd()
MINGW_DIR = os.path.join(BASE_COMPILER_PATH, 'mingw', 'bin')
MINGW_C_COMPILER = os.path.join(MINGW_DIR, 'gcc.exe')
MINGW_CXX_COMPILER = os.path.join(MINGW_DIR, 'g++.exe')
MINGW_MAKE = os.path.join(MINGW_DIR, 'mingw32-make.exe')
CMAKE_EXE = os.path.join(BASE_COMPILER_PATH, 'cmake', 'bin', 'cmake.exe')
CMAKE_ARGS = [CMAKE_EXE, '-G', 'MinGW Makefiles', '..', 
    '-DCMAKE_CXX_COMPILER=%s' % get_cmake_path(MINGW_CXX_COMPILER),
    '-DCMAKE_C_COMPILER=%s' % get_cmake_path(MINGW_C_COMPILER)
]
EXE_FILENAME = 'Chowdren.exe'

class CodeWriter(object):
    indentation = 0
    def __init__(self, filename = None):
        if filename is None:
            self.fp = StringIO()
        else:
            self.fp = open(filename, 'wb+')
    
    def format_line(self, line):
        return self.get_spaces() + line
    
    def put_line(self, *lines, **kw):
        wrap = kw.get('wrap', False)
        indent = kw.get('indent', True)
        if wrap:
            indent = self.get_spaces(1)
        for line in lines:
            if wrap:
                line = ('\n' + indent).join(textwrap.wrap(line))
            if indent:
                line = self.format_line(line)
            self.fp.write(line + '\n')

    def put_define(self, a, b):
        self.put_line('#define %s %s' % (a, b))

    def put_newline(self):
        self.put_line('')
    
    def put_indent(self, extra = 0):
        self.fp.write(self.get_spaces(extra))
    
    def put(self, value, indent = False):
        if indent:
            self.putindent()
        self.fp.write(value)
    
    def get_data(self):
        fp = self.fp
        pos = fp.tell()
        fp.seek(0)
        data = fp.read()
        fp.seek(pos)
        return data

    def put_code(self, writer):
        data = writer.get_data().splitlines()
        for line in data:
            self.put_line(line)
    
    def put_class(self, name, subclass = None):
        text = 'class %s' % name
        if subclass is not None:
            text += ' : public %s' % subclass
        self.put_line(text)
        self.start_brace()

    def put_includes(self, *names):
        for name in names:
            self.put_line('#include "%s"' % name)
        self.put_newline()

    def start_brace(self):
        self.put_line('{')
        self.indent()

    def end_brace(self, semicolon = False):
        self.dedent()
        text = '}'
        if semicolon:
            text += ';'
        self.put_line(text)
    
    def put_func(self, name, *arg):
        fullarg = list(arg)
        self.put_line('%s(%s)' % (name, ', '.join(fullarg)))
        self.start_brace()

    def put_label(self, name):
        self.dedent()
        self.put_line('%s: ;' % name)
        self.indent()

    def put_access(self, name):
        self.dedent()
        self.put_line('%s:' % name)
        self.indent()

    def start_guard(self, name):
        self.put_line('#ifndef %s' % name)
        self.put_line('#define %s' % name)
        self.put_line('')

    def close_guard(self, name):
        self.put_newline()
        self.put_line('#endif // %s' % name)
        self.put_newline()
    
    def indent(self):
        self.indentation += 1
        
    def dedent(self):
        self.indentation -= 1
        if self.indentation < 0:
            raise ValueError('indentation cannot be lower than 0')
        
    def get_spaces(self, extra = 0):
        return (self.indentation + extra) * '    '
    
    def close(self):
        self.fp.close()

def get_image(image):
    return 'image%s' % image.id

def convert_class_parameter(value):
    if isinstance(value, Image):
        return '&' + get_image(value)
    else:
        return str(value)

class Builder(object):
    def __init__(self, project, outdir):
        self.project = project
        self.data = data = project.data
        self.outdir = outdir
        copy_tree(os.path.join(os.getcwd(), RUNTIME_DIR), outdir)

        # config.h
        config = self.open_code('config.h')

        includes = ['common.h']
        scene_classes = []
        for i in xrange(len(data.scenes)):
            includes.append('scene%s.h' % (i+1))
            scene_classes.append('Scene%s' % (i+1))
        config.put_includes(*includes)

        config.put_define('WINDOW_WIDTH', 800)
        config.put_define('WINDOW_HEIGHT', 600)
        config.put_define('NAME', repr_c(data.name))

        config.put_line('static Scene ** scenes = NULL;')
        config.put_func('Scene ** get_scenes', 'GameManager * manager')
        config.put_line('if (scenes) return scenes;')
        config.put_line('scenes = new Scene*[%s];' % len(scene_classes))
        for i, scene in enumerate(scene_classes):
            config.put_line('scenes[%s] = new %s(manager);' % (i, scene))
        config.put_line('return scenes;')
        config.end_brace()
        config.close()

        # images.h
        images = self.open_code('images.h')
        images.put_includes('common.h')
        images.start_guard('IMAGES_H')

        for image_id, image in project.images.iteritems():
            image.save(self.get_filename('images', '%s.png' % image_id))
            image_name = get_image(image)
            images.put_line(to_c(
                'static Image %s(%r, %s, %s);',
                image_name, str(image_id), image.hotspot_x, 
                image.hotspot_y))
        
        images.close_guard('IMAGES_H')
        images.close()

        # objects.h
        self.object_type_names = {}
        objects = self.open_code('objects.h')
        objects.put_includes('common.h', 'images.h')
        for type_id, object_type in project.object_types.iteritems():
            self.write_object_type(type_id, object_type, objects)
        objects.close()

        # object type implementations
        type_files = set()
        for type_id, object_type in project.object_types.iteritems():
            type_files.add(object_type.get_runtime())

        type_includes = set()
        for name, path in type_files:
            ext = os.path.splitext(path)[1]
            name = '%s%s' % (name, ext)
            new_path = self.get_filename('objects', name)
            shutil.copyfile(path, new_path)
            type_includes.add('objects/%s' % name)

        types = self.open_code('objecttypes.h')
        types.put_includes(*type_includes)
        types.close()

        # scenes
        for scene in data.scenes:
            self.write_scene(scene)

    def write_scene(self, data):
        index = self.data.scenes.index(data)

        scene = self.open_code('scene%s.h' % (index+1))
        scene.put_includes('common.h', 'objects.h')

        class_name = 'Scene%s' % (index+1)
        scene.put_class(class_name, 'Scene')

        scene.put_access('public')
        scene.put_line(to_c('%s(GameManager * manager) : '
            'Scene(%r, %s, %s, %s, %s, manager)',
            class_name, data.name, data.width, data.height, 
            make_color(data.background), index))
        scene.start_brace()
        scene.end_brace()

        scene.put_func('void on_start')
        for instance in data.instances:
            scene.put_line('add_object(new %s(%s, %s));' %
                (self.object_type_names[instance.object_type], instance.x, 
                instance.y))
        scene.end_brace()

        scene.end_brace(True)

        scene.close()

    def write_object_type(self, type_id, object_type, objects):
        # XXX put object name here instead of class name
        name = object_type.get_class_name()
        subclass = object_type.get_class_name()
        class_name = to_cap_words(name, 'Obj') + str(type_id)
        self.object_type_names[type_id] = class_name
        objects.put_class(class_name, subclass)
        objects.put_access('public')
        objects.put_line('static const int type_id = %s;' % type_id)
        parameters = [to_c('%r', name), 'x', 'y', 'type_id']
        extra_parameters = [convert_class_parameter(item) 
            for item in object_type.get_parameters()]
        parameters = ', '.join(extra_parameters + parameters)
        init_list = [to_c('%s(%s)', subclass, parameters)]
        for name, value in object_type.get_init_list():
            init_list.append(to_c('%s(%r)', name, value))
        init_list = ', '.join(init_list)
        objects.put_line(to_c('%s(int x, int y) : %s', class_name, 
            init_list))
        objects.start_brace()
        object_type.write_init(objects)
        objects.end_brace()
        objects.end_brace(True)

    def open_code(self, *path):
        return CodeWriter(self.get_filename(*path))

    def open(self, *path):
        return open(self.get_filename(*path), 'wb')
    
    def get_filename(self, *path):
        return os.path.join(self.outdir, *path)

def build(data, out):
    # shutil.rmtree(outdir, ignore_errors = True)
    Builder(data, out)

    build_directory = os.path.join(out, 'build')

    # use cmake to make mingw makefiles
    process = subprocess.Popen(CMAKE_ARGS, cwd = build_directory)

    while 1:
        ret = process.poll()
        if ret is None:
            yield
            continue
        elif ret != 0:
            return
        break

    # use mingw to build executable
    process = subprocess.Popen([MINGW_MAKE], cwd = build_directory)

    while 1:
        ret = process.poll()
        if ret is None:
            yield
            continue
        elif ret != 0:
            return
        break

    subprocess.Popen([os.path.join(build_directory, EXE_FILENAME)],
        cwd = out)