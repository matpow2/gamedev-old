// Copyright (c) Mathias Kaerlev
// See LICENSE for details.

class Background : public SceneObject
{
public:
    Image * image;

    Background(Image * image, std::string name, int x, int y, int type_id) 
    : SceneObject(name, x, y, type_id), image(image)
    {
    }

    void draw()
    {
        glColor4f(1.0, 1.0, 1.0, 1.0);
        image->draw(x, y);
    }
};