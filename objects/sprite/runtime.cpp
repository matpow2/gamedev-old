// Copyright (c) Mathias Kaerlev
// See LICENSE for details.

#include "common.h"

/*class Direction
{
public:
    int index, min_speed, max_speed, back_to;
    bool repeat;
    std::vector<Image*> frames;

    Direction(int index, int min_speed, int max_speed, bool repeat, int back_to)
    : index(index), min_speed(min_speed), max_speed(max_speed), repeat(repeat),
      back_to(back_to)
    {

    }
};

class Sprite : public SceneObject
{
public:
    std::map<int, std::map<int, Direction*>> animations;
    Image * image;

    int animation;
    unsigned int direction, frame;
    unsigned int counter;

    Sprite(std::string name, int x, int y, int type_id) 
    : SceneObject(name, x, y, type_id), animation(0), direction(0),
      frame(0), counter(0)
    {
        create_attributes();
    }

    void add_direction(int animation, int direction,
                       int min_speed, int max_speed, bool repeat,
                       int back_to)
    {
        animations[animation][direction] = new Direction(direction, min_speed,
            max_speed, repeat, back_to);
    }

    void add_image(int animation, int direction, Image * image)
    {
        animations[animation][direction]->frames.push_back(image);
    }

    void update(float dt)
    {
        Direction * dir = animations[animation][direction];
        counter += dir->max_speed;
        while (counter > 100) {
            frame++;
            if (frame >= dir->frames.size())
                frame = 0;
            counter -= 100;
        }
    }

    void draw()
    {
        glColor4f(1.0, 1.0, 1.0, 1.0);
        animations[animation][direction]->frames[frame]->draw(x, y);
    }
};*/

class Sprite : public SceneObject
{
public:
    Image * image;

    Sprite(Image * image, std::string name, int x, int y, int type_id) 
    : SceneObject(name, x, y, type_id), image(image)
    {
        create_attributes();
    }

    void update(float dt)
    {
    }

    void draw()
    {
        glColor4f(1.0, 1.0, 1.0, 1.0);
        image->draw(x, y);
    }
};