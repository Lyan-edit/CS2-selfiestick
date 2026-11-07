// Render the real production UI in a hidden, standalone process. No game module,
// injection, InitializeSelfieStick, camera hook or entity lookup is started here.
#include "../selfiestick_hlae/selfiestick_hlae.cpp"
#include <cstdio>
#include <vector>
#include "player_hand_checks.h"

namespace {
LRESULT CALLBACK PreviewProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    return DefWindowProcW(window, message, wParam, lParam);
}

std::vector<std::uint32_t> ReadFrame(HWND window, int& width, int& height) {
    RECT client{};
    GetClientRect(window, &client);
    width = client.right;
    height = client.bottom;
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    HDC dc = CreateCompatibleDC(nullptr);
    void* pixels = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
    if (!bitmap || !pixels) { if (dc) DeleteDC(dc); return {}; }
    HGDIOBJ previous = SelectObject(dc, bitmap);
    ImGui_ImplGdiSoftware_Present(dc);
    GdiFlush();
    const auto* first = static_cast<const std::uint32_t*>(pixels);
    std::vector<std::uint32_t> result(first, first + width * height);
    SelectObject(dc, previous);
    DeleteObject(bitmap);
    DeleteDC(dc);
    return result;
}

bool SaveFrame(HWND window, const wchar_t* path) {
    int width{}, height{};
    const auto pixels = ReadFrame(window, width, height);
    if (pixels.empty()) return false;
    FILE* file = nullptr;
    if (_wfopen_s(&file, path, L"wb") != 0 || !file) return false;
    BITMAPFILEHEADER header{};
    header.bfType = 0x4D42;
    header.bfOffBits = sizeof(header) + sizeof(BITMAPINFOHEADER);
    header.bfSize = header.bfOffBits + static_cast<DWORD>(pixels.size() * 4);
    BITMAPINFOHEADER info{};
    info.biSize = sizeof(info);
    info.biWidth = width;
    info.biHeight = -height;
    info.biPlanes = 1;
    info.biBitCount = 32;
    const bool saved = fwrite(&header, sizeof(header), 1, file) == 1
        && fwrite(&info, sizeof(info), 1, file) == 1
        && fwrite(pixels.data(), pixels.size() * 4, 1, file) == 1;
    fclose(file);
    return saved;
}
}

int wmain() {
    using namespace selfiestick;
    if (!hand_checks::Run()) return 20;
    WNDCLASSW wc{};
    wc.lpfnWndProc = PreviewProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"SelfiestickHiddenUiCheck";
    if (!RegisterClassW(&wc)) return 1;
    HWND window = CreateWindowW(wc.lpszClassName, L"", WS_POPUP, 0, 0, 620, 760,
        nullptr, nullptr, wc.hInstance, nullptr);
    if (!window) return 2;
    if (!InitializeImGuiOverlay(window)) return 3;
    // Assert upstream style geometry/palette rather than a themed approximation.
    ImGuiStyle upstream;
    ImGui::StyleColorsDark(&upstream);
    const auto& style = ImGui::GetStyle();
    if (style.FrameRounding != upstream.FrameRounding
        || style.FramePadding.x != upstream.FramePadding.x
        || std::memcmp(style.Colors, upstream.Colors, sizeof(style.Colors)) != 0) return 4;

    g_state.initialized = true;
    g_state.offset = {};
    g_state.playerRotation = {};
    g_ui.runtimeExpanded = false;
    RenderImGuiOverlayFrame();
    RenderImGuiOverlayFrame();
    if (!SaveFrame(window, L"player.bmp")) return 5;
    int width{}, height{};
    const auto clean = ReadFrame(window, width, height);
    for (int frame = 0; frame < 12; ++frame) {
        g_state.offset.x = -24.0f + frame * 4.0f;
        RenderImGuiOverlayFrame();
    }
    g_state.offset = {};
    RenderImGuiOverlayFrame();
    if (ReadFrame(window, width, height) != clean) return 6;

    g_state.cameraMode = CameraMode::PropSelfie;
    RenderImGuiOverlayFrame();
    RenderImGuiOverlayFrame();
    if (!SaveFrame(window, L"prop.bmp")) return 7;
    g_ui.runtimeExpanded = true;
    SetWindowPos(window, nullptr, 0, 0, 620, 930, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    RenderImGuiOverlayFrame();
    RenderImGuiOverlayFrame();
    if (!SaveFrame(window, L"prop-expanded.bmp")) return 8;
    ImGui_ImplGdiSoftware_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::GetBackgroundDrawList()->AddRectFilled({0, 0}, {32, 32}, IM_COL32(200, 100, 50, 128));
    ImGui::Render();
    ImGui_ImplGdiSoftware_RenderDrawData(ImGui::GetDrawData());
    const auto flat = ReadFrame(window, width, height);
    for (int y = 2; y < 30; ++y) {
        for (int x = 2; x < 30; ++x) {
            if (flat[y * width + x] != flat[2 * width + 3]) return 9;
        }
    }
    CleanupImGuiOverlay();
    DestroyWindow(window);
    puts("production ImGui style, player/prop/status rendering and full-frame smear check passed");
    return 0;
}
