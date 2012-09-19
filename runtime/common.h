// Copyright (c) Mathias Kaerlev
// See LICENSE for details.

#ifndef COMMON_H
#define COMMON_H

#include "SOIL.h"
#include <string>
#include <list>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <GL/glfw.h>
#include <stdlib.h>

class Color
{
public:
    float r, g, b, a;

    Color(int r, int g, int b, int a = 255)
    {
        set(r, g, b, a);
    }

    Color(float r, float g, float b, float a = 1.0)
    {
        set(r, g, b, a);
    }

    void set(float r, float g, float b, float a = 1.0)
    {
        this->r = r;
        this->g = g;
        this->b = b;
        this->a = a;
    }

    void set(int r, int g, int b, int a = 255)
    {
        set(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
    }
};

class Font
{
    char * face;
    int size;
    bool bold;
    bool italic;
    bool underline;
public:
    Font(char * face, int size, bool bold, bool italic, bool underline) :
        face(face), 
        size(size), 
        bold(bold), 
        italic(italic), 
        underline(underline) {}
};

class Sound
{
    char * name;
public:
    Sound(char * name) : name(name) {}
};

void load_texture(const char *filename, int force_channels, 
                  unsigned int reuse_texture_ID, unsigned int flags,
                  GLuint * tex, int * width, int * height)
{
    unsigned char* img;
    int channels;
    img = SOIL_load_image(filename, width, height, &channels, force_channels);

    if( (force_channels >= 1) && (force_channels <= 4) )
    {
        channels = force_channels;
    }

    if(img == NULL)
    {
        *tex = 0;
        return;
    }

    *tex = SOIL_create_OGL_texture(img, *width, *height, channels,
        reuse_texture_ID, flags);

    SOIL_free_image_data(img);

    return;
}

class Image
{
public:
    std::string filename;
    int hotspot_x, hotspot_y, action_x, action_y;
    GLuint tex;
    int width, height;

    Image(std::string name, int hot_x, int hot_y) 
    : hotspot_x(hot_x), hotspot_y(hot_y), tex(0)
    {
        filename = "./images/" + name + ".png";
    }

    void load()
    {
        if (tex != 0)
            return;
        load_texture(filename.c_str(), 4, 0, SOIL_FLAG_POWER_OF_TWO,
            &tex, &width, &height);
        if (tex == 0) {
            printf("Could not load %s\n", filename.c_str());
        }
    }

    void draw(double x, double y)
    {
        load();

        x -= (double)hotspot_x;
        y -= (double)hotspot_y;
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0, 0.0);
        glVertex2d(x, y);
        glTexCoord2f(1.0, 0.0);
        glVertex2d(x + width, y);
        glTexCoord2f(1.0, 1.0);
        glVertex2d(x + width, y + height);
        glTexCoord2f(0.0, 1.0);
        glVertex2d(x, y + height);
        glEnd();
        glDisable(GL_TEXTURE_2D);
    }
};


// object types

template <class T>
class Attributes
{
public:
    std::map<std::string, T> values;

    Attributes()
    {}

    T get(std::string name)
    {
        return values[name];
    }

    void set(std::string name, T value)
    {
        return values[name] = value;
    }
};

typedef Attributes<double> AttributeValues;
typedef Attributes<std::string> AttributeStrings;

class SceneObject
{
public:
    std::string name;
    double x, y;
    int id;
    AttributeValues * attribute_values;
    AttributeStrings * attribute_strings;

    SceneObject(std::string name, int x, int y, int type_id) 
    : name(name), x(x), y(y), id(type_id)
    {
    }

    void set_position(double x, double y)
    {
        this->x = x;
        this->y = y;
    }

    void set_x(double x)
    {
        this->x = x;
    }

    void set_y(double y)
    {
        this->y = y;
    }

    void create_attributes()
    {
        attribute_values = new AttributeValues;
        attribute_strings = new AttributeStrings;
    }

    virtual void draw() {}
    virtual void update(float dt) {}
};

typedef std::vector<SceneObject*> ObjectList;

class Scene
{
public:
    std::string name;
    int width, height;
    int index;
    GameManager * manager;
    ObjectList instances;
    std::map<int, ObjectList> instance_classes;
    std::map<std::string, int> loop_indexes;
    Color background_color;

    Scene(std::string name, int width, int height, Color background_color,
          int index, GameManager * manager)
    : name(name), width(width), height(height), index(index), 
      background_color(background_color), manager(manager)
    {}

    virtual void on_start() {}
    virtual void on_end() {}
    virtual void handle_events() {}

    void update(float dt)
    {
        for (ObjectList::const_iterator iter = instances.begin(); 
             iter != instances.end(); iter++) {
            (*iter)->update(dt);
        }

        handle_events();
    }

    void draw() 
    {
        int window_width, window_height;
        glfwGetWindowSize(&window_width, &window_height);
        glViewport(0, 0, window_width, window_height);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, width, height, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glClearColor(background_color.r, background_color.g, background_color.b,
                     background_color.a);
        glClear(GL_COLOR_BUFFER_BIT);

        for (ObjectList::const_iterator iter = instances.begin(); 
             iter != instances.end(); iter++) {
            (*iter)->draw();
        }

    }

    ObjectList & get_instances(int object_id)
    {
        return instance_classes[object_id];
    }

    SceneObject & get_instance(int object_id)
    {
        return *instance_classes[object_id][0];
    }

    void add_object(SceneObject * object)
    {
        instances.push_back(object);
        instance_classes[object->id].push_back(object);
    }

    ObjectList create_object(SceneObject * object)
    {
        add_object(object);
        ObjectList new_list;
        new_list.push_back(object);
        return new_list;
    }
};

static ObjectList::iterator item;

int randrange(int range)
{
    return (rand() * range) / RAND_MAX;
}

#include "objecttypes.h"

#endif /* COMMON_H */