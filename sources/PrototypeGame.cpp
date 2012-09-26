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
#include "systems/RunnerSystem.h"

#include <cmath>
#include <vector>

#ifndef EMSCRIPTEN
#include <GL/glfw.h>
#endif

#ifndef EMSCRIPTEN
// #define IN_GAME_EDITOR 0
#endif

Entity background, startButton;
std::vector<Entity> player;
bool playing;

static void updateFps(float dt);

PrototypeGame::PrototypeGame(AssetAPI* ast, NameInputAPI* inputUI, LocalizeAPI* lAPI, AdAPI* ad, ExitAPI* exAPI) : Game() {
	asset = ast;
	exitAPI = exAPI;
}

static void reset() {
    TEXT_RENDERING(startButton)->hide = false;
    CONTAINER(startButton)->enable = true;
    RENDERING(startButton)->hide = false;
    BUTTON(startButton)->enabled = true;
    playing = false;
}

static void init() {
    for (int i=0; i<player.size(); i++) {
        theEntityManager.DeleteEntity(player[i]);
    }
    player.clear();

    Entity e = theEntityManager.CreateEntity();
    ADD_COMPONENT(e, Transformation);
    TRANSFORM(e)->position = Vector2(-9, 2);
    TRANSFORM(e)->size = Vector2(0.572173, 0.815538);
    TRANSFORM(e)->rotation = 0;
    TRANSFORM(e)->z = 0.8;
    ADD_COMPONENT(e, Rendering);
    RENDERING(e)->color = Color(0,0,1);
    RENDERING(e)->hide = false;
    ADD_COMPONENT(e, Runner);
    player.push_back(e);
}

void PrototypeGame::init(const uint8_t* in, int size) {    
	theRenderingSystem.loadAtlas("alphabet", true);   

	// init font
	loadFont(asset, "typo");
	
	PlacementHelper::GimpWidth = 800;
    PlacementHelper::GimpHeight = 600;

    background = theEntityManager.CreateEntity();
    ADD_COMPONENT(background, Transformation);
    TRANSFORM(background)->size = Vector2(PlacementHelper::ScreenWidth, PlacementHelper::ScreenHeight);
    TRANSFORM(background)->z = 0.1;
    ADD_COMPONENT(background, Rendering);
    RENDERING(background)->color = Color(0.3, 0.3, 0.3);
    RENDERING(background)->hide = false;

    startButton = theEntityManager.CreateEntity();
    ADD_COMPONENT(startButton, Transformation);
    TRANSFORM(startButton)->position = Vector2::Zero;
    TRANSFORM(startButton)->z = 0.9;
    ADD_COMPONENT(startButton, TextRendering);
    TEXT_RENDERING(startButton)->text = "Jouer";
    TEXT_RENDERING(startButton)->charHeight = 1;
    ADD_COMPONENT(startButton, Container);
    CONTAINER(startButton)->entities.push_back(startButton);
    CONTAINER(startButton)->includeChildren = true;
    ADD_COMPONENT(startButton, Rendering);
    RENDERING(startButton)->color = Color(0.2, 0.2, 0.2, 0.5);
    ADD_COMPONENT(startButton, Button);
    
    reset();
}


void PrototypeGame::backPressed() {
#if IN_GAME_EDITOR
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
#if IN_GAME_EDITOR
    if (activeIndex >= 0) {
        Entity e = decors[activeIndex];
        std::cout << "{ " << TRANSFORM(e)->position << ", " 
            << TRANSFORM(e)->size << ", " << TRANSFORM(e)->rotation << "}, " << std::endl;
        activeIndex = -1;
    }
#endif
}

void PrototypeGame::tick(float dt) {
	theTouchInputManager.Update(dt);
 
#if IN_GAME_EDITOR
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

