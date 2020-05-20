#pragma once

#include "SubmittedCamera.h"
#include "SubmittedSprite.h"

struct SubmittedFrame {
    std::vector<SubmittedCamera> cameras;
    std::vector<SubmittedSprite> sprites;
};
