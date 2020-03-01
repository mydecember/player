
#ifndef __TEX_TRANSFORM_UTIL__
#define __TEX_TRANSFORM_UTIL__

#include <cstdlib>
#include <GLES2/gl2.h>

class Texture {
public:
    int width;
    int height;
    int channel;
    GLuint id;
    bool linear;

    Texture();
    ~Texture();
    size_t size();
    void set(int width, int height, int channel, uint8_t* data);
    void setLinearInterpolation(bool linear);
    void release();
};

#endif
