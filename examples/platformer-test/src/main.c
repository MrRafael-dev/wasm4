#include "wasm4.h"

#define PLAYER_WIDTH 1
#define PLAYER_HEIGHT 1
#define TILE_SIZE 16

#define bunnyWidth 48
#define bunnyHeight 16
#define bunnyFlags BLIT_2BPP
const char bunny[192] = { 0x00,0x2c,0x0d,0x00,0x02,0x90,0x03,0xd0,0x02,0x90,0x03,0xd0,0x00,0xab,0x3f,0x40,0x0a,0xa4,0x0f,0xf4,0x0a,0xa4,0x0f,0xf4,0x00,0xab,0x3f,0x40,0x0a,0xa9,0x3f,0xf4,0x0a,0xa9,0x3f,0xf4,0x00,0x6b,0x3f,0x40,0x02,0xa9,0x57,0x50,0x02,0xa9,0x57,0x50,0x00,0x6b,0x57,0x40,0x00,0x69,0xa9,0x40,0x00,0x69,0xa9,0x40,0x00,0xab,0xa9,0x40,0x01,0xaa,0xaa,0x80,0x01,0xaa,0xaa,0x80,0x01,0xaa,0xaa,0x80,0x02,0xaa,0x5a,0x50,0x02,0xaa,0x5a,0x50,0x02,0xaa,0x5a,0x50,0x0f,0xea,0xaa,0x90,0x0f,0xea,0xaa,0x90,0x0f,0xea,0xaa,0x90,0x01,0xba,0xaa,0x90,0x01,0xba,0xaa,0x90,0x01,0xba,0xaa,0x90,0x00,0xea,0xab,0x40,0x00,0xea,0xab,0x40,0x00,0xea,0xab,0x40,0x01,0xaa,0xbd,0x00,0x00,0x6a,0x5d,0x00,0x00,0x6e,0xbd,0x00,0x0a,0xa6,0xaa,0x40,0x01,0xae,0xa6,0x40,0x01,0xba,0x6a,0x40,0x06,0x9a,0xaa,0x40,0x02,0xab,0xa6,0x40,0x01,0x9a,0x6a,0x40,0x01,0x6b,0xa9,0x40,0x01,0xda,0x5a,0x40,0x00,0x65,0xa9,0x00,0x06,0xad,0x57,0xd0,0x0f,0xf5,0x5a,0x90,0x00,0x1a,0xa4,0x00,0x01,0xa4,0x17,0x40,0x07,0xd0,0x06,0x90 };

#define crateWidth 16
#define crateHeight 16
#define crateFlags BLIT_2BPP
const char crate[64] = { 0x00,0x00,0x00,0x00,0x05,0x55,0x55,0x50,0x11,0x55,0x55,0x40,0x14,0x00,0x00,0x08,0x14,0xaa,0xaa,0x20,0x14,0xaa,0xaa,0x08,0x14,0xaa,0xaa,0x20,0x14,0xaa,0xaa,0x08,0x14,0xaa,0xaa,0x20,0x14,0xaa,0xaa,0x08,0x14,0xaa,0xaa,0x20,0x14,0xaa,0xaa,0x08,0x14,0x00,0x00,0x20,0x10,0x88,0x88,0x88,0x02,0x22,0x22,0x20,0x00,0x00,0x00,0x00 };

#define MAP_WIDTH 16
#define MAP_HEIGHT 10
const char map[] = { 1,2,129,2,1,2,1,2,13,2,1,2,97,2,1,2,1,3,255,3 };

int playerX, playerY;

int frame;

int grounded = 0;
float velY = 0;
int faceLeft;

int solid (int tileX, int tileY) {
    int idx = (tileY*MAP_WIDTH + tileX) / 8;
    int shift = (tileX & 0x7);
    return (map[idx] >> shift) & 1;
}

void handleHorizontalCollision (int velX) {
    int tileX = (playerX+velX)/TILE_SIZE;
    int tileTop = playerY/TILE_SIZE;
    int tileBottom = (playerY+(PLAYER_HEIGHT*TILE_SIZE)-1)/TILE_SIZE;

    int snapX;
    if (velX < 0) {
        snapX = (tileX+1)*TILE_SIZE;
    } else {
        tileX += PLAYER_WIDTH;
        snapX = (tileX-PLAYER_WIDTH)*TILE_SIZE;
    }
    if (solid(tileX, tileTop) || solid(tileX, tileBottom)) {
        playerX = snapX;
    } else {
        playerX += velX;
    }
}

int handleVerticalCollision (int velY) {
    int tileY = (playerY+velY)/TILE_SIZE;
    int tileLeft = playerX/TILE_SIZE;
    int tileRight = (playerX+(PLAYER_WIDTH*TILE_SIZE)-1)/TILE_SIZE;

    int snapY;
    if (velY < 0) {
        snapY = (tileY+1)*TILE_SIZE;
    } else {
        tileY += PLAYER_HEIGHT;
        snapY = (tileY-PLAYER_HEIGHT)*TILE_SIZE;
    }
    if (solid(tileLeft, tileY) || solid(tileRight, tileY)) {
        playerY = snapY;
        return 1;
    } else {
        playerY += velY;
        return 0;
    }
}

int handleDownCollision (int velY) {
    int tileY = (playerY+velY)/TILE_SIZE + PLAYER_HEIGHT;
    int tileLeft = playerX/TILE_SIZE;
    int tileRight = (playerX+(PLAYER_WIDTH*TILE_SIZE)-1)/TILE_SIZE;

    if (solid(tileLeft, tileY) || solid(tileRight, tileY)) {
        playerY = (tileY-PLAYER_HEIGHT)*TILE_SIZE;
        return 1;
    } else {
        playerY += velY;
        return 0;
    }
}

void update () {
    ++frame;

    int walking = 0;
    unsigned char buttons = *GAMEPAD1;
    if (buttons & BUTTON_LEFT) {
        faceLeft = 1;
        handleHorizontalCollision(-2);
        walking = 1;
    }
    if (buttons & BUTTON_RIGHT) {
        faceLeft = 0;
        handleHorizontalCollision(2);
        walking = 1;
    }

    if (grounded && (buttons & (BUTTON_1|BUTTON_UP))) {
        grounded = 0;
        velY = -5;
        tone(210 | (620 << 16), 16, 100, TONE_PULSE1 | TONE_MODE1);
    }

    if (handleVerticalCollision(velY)) {
        if (velY > 0) {
            grounded = 1;
        }
        velY = 0;
    } else {
        grounded = 0;
        velY += 0.3;
    }

    int animFrame = 0;
    if (!grounded) {
        animFrame = 1;
    } else if (walking) {
        animFrame = ((frame/2) % 3);
    }

    *DRAW_COLORS = 0x1213;
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            if (solid(x, y)) {
                blit(crate, 2*x*8, 2*y*8, crateWidth, crateHeight, BLIT_2BPP);
            }
        }
    }

    *DRAW_COLORS = 0x2130;
    int flags = faceLeft ? BLIT_FLIP_X : 0;
    blitSub(bunny, playerX, playerY, 16, 16, animFrame*16, 0, bunnyWidth, flags | bunnyFlags);
}