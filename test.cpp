#include "raylib.h"
#include <vector>
#include <cmath>
#include <algorithm>

struct Planet { Vector2 pos; float mass; float radius; };

const float G       = 900.0f;
const float R_MIN   = 30.0f;   // 防止引力爆炸的最小距离
const float V_MAX   = 700.0f;
const float FUEL_MAX = 100.0f;

int main() {
    InitWindow(960, 600, "Gravity Well - Prototype");
    SetTargetFPS(60);

    Vector2 shipPos = {100, 300};
    Vector2 shipVel = {0, 0};
    float fuel = FUEL_MAX;
    
    // 记录飞船拖尾
    std::vector<Vector2> trail;

    std::vector<Planet> planets = {
        {{300, 200}, 1.0f, 24},
        {{650, 420}, 1.4f, 30},
        {{500, 120}, 0.7f, 18},
    };

    float fixedDt = 1.0f / 120.0f; // 物理固定 120Hz 更新
    float accumulator = 0.0f;

    while (!WindowShouldClose()) {
        float frameTime = GetFrameTime();
        if (frameTime > 0.25f) frameTime = 0.25f; // 防止卡顿导致穿模

        bool holding = IsKeyDown(KEY_SPACE) && fuel > 0;
        
        // --- 物理更新（固定步长） ---
        accumulator += frameTime;
        while (accumulator >= fixedDt) {
            // 找最近的星球
            Planet* nearest = nullptr;
            float bestD2 = 1e18f;
            for (auto& p : planets) {
                float dx = p.pos.x - shipPos.x, dy = p.pos.y - shipPos.y;
                float d2 = dx*dx + dy*dy;
                if (d2 < bestD2) { bestD2 = d2; nearest = &p; }
            }

            // 施加引力
            if (holding && nearest) {
                float dx = nearest->pos.x - shipPos.x;
                float dy = nearest->pos.y - shipPos.y;
                float r  = std::max(std::sqrt(bestD2), R_MIN);
                float a  = G * nearest->mass / (r * r);
                shipVel.x += (dx / r) * a * fixedDt;
                shipVel.y += (dy / r) * a * fixedDt;
                fuel -= 15.0f * fixedDt;
            }

            // 限速
            float sp = std::sqrt(shipVel.x*shipVel.x + shipVel.y*shipVel.y);
            if (sp > V_MAX) { shipVel.x *= V_MAX/sp; shipVel.y *= V_MAX/sp; }
            
            // 更新位置
            shipPos.x += shipVel.x * fixedDt;
            shipPos.y += shipVel.y * fixedDt;
            
            // 碰撞星球 = 死亡重置
            for (auto& p : planets) {
                float dx = p.pos.x - shipPos.x, dy = p.pos.y - shipPos.y;
                if (dx*dx + dy*dy < p.radius*p.radius) {
                    shipPos = {100, 300}; shipVel = {0,0}; fuel = FUEL_MAX; trail.clear();
                }
            }
            
            // 记录拖尾
            trail.push_back(shipPos);
            if (trail.size() > 20) trail.erase(trail.begin());

            accumulator -= fixedDt;
        }

        // --- 渲染 ---
        BeginDrawing();
        ClearBackground({10, 10, 25, 255});

        // 画星球
        for (auto& p : planets) DrawCircleV(p.pos, p.radius, {80, 120, 200, 255});

        // 画引力指示线
        Planet* nearest = nullptr;
        float bestD2 = 1e18f;
        for (auto& p : planets) {
            float dx = p.pos.x - shipPos.x, dy = p.pos.y - shipPos.y;
            float d2 = dx*dx + dy*dy;
            if (d2 < bestD2) { bestD2 = d2; nearest = &p; }
        }
        if (holding && nearest) {
            DrawCircleLines(nearest->pos.x, nearest->pos.y, nearest->radius + 8, GREEN);
            DrawLineV(shipPos, nearest->pos, Fade(GREEN, 0.3f));
        }

        // 画飞船拖尾
        for (size_t i = 0; i < trail.size() - 1; i++) {
            float alpha = (float)i / trail.size();
            DrawLineV(trail[i], trail[i+1], Fade(WHITE, alpha));
        }

        // 画飞船
        DrawCircleV(shipPos, 6, WHITE);

        // HUD：燃料条
        DrawRectangle(10, 10, 200, 14, DARKGRAY);
        DrawRectangle(10, 10, (int)(200*fuel/FUEL_MAX), 14, ORANGE);
        DrawText("Hold SPACE to attract", 10, 30, 20, LIGHTGRAY);

        EndDrawing();
    }
    CloseWindow();
    return 0;
}