#include "raylib.h"

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
#include <emscripten/emscripten.h>
#endif

#include <math.h>

#define UPDATE_FPS 60
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define VIRTUAL_WIDTH (SCREEN_WIDTH / 2)
#define VIRTUAL_HEIGHT (SCREEN_HEIGHT / 2)
#define HALF_WIDTH (VIRTUAL_WIDTH / 2)
#define HALF_HEIGHT (VIRTUAL_HEIGHT / 2)

#define HORIZON (VIRTUAL_HEIGHT / 4)

RenderTexture renderTarget;
Rectangle sourceRect = {0, 0, VIRTUAL_WIDTH, -VIRTUAL_HEIGHT};
Rectangle destRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

void DrawQuad(Vector2 tl, Vector2 tr, Vector2 br, Vector2 bl, Color color)
{
    DrawTriangle(tl, bl, tr, color);
    DrawTriangle(tr, bl, br, color);
}

void Draw(Vector2 a, Vector2 b)
{
    static float cz = 0;

    ClearBackground(BLACK);

    BeginDrawing();
    BeginTextureMode(renderTarget);
    Vector2 bl = {HALF_WIDTH, HALF_HEIGHT}, br = bl, bri = bl, bli = bl;
    cz += 0.5f;
    DrawQuad(a, (Vector2){b.x, a.y}, (Vector2){b.x, a.y + HORIZON}, (Vector2){a.x, a.y + HORIZON}, SKYBLUE);
    DrawQuad((Vector2){a.x, a.y + HORIZON}, (Vector2){b.x, a.y + HORIZON}, b, (Vector2){a.x, b.y}, DARKGREEN);
    for (int s = 300; s > 0; s--)
    {
        float c = sinf((cz + s) * 0.1f) * 500;
        float f = cosf((cz + s) * 0.02f) * 1000;
        Vector2 tl = bl, tr = br, tli = bli, tri = bri;
        tli.y--;
        tri.y--;
        float ss = 0.003f / s;
        float w = 2000 * ss * HALF_WIDTH;
        float px = a.x + HALF_WIDTH + (f * ss * HALF_WIDTH);
        float py = a.y + HORIZON - (ss * (c * 2 - 2500) * HALF_HEIGHT);
        bl = (Vector2){px - w, py};
        br = (Vector2){px + w, py};
        w = 1750 * ss * HALF_WIDTH;
        bli = (Vector2){px - w, py};
        bri = (Vector2){px + w, py};
        if (s != 300)
        {
            bool j = fmodf(cz + s, 10) < 5;
            DrawQuad(tl, tli, bli, bl, j ? WHITE : RED);
            DrawQuad(tri, tr, br, bri, j ? WHITE : RED);
            DrawQuad(tli, tri, bri, bli, j ? DARKGRAY : GRAY);
        }
    }
    EndTextureMode();
    DrawTexturePro(renderTarget.texture, sourceRect, destRect, (Vector2){0, 0}, 0, WHITE);
    DrawFPS(4, 4);
    EndDrawing();
}

void UpdateDrawFrame()
{
    Draw((Vector2){0, 0}, (Vector2){VIRTUAL_WIDTH, VIRTUAL_HEIGHT});
}

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Racetrack");
    SetTargetFPS(UPDATE_FPS);

    renderTarget = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(renderTarget.texture, TEXTURE_FILTER_ANISOTROPIC_16X);

#if defined(PLATFORM_WEB) || defined(EMSCRIPTEN)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    while (!WindowShouldClose())
    {
        UpdateDrawFrame();
    }
#endif
    CloseWindow();

    return 0;
}
