#include "imgui_impl_gdi_software.h"

#include "../third_party/imgui/imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

namespace {

struct SoftwareTexture {
    int width{};
    int height{};
    std::vector<std::uint32_t> rgba{};
};

struct SoftwareRendererState {
    HWND window{};
    int width{};
    int height{};
    std::vector<std::uint32_t> pixels{};
    BITMAPINFO bitmapInfo{};
};

SoftwareRendererState* g_renderer{};

void ResizeBackBuffer(int width, int height) {
    if (g_renderer == nullptr || width <= 0 || height <= 0) {
        return;
    }
    if (g_renderer->width == width && g_renderer->height == height) {
        return;
    }

    g_renderer->width = width;
    g_renderer->height = height;
    g_renderer->pixels.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0u);
    g_renderer->bitmapInfo = {};
    g_renderer->bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    g_renderer->bitmapInfo.bmiHeader.biWidth = width;
    g_renderer->bitmapInfo.bmiHeader.biHeight = -height;
    g_renderer->bitmapInfo.bmiHeader.biPlanes = 1;
    g_renderer->bitmapInfo.bmiHeader.biBitCount = 32;
    g_renderer->bitmapInfo.bmiHeader.biCompression = BI_RGB;
}

void DestroyTexture(ImTextureData* texture) {
    if (texture == nullptr) {
        return;
    }
    delete static_cast<SoftwareTexture*>(texture->BackendUserData);
    texture->BackendUserData = nullptr;
    texture->SetTexID(ImTextureID_Invalid);
    texture->SetStatus(ImTextureStatus_Destroyed);
}

void UpdateTexture(ImTextureData* texture) {
    if (texture == nullptr) {
        return;
    }

    if (texture->Status == ImTextureStatus_WantCreate) {
        if (texture->Format != ImTextureFormat_RGBA32 || texture->Width <= 0 || texture->Height <= 0) {
            texture->SetStatus(ImTextureStatus_Destroyed);
            return;
        }

        auto softwareTexture = std::make_unique<SoftwareTexture>();
        softwareTexture->width = texture->Width;
        softwareTexture->height = texture->Height;
        softwareTexture->rgba.resize(
            static_cast<std::size_t>(texture->Width) * static_cast<std::size_t>(texture->Height)
        );
        for (int y = 0; y < texture->Height; ++y) {
            std::memcpy(
                softwareTexture->rgba.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(texture->Width),
                texture->GetPixelsAt(0, y),
                static_cast<std::size_t>(texture->Width) * sizeof(std::uint32_t)
            );
        }

        SoftwareTexture* rawTexture = softwareTexture.release();
        texture->BackendUserData = rawTexture;
        texture->SetTexID(static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(rawTexture)));
        texture->SetStatus(ImTextureStatus_OK);
        return;
    }

    if (texture->Status == ImTextureStatus_WantUpdates) {
        auto* softwareTexture = static_cast<SoftwareTexture*>(texture->BackendUserData);
        if (softwareTexture == nullptr
            || softwareTexture->width != texture->Width
            || softwareTexture->height != texture->Height) {
            DestroyTexture(texture);
            return;
        }

        for (const ImTextureRect& update : texture->Updates) {
            for (int y = 0; y < update.h; ++y) {
                std::memcpy(
                    softwareTexture->rgba.data()
                        + static_cast<std::size_t>(update.y + y) * static_cast<std::size_t>(softwareTexture->width)
                        + static_cast<std::size_t>(update.x),
                    texture->GetPixelsAt(update.x, update.y + y),
                    static_cast<std::size_t>(update.w) * sizeof(std::uint32_t)
                );
            }
        }
        texture->SetStatus(ImTextureStatus_OK);
        return;
    }

    if (texture->Status == ImTextureStatus_WantDestroy && texture->UnusedFrames > 0) {
        DestroyTexture(texture);
    }
}

float Edge(float ax, float ay, float bx, float by, float px, float py) {
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

std::uint32_t SampleTexture(const SoftwareTexture* texture, float u, float v) {
    if (texture == nullptr || texture->rgba.empty() || texture->width <= 0 || texture->height <= 0) {
        return 0xFFFFFFFFu;
    }

    // ImGui UVs address texel centers, as on the standard GPU backends.
    const float x = std::clamp(u * texture->width - 0.5f, 0.0f, static_cast<float>(texture->width - 1));
    const float y = std::clamp(v * texture->height - 0.5f, 0.0f, static_cast<float>(texture->height - 1));
    const int x0 = std::clamp(static_cast<int>(std::floor(x)), 0, texture->width - 1);
    const int y0 = std::clamp(static_cast<int>(std::floor(y)), 0, texture->height - 1);
    const int x1 = std::min(x0 + 1, texture->width - 1);
    const int y1 = std::min(y0 + 1, texture->height - 1);
    const float fx = x - static_cast<float>(x0);
    const float fy = y - static_cast<float>(y0);

    const std::uint32_t c00 = texture->rgba[static_cast<std::size_t>(y0) * texture->width + x0];
    const std::uint32_t c10 = texture->rgba[static_cast<std::size_t>(y0) * texture->width + x1];
    const std::uint32_t c01 = texture->rgba[static_cast<std::size_t>(y1) * texture->width + x0];
    const std::uint32_t c11 = texture->rgba[static_cast<std::size_t>(y1) * texture->width + x1];

    std::uint32_t result = 0u;
    for (int shift = 0; shift <= 24; shift += 8) {
        const float top = static_cast<float>((c00 >> shift) & 0xFFu)
            + (static_cast<float>((c10 >> shift) & 0xFFu) - static_cast<float>((c00 >> shift) & 0xFFu)) * fx;
        const float bottom = static_cast<float>((c01 >> shift) & 0xFFu)
            + (static_cast<float>((c11 >> shift) & 0xFFu) - static_cast<float>((c01 >> shift) & 0xFFu)) * fx;
        const auto channel = static_cast<std::uint32_t>(std::clamp(std::lround(top + (bottom - top) * fy), 0l, 255l));
        result |= channel << shift;
    }
    return result;
}

void BlendPixel(std::uint32_t& destination, std::uint32_t vertexColor, std::uint32_t textureColor) {
    const unsigned int vertexR = vertexColor & 0xFFu;
    const unsigned int vertexG = (vertexColor >> 8) & 0xFFu;
    const unsigned int vertexB = (vertexColor >> 16) & 0xFFu;
    const unsigned int vertexA = (vertexColor >> 24) & 0xFFu;
    const unsigned int textureR = textureColor & 0xFFu;
    const unsigned int textureG = (textureColor >> 8) & 0xFFu;
    const unsigned int textureB = (textureColor >> 16) & 0xFFu;
    const unsigned int textureA = (textureColor >> 24) & 0xFFu;

    const unsigned int sourceA = (vertexA * textureA + 127u) / 255u;
    if (sourceA == 0u) {
        return;
    }
    const unsigned int sourceR = (vertexR * textureR + 127u) / 255u;
    const unsigned int sourceG = (vertexG * textureG + 127u) / 255u;
    const unsigned int sourceB = (vertexB * textureB + 127u) / 255u;

    const unsigned int destinationR = (destination >> 16) & 0xFFu;
    const unsigned int destinationG = (destination >> 8) & 0xFFu;
    const unsigned int destinationB = destination & 0xFFu;
    const unsigned int inverseA = 255u - sourceA;
    const unsigned int outputR = (sourceR * sourceA + destinationR * inverseA + 127u) / 255u;
    const unsigned int outputG = (sourceG * sourceA + destinationG * inverseA + 127u) / 255u;
    const unsigned int outputB = (sourceB * sourceA + destinationB * inverseA + 127u) / 255u;
    destination = (outputR << 16) | (outputG << 8) | outputB;
}

void RasterizeTriangle(
    const ImDrawVert& source0,
    const ImDrawVert& source1,
    const ImDrawVert& source2,
    const ImVec2& displayPosition,
    const ImVec2& framebufferScale,
    const ImVec4& clipRect,
    const SoftwareTexture* texture
) {
    if (g_renderer == nullptr || g_renderer->pixels.empty()) {
        return;
    }

    struct Vertex {
        float x{};
        float y{};
        float u{};
        float v{};
        float r{};
        float g{};
        float b{};
        float a{};
    };
    const auto convert = [&](const ImDrawVert& source) {
        Vertex vertex{};
        vertex.x = (source.pos.x - displayPosition.x) * framebufferScale.x;
        vertex.y = (source.pos.y - displayPosition.y) * framebufferScale.y;
        vertex.u = source.uv.x;
        vertex.v = source.uv.y;
        vertex.r = static_cast<float>(source.col & 0xFFu);
        vertex.g = static_cast<float>((source.col >> 8) & 0xFFu);
        vertex.b = static_cast<float>((source.col >> 16) & 0xFFu);
        vertex.a = static_cast<float>((source.col >> 24) & 0xFFu);
        return vertex;
    };

    const Vertex v0 = convert(source0);
    Vertex v1 = convert(source1);
    Vertex v2 = convert(source2);
    float area = Edge(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y);
    if (!std::isfinite(area) || std::fabs(area) < 0.0001f) {
        return;
    }
    if (area < 0.0f) {
        std::swap(v1, v2);
        area = -area;
    }
    const auto isTopLeft = [](const Vertex& a, const Vertex& b) {
        return b.y > a.y || (b.y == a.y && b.x < a.x);
    };
    const bool include0 = isTopLeft(v1, v2);
    const bool include1 = isTopLeft(v2, v0);
    const bool include2 = isTopLeft(v0, v1);

    const float clipMinX = (clipRect.x - displayPosition.x) * framebufferScale.x;
    const float clipMinY = (clipRect.y - displayPosition.y) * framebufferScale.y;
    const float clipMaxX = (clipRect.z - displayPosition.x) * framebufferScale.x;
    const float clipMaxY = (clipRect.w - displayPosition.y) * framebufferScale.y;
    const int minX = std::max(0, static_cast<int>(std::floor(std::max(std::min({ v0.x, v1.x, v2.x }), clipMinX))));
    const int minY = std::max(0, static_cast<int>(std::floor(std::max(std::min({ v0.y, v1.y, v2.y }), clipMinY))));
    const int maxX = std::min(g_renderer->width - 1, static_cast<int>(std::ceil(std::min(std::max({ v0.x, v1.x, v2.x }), clipMaxX))) - 1);
    const int maxY = std::min(g_renderer->height - 1, static_cast<int>(std::ceil(std::min(std::max({ v0.y, v1.y, v2.y }), clipMaxY))) - 1);
    if (minX > maxX || minY > maxY) {
        return;
    }

    const float inverseArea = 1.0f / area;
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            const float sampleX = static_cast<float>(x) + 0.5f;
            const float sampleY = static_cast<float>(y) + 0.5f;
            const float e0 = Edge(v1.x, v1.y, v2.x, v2.y, sampleX, sampleY);
            const float e1 = Edge(v2.x, v2.y, v0.x, v0.y, sampleX, sampleY);
            const float e2 = Edge(v0.x, v0.y, v1.x, v1.y, sampleX, sampleY);
            // A shared edge belongs to exactly one triangle. Blending both
            // triangles along the diagonal left visible seams in every button.
            if (e0 < 0.0f || (e0 == 0.0f && !include0)
                || e1 < 0.0f || (e1 == 0.0f && !include1)
                || e2 < 0.0f || (e2 == 0.0f && !include2)) {
                continue;
            }
            const float w0 = e0 * inverseArea;
            const float w1 = e1 * inverseArea;
            const float w2 = e2 * inverseArea;

            const float u = v0.u * w0 + v1.u * w1 + v2.u * w2;
            const float v = v0.v * w0 + v1.v * w1 + v2.v * w2;
            const auto channel = [](float value) {
                return static_cast<std::uint32_t>(std::clamp(std::lround(value), 0l, 255l));
            };
            const std::uint32_t vertexColor = channel(v0.r * w0 + v1.r * w1 + v2.r * w2)
                | (channel(v0.g * w0 + v1.g * w1 + v2.g * w2) << 8)
                | (channel(v0.b * w0 + v1.b * w1 + v2.b * w2) << 16)
                | (channel(v0.a * w0 + v1.a * w1 + v2.a * w2) << 24);
            const std::uint32_t textureColor = SampleTexture(texture, u, v);
            BlendPixel(
                g_renderer->pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(g_renderer->width) + static_cast<std::size_t>(x)],
                vertexColor,
                textureColor
            );
        }
    }
}

} // namespace

bool ImGui_ImplGdiSoftware_Init(HWND window) {
    if (window == nullptr || g_renderer != nullptr) {
        return false;
    }

    g_renderer = new SoftwareRendererState();
    g_renderer->window = window;
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererUserData = g_renderer;
    io.BackendRendererName = "imgui_impl_gdi_software";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
    ImGuiPlatformIO& platformIo = ImGui::GetPlatformIO();
    platformIo.Renderer_TextureMaxWidth = 8192;
    platformIo.Renderer_TextureMaxHeight = 8192;
    ImGui_ImplGdiSoftware_NewFrame();
    return true;
}

void ImGui_ImplGdiSoftware_Shutdown() {
    if (g_renderer == nullptr) {
        return;
    }

    for (ImTextureData* texture : ImGui::GetPlatformIO().Textures) {
        if (texture != nullptr && texture->BackendUserData != nullptr) {
            DestroyTexture(texture);
        }
    }
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags &= ~ImGuiBackendFlags_RendererHasTextures;
    delete g_renderer;
    g_renderer = nullptr;
}

void ImGui_ImplGdiSoftware_NewFrame() {
    if (g_renderer == nullptr || !IsWindow(g_renderer->window)) {
        return;
    }
    RECT clientRect{};
    GetClientRect(g_renderer->window, &clientRect);
    ResizeBackBuffer(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
}

void ImGui_ImplGdiSoftware_RenderDrawData(ImDrawData* drawData) {
    if (g_renderer == nullptr || drawData == nullptr || drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f) {
        return;
    }

    if (drawData->Textures != nullptr) {
        for (ImTextureData* texture : *drawData->Textures) {
            UpdateTexture(texture);
        }
    }
    std::fill(g_renderer->pixels.begin(), g_renderer->pixels.end(), 0x0009121Fu);

    for (int listIndex = 0; listIndex < drawData->CmdListsCount; ++listIndex) {
        const ImDrawList* drawList = drawData->CmdLists[listIndex];
        for (const ImDrawCmd& command : drawList->CmdBuffer) {
            if (command.UserCallback != nullptr) {
                if (command.UserCallback != ImDrawCallback_ResetRenderState) {
                    command.UserCallback(drawList, &command);
                }
                continue;
            }

            auto* texture = reinterpret_cast<SoftwareTexture*>(static_cast<std::uintptr_t>(command.GetTexID()));
            for (unsigned int element = 0; element + 2u < command.ElemCount; element += 3u) {
                const ImDrawIdx index0 = drawList->IdxBuffer[command.IdxOffset + element + 0u];
                const ImDrawIdx index1 = drawList->IdxBuffer[command.IdxOffset + element + 1u];
                const ImDrawIdx index2 = drawList->IdxBuffer[command.IdxOffset + element + 2u];
                RasterizeTriangle(
                    drawList->VtxBuffer[command.VtxOffset + index0],
                    drawList->VtxBuffer[command.VtxOffset + index1],
                    drawList->VtxBuffer[command.VtxOffset + index2],
                    drawData->DisplayPos,
                    drawData->FramebufferScale,
                    command.ClipRect,
                    texture
                );
            }
        }
    }
}

void ImGui_ImplGdiSoftware_Present(HDC dc) {
    if (g_renderer == nullptr || dc == nullptr || g_renderer->pixels.empty()) {
        return;
    }
    StretchDIBits(
        dc,
        0,
        0,
        g_renderer->width,
        g_renderer->height,
        0,
        0,
        g_renderer->width,
        g_renderer->height,
        g_renderer->pixels.data(),
        &g_renderer->bitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}
