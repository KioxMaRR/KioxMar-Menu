#include <Windows.h>
#include "GameEngine.h"
#include <iostream>
#include "window.hpp"
#include "Memory.h"
#include <thread>
#include "offsets.h"
#include<lmcons.h>
#include"vector"
#include"../Aimbot.h"

bool ShowCar = false;
bool ShowZombi = false;
bool ShowPlayer = false;

#include <ctime>
#include <TlHelp32.h>
ImVec4 espColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
ImVec4 crosshairColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
bool LimitEspDistance = true;
float espDistanceLimit = 1000.0f;
bool isNumLockOn() {
    return (GetKeyState(VK_NUMLOCK) & 0x0001);
}

void waitForNumLock() {
 
    while (!isNumLockOn()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

}
bool IsProcessRunning(const wchar_t* processName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(entry);

    if (Process32FirstW(hSnap, &entry)) {
        do {
            if (wcscmp(entry.szExeFile, processName) == 0) {
                CloseHandle(hSnap);
                return true;
            }
        } while (Process32NextW(hSnap, &entry));
    }

    CloseHandle(hSnap);
    return false;
}

void WaitForDayZ() {
    while (!IsProcessRunning(L"DayZ_x64.exe")) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}


int main() {

  
    WaitForDayZ();
    // thread your cheat here
    CDispatcher* mem = CDispatcher::Get();
    mem->Attach("DayZ_x64.exe");



    uint64_t base = mem->GetModuleBase("DayZ_x64.exe");


    ShowWindow(GetConsoleWindow(), SW_HIDE);
    overlay.shouldRun = true;
    overlay.RenderMenu = false;

    overlay.CreateOverlay();
    overlay.CreateDevice();
    overlay.CreateImGui();



    overlay.SetForeground(GetConsoleWindow());

    while (overlay.shouldRun)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        overlay.StartRender();

        if (overlay.RenderMenu) {
            overlay.Render();
        }
        else
        if (drawCrosshair == true) {
            ImVec2 screenSize = ImGui::GetIO().DisplaySize;
            ImVec2 center(screenSize.x * 0.5f, screenSize.y * 0.5f);
            ImU32 color = ImGui::ColorConvertFloat4ToU32(espColor); 

            ImGui::GetForegroundDrawList()->AddLine(ImVec2(center.x - 20, center.y), ImVec2(center.x + 20, center.y), color, 0.3f);
            ImGui::GetForegroundDrawList()->AddLine(ImVec2(center.x, center.y - 20), ImVec2(center.x, center.y + 20), color, 0.3f);
        }
        uint64_t worldPtr = mem->ReadMemory<uint64_t>(base + offsets::world::world);
        World world = mem->ReadMemory<World>(worldPtr);
        Camera cam = mem->ReadMemory<Camera>(world.Camera);
        uint64_t EntityLink = mem->ReadMemory<uint64_t>(worldPtr + 0x2960);
        uint64_t localPlayer = mem->ReadMemory<uint64_t>(EntityLink + 0x8) - 0xA8;
        uint64_t localPlrVisState = mem->ReadMemory<uint64_t>(localPlayer + 0x1D0);
        CVector localPos = mem->ReadMemory<CVector>(mem->ReadMemory<uint64_t>(localPlayer + offsets::human::visual_state) + 0x2C);
        if (AimBot == true) {


            Imbot::Run(mem);
        }



        if (showEsp && (ShowCar || ShowPlayer || ShowZombi || ShowAnimal)) {
   

            auto DrawESP = [&](uint64_t entity) {
                uint64_t entityVT = mem->ReadMemory<uint64_t>(entity + 0x0);
                uint64_t entityVisState = mem->ReadMemory<uint64_t>(entity + 0x1D0);
                CVector pos = mem->ReadMemory<CVector>(entityVisState + 0x2C);
                CVector screen;
                uint64_t ptrrtti = mem->ReadMemory<uint64_t>(entityVT - 0x8);
                RTTI rtti = mem->ReadMemory<RTTI>(ptrrtti);
                type_descriptor type = mem->ReadMemory<type_descriptor>(base + rtti.rva_type_descriptor);
                const char* typeName = type.get_type_name();

                if (!typeName) return; 

                size_t typeNameLength = strlen(typeName);
                if (typeNameLength < 2) return;

                std::string modifiedTypeName(typeName, typeNameLength - 2);

                if (entity == localPlayer)
                    return;

                bool isCar = modifiedTypeName.find("Car") != std::string::npos;
                bool isPlayer = modifiedTypeName.find("DayZPlayer") != std::string::npos;
                bool isZombie = modifiedTypeName.find("DayZInfected") != std::string::npos;
                bool isAnimal = modifiedTypeName.find("DayZAnimal") != std::string::npos;
        

                if ((isCar && ShowCar) || (isPlayer && ShowPlayer) || (isZombie && ShowZombi) || (isAnimal && ShowAnimal)) {
                    if (cam.ScreenPosition(pos, screen)) {
                        float dx = pos.x - localPos.x;
                        float dy = pos.y - localPos.y;
                        float dz = pos.z - localPos.z;
                        float distance = sqrtf(dx * dx + dy * dy + dz * dz);
                        if (LimitEspDistance && distance > espDistanceLimit)
                            return;
                        ImU32 color = ImGui::ColorConvertFloat4ToU32(espColor);

                        if (ShowEspBox) {
                            float scale = Clamp(1.0f / distance, 0.4f, 1.0f);
                            float boxHeight = 150.0f * scale;
                            float boxWidth = boxHeight / 2.0f;
                            ImVec2 topLeft(screen.x - boxWidth / 2, screen.y - boxHeight);
                            ImVec2 bottomRight(screen.x + boxWidth / 2, screen.y);
                            ImGui::GetForegroundDrawList()->AddRect(
                                topLeft,
                                bottomRight,
                                color,
                                0,
                                1.5f
                            );
                        }

                        char buffer[128];
                        snprintf(buffer, sizeof(buffer), "%s [%.1fm]", modifiedTypeName.c_str(), distance);
                        ImGui::GetForegroundDrawList()->AddText(
                            ImVec2(screen.x + 6.0f, screen.y - 3.0f),
                            color, 
                            buffer
                        );

                        if (ShowEspLine) {
                            ImVec2 screenSize = ImGui::GetIO().DisplaySize;
                            ImVec2 center(screenSize.x * 0.5f, screenSize.y);
                            ImGui::GetForegroundDrawList()->AddLine(
                                center,
                                ImVec2(screen.x, screen.y),
                                color,
                                1.0f
                            );
                        }

                    }
                }
                };

            for (int i = 0; i < world.NearTableCount; i++) {
                uint64_t nearEntity = mem->ReadMemory<uint64_t>(world.NearTable + (i * 0x8));
                DrawESP(nearEntity);
            }

            for (int i = 0; i < world.FarTableCount; i++) {
                uint64_t farEntity = mem->ReadMemory<uint64_t>(world.FarTable + (i * 0x8));
                DrawESP(farEntity);
            }
        }
        if (showBulletESP) {
            uint64_t worldPtr = mem->ReadMemory<uint64_t>(base + offsets::world::world);
            World world = mem->ReadMemory<World>(worldPtr);
            Camera cam = mem->ReadMemory<Camera>(world.Camera);



            for (int i = 0; i < world.BulletTableCount; i++) {
                uint64_t bulletEntity = mem->ReadMemory<uint64_t>(world.BulletTable + (i * 0x8));
                uint64_t entityVisState = mem->ReadMemory<uint64_t>(bulletEntity + 0x1D0);
                CVector pos = mem->ReadMemory<CVector>(entityVisState + 0x2C);
                CVector screen;
                if (cam.ScreenPosition(pos, screen)) {
                    ImGui::GetForegroundDrawList()->AddCircle(ImVec2(screen.x, screen.y), 8.0f, IM_COL32(57, 22, 255, 220), 0, 1.0f);
                }
            }
        }
        // if you want to render here, you could move the imgui includes to your .hpp file instead of the .cpp file!
        overlay.EndRender();
    }

    overlay.DestroyImGui();
    overlay.DestroyDevice();
    overlay.DestroyOverlay();
}