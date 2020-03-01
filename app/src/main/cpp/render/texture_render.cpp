

#include "render/texture_render.h"
#include "opengl_helper.h"
#include "tex_transform_util.h"
#include <string>
const static char *VERTER_SHADER  =
        "attribute vec4 aPosition;\n"
        "attribute vec4 aTextureCoord;\n"
        "varying vec2 vTextureCoord;\n"
        "uniform mat4 modelViewProjectionMatrix;\n"
        "void main() {\n"
        "   gl_Position = modelViewProjectionMatrix*aPosition;\n"
        "   vTextureCoord = aTextureCoord.xy;\n"
        "}\n";

const static char *FRAGMENT_2D_SHADER =
        "precision mediump float;\n"
        "varying vec2 vTextureCoord;\n"
        "uniform sampler2D uTexrue;\n"
        "void main() {\n"
        "   gl_FragColor = texture2D(uTexrue, vTextureCoord);\n"
        "}\n";

const static char *FRAGMENT_OES_SHADER =
        "#extension GL_OES_EGL_image_external : require\n"
        "precision mediump float;\n"
        "varying vec2 vTextureCoord;\n"
        "uniform samplerExternalOES uTexrue;\n"
        "void main() {\n"
        "   gl_FragColor = texture2D(uTexrue, vTextureCoord);\n"
        "}\n";


void get_vertex_coords(float *data);
void get_tex_coords(float *data);
size_t get_vertex_size();
size_t get_tex_size();

static const float tex_coords[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
};

//void get_vertex_coords(float *data) {
//    memcpy(data, vertex_coords, sizeof(vertex_coords));
//}

void get_tex_coords(float *data) {
    memcpy(data, tex_coords, sizeof(tex_coords));
}

//size_t get_vertex_size() {
//    return sizeof(vertex_coords);
//}

size_t get_tex_size() {
    return sizeof(tex_coords);
}
TextureRender::TextureRender(GLenum target)
: _target(target)
, _texture_id(0)
, _program(0)
, _scale(1.0){
    prepareVertices();
}
TextureRender::~TextureRender()
{
}

int TextureRender::init()
{
    switch (_target) {
        case GL_TEXTURE_2D:
            _program = glhelp_create_program(VERTER_SHADER, FRAGMENT_2D_SHADER);
            break;
        case GL_TEXTURE_EXTERNAL_OES:
            _program = glhelp_create_program(VERTER_SHADER, FRAGMENT_OES_SHADER);
            break;
        default:
            goto error;
    }

    _aPosition_loc = glGetAttribLocation(_program, "aPosition");
    if (_aPosition_loc < 0)
        goto error;
    _aTextureCoord_loc = glGetAttribLocation(_program, "aTextureCoord");
    if (_aTextureCoord_loc < 0)
        goto error;
    _uTexrue_loc = glGetUniformLocation(_program, "uTexrue");
    if (_uTexrue_loc < 0)
        goto error;
    _mvp_loc = glGetUniformLocation(_program, "modelViewProjectionMatrix");
    if (_mvp_loc < 0)
        goto error;
    return 0;
error:
    return -1;
}

int TextureRender::draw(GLuint texture, float mvp[])
{
    glUseProgram(_program);
    CHECK_ERROR(error);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(_target, texture);
    CHECK_ERROR(error);

    if (mvp == NULL) {
        mtxLoadIdentity((float*)_mvp);
    } else {
        memcpy(_mvp, mvp, 16*sizeof(float));
    }
    glUniformMatrix4fv(_mvp_loc, 1, GL_FALSE, _mvp);

    glEnableVertexAttribArray(static_cast<GLuint>(_aPosition_loc));
    glVertexAttribPointer(static_cast<GLuint>(_aPosition_loc), 2, GL_FLOAT, GL_FALSE, 0, _vertex_coords.data());
    CHECK_ERROR(error);

    glEnableVertexAttribArray(static_cast<GLuint>(_aTextureCoord_loc));
    glVertexAttribPointer(static_cast<GLuint>(_aTextureCoord_loc), 2, GL_FLOAT, GL_FALSE, 0, _tex_coords.data());
    CHECK_ERROR(error);

    glUniform1i(_uTexrue_loc, 0);
    CHECK_ERROR(error);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    CHECK_ERROR(error);

    glBindTexture(_target, 0);
    glDisableVertexAttribArray(static_cast<GLuint>(_aPosition_loc));
    glDisableVertexAttribArray(static_cast<GLuint>(_aTextureCoord_loc));
    CHECK_ERROR(error);

error:
    return -1;
}


int TextureRender::createTexId()
{
    _texture_id = glhelp_create_texture_id(_target);
    return _texture_id;
}

void TextureRender::release()
{
    const GLuint ids[] = {_texture_id};
    glDeleteProgram(_program);
    glDeleteTextures(1, ids);
}

void TextureRender::setScale(float scale) {
    _scale *= scale;
    prepareVertices();
}

void TextureRender::prepareVertices() {
    float renderWidth = 2163;
    float renderHeight = 999;
    float frameWidth = 1920;
    float frameHegiht = 1080;
    float sr = renderWidth/renderHeight;
    float sf = frameWidth/ frameHegiht;
    float scalex = 1.0;
    float scaley = 1.0;
    if ( sf> sr) {
        scaley = renderWidth/sf/renderHeight;
    } else {
        scalex = renderHeight*sf/renderWidth;
    }
    _tex_coords = {
//        0.0f, 0.0f,
////                1.0f, 0.0f,
////                0.0f, 1.0f,
////                1.0f, 1.0f

            0.0f, 1.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,

    };

    _vertex_coords = {
        -1.0f*scalex, -1.0f*scaley,
        1.0f*scalex, -1.0f*scaley,
        -1.0f*scalex, 1.0f*scaley,
        1.0f*scalex, 1.0f*scaley
//            -1.0f*_scale, 1.0f*_scale*scaley,
//            -1.0f*_scale, -1.0f*_scale*scaley,
//            1.0f*_scale, 1.0f*_scale*scaley,
//            1.0f*_scale, -1.0f*_scale*scaley,
    };

}