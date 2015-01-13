//
//  SacWrapper.cpp
//  TestGame
//
//  Created by pierre-eric on 09/01/2015.
//  Copyright (c) 2015 Soupe au Caillou. All rights reserved.
//

#include "SacWrapper.h"

#include <app/AppSetup.h>
struct Game;

struct Game* game;

extern int iosIsTouching(int index, float* x, float* y);
extern int (*iosIsTouchingFn) (int, float* winX, float* winY);

void sac_init(float w, float h) {
    game = buildGameInstance();
    
    struct SetupInfo s;
    s.name = "Test";
    s.version = "0.0.1";
    s.resolution.x = (w > h) ? w : h;
    s.resolution.y = (w > h) ? h : w;
    
    setupEngine(game, &s);
    
    iosIsTouchingFn = &iosIsTouching;
}

void sac_tick() {
    tickEngine(game);
}