#pragma once
#include "Nalta/Util/SlotKey.h"

namespace Nalta::Graphics
{
    struct TextureKey : SlotKey { using SlotKey::SlotKey; };
    struct BufferKey : SlotKey { using SlotKey::SlotKey; };
    struct RenderSurfaceKey : SlotKey { using SlotKey::SlotKey; };
}
