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
#define MAX_ITEMS (NUM_ITEMS * 4)

typedef struct
{
    Vector2 centre;  // Centre.
    Vector2 extents; // Extents (half-widths).
} AABB;

typedef struct
{
    Vector2 origin;
    Vector2 direction; // The direction is not necessarily normalized.
} Ray2D;

typedef struct
{
    Vector2 position; // Position.
    Vector2 size;     // Drawing size.
    Vector2 velocity; // Velocity.
    Color color;      // Colour.
    AABB aabb;        // AABB.
    bool hit;         // Is it in collision?
} Item;

static int g_numItems = 0;
static Item g_items[MAX_ITEMS];

static inline float Max(float a, float b)
{
    return a > b ? a : b;
}

static inline float Min(float a, float b)
{
    return a < b ? a : b;
}

inline Vector2 Vector2Rcp(Vector2 a)
{
    return (Vector2){.x = 1.0f / a.x, .y = 1.0f / a.y};
}

inline Vector2 Vector2Min(Vector2 a, Vector2 b)
{
    return (Vector2){Min(a.x, b.x), Min(a.y, b.y)};
}

inline Vector2 Vector2Max(Vector2 a, Vector2 b)
{
    return (Vector2){Max(a.x, b.x), Max(a.y, b.y)};
}

inline float Vector2MinComponent(Vector2 a)
{
    return Min(a.x, a.y);
}

inline float Vector2MaxComponent(Vector2 a)
{
    return Max(a.x, a.y);
}

// See: https://medium.com/@bromanz/another-view-on-the-classic-ray-aabb-intersection-algorithm-for-bvh-traversal-41125138b525
// https://gist.githubusercontent.com/bromanz/ed0de6725f5e40a0afd8f50985c2f7ad/raw/be5e79e16181e4617d1a0e6e540dd25c259c76a4/efficient-slab-test-majercik-et-al
inline bool Slabs(Vector2 p0, Vector2 p1, Vector2 rayOrigin, Vector2 invRayDir)
{
    const Vector2 t0 = Vector2Multiply(Vector2Subtract(p0, rayOrigin), invRayDir);
    const Vector2 t1 = Vector2Multiply(Vector2Subtract(p1, rayOrigin), invRayDir);
    const Vector2 tmin = Vector2Min(t0, t1);
    const Vector2 tmax = Vector2Max(t0, t1);
    return Max(0.0f, Vector2MaxComponent(tmin)) <= Min(1.0f, Vector2MinComponent(tmax));
}

bool CheckCollisionRay2dAABBs(Ray2D r, AABB aabb)
{
    const Vector2 invD = Vector2Rcp(r.direction);
    const Vector2 aabbMin = Vector2Subtract(aabb.centre, aabb.extents);
    const Vector2 aabbMax = Vector2Add(aabb.centre, aabb.extents);
    return Slabs(aabbMin, aabbMax, r.origin, invD);
}

bool CheckCollisionMovingAABBs(AABB a, AABB b, Vector2 va, Vector2 vb)
{
    // An AABB at B's position with the combined size of A and B.
    const AABB aabb = {.centre = b.centre, .extents = Vector2Add(a.extents, b.extents)};

    // A ray at A's position with its direction set to B's velocity relative to A. It's a parametric representation of a
    // line representing A's position at time t, where 0 <= t <= 1.
    const Ray2D r = {.origin = a.centre, .direction = Vector2Subtract(va, vb)};

    // Does the ray hit the AABB
    return CheckCollisionRay2dAABBs(r, aabb);
}

inline bool ItemsCollide(Item* a, Item* b)
{
    return CheckCollisionMovingAABBs((AABB){.centre = Vector2Add(a->position, a->aabb.extents), .extents = a->aabb.extents},
                                     (AABB){.centre = Vector2Add(b->position, b->aabb.extents), .extents = b->aabb.extents},
                                     a->velocity, b->velocity);
}

/**
 * Called by the main loop once per fixed timestep update interval.
 */
void FixedUpdate(void)
{
    for (int i = 0; i < g_numItems; i++)
    {
        Item* a = &g_items[i];
        const Vector2 targetPos = Vector2Add(a->position, a->velocity);
        for (int j = i + 1; j < g_numItems; j++)
        {
            Item* b = &g_items[j];
            if (ItemsCollide(a, b))
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
    for (int i = 0; i < g_numItems; i++)
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
    for (int i = 0; i < g_numItems; i++)
    {
        g_items[i].hit = false;
    }
}

/**
 * Creates a shot. Shots are very fast moving objects.
 * @param start
 * @param target
 */
void AddShot(Vector2 start, Vector2 target)
{
    if (g_numItems >= MAX_ITEMS)
    {
        return;
    }
    const Vector2 extents = {.x = 2.0f, .y = 2.0f};
    const Vector2 velocity = Vector2Subtract(Vector2MoveTowards(start, target, 64.0f), start);
    const Item item = {.position = start, .size = extents, .velocity = velocity, .color = RED, .aabb = {.extents = extents}};
    g_items[g_numItems] = item;
    g_numItems++;
}

/**
 * Called by the main loop when it's time to check edge-triggered events.
 */
void CheckTriggers(void)
{
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        const Vector2 target = GetMousePosition();
        AddShot((Vector2){0.0f, 0.0f}, target);
        AddShot((Vector2){SCREEN_WIDTH - 1.0f, 0.0f}, target);
        AddShot((Vector2){SCREEN_WIDTH - 1.0f, SCREEN_HEIGHT - 1.0f}, target);
        AddShot((Vector2){0.0f, SCREEN_HEIGHT - 1.0f}, target);
    }
}

void InitItems(void)
{
    g_numItems = 0;
    const Vector2 centre = {.x = SCREEN_WIDTH / 2.0f, .y = SCREEN_HEIGHT / 2.0f};
    for (int i = 0; i < NUM_ITEMS; i++)
    {
        const Vector2 position = {.x = (float)GetRandomValue(0, SCREEN_WIDTH), .y = (float)GetRandomValue(0, SCREEN_HEIGHT)};
        const Vector2 extents = {.x = (float)GetRandomValue(4, 40), .y = (float)GetRandomValue(4, 40)};
        const float speed = 0.1f * (float)(1 + i);
        const Vector2 velocity = Vector2Scale(Vector2Subtract(Vector2MoveTowards(position, centre, 1.0f), position), speed);

        const Item item = {.position = position,
                           .size = extents,
                           .velocity = velocity,
                           .color = DARKGREEN,
                           .aabb = {.extents = extents}};

        g_items[i] = item;
        ++g_numItems;
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
