#include "raylib.h"
#include <vector>
#include <cmath>
#include <algorithm>

using namespace std;

// 星球类
struct Planet
{
    Vector2 pos;  // 位置
    float mass;   // 质量
    float radius; // 半径
    Color color;  // 颜色
};

// 飞船类
struct Ship
{
    Vector2 pos;                // 目前位置
    Vector2 vel;                // 目前速度
    float fuel;                 // 目前油量
    bool life;                  // 是否存活
    const float radius = 10.0f; // 固定半径
    const Color color = GREEN;  // 固定颜色
};

const float G = 900.0f;
const float R_MIN = 100.0f; // 防止引力爆炸的最小距离
const float V_MAX = 400.0f;
const float FUEL_MAX = 100.0f;
const float FIXED_DT = 1.0f / 120.0f;
const float FUEL_LOW_OF_TICK = 0.3f;

Planet *nearestplanet(const Ship &ship, vector<Planet> &planets)
{
    Planet *nearest = nullptr;
    float bestD2 = 1e18f;
    for (auto &p : planets)
    {
        float dx = p.pos.x - ship.pos.x, dy = p.pos.y - ship.pos.y;
        float d2 = dx * dx + dy * dy;
        if (d2 < bestD2)
        {
            bestD2 = d2;
            nearest = &p;
        }
    }
    return nearest;
}

inline float dist(float dx, float dy) { return sqrt(dx * dx + dy * dy); }

void makeshipdie(Ship& ship);
float adding=0.0f;
int main()
{
    InitWindow(1200, 1000, "Gravity-Well 引力井");
    SetTargetFPS(120);
    Ship ship = {{50, 50}, {0, 0}, FUEL_MAX, true}; // 飞船实例
    vector<Planet> planets = {                      // 星球实例
                              {{300, 400}, 1.0f, 50.0f, GRAY},
                              {{400, 200}, 1.3f, 50.6f, GREEN},
                              {{900, 500}, 0.79f, 48.5f, ORANGE}};
    while (!WindowShouldClose())
    {
        if (!ship.life)
            break;
        float frametime = GetFrameTime();
        if (frametime > 0.125f)
        {
            frametime = 0.125f;
        }
        adding += frametime;
        while(adding>=FIXED_DT){
            bool holding=IsMouseButtonDown(MOUSE_BUTTON_LEFT) && ship.fuel > 0;
            if(holding){
                auto nearest=nearestplanet(ship,planets);
                if(!nearest) break;
                float dx=nearest->pos.x-ship.pos.x;
                float dy=nearest->pos.y-ship.pos.y;
                float r=max(dist(dx,dy),R_MIN);
                float a = G * nearest->mass / (r * r);
                ship.vel.x += (dx / r) * a * FIXED_DT;
                ship.vel.y +=(dy/r)*a*FIXED_DT;
                // 限速：按速度模长缩放
                float sp = sqrt(ship.vel.x*ship.vel.x + ship.vel.y*ship.vel.y);
                if (sp > V_MAX){ ship.vel.x *= V_MAX/sp; ship.vel.y *= V_MAX/sp; }
                ship.fuel = max(ship.fuel - FUEL_LOW_OF_TICK, 0.0f);
            }
            ship.pos.x += ship.vel.x * FIXED_DT;
            ship.pos.y += ship.vel.y * FIXED_DT;
            adding -= FIXED_DT;
        }
        auto nearest = nearestplanet(ship, planets);
        if (!nearest)
            continue;
        float dx = nearest->pos.x - ship.pos.x;
        float dy = nearest->pos.y - ship.pos.y;
        float r = dist(dx, dy);
        if(r<nearest->radius||ship.fuel<=0) makeshipdie(ship);
        BeginDrawing();
        ClearBackground(BLACK);
        for (auto &p : planets)
        {
            DrawCircle(p.pos.x, p.pos.y, p.radius, p.color);
        }
        DrawCircleLinesV(ship.pos,ship.radius,ship.color);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}

// makeshipdie：标记飞船死亡，主循环检测到 life=false 会退出
void makeshipdie(Ship& ship){
    ship.life = false;
}
