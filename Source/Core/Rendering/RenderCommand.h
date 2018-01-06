#pragma once

#include <utility>

#if _MSC_VER && !__INTEL_COMPILER
#include <variant>
#else
#include <experimental/variant.hpp>
using std::variant = std::experimental::variant;
#endif

#include "Core/Maybe.h"
#include "Core/Rendering/Context/RenderContext.h"
#include "Core/Rendering/SubmittedCamera.h"

class CameraComponent;
class Framebuffer;
class Image;
class Program;
class Shader;
class Sprite;

struct RenderCommand
{
	enum Type
	{
		NOP,
		ABORT,
		CREATE_RESOURCE,
		EXECUTE_COMMAND_CONTEXT,
		SWAP_WINDOW
	};

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
		std::vector<SemaphoreHandle *> waitSem;
		std::vector<SemaphoreHandle *> signalSem;
		RenderCommandContext * ctx;
		ExecuteCommandContextParams(RenderCommandContext * ctx, std::vector<SemaphoreHandle *> waitSem, std::vector<SemaphoreHandle *> signalSem)
			: ctx(ctx), waitSem(waitSem), signalSem(signalSem) {}
	};

	struct SwapWindowParams
	{
		SemaphoreHandle * waitSem;
		FenceHandle * signalFence;
		SwapWindowParams(SemaphoreHandle * waitSem, FenceHandle * signalFence) : waitSem(waitSem), signalFence(signalFence) {
		}
	};

	using RenderCommandParams = std::variant<None, AbortParams, CreateResourceParams, ExecuteCommandContextParams, SwapWindowParams>;
	RenderCommandParams params;

	RenderCommand() : params(None{}) {}
	//Unfortunately compiling MoodyCamel's queue requires this to be implemented even if it will never be used
	RenderCommand(RenderCommand const&)
	{
		assert(false);
	}
	RenderCommand(RenderCommand&& rc) : params(std::move(rc.params)) {}
	RenderCommand(RenderCommandParams&& params) : params(std::move(params)) {}

	void operator=(RenderCommand const& rc)
	{
		assert(false);
	}
	void operator=(RenderCommand&& rc)
	{
		params = std::move(rc.params);
	}
};
