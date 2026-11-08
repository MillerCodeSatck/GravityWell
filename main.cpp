#include "raylib.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

// 星球类
struct Planet
{
    Vector2 pos;  // 位置
    float mass;   // 质量
    float radius; // 半径
    Color color;  // 颜色
};

// 核心类
struct Core
{
    Vector2 pos;            // 核心位置
    float radius;           // 核心大小
    bool iscollect = false; // 是否收集
};

// 飞船类
struct Ship
{
    Vector2 pos;                // 目前位置
    Vector2 vel;                // 目前速度
    float fuel;                 // 目前油量
    bool life = true;           // 是否存活
    const float radius = 10.0f; // 固定半径
    const Color color = GREEN;  // 固定颜色
};

// 出口类
struct Exit
{
    Vector2 pos;
    float size;
};

const float G = 10000.0f;
const float R_MIN = 0.0f; // 防止引力爆炸的最小距离
const float V_MAX = 800.0f;
const float FUEL_MAX = 100.0f;
const float FIXED_DT = 1.0f / 120.0f;
const float FUEL_LOW_OF_TICK = 0.2f;

// 读取level信息
void readleveldata(int level, Ship &ship, vector<Planet> &planets, vector<Core> &cores, Exit &exit)
{
    planets.clear();
    cores.clear();
    ifstream leveldatafile("assets\\levels\\level" + to_string(level) + ".txt");
    if (!leveldatafile.is_open())
    {
        TraceLog(LOG_WARNING, "没有文件%s", "assets\\levels\\level" + to_string(level) + ".txt");
        return;
    }
    string line;
    while (getline(leveldatafile, line))
    {
        istringstream iline(line);
        string mode;
        iline >> mode;
        if (mode == "ship")
        {
            iline >> ship.pos.x >> ship.pos.y >> ship.vel.x >> ship.vel.y >> ship.fuel;
        }
        else if (mode == "planet")
        {
            Planet p;
            int r, g, b, a;
            iline >> p.pos.x >> p.pos.y >> p.mass >> p.radius >> r >> g >> b >> a;
            p.color = {(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a};
            planets.push_back(p);
        }
        else if (mode == "core")
        {
            Core c;
            iline >> c.pos.x >> c.pos.y >> c.radius;
            cores.push_back(c);
        }
        else if (mode == "exit")
        {
            iline >> exit.pos.x >> exit.pos.y >> exit.size;
        }
    }
    leveldatafile.close();
}

// 关卡总数统计
int countlevels()
{
    int count = 0;
    while (true)
    {
        ifstream f("assets\\levels\\level" + to_string(count + 1) + ".txt");
        if (!f.is_open())
            break;
        f.close();
        count++;
    }
    return count;
}

// 加载关卡
int readlevel()
{
    ifstream levelfile("assets\\save.txt");
    int level = 1;
    if (levelfile.is_open())
        levelfile >> level;
    return level;
}

// 保存关卡
void savelevel(int level)
{
    ofstream levelfile("assets\\save.txt");
    levelfile << level;
}

// 找离飞船最近的星球
Planet *findnearestplanet(const Ship &ship, vector<Planet> &planets)
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

// 找离飞船最近的核心
Core *findnearestcore(const Ship &ship, vector<Core> &cores)
{
    Core *nearest = nullptr;
    float bestD2 = 1e18f;
    for (auto &c : cores)
    {
        float dx = c.pos.x - ship.pos.x, dy = c.pos.y - ship.pos.y;
        float d2 = dx * dx + dy * dy;
        if (d2 < bestD2 && !c.iscollect)
        {
            bestD2 = d2;
            nearest = &c;
        }
    }
    return nearest;
}

inline float dist(float dx, float dy) { return sqrt(dx * dx + dy * dy); }

// ================= 核心渲染函数 =================
// 绘制一帧：星球、能量核心、引力圈/线、飞船、油量条、核心进度、出口、关卡
void RenderGame(const Ship &ship, const vector<Planet> &planets,
                const vector<Core> &cores, const Planet *nearest, Exit exit, int level)
{
    ClearBackground({10, 10, 25, 255});

    // 1. 星球
    for (const auto &p : planets)
        DrawCircleV(p.pos, p.radius, p.color);

    // 2. 能量核心（未收集画光晕+实心，已收集跳过）
    for (const auto &c : cores)
    {
        if (c.iscollect)
            continue;
        DrawCircleLinesV(c.pos, c.radius + 4, Fade(GOLD, 0.5f));
        DrawCircleV(c.pos, c.radius, GOLD);
    }

    // 3. 引力圈 + 引力线（按住左键且仍有燃料时）
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && ship.fuel > 0 && nearest)
    {
        DrawCircleLinesV(nearest->pos, nearest->radius + 8, GREEN);
        DrawLineV(ship.pos, nearest->pos, Fade(GREEN, 0.3f));
    }

    // 4. 飞船
    DrawCircleLinesV(ship.pos, ship.radius, ship.color);

    // 5. 油量条
    DrawRectangle(10, 10, 200, 14, DARKGRAY);
    DrawRectangle(10, 10, (int)(200 * ship.fuel / FUEL_MAX), 14, ORANGE);

    // 6. 核心收集进度
    int total = (int)cores.size(), collected = 0;
    for (const auto &c : cores)
        if (c.iscollect)
            collected++;
    DrawText(TextFormat("Core %d / %d", collected, total), 10, 30, 20, LIGHTGRAY);

    // 7. 出口
    Vector2 v[4] = {
        {exit.pos.x, exit.pos.y},
        {exit.pos.x + exit.size, exit.pos.y},
        {exit.pos.x + exit.size, exit.pos.y + exit.size},
        {exit.pos.x, exit.pos.y + exit.size}};
    for (int i = 0; i < 4; i++)
    {
        DrawLineEx(v[i], v[(i + 1) % 4], 5, GREEN);
    }

    // 8. 关卡进度
    DrawText(TextFormat("Level %d", level), 10, 50, 25, LIGHTGRAY);
}
// =================================================

float adding = 0.0f;
int level = readlevel();
int main()
{
    InitWindow(1400, 1100, "Gravity-Well 引力井");
    SetTargetFPS(120);
    Ship ship;              // 飞船实例
    Exit exit;              // 出口实例
    vector<Planet> planets; // 星球实例
    vector<Core> cores;     // 核心实例
    readleveldata(level, ship, planets, cores, exit);
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
        while (adding >= FIXED_DT)
        {
            bool holding = IsMouseButtonDown(MOUSE_BUTTON_LEFT) && ship.fuel > 0;
            if (holding)
            {
                auto nearest = findnearestplanet(ship, planets);
                if (nearest) // 空指针保护：没有星球就不施力
                {
                    float dx = nearest->pos.x - ship.pos.x;
                    float dy = nearest->pos.y - ship.pos.y;
                    float r = max(dist(dx, dy), R_MIN);
                    float a = G * nearest->mass / (r * r);
                    ship.vel.x += (dx / r) * a * FIXED_DT;
                    ship.vel.y += (dy / r) * a * FIXED_DT;
                    ship.fuel = max(ship.fuel - FUEL_LOW_OF_TICK, 0.0f);
                }
            }
            // 限速：按速度模长缩放
            float sp = sqrt(ship.vel.x * ship.vel.x + ship.vel.y * ship.vel.y);
            if (sp > V_MAX)
            {
                ship.vel.x *= V_MAX / sp;
                ship.vel.y *= V_MAX / sp;
            }
            ship.pos.x += ship.vel.x * FIXED_DT;
            ship.pos.y += ship.vel.y * FIXED_DT;
            adding -= FIXED_DT;
        }

        auto nearestplanet = findnearestplanet(ship, planets);
        if (nearestplanet)
        {
            float rforplanet = dist(nearestplanet->pos.x - ship.pos.x, nearestplanet->pos.y - ship.pos.y);
            if (rforplanet < nearestplanet->radius + ship.radius)
                readleveldata(level, ship, planets, cores, exit);
        }

        auto nearestcore = findnearestcore(ship, cores);
        if (nearestcore)
        {
            float rforcore = dist(nearestcore->pos.x - ship.pos.x, nearestcore->pos.y - ship.pos.y);
            if (rforcore < nearestcore->radius + ship.radius)
                nearestcore->iscollect = true;
        }

        int total = (int)cores.size(), collected = 0;
        for (const auto &c : cores)
            if (c.iscollect)
                collected++;
        float rforexit = dist(exit.pos.x - ship.pos.x, exit.pos.y - ship.pos.y);
        if (rforexit < exit.size * 0.6f && collected == cores.size())
        {
            level++;
            savelevel(level);
            readleveldata(level, ship, planets, cores, exit);
        }
        BeginDrawing();
        RenderGame(ship, planets, cores, nearestplanet, exit, level);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}