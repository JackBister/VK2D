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

#include "cameracomponent.h"
#include "render.h"
#include "transform.h"

using namespace glm;
using namespace std;

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

struct OpenGLRendererData
{
	GLuint texture = 0;
};

//Puts a sprite and its rendererData together, for better cache locality
struct OpenGLSprite
{
	Sprite * sprite;
	OpenGLRendererData rendererData;
};

struct Shader
{
	GLuint id = 0;
	char *src = nullptr;
	bool isCompiled = false;

	static Shader FromFile(const char * const fn, GLenum type)
	{
		Shader ret;
		FILE * f = fopen(fn, "rb");
		if (f == nullptr) {
			return ret;
		}
		int err = fseek(f, 0, SEEK_END);
		if (err) {
			return ret;
		}
		size_t len = static_cast<size_t>(ftell(f));
		err = fseek(f, 0, SEEK_SET);
		if (err) {
			return ret;
		}
		//from here on out if anything fails src must be set to nullptr and id to 0 before returning
		//these two things indicate that the shader is invalid
		ret.src = static_cast<char *>(malloc(len + 1));
		if (ret.src == nullptr) {
			return ret;
		}
		fread(ret.src, 1, len, f);
		//null terminate string
		ret.src[len] = '\0';
		ret.id = glCreateShader(type);
		if (ret.id == 0) {
			free(ret.src);
			ret.src = nullptr;
			return ret;
		}
		glShaderSource(ret.id, 1, &ret.src, nullptr);
		glCompileShader(ret.id);
		GLint compileOk;
		glGetShaderiv(ret.id, GL_COMPILE_STATUS, &compileOk);
		if (compileOk != GL_TRUE) {
			GLsizei log_length = 0;
			GLchar message[1024];
			glGetShaderInfoLog(ret.id, 1024, &log_length, message);
			printf("Shader compile failed\n %s %s\n", ret.src, message);
			glDeleteShader(ret.id);
			ret.id = 0;
			free(ret.src);
			ret.src = nullptr;
		}
		return ret;
	}
};

struct Program
{
	GLuint id = 0;
	Shader shaders[2];

	//Not really sure where the VAO really should be... it is the same for all runs of the program AFAICT
	GLuint vao;

	Program() {}
	Program(Shader vShader, Shader fShader)
	{
		if (vShader.id == 0 || fShader.id == 0) {
			return;
		}
		id = glCreateProgram();
		glAttachShader(id, vShader.id);
		glAttachShader(id, fShader.id);
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
			//TODO:
			printf("Program link failed\n %s\n", message);
		}
	}
};

struct OpenGLRenderer : Renderer
{
	void Destroy() override;
	void EndFrame() override;
	bool Init(const char * title, int winX, int winY, int w, int h, uint32_t flags) override;
	void RenderCamera(CameraComponent * const) override;

	void AddSprite(Sprite * const) override;
	void DeleteSprite(Sprite * const) override;

	void AddCamera(CameraComponent * const) override;
	void DeleteCamera(CameraComponent * const) override;

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
	std::vector<OpenGLSprite> sprites;
	//The window
	SDL_Window * window;
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
	for (auto& camera : cameras) {
		RenderCamera(camera);
	}
	SDL_GL_SwapWindow(window);
}

bool OpenGLRenderer::Init(const char * title, const int winX, const int winY, const int w, const int h, const uint32_t flags)
{
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	aspectRatio = static_cast<float>(w) / static_cast<float>(h);
	dimensions = ivec2(w, h);
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

	Shader ptvShader = Shader::FromFile("shaders/passthrough-transform.vert", GL_VERTEX_SHADER);
	Shader pfShader = Shader::FromFile("shaders/passthrough.frag", GL_FRAGMENT_SHADER);
	ptProgram = Program(ptvShader, pfShader);
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

	valid = true;
	return true;
}

void OpenGLRenderer::RenderCamera(CameraComponent * camera)
{
	GLenum err = glGetError();
	if (err != 0) {
		printf("EndFrame start glGetError %d\n", err);
	}
	//TODO: Maybe unnecessary
	glBindVertexArray(ptProgram.vao);
	glBindBuffer(GL_ARRAY_BUFFER, plainQuad.vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plainQuad.ebo);
	glUseProgram(ptProgram.id);
	//TODO: Only needs to be done once
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
		glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(s.sprite->transform->GetLocalToWorldSpace()));
		glBindTexture(GL_TEXTURE_2D, s.rendererData.texture);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	//	s.sprite->transform->scale = oldScale;
	}

	err = glGetError();
	if (err != 0) {
		printf("EndFrame glGetError %d\n", err);
	}
}

void OpenGLRenderer::AddSprite(Sprite * const sprite)
{
	OpenGLSprite s;
	s.sprite = sprite;
	sprite->rendererData = &s.rendererData;
	GLenum format;
	GLint internalFormat;
	switch (sprite->components) {
	case 1:
		format = GL_RED;
		internalFormat = GL_R8;
		break;
	case 2:
		format = GL_RG;
		internalFormat = GL_RG8;
		break;
	case 3:
		format = GL_RGB;
		internalFormat = GL_RGB8;
		break;
	case 4:
		format = GL_RGBA;
		internalFormat = GL_RGBA8;
		break;
	default:
		//TODO: Sprite doesn't get added but program keeps running. Maybe should crash.
		printf("[WARNING] Weird sprite format! %d\n", sprite->components);
		return;
	}
	glGenTextures(1, &s.rendererData.texture);
	glBindTexture(GL_TEXTURE_2D, s.rendererData.texture);
	//TODO: Let user specify this
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, sprite->dimensions.x, sprite->dimensions.y, 0, format, GL_UNSIGNED_BYTE, sprite->data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	sprites.push_back(s);
}

void OpenGLRenderer::DeleteSprite(Sprite * const sprite)
{
	for (auto it = sprites.begin(); it != sprites.end(); ++it) {
		if (it->sprite == sprite) {
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

