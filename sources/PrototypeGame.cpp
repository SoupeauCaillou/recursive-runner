/*
	This file is part of Heriswap.

	@author Soupe au Caillou - Pierre-Eric Pelloux-Prayer
	@author Soupe au Caillou - Gautier Pelloux-Prayer

	Heriswap is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, version 3.

	Heriswap is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Heriswap.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "PrototypeGame.h"
#include <sstream>

#include <base/Log.h>
#include <base/TouchInputManager.h>
#include <base/MathUtil.h>
#include <base/EntityManager.h>
#include <base/TimeUtil.h>
#include <base/PlacementHelper.h>
#include "util/IntersectionUtil.h"

#include "api/NameInputAPI.h"

#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/ButtonSystem.h"
#include "systems/ADSRSystem.h"
#include "systems/TextRenderingSystem.h"
#include "systems/SoundSystem.h"
#include "systems/TaskAISystem.h"
#include "systems/MusicSystem.h"
#include "systems/ContainerSystem.h"
#include "systems/PhysicsSystem.h"
#include "systems/ParticuleSystem.h"
#include "systems/ScrollingSystem.h"
#include "systems/MorphingSystem.h"

#include <cmath>
#include <vector>

#ifndef EMSCRIPTEN
#include <GL/glfw.h>
#endif

Entity background;
std::vector<Entity> decors, murs, sols;
int activeIndex = -1;
std::vector<Entity> monstrons;

Entity zolon;

#define SOL 0.0
#define MUR 1.0

float decorsDef[] = {
-2.025, -0.875, 0.617455, 1.09657, 0.466667, MUR,
-2, -1.7, 0.482553, 1.32046, -0.433333, MUR,
-2.6, -3.175, 1, 2.39753, 0, MUR,

0.75, -4.5, 7.37226, 1, 0.0333333, SOL, 

4.4, -2.9, 0.698124, 3.53807, -0.2, MUR,
4.5, -0.0500002, 0.84981, 3.43235, 0.2, MUR,

5.1, 1.275, 2.60176, 0.764209, 0, SOL,
7.35, 0.0749998, 3.60588, 0.831206, -0.8, SOL, 
9.25, -1.375, 3.66604, 1, 0,SOL,
-8.175, -0.375, 6.47703, 1, -0.166667, SOL,
-4.3, -0.8, 4.68243, 1, 0,SOL,
-9.5, 0.275, 3.66975, 1, 0,SOL,

-7.95, 0.0749998, 0.815077, 2.87012, 0, MUR,
-10.275, 1.65, 0.815077, 2.87012, 0, MUR
};

float monstresDef[] = {
-0.65, -1.075, 0.76271, 0.76271, 
-4.6, 0.375, 0.76271, 0.76271, 
2.8, 0.475, 0.76271, 0.76271,
};

static void updateFps(float dt);

PrototypeGame::PrototypeGame(AssetAPI* ast, NameInputAPI* inputUI, LocalizeAPI* lAPI, AdAPI* ad, ExitAPI* exAPI) : Game() {
	asset = ast;
	exitAPI = exAPI;
}
void PrototypeGame::init(const uint8_t* in, int size) {    
	theRenderingSystem.loadAtlas("alphabet", true);   
    theRenderingSystem.loadAtlas("decor", true);   

	// init font
	loadFont(asset, "typo");
	
	PlacementHelper::GimpWidth = 800;
    PlacementHelper::GimpHeight = 400;

    background = theEntityManager.CreateEntity();
    ADD_COMPONENT(background, Transformation);
    TRANSFORM(background)->size = Vector2(PlacementHelper::ScreenWidth, PlacementHelper::ScreenHeight);
    TRANSFORM(background)->z = 0.1;
    ADD_COMPONENT(background, Rendering);
    RENDERING(background)->texture = theRenderingSystem.loadTextureFile("background");
    RENDERING(background)->hide = false;
    
    for (int i=0; i<14; i++) {
        Entity e = theEntityManager.CreateEntity();
        ADD_COMPONENT(e, Transformation);
        TRANSFORM(e)->position = Vector2(decorsDef[6*i], decorsDef[6*i+1]);
        TRANSFORM(e)->size = Vector2(decorsDef[6*i+2], decorsDef[6*i+3]);
        TRANSFORM(e)->rotation = decorsDef[6*i+4];
        TRANSFORM(e)->z = 0.5;
        ADD_COMPONENT(e, Rendering);
        RENDERING(e)->color = Color::random();
        RENDERING(e)->color.a = 0.5;
        RENDERING(e)->hide = false;
        
        if (decorsDef[6*i+5] == MUR)
            murs.push_back(e);
        else
            sols.push_back(e);
        
        decors.push_back(e);
    }
    zolon = theEntityManager.CreateEntity();
    ADD_COMPONENT(zolon, Transformation);
    TRANSFORM(zolon)->position = Vector2(-9, 2);
    TRANSFORM(zolon)->size = Vector2(0.572173, 0.815538);
    TRANSFORM(zolon)->rotation = 0;
    TRANSFORM(zolon)->z = 0.8;
    ADD_COMPONENT(zolon, Rendering);
    RENDERING(zolon)->texture = theRenderingSystem.loadTextureFile("zolon");
    RENDERING(zolon)->hide = false;
    ADD_COMPONENT(zolon, Physics);
    PHYSICS(zolon)->mass = 1;
    PHYSICS(zolon)->gravity = Vector2(0, -10);
    

    for (int i=0; i<3;i ++) {
        Entity e = theEntityManager.CreateEntity();
        ADD_COMPONENT(e, Transformation);
        TRANSFORM(e)->position = Vector2(monstresDef[4*i], monstresDef[4*i+1]);
        TRANSFORM(e)->size = Vector2(monstresDef[4*i+2], monstresDef[4*i+3]);
        TRANSFORM(e)->z = 0.6;
        ADD_COMPONENT(e, Rendering);
        RENDERING(e)->texture = theRenderingSystem.loadTextureFile("monstron");
        RENDERING(e)->hide = false;
        monstrons.push_back(e);
    }
}


void PrototypeGame::backPressed() {
#ifndef EMSCRIPTEN
    Entity e = theEntityManager.CreateEntity();
    ADD_COMPONENT(e, Transformation);
    TRANSFORM(e)->size = Vector2(1, 1);
    TRANSFORM(e)->z = 0.5;
    ADD_COMPONENT(e, Rendering);
    RENDERING(e)->color = Color::random();
    RENDERING(e)->color.a = 0.5;
    RENDERING(e)->hide = false;
    
    decors.push_back(e);
    activeIndex = decors.size() - 1;
#endif
}

void PrototypeGame::togglePause(bool activate) {
#ifndef EMSCRIPTEN
    if (activeIndex >= 0) {
        Entity e = decors[activeIndex];
        std::cout << "{ " << TRANSFORM(e)->position << ", " 
            << TRANSFORM(e)->size << ", " << TRANSFORM(e)->rotation << "}, " << std::endl;
        activeIndex = -1;
    }
#endif
}

void entityToLinePoints(Entity e, Vector2& a, Vector2& b, bool top) {
    const Vector2& size = TRANSFORM(e)->size;
    const Vector2& pos = TRANSFORM(e)->worldPosition;
    const float rot =  TRANSFORM(e)->rotation;
    if (top) {
        a = pos + Vector2::Rotate(Vector2(-size.X * 0.5, size.Y * 0.5), rot);
        b = pos + Vector2::Rotate(Vector2(size.X * 0.5, size.Y * 0.5), rot);
    } else {
        a = pos + Vector2::Rotate(Vector2(-size.X * 0.5, -size.Y * 0.5), rot);
        b = pos + Vector2::Rotate(Vector2(size.X * 0.5, -size.Y * 0.5), rot);
    }
}

float zolonSpeed = 1;

void PrototypeGame::tick(float dt) {
	theTouchInputManager.Update(dt);
 
#ifndef EMSCRIPTEN
    if (activeIndex >= 0) {
        Entity e = decors[activeIndex];
        if (theTouchInputManager.isTouched()) {
            TRANSFORM(e)->position = theTouchInputManager.getTouchLastPosition();
        }
            
        // mouse wheel -> rotate
        {
            static int prevWheel = 0;
            int wheel = glfwGetMouseWheel();
            int diff = wheel - prevWheel;
            if (diff) {
                bool shift = glfwGetKey( GLFW_KEY_LSHIFT );
                bool ctrl = glfwGetKey( GLFW_KEY_LCTRL );
                
                if (!shift && !ctrl) {
                    TRANSFORM(e)->rotation += 2 * diff * dt;
                } else {
                    if (shift) {
                        TRANSFORM(e)->size.X *= (1 + 1 * diff * dt); 
                    }
                    if (ctrl) {
                        TRANSFORM(e)->size.Y *= (1 + 1 * diff * dt); 
                    }
                }
                prevWheel = wheel;
            }
        }
    } else if (theTouchInputManager.isTouched()) {
        for (int i=0; i<decors.size(); i++) {
            if (IntersectionUtil::pointRectangle(
                theTouchInputManager.getTouchLastPosition(), 
                TRANSFORM(decors[i])->position,
                TRANSFORM(decors[i])->size)) {
                activeIndex = i;
                break;
            }
        }
    }
#endif

    if (TRANSFORM(zolon)->parent) {
        // check we are still on platform
        Entity plat = TRANSFORM(zolon)->parent;
        if (IntersectionUtil::rectangleRectangle(
                TRANSFORM(zolon)->worldPosition, TRANSFORM(zolon)->size, TRANSFORM(zolon)->rotation,
                TRANSFORM(plat)->position, TRANSFORM(plat)->size, TRANSFORM(plat)->rotation)) {
            // move along platform
            TRANSFORM(zolon)->position.X += zolonSpeed * dt;
        } else {
         std::cout << "plus intersect " << TRANSFORM(zolon)->parent << std::endl;
            TRANSFORM(zolon)->parent = 0;
            TRANSFORM(zolon)->position = TRANSFORM(zolon)->worldPosition;
        }
        
        // if we hit a wall -> turn back
        for (int i=0; i<murs.size(); i++) {
            if (IntersectionUtil::rectangleRectangle(
                    TRANSFORM(zolon)->worldPosition, TRANSFORM(zolon)->size, TRANSFORM(zolon)->rotation,
                    TRANSFORM(murs[i])->position, TRANSFORM(murs[i])->size, TRANSFORM(murs[i])->rotation)) {
                zolonSpeed = -zolonSpeed;
                TRANSFORM(zolon)->position.X += 2*zolonSpeed * dt;
                std::cout << zolonSpeed << std::endl;
            }
        }
    } else {
        for (int i=0; i<sols.size(); i++) {
            if (IntersectionUtil::rectangleRectangle(
                TRANSFORM(zolon)->worldPosition, TRANSFORM(zolon)->size, TRANSFORM(zolon)->rotation,
                TRANSFORM(sols[i])->position, TRANSFORM(sols[i])->size, TRANSFORM(sols[i])->rotation)) {
                 std::cout << "youpi " << sols[i] << std::endl;
                TRANSFORM(zolon)->parent = sols[i];
                TRANSFORM(zolon)->rotation = 0;
                TRANSFORM(zolon)->position = Vector2::Rotate(TRANSFORM(zolon)->worldPosition - TRANSFORM(sols[i])->position, -TRANSFORM(sols[i])->rotation);
                TRANSFORM(zolon)->position.Y = TRANSFORM(zolon)->size.Y * 0.5 + TRANSFORM(sols[i])->size.Y * 0.4;
                break;
            }
        }
    }
    


    // systems update
	theADSRSystem.Update(dt);
	theButtonSystem.Update(dt);
    theParticuleSystem.Update(dt);
	theMorphingSystem.Update(dt);
	thePhysicsSystem.Update(dt);
	theScrollingSystem.Update(dt);
	theContainerSystem.Update(dt);
	theTextRenderingSystem.Update(dt);
	theSoundSystem.Update(dt);
    theMusicSystem.Update(dt);
    theTransformationSystem.Update(dt);
    theRenderingSystem.Update(dt);
}

void updateFps(float dt) {
    #define COUNT 250
    static int frameCount = 0;
    static float accum = 0, t = 0;
    frameCount++;
    accum += dt;
    if (frameCount == COUNT) {
         LOGI("%d frames: %.3f s - diff: %.3f s - ms per frame: %.3f", COUNT, accum, TimeUtil::getTime() - t, accum / COUNT);
         t = TimeUtil::getTime();
         accum = 0;
         frameCount = 0;
     }
}

