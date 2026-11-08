#include "window.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QSlider>
#include <QWheelEvent>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace {

struct FractalVertex {
  float x;
  float y;
  float z;
  float depth;
};

void addTriangle(std::vector<FractalVertex> &out, int &primitiveCounter,
                 const QVector2D &a, const QVector2D &b, const QVector2D &c,
                 float depthNorm) {
  const float z = static_cast<float>(primitiveCounter++);
  out.push_back({a.x(), a.y(), z, depthNorm});
  out.push_back({b.x(), b.y(), z, depthNorm});
  out.push_back({c.x(), c.y(), z, depthNorm});
}

void generateSquare(std::vector<FractalVertex> &out, int &primitiveCounter,
                    const QVector2D &origin, float cosT, float sinT, float side,
                    int depth, int maxDepth, float ca, float sa) {
  const QVector2D dir(cosT, sinT);
  const QVector2D up(-sinT, cosT);

  const QVector2D a = origin;
  const QVector2D b = a + dir * side;
  const QVector2D d = a + up * side;
  const QVector2D c = b + up * side;

  const float depthNorm =
      static_cast<float>(depth) / static_cast<float>(maxDepth);
  addTriangle(out, primitiveCounter, a, b, c, depthNorm);
  addTriangle(out, primitiveCounter, a, c, d, depthNorm);

  if (depth >= maxDepth) {
    return;
  }

  const QVector2D apex = d + dir * (side * ca * ca) + up * (side * sa * ca);
  addTriangle(out, primitiveCounter, d, c, apex,
              static_cast<float>(depth + 1) / static_cast<float>(maxDepth));

  if (depth + 1 >= maxDepth) {
    return;
  }

  const float c1 = cosT * ca - sinT * sa;
  const float s1 = sinT * ca + cosT * sa;
  generateSquare(out, primitiveCounter, d, c1, s1, side * ca, depth + 2,
                 maxDepth, ca, sa);
  generateSquare(out, primitiveCounter, apex, s1, -c1, side * sa, depth + 2,
                 maxDepth, ca, sa);
}

void buildFractal(int maxDepth, float angleDeg,
                  std::vector<FractalVertex> &out) {
  const float alpha = angleDeg * (M_PIf / 180.0f);
  const float ca = std::cos(alpha);
  const float sa = std::sin(alpha);

  out.clear();

  int primitiveCount = 0;

  generateSquare(out, primitiveCount, QVector2D(0.0f, 0.0f), 1.0f, 0.0f, 1.0f,
                 0, maxDepth, ca, sa);

  for (FractalVertex &vertex : out) {
    vertex.z = 1.0f - (vertex.z + 1.0f) / (primitiveCount + 1.0f);
  }
}

} // namespace

Window::Window() {
  setFocusPolicy(Qt::StrongFocus);

  setupUi();
  layoutUi();
}

Window::~Window() {
  if (context()) {
    makeCurrent();
    vbo_.destroy();
    vao_.destroy();
    program_.reset();
    doneCurrent();
  }
}

void Window::setupUi() {
  controlBar_ = new QWidget(this);
  controlBar_->setObjectName("controlBar");

  auto *layout = new QHBoxLayout(controlBar_);

  auto *depthLabel = new QLabel("Depth", controlBar_);
  depthSlider_ = new QSlider(Qt::Horizontal, controlBar_);
  depthSlider_->setRange(0, 40);
  depthSlider_->setValue(depth_);
  depthValueLabel_ = new QLabel(QString::number(depth_), controlBar_);
  depthValueLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  auto *angleLabel = new QLabel("Angle", controlBar_);
  angleSlider_ = new QSlider(Qt::Horizontal, controlBar_);
  angleSlider_->setRange(1, 89);
  angleSlider_->setValue(angleDeg_);
  angleValueLabel_ =
      new QLabel(QStringLiteral("%1 deg").arg(angleDeg_), controlBar_);
  angleValueLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  layout->addWidget(depthLabel);
  layout->addWidget(depthSlider_, 1);
  layout->addWidget(depthValueLabel_);
  layout->addWidget(angleLabel);
  layout->addWidget(angleSlider_, 1);
  layout->addWidget(angleValueLabel_);

  connect(depthSlider_, &QSlider::valueChanged, this, [this](int value) {
    depth_ = value;
    depthValueLabel_->setText(QString::number(depth_));
    geometryDirty_ = true;
    update();
  });

  connect(angleSlider_, &QSlider::valueChanged, this, [this](int value) {
    angleDeg_ = value;
    angleValueLabel_->setText(QStringLiteral("%1 deg").arg(angleDeg_));
    geometryDirty_ = true;
    update();
  });

  fpsLabel_ = new QLabel("FPS: ", this);
  fpsLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);
  fpsLabel_->adjustSize();

  connect(this, &Window::fpsUpdated, this, [this](double fps) {
    fpsLabel_->setText(QStringLiteral("FPS: %1").arg(fps, 0, 'f', 0));
    fpsLabel_->adjustSize();
  });
}

void Window::layoutUi() {
  if (controlBar_) {
    controlBar_->setGeometry(10, 10, std::max(0, width() - 2 * 20), 50);
    controlBar_->raise();
  }

  if (fpsLabel_) {
    fpsLabel_->adjustSize();
    fpsLabel_->move(10, height() - fpsLabel_->height() - 10);
    fpsLabel_->raise();
  }
}

void Window::onInit() {
  program_ = std::make_unique<QOpenGLShaderProgram>(this);
  if (!program_->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                         ":/shaders/fractal.vs") ||
      !program_->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                         ":/shaders/fractal.fs") ||
      !program_->link()) {
    qDebug() << "Shader error:" << program_->log();
  }

  mvpUniform_ = program_->uniformLocation("mvp");

  vao_.create();
  vao_.bind();

  vbo_.create();
  vbo_.bind();
  vbo_.setUsagePattern(QOpenGLBuffer::DynamicDraw);

  program_->bind();

  constexpr GLsizei stride = static_cast<GLsizei>(sizeof(FractalVertex));
  program_->enableAttributeArray(0);
  program_->setAttributeBuffer(
      0, GL_FLOAT, static_cast<int>(offsetof(FractalVertex, x)), 2, stride);
  program_->enableAttributeArray(1);
  program_->setAttributeBuffer(
      1, GL_FLOAT, static_cast<int>(offsetof(FractalVertex, z)), 1, stride);
  program_->enableAttributeArray(2);
  program_->setAttributeBuffer(
      2, GL_FLOAT, static_cast<int>(offsetof(FractalVertex, depth)), 1, stride);

  program_->release();

  rebuildFractal();

  vao_.release();
  vbo_.release();

  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glDepthMask(GL_TRUE);
  glClearColor(0.1, 0.1, 0.1, 1.0);
}

void Window::rebuildFractal() {
  std::vector<FractalVertex> vertices;

  buildFractal(depth_, static_cast<float>(angleDeg_), vertices);

  fractalVertexCount_ = static_cast<int>(vertices.size());
  vbo_.bind();
  vbo_.setUsagePattern(QOpenGLBuffer::DynamicDraw);
  vbo_.allocate(vertices.data(),
                static_cast<int>(vertices.size() * sizeof(FractalVertex)));
  vbo_.release();

  geometryDirty_ = false;
}

QMatrix4x4 Window::mvp() const {
  const float halfWidthWorld =
      static_cast<float>(viewportPx_.width()) / (2.0f * pixelsPerUnit_);
  const float halfHeightWorld =
      static_cast<float>(viewportPx_.height()) / (2.0f * pixelsPerUnit_);

  QMatrix4x4 matrix;
  matrix.ortho(center_.x() - halfWidthWorld, center_.x() + halfWidthWorld,
               center_.y() - halfHeightWorld, center_.y() + halfHeightWorld,
               -1.0f, 1.0f);
  return matrix;
}

void Window::onRender() {
  if (geometryDirty_) {
    rebuildFractal();
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glDepthMask(GL_TRUE);

  program_->bind();
  program_->setUniformValue(mvpUniform_, mvp());

  vao_.bind();
  glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(fractalVertexCount_));
  vao_.release();

  program_->release();

  update();
}

void Window::onResize(const size_t width, const size_t height) {
  devicePixelRatio_ = static_cast<float>(devicePixelRatioF());
  viewportPx_ = QSizeF(static_cast<double>(width) * devicePixelRatio_,
                       static_cast<double>(height) * devicePixelRatio_);

  glViewport(0, 0, static_cast<GLsizei>(viewportPx_.width()),
             static_cast<GLsizei>(viewportPx_.height()));

  layoutUi();
}

QVector2D Window::screenToWorld(const QPointF &logicalPos) const {
  const float x = static_cast<float>(logicalPos.x()) * devicePixelRatio_;
  const float y = static_cast<float>(logicalPos.y()) * devicePixelRatio_;
  const float halfWidth = static_cast<float>(viewportPx_.width()) * 0.5f;
  const float halfHeight = static_cast<float>(viewportPx_.height()) * 0.5f;

  return QVector2D(center_.x() + (x - halfWidth) / pixelsPerUnit_,
                   center_.y() + (halfHeight - y) / pixelsPerUnit_);
}

void Window::zoomAt(const QPointF &logicalPos, float factor) {
  const QVector2D worldBefore = screenToWorld(logicalPos);

  const float newScale = std::clamp(pixelsPerUnit_ * factor, 0.001f, 1.0e6f);
  if (newScale == pixelsPerUnit_) {
    return;
  }
  pixelsPerUnit_ = newScale;

  const float x = static_cast<float>(logicalPos.x()) * devicePixelRatio_;
  const float y = static_cast<float>(logicalPos.y()) * devicePixelRatio_;
  const float halfWidth = static_cast<float>(viewportPx_.width()) * 0.5f;
  const float halfHeight = static_cast<float>(viewportPx_.height()) * 0.5f;

  center_.setX(worldBefore.x() - (x - halfWidth) / pixelsPerUnit_);
  center_.setY(worldBefore.y() - (halfHeight - y) / pixelsPerUnit_);

  update();
}

void Window::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    dragging_ = true;
    lastMousePos_ = event->position();
    setCursor(Qt::ClosedHandCursor);
    event->accept();
    return;
  }
  OpenGLWidget::mousePressEvent(event);
}

void Window::mouseMoveEvent(QMouseEvent *event) {
  if (dragging_ && (event->buttons() & Qt::LeftButton)) {
    const QPointF delta = event->position() - lastMousePos_;
    const float dx = static_cast<float>(delta.x()) * devicePixelRatio_;
    const float dy = static_cast<float>(delta.y()) * devicePixelRatio_;

    center_.setX(center_.x() - dx / pixelsPerUnit_);
    center_.setY(center_.y() + dy / pixelsPerUnit_);

    lastMousePos_ = event->position();
    update();
    event->accept();
    return;
  }
  OpenGLWidget::mouseMoveEvent(event);
}

void Window::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    dragging_ = false;
    unsetCursor();
    event->accept();
    return;
  }
  OpenGLWidget::mouseReleaseEvent(event);
}

void Window::wheelEvent(QWheelEvent *event) {
  const int delta = event->angleDelta().y();
  if (delta == 0) {
    event->ignore();
    return;
  }

  const float steps = static_cast<float>(delta) / 120.0f;
  zoomAt(event->position(), std::pow(1.2f, steps));
  event->accept();
}