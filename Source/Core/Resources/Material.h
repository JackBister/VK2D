#pragma once

class DescriptorSet;
class Image;

class Material
{
public:
    Material(Image * albedo, Image * normals, Image * roughness, Image * metallic);

    inline Image * GetAlbedo() { return albedo; }

    inline DescriptorSet * GetDescriptorSet() { return descriptorSet; }

private:
    Image * albedo;
    Image * normals;
    Image * roughness;
    Image * metallic;

    DescriptorSet * descriptorSet;
};
