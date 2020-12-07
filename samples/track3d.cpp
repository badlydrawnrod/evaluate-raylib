#include "raylib.h"

#include <cmath>

namespace my
{
    constexpr int updateFps = 60;
    constexpr int screenWidth = 1280;
    constexpr int screenHeight = 720;

    constexpr int virtualWidth = screenWidth / 2;
    constexpr int virtualHeight = screenHeight / 2;
    constexpr int halfWidth = virtualWidth / 2;
    constexpr int halfHeight = virtualHeight / 2;

    constexpr int horizon = virtualHeight / 4;

    RenderTexture renderTarget;
    Rectangle sourceRect{0, 0, virtualWidth, -virtualHeight};
    Rectangle destRect{0, 0, screenWidth, screenHeight};

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
        Vector2 bl = {halfWidth, halfHeight}, br = bl, bri = bl, bli = bl;
        cz += 0.5f;
        DrawQuad(a, {b.x, a.y}, {b.x, a.y + horizon}, {a.x, a.y + horizon}, SKYBLUE);
        DrawQuad({a.x, a.y + horizon}, {b.x, a.y + horizon}, b, {a.x, b.y}, DARKGREEN);
        for (int s = 300; s > 0; s--)
        {
            float c = sinf((cz + s) * 0.1f) * 500;
            float f = cosf((cz + s) * 0.02f) * 1000;
            Vector2 tl = bl, tr = br, tli = bli, tri = bri;
            tli.y--;
            tri.y--;
            float ss = 0.003f / s;
            float w = 2000 * ss * halfWidth;
            float px = a.x + halfWidth + (f * ss * halfWidth);
            float py = a.y + horizon - (ss * (c * 2 - 2500) * halfHeight);
            bl = {px - w, py};
            br = {px + w, py};
            w = 1750 * ss * halfWidth;
            bli = {px - w, py};
            bri = {px + w, py};
            if (s != 300)
            {
                bool j = fmodf(cz + s, 10) < 5;
                DrawQuad(tl, tli, bli, bl, j ? WHITE : RED);
                DrawQuad(tri, tr, br, bri, j ? WHITE : RED);
                DrawQuad(tli, tri, bri, bli, j ? DARKGRAY : GRAY);
            }
        }
        EndTextureMode();
        DrawTexturePro(renderTarget.texture, sourceRect, destRect, {0, 0}, 0, WHITE);
        DrawFPS(4, 4);
        EndDrawing();
    }

} // namespace my

int main()
{
    InitWindow(my::screenWidth, my::screenHeight, "Racetrack");
    SetTargetFPS(my::updateFps);

    my::renderTarget = LoadRenderTexture(my::virtualWidth, my::virtualHeight);
    SetTextureFilter(my::renderTarget.texture, FILTER_ANISOTROPIC_16X);

    while (!WindowShouldClose())
    {
        my::Draw({0, 0}, {my::virtualWidth, my::virtualHeight});
    }
    CloseWindow();

    return 0;
}
