
/*
 * Basic Console Flappy Bird (Windows)
 * Controls: SPACE = flap, Q = quit
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>    // _kbhit(), _getch()
#include <windows.h>  // Sleep(), console cursor control

#define WIDTH        40
#define HEIGHT       20
#define NUM_PIPES    3
#define PIPE_SPACING 14
#define PIPE_WIDTH   2
#define GAP_SIZE     6
#define BIRD_X       8
#define FRAME_MS     120

/* ---------- Game state (global for simplicity) ---------- */
int birdY, velocity, score, gameOver;
int pipeX[NUM_PIPES];
int gapY[NUM_PIPES];   // row where the gap starts

/* ---------- Helper functions ---------- */

// Random starting row for a gap (keeps it away from the borders)
int randomGap(void) {
    return rand() % (HEIGHT - GAP_SIZE - 1) + 1;
}

// Move the console cursor to top-left (avoids flicker from clearing the screen)
void moveCursorToTop(void) {
    COORD pos = {0, 0};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

// Hide the blinking cursor
void hideCursor(void) {
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 1;
    info.bVisible = FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
}

// Is there a pipe block at screen position (x, y)?
int isPipeAt(int x, int y) {
    for (int i = 0; i < NUM_PIPES; i++) {
        if (x >= pipeX[i] && x < pipeX[i] + PIPE_WIDTH) {
            if (y < gapY[i] || y >= gapY[i] + GAP_SIZE)
                return 1;
        }
    }
    return 0;
}

/* ---------- Main game functions ---------- */

void setup(void) {
    birdY = HEIGHT / 2;
    velocity = 0;
    score = 0;
    gameOver = 0;
    for (int i = 0; i < NUM_PIPES; i++) {
        pipeX[i] = WIDTH + i * PIPE_SPACING;
        gapY[i] = randomGap();
    }
}

void draw(void) {
    moveCursorToTop();
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (y == 0 || y == HEIGHT - 1)
                putchar('=');
            else if (x == BIRD_X && y == birdY)
                putchar('O');
            else if (isPipeAt(x, y))
                putchar('#');
            else
                putchar(' ');
        }
        putchar('\n');
    }
    printf("Score: %d   (SPACE = flap, Q = quit)\n", score);
}

void input(void) {
    if (_kbhit()) {
        char key = _getch();
        if (key == ' ')
            velocity = -2;          // flap: go up
        else if (key == 'q' || key == 'Q')
            gameOver = 1;
    }
}

void checkCollision(void) {
    // Hit ceiling or floor
    if (birdY <= 0 || birdY >= HEIGHT - 1)
        gameOver = 1;

    // Hit a pipe
    if (isPipeAt(BIRD_X, birdY))
        gameOver = 1;
}

void update(void) {
    // Bird physics
    birdY += velocity;
    if (velocity < 1)
        velocity++;                 // gravity pulls down

    // Move pipes
    for (int i = 0; i < NUM_PIPES; i++) {
        pipeX[i]--;

        // Bird just passed this pipe -> score
        if (pipeX[i] + PIPE_WIDTH == BIRD_X)
            score++;

        // Pipe left the screen -> recycle it on the right
        if (pipeX[i] + PIPE_WIDTH <= 0) {
            pipeX[i] += NUM_PIPES * PIPE_SPACING;
            gapY[i] = randomGap();
        }
    }

    checkCollision();
}

int main(void) {
    char again;
    srand((unsigned)time(NULL));
    hideCursor();

    do {
        system("cls");
        setup();

        while (!gameOver) {
            draw();
            input();
            update();
            Sleep(FRAME_MS);
        }

        draw();
        printf("\n*** GAME OVER ***  Final score: %d\n", score);
        printf("Play again? (y/n): ");
        do {
            again = _getch();
        } while (again != 'y' && again != 'Y' && again != 'n' && again != 'N');

    } while (again == 'y' || again == 'Y');

    return 0;
}