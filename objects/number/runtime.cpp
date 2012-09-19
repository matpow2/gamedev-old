// Copyright (c) Mathias Kaerlev
// See LICENSE for details.

class Number : public SceneObject
{
public:
    Image * images[14];
    double value;
    double minimum, maximum;
    std::string cached_string;

    Number(int init, int min, int max, std::string name, 
           int x, int y, int type_id) 
    : SceneObject(name, x, y, type_id), minimum(min), maximum(max)
    {
        set(init);
    }

    Image * get_image(char c)
    {
        switch (c) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                return images[c - '0'];
            case '-':
                return images[10];
            case '+':
                return images[11];
            case '.':
                return images[12];
            case 'e':
                return images[13];
            default:
                return NULL;
        }
    }

    void add(double value) {
        set(this->value + value);
    }

    void set(double value) {
        value = std::max<double>(std::min<double>(value, maximum), minimum);
        this->value = value;

        std::ostringstream str;
        str << value;
        cached_string = str.str();
    }

    void draw()
    {
        glColor4f(1.0, 1.0, 1.0, 1.0);
        double current_x = x;
        for (std::string::const_reverse_iterator it = cached_string.rbegin(); 
             it < cached_string.rend(); it++) {
            Image * image = get_image(it[0]);
            if (image == NULL)
                continue;
            image->draw(current_x + image->hotspot_x - image->width, 
                        y + image->hotspot_y - image->height);
            current_x -= image->width;
        }
    }
};