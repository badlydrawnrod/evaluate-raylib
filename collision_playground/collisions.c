#define PHYSICS_FPS 60.0
#define BDR_LOOP_FIXED_UPDATE_INTERVAL_SECONDS (1.0 / PHYSICS_FPS)
#define BDR_LOOP_FIXED_UPDATE FixedUpdate
#define BDR_LOOP_DRAW Draw
#define BDR_LOOP_CHECK_TRIGGERS CheckTriggers

#define BDR_LOOP_IMPLEMENTATION

#include "loop_runner.h"
#include "raylib.h"
#include "raymath.h"

#define TARGET_FPS 60

#define SCREEN_WIDTH 2048
#define SCREEN_HEIGHT 1024

#define NUM_ITEMS 128

typedef struct
{
    Vector2 c; // Centre.
    Vector2 e; // Extents (half-widths).
} AABB;

typedef struct
{
    Vector2 start;
    Vector2 end;
} Segment;

typedef struct
{
    Vector2 position; // Position.
    Vector2 size;     // Drawing size.
    Vector2 velocity; // Velocity.
    Color color;      // Colour.
    AABB aabb;        // AABB.
    bool hit;         // Is it in collision?
} Item;

Item g_items[NUM_ITEMS];

bool PointInAABB(Vector2 p, AABB aabb)
{
    return p.x >= aabb.c.x - aabb.e.x && p.x <= aabb.c.x + aabb.e.x && p.y >= aabb.c.y - aabb.e.y && p.y <= aabb.c.y + aabb.e.y;
}

inline float Sign(float n)
{
    if (n < 0.0f)
    {
        return -1.0f;
    }
    if (n > 0.0f)
    {
        return 1.0f;
    }
    return 0.0f;
}

// See: https://noonat.github.io/intersect/#intersection-tests
bool SegmentInAABB(Segment s, AABB aabb)
{
    const Vector2 delta = Vector2Subtract(s.end, s.start);
    const float scaleX = delta.x == 0.0f ? 0.0f : (1.0f / delta.x);
    const float scaleY = delta.y == 0.0f ? 0.0f : (1.0f / delta.y);
    const float signX = Sign(scaleX);
    const float signY = Sign(scaleY);
    const float nearTimeX = (aabb.c.x - signX * aabb.e.x - s.start.x) * scaleX;
    const float nearTimeY = (aabb.c.y - signY * aabb.e.y - s.start.y) * scaleY;
    const float farTimeX = (aabb.c.x + signX * aabb.e.x - s.start.x) * scaleX;
    const float farTimeY = (aabb.c.y + signY * aabb.e.y - s.start.y) * scaleY;
    if (nearTimeX > farTimeY || nearTimeY > farTimeX)
    {
        return false;
    }

    // If we don't do this, then all we achieve is determining if the two things will ever collide, which is also useful, but not
    // what we want.
    const float nearTime = nearTimeX > nearTimeY ? nearTimeX : nearTimeY;
    const float farTime = farTimeX < farTimeY ? farTimeX : farTimeY;
    if (nearTime >= 1.0f || farTime <= 0.0f)
    {
        return false;
    }

    return true;
}

/**
 * Called by the main loop once per fixed timestep update interval.
 */
void FixedUpdate(void)
{
    for (int i = 0; i < NUM_ITEMS; i++)
    {
        Item* a = &g_items[i];
        const Vector2 targetPos = Vector2Add(a->position, a->velocity);
        for (int j = 0; j < NUM_ITEMS; j++)
        {
            if (j == i)
            {
                continue;
            }

            // Effectively we're sweeping A into B.
            Item* b = &g_items[j];

            // Create an AABB at B's position, but with the combined size of A and B.
            const AABB aabb = {.c = Vector2Add(b->position, b->aabb.c), .e = Vector2Add(b->aabb.e, a->aabb.e)};

            // Create a line segment from A's position to its position plus the two items' relative velocities.
            const Segment s = {.start = a->position,
                               .end = Vector2Subtract(a->position, Vector2Subtract(b->velocity, a->velocity))};

            // If the line segment intersects the AABB then A has hit B.
            if (SegmentInAABB(s, aabb))
            {
                a->hit = true;
                b->hit = true;
            }
        }
        a->position = targetPos;
    }
}

/**
 * Called by the main loop whenever drawing is required.
 * @param alpha a value from 0.0 to 1.0 indicating how much into the next frame we are. Useful for interpolation.
 */
void Draw(double alpha)
{
    (void)alpha;
    ClearBackground(BLACK);
    BeginDrawing();
    for (int i = 0; i < NUM_ITEMS; i++)
    {
        Item* item = &g_items[i];
        DrawRectangle((int)(item->position.x - item->size.x), (int)(item->position.y - item->size.y), (int)(2 * item->size.x),
                      (int)(2 * item->size.y), item->color);
        if (item->hit)
        {
            DrawRectangleLines((int)(item->position.x - item->size.x), (int)(item->position.y - item->size.y),
                               (int)(2 * item->size.x), (int)(2 * item->size.y), WHITE);
        }
    }

    DrawFPS(4, SCREEN_HEIGHT - 20);
    EndDrawing();

    // Now that we've drawn everything, clear its hit flag.
    for (int i = 0; i < NUM_ITEMS; i++)
    {
        g_items[i].hit = false;
    }
}

/**
 * Called by the main loop when it's time to check edge-triggered events.
 */
void CheckTriggers(void)
{
}

void InitItems(void)
{
    const Vector2 centre = {.x = SCREEN_WIDTH / 2.0f, .y = SCREEN_HEIGHT / 2.0f};
    for (int i = 0; i < NUM_ITEMS; i++)
    {
        if ((i % 8) != 0)
        {
            const Vector2 position = {.x = (float)GetRandomValue(0, SCREEN_WIDTH), .y = (float)GetRandomValue(0, SCREEN_HEIGHT)};
            const Vector2 extents = {.x = (float)GetRandomValue(4, 40), .y = (float)GetRandomValue(4, 40)};
            const float speed = 0.1f * (float)(1 + i);
            const Vector2 velocity = Vector2Scale(Vector2Subtract(Vector2MoveTowards(position, centre, 1.0f), position), speed);

            const Item item = {.position = position,
                               .size = extents,
                               .velocity = velocity,
                               .color = DARKGREEN,
                               .aabb = {.e = extents}};

            g_items[i] = item;
        }
        else
        {
            const Vector2 position = {.x = 0.0f, .y = (float)GetRandomValue(0, SCREEN_HEIGHT)};
            const Vector2 extents = {.x = 2.0f, .y = 2.0f};
            const Vector2 velocity = {.x = 16.0f, .y = 0.0f};
            const Item item = {.position = position,
                               .size = extents,
                               .velocity = velocity,
                               .color = DARKGREEN,
                               .aabb = {.e = extents}};
            g_items[i] = item;
        }
    }
}

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Collisions Playground");
    SetTargetFPS(TARGET_FPS);
    SetTraceLogLevel(LOG_DEBUG);

    InitItems();
    RunMainLoop();

    CloseWindow();

    return 0;
}
