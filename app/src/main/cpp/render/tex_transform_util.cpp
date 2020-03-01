
#include <string>
#include "tex_transform_util.h"
#include "base/utils.h"
Texture::Texture() : height(0), width(0), channel(0), id(0), linear(true) {
}

size_t Texture::size() {
    return (size_t)height * width * channel;
}

static int GetFormat(int channel) {
    switch (channel) {
        case 1:
            return GL_LUMINANCE;
        case 2:
            return GL_LUMINANCE_ALPHA;
        case 3:
            return GL_RGB;
        case 4:
            return GL_RGBA;
        default:
            Log("bad channel value: %d", channel);
            exit(1);
            return 0;
    }
}

void Texture::setLinearInterpolation(bool linear) {
    this->linear = linear;
    if (id) {
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear?GL_LINEAR:GL_NEAREST);
    }
}

void Texture::set(int W, int H, int C, uint8_t* data) {
    bool renew = false;
    if (id==0 || H != height || W != width || channel != C) {
        renew = true;
        width = W;
        height = H;
        channel = C;
    }
    int align = (width * channel) & 3;
    if (align != 0) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }
    int format = GetFormat(channel);
    if (renew) {
        if (id == 0) {
            glGenTextures(1, &id);
        }
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear?GL_LINEAR:GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    } else {
        glBindTexture(GL_TEXTURE_2D, id);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);
    }
    if (align != 0) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }
}

Texture::~Texture() {
}

void Texture::release() {
    if (id) {
        glDeleteTextures(1, &id);
        id = 0;
    }
}