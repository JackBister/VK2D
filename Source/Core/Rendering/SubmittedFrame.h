#pragma once

#include "SubmittedCamera.h"
#include "SubmittedMesh.h"
#include "SubmittedSprite.h"

struct SubmittedFrame {
    std::vector<SubmittedCamera> cameras;
    std::vector<SubmittedMesh> meshes;
    std::vector<SubmittedSprite> sprites;
};
