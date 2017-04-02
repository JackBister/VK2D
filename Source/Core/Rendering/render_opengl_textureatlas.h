#pragma once

#include "GL/glew.h"
#include "glm/glm.hpp"

#include "sprite.h"

struct TexAtlasSubImg
{
	GLuint atlasID;
	GLint zOffset;
	glm::vec2 minUV, maxUV;
	Sprite * sprite;
};

struct TextureAtlas
{
	TextureAtlas(GLsizei width = 2048, GLsizei height = 2048, GLsizei depth = 1);
	GLuint GetID();
	TexAtlasSubImg PutSprite(Sprite *);
	void RemoveSprite(Sprite *);

private:
	GLsizei width, height, depth;
	GLuint id;
	std::vector<TexAtlasSubImg> subImgs;

	glm::vec2 CalculateUVSize(const glm::ivec2& dimensions);
};