#pragma once

class ShaderProgramFactory
{
public:
    static void CreateResources();

private:
    static void CreatePassthroughTransformShaderProgram();
    static void CreateMeshShaderProgram();
    static void CreateTransparentMeshShaderProgram();
    static void CreateUiShaderProgram();
    static void CreatePostprocessShaderProgram();
};
