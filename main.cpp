#include "raylib.h"
#include <vector>
#include <cmath>
#include <algorithm>

using namespace std;

//星球类
struct Planet{
    Vector2 pos; //位置
    float mass; //质量
    float radius; //半径
    Color color; //颜色
};

//飞船类
struct Ship{
    Vector2 pos; //目前位置
    Vector2 vel; //目前速度
    float fuel; //目前油量
};

const float G = 900.0f;
const float R_MIN = 30.0f; // 防止引力爆炸的最小距离
const float V_MAX = 700.0f;
const float FUEL_MAX = 100.0f;

int main(){
    InitWindow(1200,1000,"Gravity-Well 引力井");
    SetTargetFPS(120);
    Ship ship={{50,50},{0,0},FUEL_MAX}; //飞船实例
    vector<Planet> planet = { // 星球实例
        {{300, 400}, 1.0f, 18.0f,GRAY},
        {{400, 200}, 1.3f, 18.6f,GREEN},
        {{900, 500}, 0.79f, 17.5f,ORANGE}
    };
    while (!WindowShouldClose())
    {
        BeginDrawing();
        for(auto& p : planet){
            DrawCircle(p.pos.x,p.pos.y,p.radius,p.color);
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}