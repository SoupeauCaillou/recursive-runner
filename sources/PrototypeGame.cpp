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
#include "systems/CameraTargetSystem.h"

#include <cmath>
#include <vector>



#ifndef EMSCRIPTEN
// #define IN_GAME_EDITOR 0
#endif
#if IN_GAME_EDITOR
#include <GL/glfw.h>
#endif

Entity background, startButton, scoreText;
std::vector<Entity> player;
int playerCount;
std::vector<Entity> coins, gains, explosions;
bool playing;
int score;
float cameraMaxAccel = 0.5;
float cameraSpeed;
float playerSpeed = 6;

#define LEVEL_SIZE 3
extern float MaxJumpDuration;

static void updateFps(float dt);

PrototypeGame::PrototypeGame(AssetAPI* ast, NameInputAPI* inputUI, LocalizeAPI* lAPI, AdAPI* ad, ExitAPI* exAPI) : Game() {
	asset = ast;
	exitAPI = exAPI;
}

static void resetGame() {
    theRenderingSystem.cameraPosition = Vector2::Zero;
    TEXT_RENDERING(startButton)->hide = false;
    TEXT_RENDERING(scoreText)->hide = false;
    CONTAINER(startButton)->enable = true;
    RENDERING(startButton)->hide = false;
    BUTTON(startButton)->enabled = true;
    playing = false;
    
    for (int i=0; i<coins.size(); i++) {
        theEntityManager.DeleteEntity(coins[i]);
    }
    coins.clear();
    for (int i=0; i<20; i++) {
        Entity e = theEntityManager.CreateEntity();
        ADD_COMPONENT(e, Transformation);
        TRANSFORM(e)->size = Vector2(0.3, 0.3);
        TRANSFORM(e)->position = Vector2(
            MathUtil::RandomFloatInRange(
                -LEVEL_SIZE * 0.5 * PlacementHelper::ScreenWidth,
                LEVEL_SIZE * 0.5 * PlacementHelper::ScreenWidth),
            MathUtil::RandomFloatInRange(
                -0.4 * PlacementHelper::ScreenHeight,
                -0.2 * PlacementHelper::ScreenHeight));
        TRANSFORM(e)->rotation = MathUtil::RandomFloat() * 6.28;
        TRANSFORM(e)->z = 0.5;
        ADD_COMPONENT(e, Rendering);
        RENDERING(e)->color = Color(1, 1, 0);
        RENDERING(e)->hide = false;
        coins.push_back(e);
    }
}

static void spawnGainEntity(int gain, const Vector2& pos) {
    Entity e = theEntityManager.CreateEntity();
    ADD_COMPONENT(e, Transformation);
    TRANSFORM(e)->position = pos;
    TRANSFORM(e)->z = 0.7;
    ADD_COMPONENT(e, TextRendering);
    std::stringstream a;
    a << gain;
    TEXT_RENDERING(e)->text = a.str();
    TEXT_RENDERING(e)->charHeight = 0.5;
    TEXT_RENDERING(e)->color = Color(1, 1, 0);
    TEXT_RENDERING(e)->hide = false;
    ADD_COMPONENT(e, Physics);
    PHYSICS(e)->mass = 1;
    PHYSICS(e)->gravity = Vector2(0, 6);
    gains.push_back(e);
}

static void addPlayer() {
    int direction = (playerCount % 2) ? -1 : 1;
    Entity e = theEntityManager.CreateEntity();
    ADD_COMPONENT(e, Transformation);
    TRANSFORM(e)->position = Vector2(-9, 2);
    TRANSFORM(e)->size = Vector2(0.4,1);//0.572173, 0.815538);
    TRANSFORM(e)->rotation = 0;
    TRANSFORM(e)->z = 0.8;
    ADD_COMPONENT(e, Rendering);
    RENDERING(e)->color = Color::random();
    RENDERING(e)->hide = false;
    ADD_COMPONENT(e, Runner);
    TRANSFORM(e)->position = RUNNER(e)->startPoint = Vector2(
        direction * -LEVEL_SIZE * 0.5 * PlacementHelper::ScreenWidth,
        -0.5 * PlacementHelper::ScreenHeight + TRANSFORM(e)->size.Y * 0.5);
    RUNNER(e)->endPoint = RUNNER(e)->startPoint + Vector2(direction * LEVEL_SIZE * PlacementHelper::ScreenWidth, 0);
    RUNNER(e)->maxSpeed = RUNNER(e)->speed = direction * playerSpeed * (1 + 0.1 * playerCount);
    RUNNER(e)->startTime = 0;//MathUtil::RandomFloatInRange(1,3);
    ADD_COMPONENT(e, CameraTarget);
    CAM_TARGET(e)->enabled = true;
    CAM_TARGET(e)->maxCameraSpeed = direction * RUNNER(e)->speed;
    CAM_TARGET(e)->offset = Vector2(
        direction * 0.4 * PlacementHelper::ScreenWidth, 
        0 - TRANSFORM(e)->position.Y);
    ADD_COMPONENT(e, Physics);
    PHYSICS(e)->mass = 1;

    playerCount++;
    player.push_back(e);
    std::cout << "Add player " << e << " at pos : " << TRANSFORM(e)->position << ", speed= " << RUNNER(e)->speed << std::endl;
}

static void startGame() {
    for (int i=0; i<player.size(); i++) {
        theEntityManager.DeleteEntity(player[i]);
    }
    player.clear();
    playerCount = 0;
    addPlayer();
    score = 0;
    playing = true;
    
    TEXT_RENDERING(scoreText)->hide = false;
    TEXT_RENDERING(startButton)->hide = true;
    RENDERING(startButton)->hide = true;
    cameraSpeed = 0;
    theRenderingSystem.cameraPosition.X = TRANSFORM(player[0])->position.X + PlacementHelper::ScreenWidth * 0.5;
}

void PrototypeGame::init(const uint8_t* in, int size) {
	theRenderingSystem.loadAtlas("alphabet", true);   
 
    RunnerSystem::CreateInstance();
    CameraTargetSystem::CreateInstance();
 
	// init font
	loadFont(asset, "typo");
	
	PlacementHelper::GimpWidth = 800;
    PlacementHelper::GimpHeight = 500;

    background = theEntityManager.CreateEntity();
    ADD_COMPONENT(background, Transformation);
    TRANSFORM(background)->size = Vector2(LEVEL_SIZE * PlacementHelper::ScreenWidth, 0.7 * PlacementHelper::ScreenHeight);
    TRANSFORM(background)->position.Y = -(PlacementHelper::ScreenHeight - TRANSFORM(background)->size.Y)*0.5;
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
    
    scoreText = theEntityManager.CreateEntity();
    ADD_COMPONENT(scoreText, Transformation);
    TRANSFORM(scoreText)->position = Vector2(0, 0.35 * PlacementHelper::ScreenHeight);
    TRANSFORM(scoreText)->z = 0.9;
    ADD_COMPONENT(scoreText, TextRendering);
    TEXT_RENDERING(scoreText)->text = "";
    TEXT_RENDERING(scoreText)->charHeight = 1;
    
    resetGame();
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

    if (playing) {
        if (!player.empty()) {
            Entity current = *player.rbegin();

            std::stringstream a;
            a << "Score: " << score;
            TEXT_RENDERING(scoreText)->text = a.str();

            if (RUNNER(current)->finished) {
                if (playerCount == 10) {
                    CAM_TARGET(current)->enabled = false;
                    theRenderingSystem.cameraPosition = Vector2::Zero;
                    // end of game
                    resetGame();
                } else {
                    // re-init all players
                    for (int i=0; i<player.size(); i++) {
                        CAM_TARGET(player[i])->enabled = false;
                    }
                    // add a new one
                    addPlayer();
                    current = *player.rbegin();
                }
            }
            
            PhysicsComponent* pc = PHYSICS(current);
            if (pc->gravity.Y >= 0) {
                if (theTouchInputManager.isTouched()) {
                    if (!theTouchInputManager.wasTouched()) {
                        if (RUNNER(current)->jumpingSince <= 0) {
                            RUNNER(current)->jumpTimes.push_back(RUNNER(current)->elapsed);
                            RUNNER(current)->jumpDurations.push_back(0.001);
                        }
                    } else if (!RUNNER(current)->jumpTimes.empty()) {
                     
                        float& d = *(RUNNER(current)->jumpDurations.rbegin());
                        if (d < MaxJumpDuration) {
                            d += dt;
                        }
                    }
                }
            }
            
            TransformationComponent* tc = TRANSFORM(current);
            CAM_TARGET(current)->offset.Y = 0 - tc->position.Y;
            
            // check for collisions
            for (int i=0; i<player.size()-1; i++) {
                if (current == player[i])
                    continue;
                if (IntersectionUtil::rectangleRectangle(tc, TRANSFORM(player[i]))) {
                    std::cout << current << " killed " << player[i] << ", " << tc->position << std::endl;                    
                    // create explosions
                    int c = MathUtil::RandomIntInRange(4, 7);
                    for (int j=0; j<c; j++) {
                        Entity e = theEntityManager.CreateEntity();
                        ADD_COMPONENT(e, Transformation);
                        Vector2 size = Vector2(0.4, 1.0/c);
                        TRANSFORM(e)->size = size;
                        TRANSFORM(e)->position = 
                            TRANSFORM(player[i])->position + Vector2(0, 0.5 - (j+0.5) * size.Y);
                        TRANSFORM(e)->z = TRANSFORM(player[i])->z;
                        ADD_COMPONENT(e, Rendering);
                        RENDERING(e)->color = RENDERING(player[i])->color;
                        RENDERING(e)->hide = false;
                        ADD_COMPONENT(e, Physics);
                        PHYSICS(e)->mass = 1;
                        PHYSICS(e)->gravity.Y = -10;
                        PHYSICS(e)->forces.push_back(std::make_pair(
                            Force(Vector2::Rotate(Vector2(1000, 0), MathUtil::RandomFloat(3.14)),
                                  Vector2::Rotate(size, MathUtil::RandomFloat(6.28))), 0.016));
                        explosions.push_back(e);
                    }
                    // remove player
                    theEntityManager.DeleteEntity(player[i]);
                    player.erase(player.begin() + i);
                    i--;
                }
            }
        }
    } else {
        if (BUTTON(startButton)->clicked) {
            std::cout << "Start game!" << std::endl;
            startGame();
        }
    }
    
    if (playing) {
        for (int i=0; i<player.size(); i++) {
            Entity e = player[i];
            TransformationComponent* tc = TRANSFORM(e);
            RunnerComponent* rc = RUNNER(e);
            PhysicsComponent* pc = PHYSICS(e);
            
            // check jumps
            if (pc->gravity.Y < 0) {
                // if (pc->linearVelocity.Y < 0)
                //    pc->gravity.Y = -80;
                if ((tc->position.Y - tc->size.Y * 0.5) <= -PlacementHelper::ScreenHeight * 0.5) {
                    pc->gravity.Y = 0;
                    pc->linearVelocity = Vector2::Zero;
                    tc->position.Y = -PlacementHelper::ScreenHeight * 0.5 + tc->size.Y * 0.5;
                }
            }
            // check coins
            for (std::vector<Entity>::iterator it=coins.begin(); it!=coins.end(); ++it) {
                Entity coin = *it;
                if (std::find(rc->coins.begin(), rc->coins.end(), coin) == rc->coins.end()) {
                    if (IntersectionUtil::rectangleRectangle(tc, TRANSFORM(coin))) {
                        rc->coins.push_back(coin);
                        int gain = 10 * (player.size() - i);
                        score += gain;
                        spawnGainEntity(gain, TRANSFORM(coin)->position);
                    }
                }
            }
        }
        
        for (int i=0; i<gains.size(); i++) {
            Entity e = gains[i];
            float& a = TEXT_RENDERING(e)->color.a;
            a -= 1 * dt;
            if (a <= 0) {
                theTextRenderingSystem.DeleteEntity(e);
                gains.erase(gains.begin() + i);
                i--;
            }
        }
        for (int i=0; i<explosions.size(); i++) {
            if (TRANSFORM(explosions[i])->position.Y < PlacementHelper::ScreenHeight * -0.5) {
                theEntityManager.DeleteEntity(explosions[i]);
                explosions.erase(explosions.begin() + i);
                i--;
            }
        }
    }

    if (playing) {
        theRunnerSystem.Update(dt);
        theCameraTargetSystem.Update(dt);
    }
    
    // limit cam pos
    if (theRenderingSystem.cameraPosition.X < - PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5)) {
        theRenderingSystem.cameraPosition.X = - PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5);
    } else if (theRenderingSystem.cameraPosition.X > PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5)) {
        theRenderingSystem.cameraPosition.X = PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5);
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

