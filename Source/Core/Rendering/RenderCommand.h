#pragma once

#include <functional>
#include <utility>
#include <variant>

#include "Core/Rendering/Context/RenderContext.h"
#include "Core/Rendering/SubmittedCamera.h"

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
		FenceHandle * signalFence;
		CommandBuffer * ctx;
		ExecuteCommandContextParams(CommandBuffer * ctx, std::vector<SemaphoreHandle *> waitSem, std::vector<SemaphoreHandle *> signalSem, FenceHandle * signalFence)
			: ctx(ctx), waitSem(waitSem), signalSem(signalSem), signalFence(signalFence) {}
	};

	struct NopParams
	{
	};

	struct SwapWindowParams
	{
		SemaphoreHandle * waitSem;
		uint32_t imageIndex;
		SwapWindowParams(uint32_t imageIndex, SemaphoreHandle * waitSem) : waitSem(waitSem), imageIndex(imageIndex) {
		}
	};

	using RenderCommandParams = std::variant<NopParams, AbortParams, CreateResourceParams, ExecuteCommandContextParams, SwapWindowParams>;
	RenderCommandParams params;

	RenderCommand() : params(NopParams())
	{
	}

	//Unfortunately compiling MoodyCamel's queue requires this to be implemented even if it will never be used
	RenderCommand(RenderCommand const& rc)
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
