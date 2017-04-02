#include "render_opengl_textureatlas.h"

TextureAtlas::TextureAtlas(GLsizei width, GLsizei height, GLsizei depth) : width(width), height(height), depth(depth)
{
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D_ARRAY, id);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, width, height, depth);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glGetError();
}

GLuint TextureAtlas::GetID()
{
	return id;
}

TexAtlasSubImg TextureAtlas::PutSprite(Sprite * s)
{
	TexAtlasSubImg ret;
	ret.atlasID = id;
	ret.zOffset = 0;
	glm::vec2 requiredUVSpace = CalculateUVSize(s->dimensions);
	float maxY = 0.f;
	bool placed = false;
	for (size_t i = 0; i < subImgs.size(); ++i) {
		if (subImgs[i].maxUV.y - subImgs[i].minUV.y > maxY) {
			maxY = subImgs[i].maxUV.y - subImgs[i].minUV.y;
		}
		ret.zOffset = subImgs[i].zOffset;
		bool last = false;
		glm::vec2 availableUVSpace;
		if (i == subImgs.size() - 1 || subImgs[i+1].zOffset > ret.zOffset) {
			availableUVSpace = glm::vec2(1.f - subImgs[i].maxUV.x, 1.f - subImgs[i].maxUV.y);
			last = true;
		} else {
			availableUVSpace = glm::vec2(subImgs[i + 1].minUV.x - subImgs[i].maxUV.x, subImgs[i + 1].minUV.y - subImgs[i].maxUV.y);
		}
		if (availableUVSpace.x >= requiredUVSpace.x || availableUVSpace.y >= requiredUVSpace.y) {
			ret.minUV = glm::vec2(subImgs[i].maxUV.x, subImgs[i].maxUV.y);
			ret.maxUV = glm::vec2(ret.minUV.x + requiredUVSpace.x, ret.minUV.y + requiredUVSpace.y);
			placed = true;
		} else if (last) {
			float newY = subImgs[i].maxUV.y + maxY;
			if (newY >= 1.f) {
				continue;
			}
			availableUVSpace = glm::vec2(1.f, 1.f - newY);
			if (availableUVSpace.x >= requiredUVSpace.x || availableUVSpace.y >= requiredUVSpace.y) {
				ret.minUV = glm::vec2(subImgs[i].maxUV.x, subImgs[i].maxUV.y);
				ret.maxUV = glm::vec2(ret.minUV.x + requiredUVSpace.x, ret.minUV.y + requiredUVSpace.y);
				subImgs.insert(subImgs.begin() + i + 1, ret);
				placed = true;
			}
		}
	}
	if (!placed) {
		if (subImgs.size() == 0) {
			ret.zOffset = 0;
			ret.minUV = glm::vec2(0.f, 0.f);
			ret.maxUV = requiredUVSpace;
			subImgs.push_back(ret);
		} else {
			auto& lastSubImg = subImgs[subImgs.size() - 1];
			if (lastSubImg.zOffset < depth) {
				ret.zOffset = lastSubImg.zOffset + 1;
				ret.minUV = glm::vec2(0.f, 0.f);
				ret.maxUV = requiredUVSpace;
				subImgs.push_back(ret);
			}
		}

	}

	glBindTexture(GL_TEXTURE_2D_ARRAY, id);

	//TODO: Choose format based on sprite
	printf("%d %d %d %d %d %d\n", glGetError(), (GLint)(ret.minUV.x * width), GLint(ret.minUV.y * height), ret.zOffset, GLsizei(requiredUVSpace.x * width), GLsizei(requiredUVSpace.y * height));
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, (GLint)(ret.minUV.x * width), GLint(ret.minUV.y * height), ret.zOffset, GLsizei(requiredUVSpace.x * width), GLsizei(requiredUVSpace.y * height), depth, GL_RGB, GL_UNSIGNED_BYTE, s->data);

	return ret;
}

void TextureAtlas::RemoveSprite(Sprite * s)
{
	for (auto it = subImgs.begin(); it != subImgs.end(); ++it) {
		if (it->sprite == s) {
			subImgs.erase(it);
		}
	}
}

glm::vec2 TextureAtlas::CalculateUVSize(const glm::ivec2& dimensions)
{
	return glm::vec2((float)dimensions.x/width, (float)dimensions.y/height);
}
