#ifndef FLOWVIS_HPP
#define FLOWVIS_HPP

#include <QOpenGLShaderProgram>
#include <QVector3D>

#include "cgbase/cgopenglwidget.hpp"

class FlowVis : public Cg::OpenGLWidget
{
private:
    // Constants describing the flow data set
    static constexpr const char* _filename = "flow.raw";
    static constexpr int _x_cells = 400;
    static constexpr float _x_start = -0.5f;
    static constexpr float _x_end = +7.5f;
    static constexpr float _x_step = (_x_end - _x_start) / _x_cells;
    static constexpr int _y_cells = 50;
    static constexpr float _y_start = -0.5f;
    static constexpr float _y_end = +0.5f;
    static constexpr float _y_step = (_y_end - _y_start) / _y_cells;
    static constexpr int _t_cells = 1001;
    static constexpr float _t_start = 15.0f;
    static constexpr float _t_end = 23.0f;
    static constexpr float _t_step = (_t_end - _t_start) / _t_cells;
    // The flow data
    QVector<float> _data;
    // State
    int _time_cell;
    int _time_cell_in_texture;
    // OpenGL objects
    unsigned int _texture_noise,
        _texture_velocity;
    unsigned int _vertexArrayObject;
    unsigned int _indexCount;
    QOpenGLShaderProgram _prg;

    QVector2D getFlowVector(int t, int y, int x);

public:
    FlowVis();
    ~FlowVis();

    void initializeGL() override;
    void paintGL(const QMatrix4x4& P, const QMatrix4x4& V, int w, int h) override;
    void keyPressEvent(QKeyEvent* event) override;
};

#endif
