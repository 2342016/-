#include "GL/glut.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#define MAX_OBSTACLES 1000000
#define GAME_DURATION 360000000000000
#define OBSTACLE_INTERVAL 90

typedef struct {
    float x, y, z;
    int active;
} Obstacle;

float playerX = 0.0f;
float playerY = 1.0f;

float cameraZ = 0.0f;
int isJumping = 0; // ジャンプ中かどうか
float targetY = 2.5f; // ジャンプの最高点
float hoverTime = 90.0f; // 滞空時間（フレーム単位）
float hoverCounter = 0.0f; // 滞空時間をカウントする
int jumpPhase = 0; // ジャンプのフェーズ (0: 上昇, 1: 対空, 2: 着地)

Obstacle obstacles[MAX_OBSTACLES];
int score = 0;
int frameCount = 0;

void drawText(float x, float y, const char* str) {
    glRasterPos2f(x, y);
    while (*str) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *str++);
    }
}

void drawPlayer(void) {
    glPushMatrix();
    glTranslatef(playerX, playerY, cameraZ + 2.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glutSolidCube(0.5f);
    glPopMatrix();
}

void drawObstacles(void) {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active) {
            glPushMatrix();
            glTranslatef(obstacles[i].x, obstacles[i].y, obstacles[i].z);
            glColor3f(1.0f, 0.0f, 0.0f);
            glutSolidCube(1.0f);
            glPopMatrix();
        }
    }
}

void drawGround(void) {
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINES);
    for (int i = -20; i <= 20; i++) {
        glVertex3f((float)i, 0.0f, cameraZ - 10.0f);
        glVertex3f((float)i, 0.0f, cameraZ + 50.0f);
        glVertex3f(-20.0f, 0.0f, cameraZ + (float)i);
        glVertex3f(20.0f, 0.0f, cameraZ + (float)i);
    }
    glEnd();
}

void drawScore(void) {
    char buffer[64];
    sprintf(buffer, "Score: %03d", score);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, 1.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(0.8f, 0.95f, buffer);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    gluLookAt(0.0f, 4.0f, cameraZ - 6.0f,
              0.0f, 1.0f, cameraZ + 5.0f,
              0.0f, 1.0f, 0.0f);

    drawGround();
    drawPlayer();
    drawObstacles();
    drawScore();

    glutSwapBuffers();
}

void updateObstacles(void) {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active && obstacles[i].z < cameraZ - 2.0f) {
            obstacles[i].active = 0;
        }
    }

    if (frameCount % OBSTACLE_INTERVAL == 0) {
        int created = 0;
        for (int i = 0; i < MAX_OBSTACLES && !created; i++) {
            if (!obstacles[i].active) {
                float lane[] = {-2.0f, -1.0f, 0.0f, 1.0f, 2.0f};
                int laneIndex = rand() % 5;

                obstacles[i].x = lane[laneIndex];
                obstacles[i].y = 1.0f;
                obstacles[i].z = cameraZ + 20.0f + (rand() % 10);
                obstacles[i].active = 1;
                created = 1;
            }
        }
    }
}

void checkCollisionAndScore(void) {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active &&
            fabs(obstacles[i].z - (cameraZ + 2.0f)) < 0.5f) {
            if (fabs(obstacles[i].x - playerX) > 0.6f || playerY > 1.2f) {
                score++;
                obstacles[i].active = 0;
            } else {
                printf("Game Over! You hit an obstacle. Score: %d\n", score);
                exit(0);
            }
        }
    }
}

void idle(void) {
    frameCount++;
    cameraZ += 0.03f;

    // ジャンプの挙動
    if (isJumping) {
        switch (jumpPhase) {
            case 0: // 上昇フェーズ
                playerY += 0.05f;
                if (playerY >= targetY) {
                    playerY = targetY;
                    jumpPhase = 1; // 次のフェーズ: 対空
                    hoverCounter = 0.0f;
                }
                break;
            case 1: // 対空フェーズ
                hoverCounter++;
                if (hoverCounter >= hoverTime) {
                    jumpPhase = 2; // 次のフェーズ: 着地
                }
                break;
            case 2: // 着地フェーズ
                playerY -= 0.05f;
                if (playerY <= 1.0f) {
                    playerY = 1.0f;
                    isJumping = 0; // ジャンプ終了
                    jumpPhase = 0;
                }
                break;
        }
    }

    updateObstacles();
    checkCollisionAndScore();

    if (frameCount > GAME_DURATION) {
        printf("Game Over! Time up. Final Score: %d\n", score);
        exit(0);
    }

    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) exit(0);
}

void specialKeys(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_LEFT:
            playerX += 1.0f;
            break;
        case GLUT_KEY_RIGHT:
            playerX -= 1.0f;
            break;
        case GLUT_KEY_UP:
            if (!isJumping) {
                isJumping = 1;
                jumpPhase = 0; // 上昇フェーズを開始
            }
            break;
    }
}

void init(void) {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1280.0 / 960.0, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);

    srand((unsigned int)time(NULL));
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        obstacles[i].active = 0;
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 960);
    glutCreateWindow("Endless Runner Game");

    init();
    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);

    glutMainLoop();
    return 0;
}
