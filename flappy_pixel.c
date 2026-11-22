/*
 * Flappy Bird - Pixel Art + Themes (raylib)
 *
 * Controls:
 *   Start screen : LEFT/RIGHT, A/D, keys 1-4, or click a bird to choose a theme
 *   SPACE / Click: flap, start, restart
 *   ESC          : quit
 *
 * Build (Windows, MSYS2 UCRT64):
 *   gcc flappy_pixel.c -o flappy_pixel.exe -lraylib -lopengl32 -lgdi32 -lwinmm
 */
#include "raylib.h"
#include <math.h>
#include <stdio.h>

/* ---------- Settings ---------- */
#define SCREEN_W     480
#define SCREEN_H     640
#define GROUND_H     80
#define GROUND_TOP   (SCREEN_H - GROUND_H)
#define PX           4        // size of one "pixel block" for pipes/background
#define NUM_PIPES    3
#define PIPE_W       72       // multiple of PX
#define CAP_H        28       // pipe cap height (multiple of PX)
#define CAP_EXTRA    8        // how much wider the cap is on each side
#define PIPE_SPACING 220
#define GAP_SIZE     170
#define BIRD_X       120
#define BIRD_R       13       // collision radius
#define BIRD_SCALE   3        // size of one bird pixel
#define GRAVITY      1400.0f
#define FLAP_SPEED   -400.0f
#define PIPE_SPEED   160.0f
#define NUM_THEMES   4
#define PREVIEW_Y    380

typedef enum { STATE_START, STATE_PLAY, STATE_OVER } GameState;

/* ---------- Themes: every color of the game lives here ---------- */
typedef struct {
    const char *name;
    Color birdBody, birdWing, birdOutline;
    Color skyTop, skyBottom, hill, cloud, sun;
    Color pipeLight, pipeMid, pipeDark, pipeOutline;
    Color groundTop, ground, groundStripe;
    int night;     // 1 = draw stars (sun becomes the moon)
    int sunY;      // height of sun/moon in the sky
} Theme;

Theme themes[NUM_THEMES] = {
    { "Sunny Day",
      {255,215, 60,255}, {255,150, 30,255}, {110, 60,  0,255},          // bird
      { 70,165,235,255}, {190,232,250,255}, {110,200,100,255},
      {255,255,255,200}, {255,235,120,255},                              // sky, hills, cloud, sun
      {150,235,110,255}, { 80,190, 60,255}, { 45,130, 40,255}, { 25, 70, 25,255},  // pipes
      {100,200, 70,255}, {222,200,140,255}, {190,165,110,255},           // ground
      0, 110 },

    { "Sunset",
      {235, 70, 60,255}, {150, 30, 40,255}, { 70, 10, 15,255},
      { 85, 45,115,255}, {255,160, 95,255}, {120, 45, 85,255},
      {255,190,150,170}, {255,215,110,255},
      {245,170,110,255}, {205,105, 55,255}, {120, 55, 35,255}, { 65, 28, 18,255},
      {190, 80, 60,255}, {150, 90, 75,255}, {120, 65, 55,255},
      0, 320 },

    { "Night",
      { 80,150,240,255}, { 40, 80,180,255}, { 15, 30, 90,255},
      { 10, 14, 48,255}, { 55, 75,135,255}, { 25, 38, 80,255},
      {120,140,200, 90}, {240,240,200,255},
      {140,170,235,255}, { 75,105,185,255}, { 40, 60,125,255}, { 15, 20, 55,255},
      { 60, 80,140,255}, { 40, 50, 95,255}, { 30, 38, 75,255},
      1, 95 },

    { "Forest",
      {100,210, 90,255}, { 40,140, 60,255}, { 15, 70, 25,255},
      { 40,120,100,255}, {175,232,185,255}, { 35,115, 70,255},
      {230,255,235,150}, {255,250,200,255},
      {205,155,105,255}, {155,105, 62,255}, {105, 65, 38,255}, { 55, 32, 16,255},
      { 70,150, 60,255}, {115, 85, 55,255}, { 90, 65, 40,255},
      0, 130 },
};

/* ---------- Bird sprite: 14 x 10 pixels ----------
 * K outline | B body | L wing | W eye white | P pupil
 * O beak    | R beak shadow | . transparent               */
#define BIRD_COLS 14
#define BIRD_ROWS 10
const char *BIRD_SPRITE[BIRD_ROWS] = {
    "....KKKKKK....",
    "..KKBBBBBBKK..",
    ".KBBBBBBBWWWK.",
    "KBBBBBBBWWPWK.",
    "KBBBBBBBBBBKOO",
    "KLLLLBBBBBBOOO",
    "KLLLLLBBBBKRRR",
    ".KBLLBBBBBBK..",
    "..KKBBBBBBKK..",
    "....KKKKKK....",
};

/* ---------- Game state ---------- */
GameState state;
float birdY, velocity;
float pipeX[NUM_PIPES];
int   gapY[NUM_PIPES];
int   passed[NUM_PIPES];
int   score, highScore;
int   themeIndex = 0;
float groundScroll, cloudScroll;

/* ---------- Helper functions ---------- */
int randomGapY(void) {
    return GetRandomValue(80, GROUND_TOP - GAP_SIZE - 80);
}

int flapPressed(void) {
    return IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

Color lerpColor(Color a, Color b, float t) {
    Color c;
    c.r = (unsigned char)(a.r + (b.r - a.r) * t);
    c.g = (unsigned char)(a.g + (b.g - a.g) * t);
    c.b = (unsigned char)(a.b + (b.b - a.b) * t);
    c.a = 255;
    return c;
}

void drawCenteredText(const char *text, int y, int size, Color color) {
    int w = MeasureText(text, size);
    DrawText(text, (SCREEN_W - w) / 2 + 2, y + 2, size, BLACK);   // shadow
    DrawText(text, (SCREEN_W - w) / 2, y, size, color);
}

Rectangle previewBox(int i) {
    float cx = 90.0f + i * 100.0f;
    return (Rectangle){ cx - 34, PREVIEW_Y - 26, 68, 52 };
}

/* Returns 1 if the player used the theme selector this frame */
int handleThemeSelect(void) {
    if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) { themeIndex = (themeIndex + NUM_THEMES - 1) % NUM_THEMES; return 1; }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) { themeIndex = (themeIndex + 1) % NUM_THEMES; return 1; }
    if (IsKeyPressed(KEY_ONE))   { themeIndex = 0; return 1; }
    if (IsKeyPressed(KEY_TWO))   { themeIndex = 1; return 1; }
    if (IsKeyPressed(KEY_THREE)) { themeIndex = 2; return 1; }
    if (IsKeyPressed(KEY_FOUR))  { themeIndex = 3; return 1; }
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 m = GetMousePosition();
        for (int i = 0; i < NUM_THEMES; i++)
            if (CheckCollisionPointRec(m, previewBox(i))) { themeIndex = i; return 1; }
    }
    return 0;
}

/* ---------- Game logic ---------- */
void resetGame(void) {
    birdY = SCREEN_H / 2.0f;
    velocity = 0;
    score = 0;
    for (int i = 0; i < NUM_PIPES; i++) {
        pipeX[i] = SCREEN_W + i * PIPE_SPACING;
        gapY[i] = randomGapY();
        passed[i] = 0;
    }
}

int checkCollision(void) {
    Vector2 center = { BIRD_X, birdY };

    if (birdY - BIRD_R < 0 || birdY + BIRD_R >= GROUND_TOP)
        return 1;

    for (int i = 0; i < NUM_PIPES; i++) {
        int bottomY = gapY[i] + GAP_SIZE;
        Rectangle parts[4] = {
            { pipeX[i], 0, PIPE_W, (float)gapY[i] },                                   // top body
            { pipeX[i], (float)bottomY, PIPE_W, (float)(GROUND_TOP - bottomY) },       // bottom body
            { pipeX[i] - CAP_EXTRA, (float)(gapY[i] - CAP_H), PIPE_W + 2 * CAP_EXTRA, CAP_H }, // top cap
            { pipeX[i] - CAP_EXTRA, (float)bottomY, PIPE_W + 2 * CAP_EXTRA, CAP_H },           // bottom cap
        };
        for (int p = 0; p < 4; p++)
            if (CheckCollisionCircleRec(center, BIRD_R, parts[p]))
                return 1;
    }
    return 0;
}

void updatePlay(float dt) {
    if (flapPressed())
        velocity = FLAP_SPEED;

    velocity += GRAVITY * dt;
    birdY += velocity * dt;

    for (int i = 0; i < NUM_PIPES; i++) {
        pipeX[i] -= PIPE_SPEED * dt;

        if (!passed[i] && pipeX[i] + PIPE_W < BIRD_X) {
            passed[i] = 1;
            score++;
        }
        if (pipeX[i] + PIPE_W + CAP_EXTRA < 0) {
            pipeX[i] += NUM_PIPES * PIPE_SPACING;
            gapY[i] = randomGapY();
            passed[i] = 0;
        }
    }

    if (checkCollision()) {
        state = STATE_OVER;
        if (score > highScore) highScore = score;
    }
}

void updateGame(float dt) {
    cloudScroll += 20.0f * dt;

    if (state == STATE_START) {
        groundScroll += PIPE_SPEED * dt;
        birdY = SCREEN_H / 2.0f + sinf((float)GetTime() * 5.0f) * 8.0f;
        if (!handleThemeSelect() && flapPressed()) {
            state = STATE_PLAY;
            velocity = FLAP_SPEED;
        }
    }
    else if (state == STATE_PLAY) {
        groundScroll += PIPE_SPEED * dt;
        updatePlay(dt);
    }
    else {  // STATE_OVER
        if (birdY + BIRD_R < GROUND_TOP) {
            velocity += GRAVITY * dt;
            birdY += velocity * dt;
        }
        if (flapPressed()) {
            resetGame();
            state = STATE_START;
        }
    }
}

/* ---------- Pixel-art drawing ---------- */
void drawPixelCircle(int cx, int cy, int r, Color c) {
    for (int dy = -r; dy <= r; dy += PX) {
        int dx = (int)sqrtf((float)(r * r - dy * dy));
        dx = (dx / PX) * PX;
        DrawRectangle(cx - dx, cy + dy, 2 * dx, PX, c);
    }
}

void drawPixelCloud(int x, int y, Color c) {
    DrawRectangle(x + 4 * PX, y - PX, 3 * PX, PX, c);
    DrawRectangle(x + 2 * PX, y,      6 * PX, PX, c);
    DrawRectangle(x + 1 * PX, y + PX, 8 * PX, PX, c);
    DrawRectangle(x,          y + 2 * PX, 10 * PX, 2 * PX, c);
}

void drawSky(const Theme *t) {
    // stepped (banded) gradient looks more "pixel" than a smooth one
    int bands = 14;
    int bandH = (GROUND_TOP + bands - 1) / bands;
    for (int i = 0; i < bands; i++) {
        Color c = lerpColor(t->skyTop, t->skyBottom, i / (float)(bands - 1));
        DrawRectangle(0, i * bandH, SCREEN_W, bandH, c);
    }

    if (t->night) {                                   // twinkling stars
        for (int i = 0; i < 28; i++) {
            int x = ((i * 97 + 31) % SCREEN_W) / PX * PX;
            int y = ((i * 53 + 17) % 300) / PX * PX;
            if (((int)(GetTime() * 2.0) + i) % 4 != 0)
                DrawRectangle(x, y, PX, PX, WHITE);
        }
    }

    drawPixelCircle(370, t->sunY, 28, t->sun);        // sun or moon

    for (int i = 0; i < 4; i++) {                     // drifting clouds
        float x = fmodf(i * 170.0f - cloudScroll, SCREEN_W + 120);
        if (x < -100) x += SCREEN_W + 120;
        int y = 70 + (i % 2) * 60 + i * 20;
        drawPixelCloud(((int)x / PX) * PX, y, t->cloud);
    }
}

void drawHills(const Theme *t) {
    Color far = lerpColor(t->hill, t->skyBottom, 0.5f);
    for (int x = 0; x < SCREEN_W; x += 8) {           // far hills (slow)
        float sx = x + groundScroll * 0.15f;
        int h = 70 + (int)(sinf(sx * 0.011f) * 28 + sinf(sx * 0.027f) * 12);
        h = (h / 8) * 8;
        DrawRectangle(x, GROUND_TOP - h, 8, h, far);
    }
    for (int x = 0; x < SCREEN_W; x += 8) {           // near hills (faster)
        float sx = x + groundScroll * 0.4f;
        int h = 32 + (int)(sinf(sx * 0.016f) * 14 + sinf(sx * 0.04f) * 6);
        h = (h / 8) * 8;
        DrawRectangle(x, GROUND_TOP - h, 8, h, t->hill);
    }
}

// One chunky pipe section: outline, highlight on the left, shadow on the right
void drawPipePart(int x, int y, int w, int h, const Theme *t) {
    DrawRectangle(x, y, w, h, t->pipeOutline);
    DrawRectangle(x + PX, y + PX, w - 2 * PX, h - 2 * PX, t->pipeMid);
    DrawRectangle(x + PX, y + PX, 2 * PX, h - 2 * PX, t->pipeLight);
    DrawRectangle(x + w - 4 * PX, y + PX, 3 * PX, h - 2 * PX, t->pipeDark);
}

void drawPipes(const Theme *t) {
    for (int i = 0; i < NUM_PIPES; i++) {
        int x = ((int)floorf(pipeX[i] / PX)) * PX;    // snap to the pixel grid
        int bottomY = gapY[i] + GAP_SIZE;

        // top pipe: body then cap
        drawPipePart(x, -PX, PIPE_W, gapY[i] - CAP_H + PX, t);
        drawPipePart(x - CAP_EXTRA, gapY[i] - CAP_H, PIPE_W + 2 * CAP_EXTRA, CAP_H, t);

        // bottom pipe: cap then body
        drawPipePart(x - CAP_EXTRA, bottomY, PIPE_W + 2 * CAP_EXTRA, CAP_H, t);
        drawPipePart(x, bottomY + CAP_H, PIPE_W, GROUND_TOP - bottomY - CAP_H + PX, t);
    }
}

void drawGround(const Theme *t) {
    DrawRectangle(0, GROUND_TOP, SCREEN_W, GROUND_H, t->ground);
    DrawRectangle(0, GROUND_TOP, SCREEN_W, 16, t->groundTop);
    DrawRectangle(0, GROUND_TOP + 16, SCREEN_W, PX, t->groundStripe);
    int off = (((int)groundScroll) % 32 / PX) * PX;   // scrolling dirt blocks
    for (int x = -32; x < SCREEN_W + 32; x += 32) {
        DrawRectangle(x - off,      GROUND_TOP + 28, 16, 8, t->groundStripe);
        DrawRectangle(x - off + 16, GROUND_TOP + 48, 16, 8, t->groundStripe);
    }
}

// Draw the sprite centered on (cx, cy). Used for the game bird and menu previews.
void drawBirdSprite(int cx, int cy, int scale, const Theme *t) {
    int x0 = cx - (BIRD_COLS * scale) / 2;
    int y0 = cy - (BIRD_ROWS * scale) / 2;
    for (int r = 0; r < BIRD_ROWS; r++) {
        for (int c = 0; c < BIRD_COLS; c++) {
            Color col;
            switch (BIRD_SPRITE[r][c]) {
                case 'K': col = t->birdOutline;           break;
                case 'B': col = t->birdBody;              break;
                case 'L': col = t->birdWing;              break;
                case 'W': col = WHITE;                    break;
                case 'P': col = BLACK;                    break;
                case 'O': col = (Color){255,140, 40,255}; break;
                case 'R': col = (Color){205, 85, 20,255}; break;
                default:  continue;                       // '.' = transparent
            }
            DrawRectangle(x0 + c * scale, y0 + r * scale, scale, scale, col);
        }
    }
}

void drawUI(const Theme *t) {
    char buf[32];

    if (state == STATE_START) {
        drawCenteredText("FLAPPY BIRD", 110, 48, WHITE);
        drawCenteredText(t->name, 305, 26, WHITE);

        for (int i = 0; i < NUM_THEMES; i++) {         // bird color choices
            Rectangle b = previewBox(i);
            if (i == themeIndex) {
                DrawRectangleRec(b, Fade(BLACK, 0.4f));
                DrawRectangleLinesEx(b, 3, WHITE);
            } else {
                DrawRectangleRec(b, Fade(BLACK, 0.15f));
            }
            drawBirdSprite((int)(b.x + b.width / 2), PREVIEW_Y, BIRD_SCALE, &themes[i]);
        }

        drawCenteredText("LEFT / RIGHT or 1-4 to pick a bird", 440, 16, WHITE);
        drawCenteredText("SPACE or click to play", 480, 22, WHITE);
    }
    else {
        snprintf(buf, sizeof(buf), "%d", score);
        int w = MeasureText(buf, 60);
        DrawText(buf, (SCREEN_W - w) / 2 + 3, 43, 60, BLACK);
        DrawText(buf, (SCREEN_W - w) / 2, 40, 60, WHITE);
    }

    if (state == STATE_OVER) {
        DrawRectangle(90, 200, 300, 200, Fade(BLACK, 0.65f));
        DrawRectangleLinesEx((Rectangle){ 90, 200, 300, 200 }, 3, WHITE);
        drawCenteredText("GAME OVER", 220, 36, RED);
        snprintf(buf, sizeof(buf), "Score: %d", score);
        drawCenteredText(buf, 285, 26, WHITE);
        snprintf(buf, sizeof(buf), "Best: %d", highScore);
        drawCenteredText(buf, 320, 26, GOLD);
        drawCenteredText("SPACE to restart", 365, 18, LIGHTGRAY);
    }
}

void drawGame(void) {
    const Theme *t = &themes[themeIndex];
    BeginDrawing();
    drawSky(t);
    drawHills(t);
    drawPipes(t);
    drawGround(t);
    drawBirdSprite(BIRD_X, (int)birdY, BIRD_SCALE, t);
    drawUI(t);
    EndDrawing();
}

/* ---------- Main ---------- */
int main(void) {
    InitWindow(SCREEN_W, SCREEN_H, "Flappy Bird - Pixel Edition");
    SetTargetFPS(60);

    highScore = 0;
    resetGame();
    state = STATE_START;

    while (!WindowShouldClose()) {
        updateGame(GetFrameTime());
        drawGame();
    }

    CloseWindow();
    return 0;
}