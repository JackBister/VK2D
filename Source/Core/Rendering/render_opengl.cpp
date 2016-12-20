#include <cmath>
#include <cstdio>
#include <cstddef>
#include <cstring>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
//Because GLEW seems to assume everyone building on Windows uses MSVC these defines are necessary
#ifdef __CYGWIN__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreserved-id-macro"
#define GLEW_STATIC 1
#define __int64 __INT64_TYPE__
#define _WIN32 1
#define _WIN64 1
#pragma GCC diagnostic pop
#endif
#define GLEW_STATIC 1
#include "GL/glew.h"
#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"
#include "stb_image.h"

#include "Core/Components/cameracomponent.h"
#include "Core/Rendering/OpenGL/OpenGLRendererData.h"
#include "Core/Rendering/Framebuffer.h"
#include "Core/Rendering/render.h"
#include "Core/ResourceManager.h"
#include "Core/Rendering/Image.h"
#include "Core/Rendering/Shader.h"
#include "Core/sprite.h"
#include "Core/transform.h"

//The number of floats contained in one vertex
//vec3 pos, vec3 color, vec2 UV
#define VERTEX_SIZE 8

//Realistically this should only be used to represent different quads
//Just wanted the convenience of putting VBO, EBO and verts together
struct Model
{
	size_t verticesLength;
	float * vertices;
	size_t elementsLength;
	GLuint * elements;
	GLuint vbo;
	GLuint ebo;
};

struct Program
{
	GLuint id = 0;

	//Not really sure where the VAO really should be... it is the same for all runs of the program AFAICT
	GLuint vao;

	Program() {}
	Program(Shader vShader, Shader fShader)
	{
		if (((OpenGLShaderRendererData *)vShader.rendererData)->shader == 0 || ((OpenGLShaderRendererData *)fShader.rendererData)->shader == 0) {
			return;
		}
		id = glCreateProgram();
		glAttachShader(id, ((OpenGLShaderRendererData *)vShader.rendererData)->shader);
		glAttachShader(id, ((OpenGLShaderRendererData *)fShader.rendererData)->shader);
		glBindFragDataLocation(id, 0, "outColor");
		glLinkProgram(id);
		GLint linkOk;
		glGetProgramiv(id, GL_LINK_STATUS, &linkOk);
		if (linkOk != GL_TRUE) {
			GLsizei log_length = 0;
			GLchar message[1024];
			glGetProgramInfoLog(id, 1024, &log_length, message);
			glDeleteProgram(id);
			id = 0;
			printf("Program link failed\n %s\n", message);
		}
	}
};

struct OpenGLRenderer : Renderer
{
	void Destroy() final override;
	void EndFrame() final override;
	bool Init(ResourceManager *, const char * title, int winX, int winY, int w, int h, uint32_t flags) final override;
	void RenderCamera(CameraComponent * const) final override;
	uint64_t GetFrameTime() final override;

	void AddSprite(Sprite * const) final override;
	void DeleteSprite(Sprite * const) final override;

	void AddCamera(CameraComponent * const) final override;
	void DeleteCamera(CameraComponent * const) final override;

	void AddFramebuffer(Framebuffer * const) final override;
	void DeleteFramebuffer(Framebuffer * const) final override;

	void AddImage(Image * const) final override;
	void DeleteImage(Image * const) final override;

	void AddShader(Shader * const) final override;
	void DeleteShader(Shader * const) final override;

	ResourceManager * resourceManager;

	//The aspect ratio of the viewport
	//TODO: Should this be attached to the camera?
	float aspectRatio;
	//All cameras the renderer knows about
	//TODO: Should Camera be separated from CameraComponent like Sprite from SpriteComponent?
	//TODO: Does Camera need a rendererData?
	std::vector<CameraComponent *> cameras;
	//The dimensions of the screen, in pixels
	glm::ivec2 dimensions;
	//All sprites the renderer knows about
	std::vector<Sprite *> sprites;
	//The window
	SDL_Window * window;

	//TODO: Image and Framebuffer can be stored here along with pointer too.
	OpenGLImageRendererData backbufferTexture;
	OpenGLFramebufferRendererData backbuffer;
	std::shared_ptr<Image> backBufferImage;
	std::shared_ptr<Framebuffer> backbufferRenderTarget;

	GLuint timeQuery;
};

//A quad with color 1.0, 1.0, 1.0
static Model plainQuad;
static GLuint plainQuadElems[] = {
	0, 1, 2,
	2, 3, 0
};
static float plainQuadVerts[] = {
	//vec3 pos, vec3 color, vec2 texcoord
	-1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, // Top left
	1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, // Top right
	1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, // Bottom right
	-1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, // Bottom left
};
//The passthrough-transform program
static Program ptProgram;

Renderer * GetOpenGLRenderer()
{
	return new OpenGLRenderer();
}

void OpenGLRenderer::Destroy()
{
	SDL_DestroyWindow(window);
	valid = false;
}

void OpenGLRenderer::EndFrame()
{
	glBeginQuery(GL_TIME_ELAPSED, timeQuery);
	for (auto& camera : cameras) {
		RenderCamera(camera);
	}
	glBindTexture(GL_TEXTURE_2D, backbufferTexture.texture);
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 0, 0, dimensions.x, dimensions.y, 0);
	SDL_GL_SwapWindow(window);
	glEndQuery(GL_TIME_ELAPSED);
}

bool OpenGLRenderer::Init(ResourceManager * resMan, const char * title, const int winX, const int winY, const int w, const int h, const uint32_t flags)
{
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	resourceManager = resMan;
	aspectRatio = static_cast<float>(w) / static_cast<float>(h);
	dimensions = glm::ivec2(w, h);
	window = SDL_CreateWindow(title, winX, winY, w, h, flags | SDL_WINDOW_OPENGL);
	SDL_GL_CreateContext(window);

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
		return false;
	}
	printf("Program start glGetError(Expected 1280): %d\n", glGetError());

	stbi_set_flip_vertically_on_load(true);

	glGenQueries(1, &timeQuery);

	//TODO: depth and stencil buffers as images
	ImageCreateInfo backBufferTexCreateInfo = {
		"__Scratch/Backbuffer.tex",
		Image::Format::RGBA8,
		h, w,
		&backbufferTexture
	};
	backBufferImage = std::make_shared<Image>(backBufferTexCreateInfo);

	glGenTextures(1, &backbufferTexture.texture);
	glBindTexture(GL_TEXTURE_2D, backbufferTexture.texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	resMan->AddResourceRefCounted<Image>("__Scratch/Backbuffer.tex", backBufferImage);

	//No idea why it wont let me construct this as I normally would.
/*
	FramebufferCreateInfo backbufferFBCreateInfo = {
		"__Scratch/Backbuffer.rendertarget",
		{{Framebuffer::Attachment::COLOR0, backBufferImage}},
		&backbuffer
	};
*/
	FramebufferCreateInfo backbufferFBCreateInfo;
	backbufferFBCreateInfo.name = "__Scratch/Backbuffer.rendertarget";
	backbufferFBCreateInfo.imgs = {{Framebuffer::Attachment::COLOR0, backBufferImage }};
	backbufferFBCreateInfo.rendererData = &backbuffer;

	backbufferRenderTarget = std::make_shared<Framebuffer>(backbufferFBCreateInfo);
	resMan->AddResourceRefCounted<Framebuffer>("__Scratch/Backbuffer.rendertarget", backbufferRenderTarget);

	glReadBuffer(GL_BACK);

	glGenBuffers(1, &plainQuad.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, plainQuad.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(plainQuadVerts), plainQuadVerts, GL_STATIC_DRAW);
	plainQuad.vertices = plainQuadVerts;
	plainQuad.verticesLength = 4 * VERTEX_SIZE;
	glGenBuffers(1, &plainQuad.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plainQuad.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(plainQuadElems), plainQuadElems, GL_STATIC_DRAW);
	plainQuad.elements = plainQuadElems;
	plainQuad.elementsLength = 6;

	auto ptvShader = resourceManager->LoadResourceRefCounted<Shader>("shaders/passthrough-transform.vert");
	auto pfShader = resourceManager->LoadResourceRefCounted<Shader>("shaders/passthrough.frag");
	ptProgram = Program(*ptvShader, *pfShader);
	glGenVertexArrays(1, &ptProgram.vao);
	glBindVertexArray(ptProgram.vao);
	glUseProgram(ptProgram.id);

	GLint posLoc = glGetAttribLocation(ptProgram.id, "pos");
	glEnableVertexAttribArray(posLoc);
	glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), nullptr);

	GLint normLoc = glGetAttribLocation(ptProgram.id, "color");
	glEnableVertexAttribArray(normLoc);
	glVertexAttribPointer(normLoc, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));

	GLint coordLoc = glGetAttribLocation(ptProgram.id, "texcoord");
	glEnableVertexAttribArray(coordLoc);
	glVertexAttribPointer(coordLoc, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void *>(6 * sizeof(GLfloat)));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	valid = true;
	return true;
}

void OpenGLRenderer::RenderCamera(CameraComponent * camera)
{
	GLenum err = glGetError();
	if (err != 0) {
		printf("EndFrame start glGetError %d\n", err);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, ((OpenGLFramebufferRendererData *)camera->GetRenderTarget()->GetRendererData())->framebuffer);
	//TODO: Maybe unnecessary
	glBindVertexArray(ptProgram.vao);
	glBindBuffer(GL_ARRAY_BUFFER, plainQuad.vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plainQuad.ebo);
	glUseProgram(ptProgram.id);
	//TODO: Only needs to be done once
	GLint minUVloc = glGetUniformLocation(ptProgram.id, "minUV");
	GLint sizeUVloc = glGetUniformLocation(ptProgram.id, "sizeUV");
	GLint mLoc = glGetUniformLocation(ptProgram.id, "model");
	GLint vLoc = glGetUniformLocation(ptProgram.id, "view");
	glUniformMatrix4fv(vLoc, 1, GL_FALSE, value_ptr(camera->GetViewMatrix()));
	GLint pLoc = glGetUniformLocation(ptProgram.id, "projection");
	glUniformMatrix4fv(pLoc, 1, GL_FALSE, value_ptr(camera->GetProjectionMatrix()));
	GLint texLoc = glGetUniformLocation(ptProgram.id, "tex");
	glUniform1i(texLoc, 0);
	glActiveTexture(GL_TEXTURE0);

	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	err = glGetError();
	if (err != 0) {
		printf("EndFrame pre loop glGetError %d\n", err);
	}
	//TODO: Switch frag shader depending on components
	for (auto& s : sprites) {
		//On the screen, a 1x1 quad should occupy one pixel, but the user shouldn't be exposed to this
		//This is not the best way of doing it - it assumes the scale changes each frame
	//	vec3 oldScale = s.sprite->transform->scale;
		//TODO: HACK: The /2.f is there because otherwise stuff is twice as big as it should be... but I don't know why.
	//	s.sprite->transform->scale = glm::vec3(oldScale.x * s.sprite->dimensions.x / 2.f, oldScale.y * s.sprite->dimensions.y / 2.f, oldScale.z);
		glUniform2f(minUVloc, s->minUV.x, s->minUV.y);
		glUniform2f(sizeUVloc, s->sizeUV.x, s->sizeUV.y);
		glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(s->transform->GetLocalToWorldSpace()));
		glBindTexture(GL_TEXTURE_2D, ((OpenGLImageRendererData *)s->image->GetRendererData())->texture);
	//	glBindTexture(GL_TEXTURE_2D, s.rendererData.texture);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	//	s.sprite->transform->scale = oldScale;
	}

	err = glGetError();
	if (err != 0) {
		printf("EndFrame glGetError %d\n", err);
	}
}

uint64_t OpenGLRenderer::GetFrameTime()
{
	uint64_t ret = 0;
	glGetQueryObjectui64v(timeQuery, GL_QUERY_RESULT, &ret);
	return ret;
}

void OpenGLRenderer::AddSprite(Sprite * const sprite)
{
	sprites.push_back(sprite);
}

void OpenGLRenderer::DeleteSprite(Sprite * const sprite)
{
	for (auto it = sprites.begin(); it != sprites.end(); ++it) {
		if (*it == sprite) {
			sprites.erase(it);
		}
	}
}

void OpenGLRenderer::AddCamera(CameraComponent * const camera)
{
	cameras.push_back(camera);
}

void OpenGLRenderer::DeleteCamera(CameraComponent * const camera)
{
	for (auto it = cameras.begin(); it != cameras.end(); ++it) {
		if (*it == camera) {
			cameras.erase(it);
		}
	}
}

void OpenGLRenderer::AddFramebuffer(Framebuffer * const fb)
{
	auto rData = new OpenGLFramebufferRendererData();
	fb->SetRendererData(rData);
	glGenFramebuffers(1, &rData->framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, rData->framebuffer);
	for (auto& kv : fb->GetImages()) {
		GLuint attachment;
		switch (kv.first) {
		case Framebuffer::Attachment::COLOR0:
			attachment = GL_COLOR_ATTACHMENT0;
			break;
		case Framebuffer::Attachment::DEPTH:
			attachment = GL_DEPTH_ATTACHMENT;
			break;
		case Framebuffer::Attachment::STENCIL:
			attachment = GL_STENCIL_ATTACHMENT;
			break;
		case Framebuffer::Attachment::DEPTH_STENCIL:
			attachment = GL_DEPTH_STENCIL_ATTACHMENT;
			break;
		default:
			printf("[WARNING] Unknown framebuffer attachment %d.", kv.first);
			attachment = GL_COLOR_ATTACHMENT0;
		}
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, ((OpenGLImageRendererData *)kv.second->GetRendererData())->texture, 0);
	}
}

void OpenGLRenderer::DeleteFramebuffer(Framebuffer * const fb)
{
	glDeleteFramebuffers(1, &((OpenGLFramebufferRendererData *)fb->GetRendererData())->framebuffer);
}

void OpenGLRenderer::AddImage(Image * const img)
{
	auto rData = new OpenGLImageRendererData();
	img->SetRendererData(rData);
	GLenum format;
	GLint internalFormat;
	switch (img->GetFormat()) {
	case Image::Format::R8:
		format = GL_RED;
		internalFormat = GL_R8;
		break;
	case Image::Format::RG8:
		format = GL_RG;
		internalFormat = GL_RG8;
		break;
	case Image::Format::RGB8:
		format = GL_RGB;
		internalFormat = GL_RGB8;
		break;
	case Image::Format::RGBA8:
		format = GL_RGBA;
		internalFormat = GL_RGBA8;
		break;
	default:
		printf("[TODO] Unimplemented image format %d\n", img->GetFormat());
		break;
	}
	glGenTextures(1, &(rData->texture));
	glBindTexture(GL_TEXTURE_2D, rData->texture);
	//TODO: Let user specify this
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, img->GetWidth(), img->GetHeight(), 0, format, GL_UNSIGNED_BYTE, img->GetData().data());
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
}

void OpenGLRenderer::DeleteImage(Image * const img)
{
	auto rData = (OpenGLImageRendererData *)img->GetRendererData();
	if (rData->texture != 0) {
		glDeleteTextures(1, &(rData->texture));
	}
}

void OpenGLRenderer::AddShader(Shader * const s)
{
	auto rData = new OpenGLShaderRendererData();
	s->rendererData = rData;
	GLuint shaderType;
	auto type = s->GetType();
	switch (type) {
	case Shader::Type::VERTEX_SHADER:
		shaderType = GL_VERTEX_SHADER;
		break;
	case Shader::Type::FRAGMENT_SHADER:
		shaderType = GL_FRAGMENT_SHADER;
		break;
	case Shader::Type::GEOMETRY_SHADER:
		shaderType = GL_GEOMETRY_SHADER;
		break;
	case Shader::Type::COMPUTE_SHADER:
		shaderType = GL_COMPUTE_SHADER;
		break;
	}
	rData->shader = glCreateShader(shaderType);
	const char * src = s->GetSource().c_str();
	glShaderSource(rData->shader, 1, &src, nullptr);
	glCompileShader(rData->shader);
	GLint compileOk;
	glGetShaderiv(rData->shader, GL_COMPILE_STATUS, &compileOk);
	if (compileOk != GL_TRUE) {
		GLsizei log_length = 0;
		GLchar message[1024];
		glGetShaderInfoLog(rData->shader, 1024, &log_length, message);
		printf("Shader compile failed\n %s %s\n", s->name, message);
		glDeleteShader(rData->shader);
		rData->shader = 0;
	}
}

void OpenGLRenderer::DeleteShader(Shader * const s)
{
	auto rData = (OpenGLShaderRendererData *)s->rendererData;
	if (rData->shader != 0) {
		glDeleteShader(rData->shader);
	}
}

