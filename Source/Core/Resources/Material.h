#pragma once

class DescriptorSet;
class Image;

class Material
{
public:
    Material(Image * albedo);

    inline DescriptorSet * GetDescriptorSet() { return descriptorSet; }

private:
    Image * albedo;

    DescriptorSet * descriptorSet;
};
