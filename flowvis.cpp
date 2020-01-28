#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdio>

#include <QApplication>
#include <QSurfaceFormat>
#include <QKeyEvent>
#include <QtMath>

#include "cgbase/cggeometries.hpp"
#include "cgbase/cgtools.hpp"

#include "flowvis.hpp"


FlowVis::FlowVis() :
    _time_cell(0),
    _time_cell_in_texture(-1)
{
    _data.resize(_x_cells * _y_cells * _t_cells * 2);
    FILE* f = std::fopen("flow.raw", "rb");
    if (f) {
        fread(_data.data(), 1, _data.size() * sizeof(float), f);
        fclose(f);
    }
}

FlowVis::~FlowVis()
{
}

void FlowVis::initializeGL()
{
    Cg::OpenGLWidget::initializeGL();
    QVector3D center((_x_end + _x_start) / 2.0f, (_y_end + _y_start) / 2.0f, 0.0f);
    float radius = _x_end - center.x();
    navigator()->initialize(center, radius);

    // Set up buffer objects for the geometry
    const QVector<float> positions({
            _x_start, _y_end, 0.0f,   _x_end, _y_end, 0.0f,
            _x_end, _y_start, 0.0f,   _x_start, _y_start, 0.0f
    });
    const QVector<float> normals({
            0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f
    });
    const QVector<float> texcoords({
            0.0f, 0.0f,   1.0f, 0.0f,
            1.0f, 1.0f,   0.0f, 1.0f
    });
    const QVector<unsigned int> indices({
            0, 1, 3, 1, 2, 3
    });
    _vertexArrayObject = Cg::createVertexArrayObject(positions, normals, texcoords, indices);
    _indexCount = indices.size();
    CG_ASSERT_GLCHECK();

    // Set up texture for velocity
    glGenTextures(1, &this->_texture_velocity);
    glBindTexture(GL_TEXTURE_2D, this->_texture_velocity);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); // need to perform border chek in shader
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    CG_ASSERT_GLCHECK();

    // Set up texture for noise, and upload it
    glGenTextures(1, &this->_texture_noise);
    glBindTexture(GL_TEXTURE_2D, this->_texture_noise);
    Cg::loadIntoTexture("noisy-texture.png", GL_TEXTURE_2D, false);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    CG_ASSERT_GLCHECK();

    // Set up the programmable pipeline
    _prg.addShaderFromSourceCode(QOpenGLShader::Vertex,
            Cg::prependGLSLVersion(Cg::loadFile(":vs.glsl")));
    _prg.addShaderFromSourceCode(QOpenGLShader::Fragment,
            Cg::prependGLSLVersion(Cg::loadFile(":fs.glsl")));
    _prg.link();
    _prg.bind();
    _prg.setUniformValue("g_velocity", 0);
    _prg.setUniformValue("noise_tex", 1);
    _prg.setUniformValue("cell_dim", QVector2D(this->_x_cells, this->_y_cells));
    CG_ASSERT_GLCHECK();
}

QVector2D FlowVis::getFlowVector(int t, int y, int x)
{
    return QVector2D(
            _data[2 * (t * _y_cells * _x_cells + y * _x_cells + x) + 0],
            _data[2 * (t * _y_cells * _x_cells + y * _x_cells + x) + 1]);
}

void FlowVis::paintGL(const QMatrix4x4& P, const QMatrix4x4& V, int w, int h)
{
    Cg::OpenGLWidget::paintGL(P, V, w, h);

    
    if (_time_cell != _time_cell_in_texture) 
    {
        // Upload the actual velocity array corresponding to current time step
        const float* vector_data = this->_data.data() + 2 * (this->_time_cell * static_cast<int>(this->_y_cells * this->_x_cells));
        glBindTexture(GL_TEXTURE_2D, this->_texture_velocity);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, this->_x_cells, this->_y_cells, 0, GL_RG, GL_FLOAT, vector_data);
        this->_time_cell_in_texture = this->_time_cell;
    }

    // Set up view
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // Render: draw triangles
    _prg.bind();
    _prg.setUniformValue("projection_matrix", P);
    _prg.setUniformValue("modelview_matrix", V);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->_texture_velocity);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, this->_texture_noise);
    glBindVertexArray(_vertexArrayObject);
    glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_INT, 0);
    CG_ASSERT_GLCHECK();
}

void FlowVis::keyPressEvent(QKeyEvent* event)
{
    switch(event->key()) {
    case Qt::Key_Escape:
        quit();
        break;
    case Qt::Key_T:
        _time_cell++;
        if (_time_cell >= _t_cells)
            _time_cell = 0;
        break;
    }
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QSurfaceFormat format;
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(4, 5);
    QSurfaceFormat::setDefaultFormat(format);
    FlowVis example;
    Cg::init(argc, argv, &example);
    return app.exec();
}
