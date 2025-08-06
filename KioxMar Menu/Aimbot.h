#pragma once

#include "Vector.h"
#include "Offsets.h"
#include <windows.h>
#include <cmath>
#include <iostream>
#include"../src/window.hpp"
#include "../z3n1th_v69420/src/Memory.h"
#include "Vector.h"
#include "offsets.h"

#include <Windows.h>

#include "Memory.h"
#include "Vector.h"
#include <Windows.h>
#pragma once

#include "Memory.h"
#include "Vector.h"

#include <cmath>
#include <Windows.h>

namespace Imbot {

    float GetDistance3D(const CVector& a, const CVector& b) {
        return sqrtf(
            (a.x - b.x) * (a.x - b.x) +
            (a.y - b.y) * (a.y - b.y) +
            (a.z - b.z) * (a.z - b.z)
        );
    }

    ImVec2 GetScreenCenter() {
        RECT desktop;
        GetClientRect(GetDesktopWindow(), &desktop);
        return ImVec2(desktop.right / 2.0f, desktop.bottom / 2.0f);
    }

    void Run(CDispatcher* mem) {
        uint64_t base = mem->GetModuleBase("DayZ_x64.exe");

 
        uint64_t worldPtr = mem->ReadMemory<uint64_t>(base + offsets::world::world);
        if (!worldPtr) return;

        World world = mem->ReadMemory<World>(worldPtr);
        Camera cam = mem->ReadMemory<Camera>(world.Camera);


        uint64_t EntityLink = mem->ReadMemory<uint64_t>(worldPtr + 0x2960);
        uint64_t localPlayer = mem->ReadMemory<uint64_t>(EntityLink + 0x8) - 0xA8;
        uint64_t localPlrVisState = mem->ReadMemory<uint64_t>(localPlayer + 0x1D0);
        CVector localPos = mem->ReadMemory<CVector>(mem->ReadMemory<uint64_t>(localPlayer + offsets::human::visual_state) + 0x2C);

     
        uint64_t nearTable = mem->ReadMemory<uint64_t>(worldPtr + offsets::world::near_entity_table);
        int nearTableSize = mem->ReadMemory<int>(worldPtr + offsets::world::near_entity_table_size);

        float closestDist = FLT_MAX;
        CVector closestBone{};

        for (int i = 0; i < nearTableSize; i++) {
            uint64_t entity = mem->ReadMemory<uint64_t>(nearTable + i * 0x8);
            if (!entity || entity == localPlayer) continue;

            uint64_t visState = mem->ReadMemory<uint64_t>(entity + offsets::human::visual_state);
            CVector entityPos = mem->ReadMemory<CVector>(mem->ReadMemory<uint64_t>(entity + offsets::human::visual_state) + 0x2C);
            float dist = GetDistance3D(localPos, entityPos);
            if (dist > 500.0f) continue;

       
            uint64_t skeleton = mem->ReadMemory<uint64_t>(entity + offsets::dayz_player::skeleton);
            uint64_t animClass1 = mem->ReadMemory<uint64_t>(skeleton + offsets::skeleton::anim_class1);
            uint64_t animClass2 = mem->ReadMemory<uint64_t>(animClass1 + offsets::skeleton::anim_class2);
            uint64_t matrixPtr = mem->ReadMemory<uint64_t>(animClass2 + offsets::anim_class::matrix_array);
            CVector head = mem->ReadMemory<CVector>(matrixPtr + (0x1E * sizeof(CVector)));

         
            CVector screenPos;
            if (cam.ScreenPosition(head, screenPos)) {
                ImVec2 center = GetScreenCenter();
                float dx = screenPos.x - center.x;
                float dy = screenPos.y - center.y;
                float dist2D = sqrtf(dx * dx + dy * dy);

                if (dist2D < 100.0f && dist < closestDist) {
                    closestDist = dist;
                    closestBone = head;
                }
            }
        }

       
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000 && closestDist < FLT_MAX) {
            CVector screenPos;
            if (cam.ScreenPosition(closestBone, screenPos)) {
                ImVec2 center = GetScreenCenter();
                float dx = screenPos.x - center.x;
                float dy = screenPos.y - center.y;

                mouse_event(MOUSEEVENTF_MOVE, static_cast<DWORD>(dx), static_cast<DWORD>(dy), 0, 0);
            }
        }
    }
}