#pragma once

class ShaderProgramFactory
{
public:
    static void CreateResources();

private:
    static void CreatePassthroughTransformShaderProgram();
    static void CreateUiShaderProgram();
    static void CreatePostprocessShaderProgram();
};
