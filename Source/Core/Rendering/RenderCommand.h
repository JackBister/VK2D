#pragma once

#include <experimental/variant.hpp>

#include "Core/Maybe.h"
#include "Core/Rendering/SubmittedCamera.h"
#include "Core/Rendering/SubmittedSprite.h"

struct CameraComponent;
struct Framebuffer;
struct Image;
struct Program;
struct Shader;
struct Sprite;
struct ViewDef;

struct RenderCommand
{
	enum Type
	{
		NOP,
		ABORT,
		ADD_FRAMEBUFFER,
		DELETE_FRAMEBUFFER,
		ADD_IMAGE,
		DELETE_IMAGE,
		ADD_PROGRAM,
		DELETE_PROGRAM,
		ADD_SHADER,
		DELETE_SHADER,
		END_FRAME,
		DRAW_VIEW
	};

	//TODO: Could template most of this stuff pretty easily

	struct AbortParams
	{
		int errorCode;
		AbortParams(int errorCode) : errorCode(errorCode) {}
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

	struct AddImageParams
	{
		Image * img;
		AddImageParams(Image * img) : img(img) {}
	};
	
	struct DeleteImageParams
	{
		Image * img;
		DeleteImageParams(Image * img) : img(img) {}
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

	struct EndFrameParams
	{
		std::vector<SubmittedCamera> cameras;
		//TODO: This won't be necessary in the future with custom mesh stuff.
		std::vector<SubmittedSprite> sprites;
		EndFrameParams(std::vector<SubmittedCamera>& cameras, std::vector<SubmittedSprite>& sprites) : cameras(cameras), sprites(sprites) {}
	};

	struct DrawViewParams
	{
		ViewDef * view;
		DrawViewParams(ViewDef * view) : view(view) {}
	};

	using RenderCommandParams = std::experimental::variant<None, AbortParams, AddFramebufferParams, DeleteFramebufferParams,
														   AddImageParams, DeleteImageParams, AddProgramParams, DeleteProgramParams, AddShaderParams,
														   DeleteShaderParams, EndFrameParams, DrawViewParams>;

	RenderCommandParams params;

	RenderCommand() : params(None{}) {}
	RenderCommand(const RenderCommandParams& params) : params(params) {}
};
