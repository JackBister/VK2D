#pragma once

class ShaderProgramFactory
{
public:
    static void CreateResources();

private:
    static void CreateDebugDrawShaderProgram();
    static void CreatePassthroughTransformShaderProgram();
    static void CreateMeshShaderProgram();
    static void CreateSkeletalMeshShaderProgram();
    static void CreateTransparentMeshShaderProgram();
    static void CreateAmbientOcclusionProgram();
    static void CreateAmbientOcclusionBlurProgram();
    static void CreateParticleRenderingProgram();
    static void CreateTonemapProgram();
    static void CreateUiShaderProgram();
    static void CreatePostprocessShaderProgram();
};
