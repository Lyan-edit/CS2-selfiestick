#pragma once

#include <windows.h>

struct ImDrawData;

// Dear ImGui renderer backend that rasterizes ImDrawData into a top-down
// 32-bit DIB in system memory and presents it with StretchDIBits. It creates no
// D3D/OpenGL/Vulkan device, swap chain, or Present hook, which keeps the panel
// outside ReShade's graphics path even though all widgets are genuine ImGui.
bool ImGui_ImplGdiSoftware_Init(HWND window);
void ImGui_ImplGdiSoftware_Shutdown();
void ImGui_ImplGdiSoftware_NewFrame();
void ImGui_ImplGdiSoftware_RenderDrawData(ImDrawData* drawData);
void ImGui_ImplGdiSoftware_Present(HDC dc);
