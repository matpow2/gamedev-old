# Copyright (c) Mathias Kaerlev
# See LICENSE for details.

import shutil
import os
import string

class StringWrapper(object):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return self.value

    def __repr__(self):
        return '"%s"' % self.value.replace('"', '')

def to_c(format_spec, *args):
    new_args = []
    for arg in args:
        if isinstance(arg, str):
            arg = StringWrapper(arg)
        elif isinstance(arg, bool):
            if arg:
                arg = 'true'
            else:
                arg = 'false'
        new_args.append(arg)
    return format_spec % tuple(new_args)

def repr_c(value):
    return to_c('%r', value)

def copy_tree(src, dst):
    names = os.listdir(src)
    try:
        os.makedirs(dst)
    except OSError:
        pass
    errors = []
    for name in names:
        srcname = os.path.join(src, name)
        dstname = os.path.join(dst, name)
        try:
            if os.path.isdir(srcname):
                copy_tree(srcname, dstname)
            else:
                shutil.copy2(srcname, dstname)
        # catch the Error from the recursive copytree so that we can
        # continue with other files
        except shutil.Error, err:
            errors.extend(err.args[0])
        except EnvironmentError, why:
            errors.append((srcname, dstname, str(why)))
    try:
        shutil.copystat(src, dst)
    except OSError, why:
        if WindowsError is not None and isinstance(why, WindowsError):
            # Copying file access times may fail on Windows
            pass
        else:
            errors.extend((src, dst, str(why)))
    if errors:
        raise shutil.Error, errors

def make_color(value):
    return 'Color(%s)' % ', '.join([str(item) for item in value])

VALID_CHARACTERS = string.ascii_letters + string.digits
DIGITS = string.digits

def to_cap_words(value, prefix = 'Class'):
    new_name = ''
    go_upper = True
    for c in value:
        if c in VALID_CHARACTERS:
            if go_upper:
                c = c.upper()
                go_upper = False
            new_name += c
        else:
            go_upper = True
    return check_leading_digits(new_name, prefix)

def check_leading_digits(value, prefix):
    if not value:
        return value
    if value[0] in DIGITS:
        value = prefix + value
    return value

class IDPool(object):
    def __init__(self):
        self.taken = set()

    def pop(self, existing = None):
        if existing is not None:
            self.taken.add(existing)
            return existing
        value = 0
        while value in self.taken:
            value += 1
        self.taken.add(value)
        return value

    def put_back(self, value):
        self.taken.discard(value)