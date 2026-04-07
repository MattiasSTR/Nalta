#pragma once
#include "Nalta/Util/SlotKey.h"

namespace Nalta::Graphics
{
    struct TextureKey : SlotKey { using SlotKey::SlotKey; };
    struct BufferKey : SlotKey { using SlotKey::SlotKey; };
    struct RenderSurfaceKey : SlotKey { using SlotKey::SlotKey; };
    struct ShaderKey : SlotKey { using SlotKey::SlotKey; };
    struct PipelineKey : SlotKey { using SlotKey::SlotKey; };
}
