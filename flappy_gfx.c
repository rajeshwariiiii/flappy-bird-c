/*
 * Flappy Bird - raylib graphics version
 * Controls: SPACE or Left Click = flap / start / restart
 *
 * Build (Windows, MSYS2 UCRT64):
 *   gcc flappy_gfx.c -o flappy_gfx.exe -lraylib -lopengl32 -lgdi32 -lwinmm
 */
#include "raylib.h"
#include <math.h>
#include <stdio.h>

/* ---------- Settings ---------- */
#define SCREEN_W     480
#define SCREEN_H     640
#define GROUND_H     80
#define NUM_PIPES    3
#define PIPE_W       70
#define PIPE_SPACING 220     // horizontal distance between pipes
#define GAP_SIZE     170     // vertical opening between pipes
#define BIRD_X       120
#define BIRD_R       16      // bird radius
#define GRAVITY      1400.0f // pixels per second^2
#define FLAP_SPEED   -400.0f // upward speed on flap
#define PIPE_SPEED   160.0f  // pixels per second

typedef enum { STATE_START, STATE_PLAY, STATE_OVER } GameState;

/* ---------- Game state ---------- */
GameState state;
float birdY, velocity;
float pipeX[NUM_PIPES];
int   gapY[NUM_PIPES];       // top y of each gap
int   passed[NUM_PIPES];     // has the bird scored on this pipe?
int   score, highScore;
float groundScroll, cloudScroll;

/* ---------- Helper functions ---------- */
int randomGapY(void) {
    return GetRandomValue(70, SCREEN_H - GROUND_H - GAP_SIZE - 70);
}

int flapPressed(void) {
    return IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void drawCenteredText(const char *text, int y, int size, Color color) {
    int w = MeasureText(text, size);
    DrawText(text, (SCREEN_W - w) / 2, y, size, color);
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

    // ceiling / ground
    if (birdY - BIRD_R < 0 || birdY + BIRD_R >= SCREEN_H - GROUND_H)
        return 1;

    // pipes (top and bottom rectangles)
    for (int i = 0; i < NUM_PIPES; i++) {
        Rectangle top = { pipeX[i], 0, PIPE_W, (float)gapY[i] };
        Rectangle bottom = { pipeX[i], (float)(gapY[i] + GAP_SIZE), PIPE_W,
                             (float)(SCREEN_H - GROUND_H - gapY[i] - GAP_SIZE) };
        if (CheckCollisionCircleRec(center, BIRD_R, top) ||
            CheckCollisionCircleRec(center, BIRD_R, bottom))
            return 1;
    }
    return 0;
}

void updatePlay(float dt) {
    if (flapPressed())
        velocity = FLAP_SPEED;

    // bird physics
    velocity += GRAVITY * dt;
    birdY += velocity * dt;

    // pipes
    for (int i = 0; i < NUM_PIPES; i++) {
        pipeX[i] -= PIPE_SPEED * dt;

        if (!passed[i] && pipeX[i] + PIPE_W < BIRD_X) {
            passed[i] = 1;
            score++;
        }
        if (pipeX[i] + PIPE_W < 0) {            // recycle on the right
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
        birdY = SCREEN_H / 2.0f + sinf((float)GetTime() * 5.0f) * 8.0f;  // bobbing
        if (flapPressed()) {
            state = STATE_PLAY;
            velocity = FLAP_SPEED;
        }
    }
    else if (state == STATE_PLAY) {
        groundScroll += PIPE_SPEED * dt;
        updatePlay(dt);
    }
    else {  // STATE_OVER: bird falls to the ground, wait for restart
        if (birdY + BIRD_R < SCREEN_H - GROUND_H) {
            velocity += GRAVITY * dt;
            birdY += velocity * dt;
        }
        if (flapPressed()) {
            resetGame();
            state = STATE_START;
        }
    }
}

/* ---------- Drawing ---------- */
void drawSky(void) {
    DrawRectangleGradientV(0, 0, SCREEN_W, SCREEN_H - GROUND_H,
                           (Color){ 90, 180, 240, 255 }, (Color){ 200, 235, 250, 255 });
    // a few simple clouds that drift left
    for (int i = 0; i < 4; i++) {
        float x = fmodf(i * 170.0f - cloudScroll, SCREEN_W + 120) ;
        if (x < -100) x += SCREEN_W + 120;
        float y = 70 + (i % 2) * 60 + i * 20;
        DrawCircle((int)x, (int)y, 22, Fade(WHITE, 0.8f));
        DrawCircle((int)x + 22, (int)y + 6, 18, Fade(WHITE, 0.8f));
        DrawCircle((int)x - 22, (int)y + 6, 18, Fade(WHITE, 0.8f));
    }
}

void drawPipes(void) {
    Color body = (Color){ 80, 190, 60, 255 };
    Color edge = (Color){ 40, 120, 30, 255 };
    int capH = 24, capExtra = 6;

    for (int i = 0; i < NUM_PIPES; i++) {
        int x = (int)pipeX[i];
        int bottomY = gapY[i] + GAP_SIZE;
        int bottomH = SCREEN_H - GROUND_H - bottomY;

        // top pipe
        DrawRectangle(x, 0, PIPE_W, gapY[i], body);
        DrawRectangleLines(x, 0, PIPE_W, gapY[i], edge);
        DrawRectangle(x - capExtra, gapY[i] - capH, PIPE_W + 2 * capExtra, capH, body);
        DrawRectangleLines(x - capExtra, gapY[i] - capH, PIPE_W + 2 * capExtra, capH, edge);

        // bottom pipe
        DrawRectangle(x, bottomY, PIPE_W, bottomH, body);
        DrawRectangleLines(x, bottomY, PIPE_W, bottomH, edge);
        DrawRectangle(x - capExtra, bottomY, PIPE_W + 2 * capExtra, capH, body);
        DrawRectangleLines(x - capExtra, bottomY, PIPE_W + 2 * capExtra, capH, edge);
    }
}

void drawGround(void) {
    int top = SCREEN_H - GROUND_H;
    DrawRectangle(0, top, SCREEN_W, GROUND_H, (Color){ 222, 200, 140, 255 });
    DrawRectangle(0, top, SCREEN_W, 14, (Color){ 100, 200, 70, 255 });
    // scrolling stripes give a feeling of movement
    int offset = (int)fmodf(groundScroll, 30.0f);
    for (int x = -30; x < SCREEN_W + 30; x += 30)
        DrawRectangle(x - offset, top + 14, 15, 8, (Color){ 190, 165, 110, 255 });
}

void drawBird(void) {
    int x = BIRD_X, y = (int)birdY;
    DrawCircle(x, y, BIRD_R, (Color){ 255, 220, 50, 255 });          // body
    DrawCircleLines(x, y, BIRD_R, (Color){ 200, 150, 0, 255 });
    DrawEllipse(x - 5, y + 4, 9, 6, (Color){ 255, 170, 30, 255 });   // wing
    DrawCircle(x + 7, y - 5, 5, WHITE);                              // eye
    DrawCircle(x + 9, y - 5, 2, BLACK);
    DrawTriangle((Vector2){ x + BIRD_R - 2, y - 3 },                 // beak
                 (Vector2){ x + BIRD_R - 2, y + 5 },
                 (Vector2){ x + BIRD_R + 10, y + 1 }, (Color){ 255, 110, 30, 255 });
}

void drawUI(void) {
    char buf[32];

    if (state == STATE_START) {
        drawCenteredText("FLAPPY BIRD", 130, 48, WHITE);
        drawCenteredText("Press SPACE or click to flap", 400, 22, DARKBLUE);
    }
    else {
        snprintf(buf, sizeof(buf), "%d", score);
        int w = MeasureText(buf, 60);
        DrawText(buf, (SCREEN_W - w) / 2 + 2, 42, 60, BLACK);       // shadow
        DrawText(buf, (SCREEN_W - w) / 2, 40, 60, WHITE);
    }

    if (state == STATE_OVER) {
        DrawRectangle(90, 200, 300, 200, Fade(BLACK, 0.65f));
        DrawRectangleLines(90, 200, 300, 200, WHITE);
        drawCenteredText("GAME OVER", 220, 36, RED);
        snprintf(buf, sizeof(buf), "Score: %d", score);
        drawCenteredText(buf, 285, 26, WHITE);
        snprintf(buf, sizeof(buf), "Best: %d", highScore);
        drawCenteredText(buf, 320, 26, GOLD);
        drawCenteredText("SPACE to restart", 365, 18, LIGHTGRAY);
    }
}

void drawGame(void) {
    BeginDrawing();
    drawSky();
    drawPipes();
    drawGround();
    drawBird();
    drawUI();
    EndDrawing();
}

/* ---------- Main ---------- */
int main(void) {
    InitWindow(SCREEN_W, SCREEN_H, "Flappy Bird");
    SetTargetFPS(60);

    highScore = 0;
    resetGame();
    state = STATE_START;

    while (!WindowShouldClose()) {      // closes on window X or ESC
        updateGame(GetFrameTime());
        drawGame();
    }

    CloseWindow();
    return 0;
}