#pragma once

#include <utility>

#if _MSC_VER && !__INTEL_COMPILER
#include <variant>
#else
#include <experimental/variant.hpp>
using std::variant = std::experimental::variant;
#endif

#include "Core/Maybe.h"
#include "Core/Rendering/Buffer.h"
#include "Core/Rendering/Context/RenderContext.h"
#include "Core/Rendering/RendererData.h"
#include "Core/Rendering/SubmittedCamera.h"
#include "Core/Rendering/SubmittedSprite.h"

struct CameraComponent;
struct Framebuffer;
struct Image;
struct Program;
struct Shader;
struct Sprite;

struct RenderCommand
{
	enum Type
	{
		NOP,
		ABORT,
		CREATE_RESOURCE,
		EXECUTE_COMMAND_CONTEXT,
		ADD_BUFFER,
		DELETE_BUFFER,
		ADD_FRAMEBUFFER,
		DELETE_FRAMEBUFFER,
		ADD_PROGRAM,
		DELETE_PROGRAM,
		ADD_SHADER,
		DELETE_SHADER,
	};

	//TODO: Could template most of this stuff pretty easily

	struct AbortParams
	{
		int errorCode;
		AbortParams(int errorCode) : errorCode(errorCode) {}
	};

	struct CreateResourceParams
	{
		std::function<void(ResourceCreationContext&)> fun;
		CreateResourceParams(std::function<void(ResourceCreationContext&)> fun) : fun(fun) {}
	};

	struct ExecuteCommandContextParams
	{
		std::unique_ptr<RenderCommandContext> ctx;
		ExecuteCommandContextParams(std::unique_ptr<RenderCommandContext>&& ctx) : ctx(std::move(ctx)) {}
	};

	struct AddBufferParams
	{
		BufferRendererData * rData;
		size_t size;
		Buffer::Type type;

		AddBufferParams(BufferRendererData * rData, size_t size, Buffer::Type type) : rData(rData), size(size), type(type) {}
	};

	struct DeleteBufferParams
	{
		BufferRendererData * rData;
	};

	struct AddFramebufferParams
	{
		Framebuffer * fb;
		AddFramebufferParams(Framebuffer * fb) : fb(fb) {}
	};
	
	struct DeleteFramebufferParams
	{
		Framebuffer * fb;
		DeleteFramebufferParams(Framebuffer * fb) : fb(fb) {}
	};

	struct AddProgramParams
	{
		Program * prog;
		AddProgramParams(Program * prog) : prog(prog) {}
	};
	
	struct DeleteProgramParams
	{
		Program * prog;
		DeleteProgramParams(Program * prog) : prog(prog) {}
	};

	struct AddShaderParams
	{
		Shader * shader;
		AddShaderParams(Shader * shader) : shader(shader) {}
	};

	struct DeleteShaderParams
	{
		Shader * shader;
		DeleteShaderParams(Shader * shader) : shader(shader) {}
	};

	using RenderCommandParams = std::variant<None, AbortParams, CreateResourceParams, ExecuteCommandContextParams,
														   AddBufferParams, DeleteBufferParams, AddFramebufferParams, DeleteFramebufferParams,
														   AddProgramParams, DeleteProgramParams, AddShaderParams,
														   DeleteShaderParams>;

	RenderCommandParams params;

	RenderCommand() : params(None{}) {}
	//Unfortunately compiling MoodyCamel's queue requires this to be implemented even if it will never be used
	RenderCommand(const RenderCommand&)
	{
		assert(false);
	}
	RenderCommand(RenderCommand&& rc) : params(std::move(rc.params)) {}
	RenderCommand(RenderCommandParams&& params) : params(std::move(params)) {}

	void operator=(const RenderCommand& rc)
	{
		assert(false);
		//params = rc.params;
	}
	void operator=(RenderCommand&& rc)
	{
		params = std::move(rc.params);
	}
};
