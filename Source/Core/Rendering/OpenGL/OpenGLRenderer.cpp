#include "Core/Rendering/OpenGL/OpenGLRenderer.h"

#include <glm/gtc/type_ptr.hpp>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <SDL/SDL_syswm.h>
#include <stb_image.h>

#include "Core/Components/CameraComponent.h"
#include "Core/Rendering/Framebuffer.h"
#include "Core/Rendering/Image.h"
#include "Core/Rendering/OpenGL/OpenGLEnumConversions.h"
#include "Core/Rendering/OpenGL/OpenGLRenderContext.h"
#include "Core/Rendering/Program.h"
#include "Core/Rendering/Shader.h"
#include "Core/Rendering/ViewDef.h"
#include "Core/ResourceManager.h"
#include "Core/Sprite.h"

//The number of floats contained in one vertex
//vec3 pos, vec3 normal, vec2 UV
#define VERTEX_SIZE 8

OpenGLFramebufferHandle Renderer::Backbuffer;
OpenGLShaderModuleHandle Renderer::ptVertexModule;
OpenGLShaderModuleHandle Renderer::ptFragmentModule;

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

Renderer::~Renderer() noexcept
{
	SDL_DestroyWindow(window);
}

std::unique_ptr<OpenGLRenderCommandContext> Renderer::CreateCommandContext()
{
	return std::make_unique<OpenGLRenderCommandContext>(new std::allocator<uint8_t>());
}

void Renderer::EndFrame(std::vector<std::unique_ptr<RenderCommandContext>>& commandBuffers) noexcept
{
	glBeginQuery(GL_TIME_ELAPSED, timeQuery);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	for (auto& ctx : commandBuffers) {
		ctx->Execute();
	}
	glBindTexture(GL_TEXTURE_2D, backbuffer.nativeHandle);
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 0, 0, dimensions.x, dimensions.y, 0);
	SDL_GL_SwapWindow(window);
	auto currTime = std::chrono::high_resolution_clock::now();
	frameTime = std::chrono::duration<float>(currTime - lastTime).count();
	lastTime = currTime;
	glEndQuery(GL_TIME_ELAPSED);
}

Renderer::Renderer(ResourceManager * resMan, Queue<RenderCommand>::Reader&& reader, Queue<ViewDef *>::Writer&& viewDefQueue,
				   const char * title, const int winX, const int winY, const int w, const int h, const uint32_t flags) noexcept
	: renderQueue(std::move(reader)), viewDefQueue(std::move(viewDefQueue))
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
	}
	printf("Program start glGetError(Expected 1280): %d\n", glGetError());

	stbi_set_flip_vertically_on_load(true);

	glGenQueries(1, &timeQuery);

	glGenTextures(1, &backbuffer.nativeHandle);
	glBindTexture(GL_TEXTURE_2D, backbuffer.nativeHandle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	//TODO: depth and stencil buffers as images
	ImageCreateInfo backBufferTexCreateInfo = {
		"__Scratch/Backbuffer.tex",
		h, w,
		resMan,
		&backbuffer
	};

	backBufferImage = std::make_shared<Image>(backBufferTexCreateInfo);

	resMan->AddResourceRefCounted<Image>("__Scratch/Backbuffer.tex", backBufferImage);

	FramebufferCreateInfo backbufferFBCreateInfo = {
		"__Scratch/Backbuffer.rendertarget",
		{{Framebuffer::Attachment::COLOR0, backBufferImage}},
		resMan,
		FramebufferRendererData(0)
	};

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

	ptvShader = resourceManager->LoadResource<Shader>("shaders/passthrough-transform.vert");
	pfShader = resourceManager->LoadResource<Shader>("shaders/passthrough.frag");
	ptProgram = resourceManager->LoadResourceOrConstruct<Program>("__Scratch/Programs/passthrough.prog", ptvShader, pfShader);


	glGenVertexArrays(1, &ptVAO);
	glBindVertexArray(ptVAO);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), nullptr);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void *>(6 * sizeof(GLfloat)));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	DrainQueue();

	//TODO:
	ptVertexModule.nativeHandle = ptvShader.get()->rendererData.shader;
	ptFragmentModule.nativeHandle = pfShader.get()->rendererData.shader;

	lastTime = std::chrono::high_resolution_clock::now();
}

uint64_t Renderer::GetFrameTime() noexcept
{
	uint64_t ret = 0;
	glGetQueryObjectui64v(timeQuery, GL_QUERY_RESULT, &ret);
	return ret;
}

void Renderer::AddBuffer(RenderCommand::AddBufferParams params) noexcept
{
	//Only copy the handle to rData when we've finished uploading
	GLuint ret;
	glGenBuffers(1, &ret);
	//TODO: STATIC_DRAW should be parametrized, but glTF doesn't allow it it seems - use extension
	glNamedBufferData(ret, params.size, nullptr, static_cast<GLenum>(params.type));

	params.rData->buffer = ret;
}

void Renderer::DeleteBuffer(BufferRendererData * rData) noexcept
{
	//TODO: Potential race condition
	glDeleteBuffers(1, &rData->buffer);
	rData->buffer = 0;
}

void Renderer::AddFramebuffer(Framebuffer * const fb) noexcept
{
	glGenFramebuffers(1, &fb->rendererData.framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, fb->rendererData.framebuffer);
	for (auto& kv : fb->GetImages()) {
		GLenum attachment = AttachmentGL(kv.first);
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, ((OpenGLImageHandle *)kv.second->GetImageHandle())->nativeHandle, 0);
	}
}

void Renderer::DeleteFramebuffer(Framebuffer * const fb) noexcept
{
	glDeleteFramebuffers(1, &fb->GetRendererData().framebuffer);
}

void Renderer::AddProgram(Program * const prog) noexcept
{
	prog->rendererData.program = glCreateProgram();
	if (prog->rendererData.program == 0) {
		printf("[ERROR] AddProgram: glCreateProgram returned 0.\n");
	}
	for (auto s : prog->shaders) {
		if (s->rendererData.shader == 0) {
			AddShader(s.get());
		}
		glAttachShader(prog->rendererData.program, s->rendererData.shader);
	}
	glLinkProgram(prog->rendererData.program);
	GLint linkOk;
	glGetProgramiv(prog->rendererData.program, GL_LINK_STATUS, &linkOk);
	if (linkOk != GL_TRUE) {
		GLsizei log_length = 0;
		GLchar message[1024];
		glGetProgramInfoLog(prog->rendererData.program, 1024, &log_length, message);
		glDeleteProgram(prog->rendererData.program);
		prog->rendererData.program = 0;
		printf("Program link failed\n %s\n", message);
	}
}

void Renderer::DeleteProgram(Program * const prog) noexcept
{
	glDeleteProgram(prog->rendererData.program);
}

void Renderer::AddShader(Shader * const s) noexcept
{
	GLuint type = ShaderTypeGL(s->type);
	s->rendererData.shader = glCreateShader(type);
	const char * src = s->GetSource().c_str();
	printf("pre shadersource err %d\n", glGetError());
	glShaderSource(s->rendererData.shader, 1, &src, nullptr);
	printf("post shadersource err %d\n", glGetError());
	glCompileShader(s->rendererData.shader);
	GLint compileOk;
	glGetShaderiv(s->rendererData.shader, GL_COMPILE_STATUS, &compileOk);
	if (compileOk != GL_TRUE) {
		GLsizei log_length = 0;
		GLchar message[1024];
		glGetShaderInfoLog(s->rendererData.shader, 1024, &log_length, message);
		printf("Shader compile failed\n %s %s\n", s->name.c_str(), message);
		glDeleteShader(s->rendererData.shader);
		s->rendererData.shader = 0;
	}
}

void Renderer::DeleteShader(Shader * const s) noexcept
{
	if (s->rendererData.shader != 0) {
		glDeleteShader(s->rendererData.shader);
	}
}

void Renderer::DrainQueue() noexcept
{
	//TODO: This is currently used to delay pushing viewdefs back to CPU thread so we wont get stuck in an infinite loop here
	//If the CPU thread sends viewdefs faster than we can render them the renderqueue will become infinite.
	//Alternatively, the GPU could return after rendering a set number of frames, but would cause frame time spikes and uneven input polling.
	std::vector<ViewDef *> viewDefsToPush;
	Maybe<RenderCommand> renderCommand;
	do {
		renderCommand = std::move(renderQueue.Pop());
		if (renderCommand.index() != 0) {
			RenderCommand command = std::move(std::get<RenderCommand>(renderCommand));
			switch (command.params.index()) {
			case RenderCommand::Type::NOP:
				printf("[WARNING] NOP render command.\n");
				break;
			case RenderCommand::Type::ABORT:
				isAborting = true;
				abortCode = std::get<RenderCommand::AbortParams>(command.params).errorCode;
				break;
			case RenderCommand::Type::CREATE_RESOURCE:
			{
				auto fun = std::get<RenderCommand::CreateResourceParams>(command.params).fun;
				OpenGLResourceContext ctx;
				fun(ctx);
				break;
			}
			case RenderCommand::Type::EXECUTE_COMMAND_CONTEXT:
			{
				auto ctx = std::move(std::get<RenderCommand::ExecuteCommandContextParams>(command.params).ctx);
				ctx->Execute();
				break;
			}
			case RenderCommand::Type::ADD_BUFFER:
			{
				AddBuffer(std::get<RenderCommand::AddBufferParams>(command.params));
				break;
			}
			case RenderCommand::Type::DELETE_BUFFER:
				DeleteBuffer(std::get<RenderCommand::DeleteBufferParams>(command.params).rData);
				break;
			case RenderCommand::Type::ADD_FRAMEBUFFER:
				AddFramebuffer(std::get<RenderCommand::AddFramebufferParams>(command.params).fb);
				break;
			case RenderCommand::Type::DELETE_FRAMEBUFFER:
				DeleteFramebuffer(std::get<RenderCommand::DeleteFramebufferParams>(command.params).fb);
				break;
			case RenderCommand::Type::ADD_PROGRAM:
				AddProgram(std::get<RenderCommand::AddProgramParams>(command.params).prog);
				break;
			case RenderCommand::Type::DELETE_PROGRAM:
				DeleteProgram(std::get<RenderCommand::DeleteProgramParams>(command.params).prog);
				break;
			case RenderCommand::Type::ADD_SHADER:
				AddShader(std::get<RenderCommand::AddShaderParams>(command.params).shader);
				break;
			case RenderCommand::Type::DELETE_SHADER:
				DeleteShader(std::get<RenderCommand::DeleteShaderParams>(command.params).shader);
				break;
			case RenderCommand::Type::END_FRAME:
			{
				RenderCommand::EndFrameParams params = std::get<RenderCommand::EndFrameParams>(command.params);
				//EndFrame(params.cameras, params.sprites);
				break;
			}
			case RenderCommand::Type::DRAW_VIEW:
			{
				RenderCommand::DrawViewParams params = std::get<RenderCommand::DrawViewParams>(command.params);
				EndFrame(params.view->commandBuffers);
				viewDefsToPush.push_back(params.view);
				break;
			}
			default:
				printf("[WARNING] Unimplemented render command: %zu\n", command.params.index());
			}
			GLenum err = glGetError();
			if (err) {
				printf("RenderQueue pop error %u. RenderCommand %zu.\n", err, command.params.index());
			}
		}
	} while (renderCommand.index() != 0);

	for (auto vd : viewDefsToPush) {
		viewDefQueue.Push(std::move(vd));
	}
}
