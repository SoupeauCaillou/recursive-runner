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
#include "systems/AnimationSystem.h"
#include "systems/AutoDestroySystem.h"
#include "systems/NetworkSystem.h"
#include "systems/RunnerSystem.h"
#include "systems/CameraTargetSystem.h"
#include "systems/PlayerSystem.h"
#include "systems/GameSystem.h"
#include "api/linux/NetworkAPILinuxImpl.h"

#include <cmath>
#include <vector>



#ifndef EMSCRIPTEN
// #define IN_GAME_EDITOR 0
#endif
#if IN_GAME_EDITOR
#include <GL/glfw.h>
#endif

static void spawnGainEntity(int gain, const Vector2& pos);
static Entity addRunnerToPlayer(Entity player, PlayerComponent* p);
static void updateFps(float dt);


// PURE LOCAL VARS
Entity background, startSingleButton, startMultiButton, scoreText, goldCoin;

enum GameState {
    Menu,
    WaitingPlayers,
    Playing
} gameState;

struct GameTempVar {
    void syncRunners();
    void syncCoins();
    void cleanup();
    int playerIndex();

    unsigned numPlayers;
    bool isGameMaster;
    Entity currentRunner;
    std::vector<Entity> runners[2], coins, players; 
 
} gameTempVars;

static GameState updateMenu(float dt);
static void transitionMenuWaitingPlayers();
static GameState updateWaitingPlayers(float dt);
static void transitionWaitingPlayersPlaying();
static void transitionWaitingPlayersMenu();
static GameState updatePlaying(float dt);
static void transitionPlayingMenu();


static void createCoins(int count);

const float playerSpeed = 6;

#define LEVEL_SIZE 3
extern float MaxJumpDuration;


PrototypeGame::PrototypeGame(AssetAPI* ast, NameInputAPI* inputUI, LocalizeAPI* lAPI, AdAPI* ad, ExitAPI* exAPI) : Game() {
	asset = ast;
	exitAPI = exAPI;
}

void PrototypeGame::sacInit(int windowW, int windowH) {
    PlacementHelper::GimpWidth = 800;
    PlacementHelper::GimpHeight = 500;

    Game::sacInit(windowW, windowH);
    theRenderingSystem.loadAtlas("alphabet", true);
    theRenderingSystem.loadAtlas("dummy", false);
    theRenderingSystem.loadAtlas("decor", false);
    
// register 4 animations
    std::string runL2R[] = { "obj_Run000", "obj_Run001", "obj_Run002", "obj_Run003", "obj_Run004", "obj_Run005", "obj_Run006", "obj_Run007" }; 
    std::string runR2L[] = { "obj_Run100", "obj_Run101", "obj_Run102", "obj_Run103", "obj_Run104", "obj_Run105", "obj_Run106", "obj_Run107" }; 
    std::string jumpL2R[] = { "obj_Run004" };
    std::string jumpR2L[] = { "obj_Run104" };
    std::string flyL2R[] = { "obj_Flying000", "obj_Flying001" };
    std::string flyR2L[] = { "obj_Flying100", "obj_Flying101" };

    theAnimationSystem.registerAnim("runL2R", runL2R, 8, 16);
    theAnimationSystem.registerAnim("runR2L", runR2L, 8, 16);
    theAnimationSystem.registerAnim("jumpL2R", jumpL2R, 1, 0);
    theAnimationSystem.registerAnim("jumpR2L", jumpR2L, 1, 0);
    theAnimationSystem.registerAnim("flyL2R", flyL2R, 2, 3);
    theAnimationSystem.registerAnim("flyR2L", flyR2L, 2, 3);
    
    // init font
    loadFont(asset, "typo");
}

void PrototypeGame::init(const uint8_t* in, int size) {
    RunnerSystem::CreateInstance();
    CameraTargetSystem::CreateInstance();
    GameSystem::CreateInstance();
    PlayerSystem::CreateInstance();

    background = theEntityManager.CreateEntity();
    ADD_COMPONENT(background, Transformation);
    TRANSFORM(background)->size = Vector2(LEVEL_SIZE * PlacementHelper::ScreenWidth, 0.7 * PlacementHelper::ScreenHeight);
    TRANSFORM(background)->position.X = 0;
    TRANSFORM(background)->position.Y = -(PlacementHelper::ScreenHeight - TRANSFORM(background)->size.Y)*0.5;
    TRANSFORM(background)->z = 0.1;
    ADD_COMPONENT(background, Rendering);
    RENDERING(background)->color = Color(0.3, 0.3, 0.3);
    RENDERING(background)->hide = false;
    RENDERING(background)->opaqueType = RenderingComponent::FULL_OPAQUE;

    startSingleButton = theEntityManager.CreateEntity();
    ADD_COMPONENT(startSingleButton, Transformation);
    TRANSFORM(startSingleButton)->position = Vector2(-PlacementHelper::ScreenWidth /6, 0);
    TRANSFORM(startSingleButton)->z = 0.9;
    ADD_COMPONENT(startSingleButton, TextRendering);
    TEXT_RENDERING(startSingleButton)->text = "Jouer solo";
    TEXT_RENDERING(startSingleButton)->charHeight = 1;
    ADD_COMPONENT(startSingleButton, Container);
    CONTAINER(startSingleButton)->entities.push_back(startSingleButton);
    CONTAINER(startSingleButton)->includeChildren = true;
    ADD_COMPONENT(startSingleButton, Rendering);
    RENDERING(startSingleButton)->color = Color(0.2, 0.2, 0.2, 0.5);
    ADD_COMPONENT(startSingleButton, Button);

    startMultiButton = theEntityManager.CreateEntity();
    ADD_COMPONENT(startMultiButton, Transformation);
    TRANSFORM(startMultiButton)->position = Vector2(PlacementHelper::ScreenWidth /6, 0);
    TRANSFORM(startMultiButton)->z = 0.9;
    ADD_COMPONENT(startMultiButton, TextRendering);
    TEXT_RENDERING(startMultiButton)->text = "Jouer multi";
    TEXT_RENDERING(startMultiButton)->charHeight = 1;
    ADD_COMPONENT(startMultiButton, Container);
    CONTAINER(startMultiButton)->entities.push_back(startMultiButton);
    CONTAINER(startMultiButton)->includeChildren = true;
    ADD_COMPONENT(startMultiButton, Rendering);
    RENDERING(startMultiButton)->color = Color(0.2, 0.2, 0.2, 0.5);
    ADD_COMPONENT(startMultiButton, Button);
    
    scoreText = theEntityManager.CreateEntity();
    ADD_COMPONENT(scoreText, Transformation);
    TRANSFORM(scoreText)->position = Vector2(0, 0.35 * PlacementHelper::ScreenHeight);
    TRANSFORM(scoreText)->z = 0.9;
    ADD_COMPONENT(scoreText, TextRendering);
    TEXT_RENDERING(scoreText)->text = "";
    TEXT_RENDERING(scoreText)->charHeight = 1;
    
    transitionPlayingMenu();
}


void PrototypeGame::backPressed() {

}

void PrototypeGame::togglePause(bool activate) {

}

void PrototypeGame::tick(float dt) {
	theTouchInputManager.Update(dt);
 
    GameState next;
    switch(gameState) {
        case Menu:
            next = updateMenu(dt);
            break;
        case WaitingPlayers:
            next = updateWaitingPlayers(dt);
            break;
        case Playing:
            next = updatePlaying(dt);
            break;
    }
    
    if (next != gameState) {
        switch(gameState) {
            case Menu:
                if (next == WaitingPlayers)
                    transitionMenuWaitingPlayers();
                break;
            case WaitingPlayers:
                if (next == Playing)
                    transitionWaitingPlayersPlaying();
                break;
            case Playing:
                if (next == Menu)
                    transitionPlayingMenu();
                break;
        }
        gameState = next;
    }

    // limit cam pos
    if (theRenderingSystem.cameraPosition.X < - PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5)) {
        theRenderingSystem.cameraPosition.X = - PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5);
    } else if (theRenderingSystem.cameraPosition.X > PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5)) {
        theRenderingSystem.cameraPosition.X = PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5);
    }
    

    // systems update
    theNetworkSystem.Update(dt);
	theADSRSystem.Update(dt);
    theAnimationSystem.Update(dt);
	theButtonSystem.Update(dt);
    theParticuleSystem.Update(dt);
	theMorphingSystem.Update(dt);
	thePhysicsSystem.Update(dt);
	theScrollingSystem.Update(dt);
    theTextRenderingSystem.Update(dt);
	theSoundSystem.Update(dt);
    theMusicSystem.Update(dt);
    theTransformationSystem.Update(dt);
    theContainerSystem.Update(dt);
    theAutoDestroySystem.Update(dt);
    theRenderingSystem.Update(dt);

    updateFps(dt);
}

static GameState updateMenu(float dt) {
    if (BUTTON(startSingleButton)->clicked) {
        gameTempVars.numPlayers = 1;
        gameTempVars.isGameMaster = true;
        return WaitingPlayers;
    } else if (BUTTON(startMultiButton)->clicked) {
        TEXT_RENDERING(startMultiButton)->text = "Finding opp.";
        NetworkAPILinuxImpl* net = new NetworkAPILinuxImpl();
        net->connectToLobby("my_name", "66.228.34.226");//127.0.0.1");
        theNetworkSystem.networkAPI = net;
        gameTempVars.numPlayers = 2;
        gameTempVars.isGameMaster = false;
        return WaitingPlayers;
    }
    return Menu;
}

static void transitionMenuWaitingPlayers() {
    BUTTON(startSingleButton)->enabled = false;
    BUTTON(startMultiButton)->enabled = false;
    theNetworkSystem.deleteAllNonLocalEntities();
    // Create coins for next game
    if (gameTempVars.isGameMaster) {
        createCoins(20);
    }
}

static GameState updateWaitingPlayers(float dt) {
    if (theNetworkSystem.networkAPI) {
        if (theNetworkSystem.networkAPI->isConnectedToAnotherPlayer()) {
            gameTempVars.isGameMaster = theNetworkSystem.networkAPI->amIGameMaster();
        }
    }
    gameTempVars.players = thePlayerSystem.RetrieveAllEntityWithComponent();
    if (gameTempVars.players.size() != gameTempVars.numPlayers) {
        // create both players
        if (gameTempVars.isGameMaster) {
            for (unsigned i=0; i<gameTempVars.numPlayers; i++) {
                Entity e = theEntityManager.CreateEntity();
                ADD_COMPONENT(e, Player);
                ADD_COMPONENT(e, Network);
                NETWORK(e)->systemUpdatePeriod[thePlayerSystem.getName()] = 0;

                addRunnerToPlayer(e, PLAYER(e));
            }
        }
        return WaitingPlayers;
    }
    PLAYER(gameTempVars.players[gameTempVars.isGameMaster ? 0 : 1])->ready = true;
    for (std::vector<Entity>::iterator it=gameTempVars.players.begin(); it!=gameTempVars.players.end(); ++it) {
        if (!PLAYER(*it)->ready) {
            return WaitingPlayers;
        }
    }
    return Playing;
}

static void transitionWaitingPlayersPlaying() {
    // store a few entities to avoid permanent lookups
    gameTempVars.syncCoins();
    gameTempVars.syncRunners();

    TEXT_RENDERING(scoreText)->hide = false;
    TEXT_RENDERING(startSingleButton)->hide = true;
    RENDERING(startSingleButton)->hide = true;
    TEXT_RENDERING(startMultiButton)->hide = true;
    RENDERING(startMultiButton)->hide = true;
    // hmm
    theRenderingSystem.cameraPosition.X = TRANSFORM(gameTempVars.currentRunner)->position.X + PlacementHelper::ScreenWidth * 0.5;
}

static void transitionWaitingPlayersMenu() {
    // bah
}

static GameState updatePlaying(float dt) {
    int myPlayerIndex = gameTempVars.playerIndex();
    PlayerComponent* myPlayer = PLAYER(gameTempVars.players[myPlayerIndex]);
    gameTempVars.syncRunners();

    // Manage player's current runner
    if (!gameTempVars.runners[myPlayerIndex].empty()) {
        // If current runner has reached the edge of the screen
        if (RUNNER(gameTempVars.currentRunner)->finished) {
            if (myPlayer->runnersCount == 10) {
                CAM_TARGET(gameTempVars.currentRunner)->enabled = false;
                theRenderingSystem.cameraPosition = Vector2::Zero;
                // end of game
                // resetGame();
                return Menu;
            } else {
                CAM_TARGET(gameTempVars.currentRunner)->enabled = false;
                // add a new one
                gameTempVars.currentRunner = addRunnerToPlayer(gameTempVars.players[myPlayerIndex], myPlayer);
            }
        }

        // Input (jump) handling
        RunnerComponent* rc = RUNNER(gameTempVars.currentRunner);
        PhysicsComponent* pc = PHYSICS(gameTempVars.currentRunner);
        if (pc->gravity.Y >= 0) {
            if (theTouchInputManager.isTouched()) {
                if (!theTouchInputManager.wasTouched()) {
                    if (rc->jumpingSince <= 0) {
                        rc->jumpTimes.push_back(rc->elapsed);
                        rc->jumpDurations.push_back(0.001);
                    }
                } else if (!rc->jumpTimes.empty()) {
                    float& d = *(rc->jumpDurations.rbegin());
                    if (d < MaxJumpDuration) {
                        d += dt;
                    }
                }
            }
        }

        TransformationComponent* tc = TRANSFORM(gameTempVars.currentRunner);
        CAM_TARGET(gameTempVars.currentRunner)->offset.Y = 0 - tc->position.Y;
    }

    if (gameTempVars.isGameMaster) { // maybe do it for non master too (but do not delete entities, maybe only hide ?)
        std::vector<TransformationComponent*> actives;
        // check for collisions for non-ghost runners
        for (unsigned i=0; i<gameTempVars.numPlayers; i++) {
            for (unsigned j=0; j<gameTempVars.runners[i].size(); j++) {
                if (RUNNER(gameTempVars.runners[i][j])->ghost)
                    continue;
                actives.push_back(TRANSFORM(gameTempVars.runners[i][j]));
            }
        }
        const unsigned count = actives.size();
        for (unsigned i=0; i<gameTempVars.numPlayers; i++) {
            for (unsigned j=0; j<gameTempVars.runners[i].size(); j++) {
                Entity ghost = gameTempVars.runners[i][j];
                TransformationComponent* ghostTc = TRANSFORM(ghost);
                for (unsigned k=0; k<count; k++) {
                    if (ghostTc == actives[k]) continue;
                    if (IntersectionUtil::rectangleRectangle(ghostTc, actives[k])) {
                        RUNNER(ghost)->killed = true;
                        gameTempVars.runners[i].erase(gameTempVars.runners[i].begin() + j);
                        j--;
                        break;
                    }
                }
            }
        }
    }

    for (unsigned i=0; i<gameTempVars.numPlayers; i++) {
        for (unsigned j=0; j<gameTempVars.runners[i].size(); j++) {
            Entity e = gameTempVars.runners[i][j];
            RunnerComponent* rc = RUNNER(e);
            if (rc->killed)
                continue;
            TransformationComponent* tc = TRANSFORM(e);
            PhysicsComponent* pc = PHYSICS(e);
    
            // check jumps
            if (pc->gravity.Y < 0) {
                if ((tc->position.Y - tc->size.Y * 0.5) <= -PlacementHelper::ScreenHeight * 0.5) {
                    pc->gravity.Y = 0;
                    pc->linearVelocity = Vector2::Zero;
                    tc->position.Y = -PlacementHelper::ScreenHeight * 0.5 + tc->size.Y * 0.5;
                    ANIMATION(e)->name = (rc->speed > 0) ? "runL2R" : "runR2L";
                }
            }
            // check coins
            for (std::vector<Entity>::iterator it=gameTempVars.coins.begin(); it!=gameTempVars.coins.end(); ++it) {
                Entity coin = *it;
                if (std::find(rc->coins.begin(), rc->coins.end(), coin) == rc->coins.end()) {
                    if (IntersectionUtil::rectangleRectangle(tc, TRANSFORM(coin))) {
                        rc->coins.push_back(coin);
                        int gain = 10; // TODO ((coin == goldCoin) ? 30 : 10) * pow(2, player.size() - i - 1);
                        myPlayer->score += gain;
                        spawnGainEntity(gain, TRANSFORM(coin)->position);
                    }
                }
            }
        }
    }
    
    std::stringstream a;
    a << "Score: ";
    for (unsigned i=0; i<gameTempVars.players.size(); i++) {
        a << "  " << PLAYER(gameTempVars.players[i])->score;
    }
    TEXT_RENDERING(scoreText)->text = a.str();
        
    theGameSystem.Update(dt);
    thePlayerSystem.Update(dt);
    theRunnerSystem.Update(dt);
    theCameraTargetSystem.Update(dt);

    return Playing;
}

static void transitionPlayingMenu() {
    // Restore camera position
    theRenderingSystem.cameraPosition = Vector2::Zero;
    // Show menu UI
    TEXT_RENDERING(startSingleButton)->hide = false;
    CONTAINER(startSingleButton)->enable = true;
    RENDERING(startSingleButton)->hide = false;
    BUTTON(startSingleButton)->enabled = true;
    TEXT_RENDERING(startMultiButton)->hide = false;
    CONTAINER(startMultiButton)->enable = true;
    RENDERING(startMultiButton)->hide = false;
    BUTTON(startMultiButton)->enabled = true;
    TEXT_RENDERING(scoreText)->hide = false;
    // Cleanup previous game variables
    gameTempVars.cleanup();
}


void GameTempVar::cleanup() {
    for (unsigned i=0; i<coins.size(); i++) {
        theEntityManager.DeleteEntity(coins[i]);
    }
    coins.clear();
    for (unsigned i=0; i<gameTempVars.numPlayers; i++) {
        for (unsigned j=0; j<runners[i].size(); j++) {
            theEntityManager.DeleteEntity(runners[i][j]);
        }
        runners[i].clear();
        theEntityManager.DeleteEntity(players[i]);
    }
}

void GameTempVar::syncRunners() {
    std::vector<Entity> r = theRunnerSystem.RetrieveAllEntityWithComponent();
    for (unsigned i=0; i<players.size(); i++) {
        for (unsigned j=0; j<r.size(); j++) {
            if (RUNNER(r[j])->playerOwner == gameTempVars.players[i]) {
                runners[i].push_back(r[j]);
                break;
            }
        }
    }
    currentRunner = *(runners[playerIndex()].rbegin());
}

void GameTempVar::syncCoins() {
    std::vector<Entity> t = theTransformationSystem.RetrieveAllEntityWithComponent();
    for (unsigned i=0; i<t.size(); i++) {
        //...
        float x = TRANSFORM(t[i])->size.X;
        if (MathUtil::Abs(x - 0.3) < 0.01) {
            coins.push_back(t[i]);
        }
    }
}

int GameTempVar::playerIndex() {
    return (isGameMaster ? 0 : 1);
}

static void createCoins(int count) {
    float min = LEVEL_SIZE * PlacementHelper::ScreenWidth;
    for (int i=0; i<count; i++) {
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
        if (MathUtil::Abs(TRANSFORM(e)->position.X) < min) {
            goldCoin = e;
            min = MathUtil::Abs(TRANSFORM(e)->position.X);
        }
        ADD_COMPONENT(e, Network);
        NETWORK(e)->systemUpdatePeriod[theTransformationSystem.getName()] = 0;
        NETWORK(e)->systemUpdatePeriod[theRenderingSystem.getName()] = 0;
    }
    
    RENDERING(goldCoin)->color = Color(0, 1, 0);
}

static void updateFps(float dt) {
    #define COUNT 1000
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
    ADD_COMPONENT(e, AutoDestroy);
    AUTO_DESTROY(e)->type = AutoDestroyComponent::LIFETIME;
    AUTO_DESTROY(e)->params.lifetime.value = 1;
    AUTO_DESTROY(e)->params.lifetime.map2AlphaTextRendering = true;
    AUTO_DESTROY(e)->hasTextRendering = true;
}

static Entity addRunnerToPlayer(Entity player, PlayerComponent* p) {
    int direction = (p->runnersCount % 2) ? -1 : 1;
    Entity e = theEntityManager.CreateEntity();
    ADD_COMPONENT(e, Transformation);
    TRANSFORM(e)->position = Vector2(-9, 2);
    TRANSFORM(e)->size = Vector2(0.85,1);//0.4,1);//0.572173, 0.815538);
    TRANSFORM(e)->rotation = 0;
    TRANSFORM(e)->z = 0.8;
    ADD_COMPONENT(e, Rendering);
    // RENDERING(e)->color = Color::random();
    RENDERING(e)->hide = false;
    ADD_COMPONENT(e, Runner);
    TRANSFORM(e)->position = RUNNER(e)->startPoint = Vector2(
        direction * -LEVEL_SIZE * 0.5 * PlacementHelper::ScreenWidth,
        -0.5 * PlacementHelper::ScreenHeight + TRANSFORM(e)->size.Y * 0.5);
    RUNNER(e)->endPoint = RUNNER(e)->startPoint + Vector2(direction * LEVEL_SIZE * PlacementHelper::ScreenWidth, 0);
    RUNNER(e)->maxSpeed = RUNNER(e)->speed = direction * playerSpeed * (1 + 0.05 * p->runnersCount);
    RUNNER(e)->startTime = 0;//MathUtil::RandomFloatInRange(1,3);
    RUNNER(e)->playerOwner = player;
    ADD_COMPONENT(e, CameraTarget);
    CAM_TARGET(e)->enabled = true;
    CAM_TARGET(e)->maxCameraSpeed = direction * RUNNER(e)->speed;
    CAM_TARGET(e)->offset = Vector2(
        direction * 0.4 * PlacementHelper::ScreenWidth, 
        0 - TRANSFORM(e)->position.Y);
    ADD_COMPONENT(e, Physics);
    PHYSICS(e)->mass = 1;
    ADD_COMPONENT(e, Animation);
    ANIMATION(e)->name = (direction > 0) ? "runL2R" : "runR2L";

    ADD_COMPONENT(e, Network);
    NETWORK(e)->systemUpdatePeriod[theTransformationSystem.getName()] = 0.016;
    NETWORK(e)->systemUpdatePeriod[theRunnerSystem.getName()] = 0.016;
    NETWORK(e)->systemUpdatePeriod[thePhysicsSystem.getName()] = 0.016;
    NETWORK(e)->systemUpdatePeriod[theRenderingSystem.getName()] = 0;
        
    p->runnersCount++;
    std::cout << "Add player " << e << " at pos : " << TRANSFORM(e)->position << ", speed= " << RUNNER(e)->speed << std::endl;
    return e;
}