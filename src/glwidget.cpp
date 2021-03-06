#include "glwidget.h"
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVersionFunctionsFactory>
#include <QMouseEvent>
#include <QScreen>

GLWidget::GLWidget()
	: timer(new QTimer(this))
	, elapsedTimer(new QElapsedTimer)
{
	connect(timer, &QTimer::timeout, this, &GLWidget::onTimeout);

	timer->start();
	elapsedTimer->start();
}

GLWidget::~GLWidget()
{
	auto *f = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Core>();

	// Make sure the context is current before deleting resources
	makeCurrent();

	delete vShader;
	delete fShader;
	delete program;

	/* is this needed ??

	delete texture0;
	delete texture1;

	*/

	f->glDeleteVertexArrays(1, &vao);
	f->glDeleteBuffers(1, &vbo);
	f->glDeleteBuffers(1, &ebo);

	doneCurrent();

	delete elapsedTimer;
}

void GLWidget::loadShaders()
{
	// Build and compile our shader program

	vShader = new QOpenGLShader(QOpenGLShader::Vertex);
	vShader->compileSourceFile(files.value("vertex"));

	fShader = new QOpenGLShader(QOpenGLShader::Fragment);
	fShader->compileSourceFile(files.value("fragment"));

	program = new QOpenGLShaderProgram;
	program->addShader(vShader);
	program->addShader(fShader);
	program->link();
	program->bind();
}

void GLWidget::loadTextures()
{
	// Load our textures

	for (int i = 0; i < numTextures; i++) {
		textures.append(new QOpenGLTexture(QImage(files.value("texture" + QString::number(i))).mirrored()));
		textures.last()->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
		textures.last()->setMagnificationFilter(QOpenGLTexture::Linear);
		textures.last()->setWrapMode(QOpenGLTexture::Repeat);

		QByteArray ba = QString("iChannel" + QString::number(i)).toLatin1();
		program->setUniformValue(ba.data(), i);

		ba = QString("iChannelResolution[" + QString::number(i) + "]").toLatin1();
		program->setUniformValue(ba.data(), textures.last()->width(), textures.last()->height(), this->screen()->devicePixelRatio());
	}
}

void GLWidget::initializeGL()
{
	// Load shaders, initialize vertex data, etc.

	auto *f = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Core>();

	loadShaders();

	float vertices[] = {
		// positions          // texture coords
		 1.0f,  1.0f, 0.0f,   1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f,   1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,   0.0f, 1.0f
	};
	unsigned int indices[] = {
		0, 1, 3,
		1, 2, 3
	};

	f->glGenVertexArrays(1, &vao);
	f->glGenBuffers(1, &vbo);
	f->glGenBuffers(1, &ebo);

	f->glBindVertexArray(vao);

	f->glBindBuffer(GL_ARRAY_BUFFER, vbo);
	f->glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	f->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// position attribute
	f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	f->glEnableVertexAttribArray(0);

	// texture coord attribute
	f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	f->glEnableVertexAttribArray(1);

	loadTextures();
}

void GLWidget::resizeGL(int w, int h)
{
	Q_UNUSED(w);
	Q_UNUSED(h);

	// Do nothing
}

void GLWidget::paintGL()
{
	// Draw the scene

	auto *f = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Core>();

	program->setUniformValue("iResolution", this->width(), this->height(), this->screen()->devicePixelRatio());
	program->setUniformValue("iMouse", mousePos.x(), mousePos.y(), mousePos.z(), mousePos.w());

	GLfloat elapsed = elapsedTimer->elapsed();
	program->setUniformValue("iTime", elapsed / 1000);

	f->glActiveTexture(GL_TEXTURE0);
	textures.at(0)->bind();

	f->glActiveTexture(GL_TEXTURE1);
	textures.at(1)->bind();

	f->glActiveTexture(GL_TEXTURE2);
	textures.at(2)->bind();

	f->glActiveTexture(GL_TEXTURE3);
	textures.at(3)->bind();

	f->glBindVertexArray(vao);
	f->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
	// left button down while moving
	if (event->buttons() & Qt::LeftButton) {
		mousePos.setX(event->position().x());
		mousePos.setY(this->height() - event->position().y() - 1); // bottom is 0
	}
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
	// left button pressed
	if (event->buttons() & Qt::LeftButton) {
		mousePos.setX(event->position().x());
		mousePos.setY(this->height() - event->position().y() - 1);
		mousePos.setZ(event->position().x());
		mousePos.setW(this->height() - event->position().y() - 1);
	}
}

// SLOTS

void GLWidget::onTimeout()
{
	update();
}
