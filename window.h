#pragma once

#include "openglwidget.h"

#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QPointF>
#include <QSizeF>
#include <QVector2D>
#include <QWindow>

#include <memory>

class QLabel;
class QSlider;
class QWidget;

class Window : public OpenGLWidget {
  Q_OBJECT

public:
  Window();
  ~Window() override;

  void onInit() override;
  void onRender() override;
  void onResize(size_t width, size_t height) override;

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private:
  void setupUi();
  void layoutUi();

  void rebuildFractal();

  QVector2D screenToWorld(const QPointF &logicalPos) const;
  void zoomAt(const QPointF &logicalPos, float factor);
  QMatrix4x4 mvp() const;

  QVector2D center_{0.0f, 0.0f};
  float pixelsPerUnit_ = 200.0f;
  QSizeF viewportPx_{640.0f, 480.0f};
  float devicePixelRatio_ = 1.0f;

  int depth_ = 25;
  int angleDeg_ = 45;
  bool geometryDirty_ = false;
  int fractalVertexCount_ = 0;

  bool dragging_ = false;
  QPointF lastMousePos_;

  QWidget *controlBar_ = nullptr;
  QLabel *depthValueLabel_ = nullptr;
  QLabel *angleValueLabel_ = nullptr;
  QLabel *fpsLabel_ = nullptr;
  QSlider *depthSlider_ = nullptr;
  QSlider *angleSlider_ = nullptr;

  std::unique_ptr<QOpenGLShaderProgram> program_;

  QOpenGLVertexArrayObject vao_;
  QOpenGLBuffer vbo_{QOpenGLBuffer::VertexBuffer};

  GLint mvpUniform_ = -1;
};
