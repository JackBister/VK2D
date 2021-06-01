#pragma once
#ifdef USE_OGL_RENDERER
#include <cassert>
#include <type_traits>

#include <GL/glew.h>

#include "../Abstract/RenderResources.h"

template <typename T>
typename std::underlying_type<T>::type ToUnderlyingType(T in)
{
    return static_cast<typename std::underlying_type<T>::type>(in);
}

GLint ToGLAddressMode(AddressMode addressMode);
GLenum ToGLCompareOp(CompareOp compareOp);
GLenum ToGLCullMode(CullMode cullMode);
GLint ToGLFilter(Filter filter);
GLint ToGLFormat(Format format);
GLenum ToGLFrontFace(FrontFace frontFace);
GLint ToGLInternalFormat(Format format);
GLenum ToGLPrimitiveTopology(PrimitiveTopology topology);

#endif
