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
#include "RecursiveRunnerGame.h"
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
#ifdef SAC_NETWORK
#include "systems/NetworkSystem.h"
#include "api/linux/NetworkAPILinuxImpl.h"
#endif
#include "systems/RunnerSystem.h"
#include "systems/CameraTargetSystem.h"
#include "systems/PlayerSystem.h"

#include <cmath>
#include <vector>



#ifndef EMSCRIPTEN
// #define IN_GAME_EDITOR 0
#endif
#if IN_GAME_EDITOR
#include <GL/glfw.h>
#endif

enum CameraMode {
    CameraModeMenu,
    CameraModeSingle,
    CameraModeSplit
};
    

static void spawnGainEntity(int gain, const Vector2& pos);
static Entity addRunnerToPlayer(Entity player, PlayerComponent* p, int playerIndex);
static void updateFps(float dt);
static void setupCamera(CameraMode mode);


// PURE LOCAL VARS
Entity background, startSingleButton, startSplitButton;
#ifdef SAC_NETWORK
Entity startMultiButton;
Entity networkUL, networkDL;
#endif
Entity scoreText[2], goldCoin;


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
    Entity currentRunner[2];
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


RecursiveRunnerGame::RecursiveRunnerGame(AssetAPI* ast, NameInputAPI* inputUI, LocalizeAPI* lAPI, AdAPI* ad, ExitAPI* exAPI) : Game() {
	asset = ast;
	exitAPI = exAPI;
}

void RecursiveRunnerGame::sacInit(int windowW, int windowH) {
    PlacementHelper::GimpWidth = 800;
    PlacementHelper::GimpHeight = 500;

    Game::sacInit(windowW, windowH);
    theRenderingSystem.loadAtlas("alphabet", true);
    theRenderingSystem.loadAtlas("dummy", false);
    theRenderingSystem.loadAtlas("decor", false);
    
    // register 4 animations
    std::string runL2R[] = { "run_l2r_0002",
        "run_l2r_0003", "run_l2r_0004", "run_l2r_0005",
        "run_l2r_0006", "run_l2r_0007", "run_l2r_0008",
        "run_l2r_0009", "run_l2r_0010", "run_l2r_0011", "run_l2r_0000", "run_l2r_0001"};
    std::string jumpL2R[] = { "jump_l2r_0004", "jump_l2r_0005",
        "jump_l2r_0006", "jump_l2r_0007", "jump_l2r_0008",
        "jump_l2r_0009", "jump_l2r_0010", "jump_l2r_0011"};
    std::string jumpL2Rtojump[] = { "jump_l2r_0012", "jump_l2r_0013", "jump_l2r_0015"};

    theAnimationSystem.registerAnim("runL2R", runL2R, 12, 20, true);
    theAnimationSystem.registerAnim("jumpL2R_up", jumpL2R, 6, 20, false);
    theAnimationSystem.registerAnim("jumpL2R_down", &jumpL2R[6], 2, 20, false);
    theAnimationSystem.registerAnim("jumptorunL2R", jumpL2Rtojump, 3, 40, false, "runL2R");
    

    // init font
    loadFont(asset, "typo");
}

void RecursiveRunnerGame::init(const uint8_t* in, int size) {
    RunnerSystem::CreateInstance();
    CameraTargetSystem::CreateInstance();
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
    RENDERING(background)->cameraBitMask = (0x3 << 1);

    startSingleButton = theEntityManager.CreateEntity();
    ADD_COMPONENT(startSingleButton, Transformation);
    TRANSFORM(startSingleButton)->position = Vector2(-PlacementHelper::ScreenWidth /6, 0);
    TRANSFORM(startSingleButton)->z = 0.9;
    ADD_COMPONENT(startSingleButton, TextRendering);
    TEXT_RENDERING(startSingleButton)->text = "Jouer solo";
    TEXT_RENDERING(startSingleButton)->charHeight = 1;
    TEXT_RENDERING(startSingleButton)->cameraBitMask = 0x1;
    ADD_COMPONENT(startSingleButton, Container);
    CONTAINER(startSingleButton)->entities.push_back(startSingleButton);
    CONTAINER(startSingleButton)->includeChildren = true;
    ADD_COMPONENT(startSingleButton, Rendering);
    RENDERING(startSingleButton)->color = Color(0.8, 0.8, 0.2, 0.5);
    RENDERING(startSingleButton)->cameraBitMask = 0x1;
    ADD_COMPONENT(startSingleButton, Button);
    BUTTON(startSingleButton)->overSize = 2;

    startSplitButton = theEntityManager.CreateEntity();
    ADD_COMPONENT(startSplitButton, Transformation);
    TRANSFORM(startSplitButton)->position = Vector2(PlacementHelper::ScreenWidth /6, 0);
    TRANSFORM(startSplitButton)->z = 0.9;
    ADD_COMPONENT(startSplitButton, TextRendering);
    TEXT_RENDERING(startSplitButton)->text = "Splitscreen";
    TEXT_RENDERING(startSplitButton)->charHeight = 1;
    TEXT_RENDERING(startSplitButton)->cameraBitMask = 0x1;
    ADD_COMPONENT(startSplitButton, Container);
    CONTAINER(startSplitButton)->entities.push_back(startSplitButton);
    CONTAINER(startSplitButton)->includeChildren = true;
    ADD_COMPONENT(startSplitButton, Rendering);
    RENDERING(startSplitButton)->color = Color(0.8, 0.8, 0.2, 0.5);
    RENDERING(startSplitButton)->cameraBitMask = 0x1;
    ADD_COMPONENT(startSplitButton, Button);
    BUTTON(startSplitButton)->overSize = 2;

#ifdef SAC_NETWORK
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
#endif
    for (int i=0; i<2; i++) {
        scoreText[i] = theEntityManager.CreateEntity();
        ADD_COMPONENT(scoreText[i], Transformation);
        TRANSFORM(scoreText[i])->position = Vector2(0, 0.35 * PlacementHelper::ScreenHeight);
        TRANSFORM(scoreText[i])->z = 0.9;
        ADD_COMPONENT(scoreText[i], TextRendering);
        TEXT_RENDERING(scoreText[i])->text = "";
        TEXT_RENDERING(scoreText[i])->charHeight = 1;
        // TEXT_RENDERING(scoreText[i])->cameraBitMask = 0x3 << 1;
        TEXT_RENDERING(scoreText[i])->color = Color(1 - i, i, 1);
    }
#ifdef SAC_NETWORK
    networkUL = theEntityManager.CreateEntity();
    ADD_COMPONENT(networkUL, Transformation);
    TRANSFORM(networkUL)->position = Vector2(-PlacementHelper::ScreenWidth * 0.5, 0.46 * PlacementHelper::ScreenHeight);
    TRANSFORM(networkUL)->z = 0.9;
    ADD_COMPONENT(networkUL, TextRendering);
    TEXT_RENDERING(networkUL)->text = "";
    TEXT_RENDERING(networkUL)->charHeight = 0.35;
    TEXT_RENDERING(networkUL)->color = Color(0, 1, 0);
    TEXT_RENDERING(networkUL)->positioning = TextRenderingComponent::LEFT;

    networkDL = theEntityManager.CreateEntity();
    ADD_COMPONENT(networkDL, Transformation);
    TRANSFORM(networkDL)->position = Vector2(-PlacementHelper::ScreenWidth * 0.5, 0.43 * PlacementHelper::ScreenHeight);
    TRANSFORM(networkDL)->z = 0.9;
    ADD_COMPONENT(networkDL, TextRendering);
    TEXT_RENDERING(networkDL)->text = "";
    TEXT_RENDERING(networkDL)->charHeight = 0.35;
    TEXT_RENDERING(networkDL)->color = Color(1, 0, 0);
    TEXT_RENDERING(networkDL)->positioning = TextRenderingComponent::LEFT;
#endif
    transitionPlayingMenu();

/*
    theRenderingSystem.cameras[0].worldSize.Y *= 0.50;
    theRenderingSystem.cameras[0].worldPosition.Y -= theRenderingSystem.cameras[0].worldSize.Y * 0.5;
    theRenderingSystem.cameras[0].screenSize.Y *= 0.50;
    theRenderingSystem.cameras[0].screenPosition.Y  = 0.25;
*/
    // 3 cameras
    // Default camera (UI)
    RenderingSystem::Camera cam = theRenderingSystem.cameras[0];
    cam.enable = false;
    // 1st player
    theRenderingSystem.cameras.push_back(cam);
    // 2nd player
    theRenderingSystem.cameras.push_back(cam);
}


void RecursiveRunnerGame::backPressed() {
    Game::backPressed();
}

void RecursiveRunnerGame::togglePause(bool activate) {

}

void RecursiveRunnerGame::tick(float dt) {
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
    for (unsigned i=0; i<theRenderingSystem.cameras.size(); i++) {
        float& camPosX = theRenderingSystem.cameras[i].worldPosition.X;

        if (camPosX < - PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5)) {
            camPosX = - PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5);
        } else if (camPosX > PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5)) {
            camPosX = PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5);
        }
    }
    

    // systems update
#ifdef SAC_NETWORK
    theNetworkSystem.Update(dt);
#endif
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
        setupCamera(CameraModeSingle);
        return WaitingPlayers;
    } else if (BUTTON(startSplitButton)->clicked) {
        setupCamera(CameraModeSplit);
        gameTempVars.numPlayers = 2;
        gameTempVars.isGameMaster = true;
        return WaitingPlayers;
    }
#ifdef SAC_NETWORK
    else if (BUTTON(startMultiButton)->clicked) {
        TEXT_RENDERING(startMultiButton)->text = "Finding opp.";
        TEXT_RENDERING(networkUL)->hide = false;
        TEXT_RENDERING(networkDL)->hide = false;
        NetworkAPILinuxImpl* net = new NetworkAPILinuxImpl();
        net->connectToLobby("my_name", "127.0.0.1"); //66.228.34.226");//127.0.0.1");
        theNetworkSystem.networkAPI = net;
        gameTempVars.numPlayers = 2;
        gameTempVars.isGameMaster = false;
        return WaitingPlayers;
    }
#endif
    return Menu;
}

static void transitionMenuWaitingPlayers() {
    LOGI("Change state");
    BUTTON(startSingleButton)->enabled = false;
    BUTTON(startSplitButton)->enabled = false;
#ifdef SAC_NETWORK
    BUTTON(startMultiButton)->enabled = false;
    theNetworkSystem.deleteAllNonLocalEntities();
#endif
}

static GameState updateWaitingPlayers(float dt) {
#ifdef SAC_NETWORK
    if (theNetworkSystem.networkAPI) {
        if (theNetworkSystem.networkAPI->isConnectedToAnotherPlayer()) {
            gameTempVars.isGameMaster = theNetworkSystem.networkAPI->amIGameMaster();
            if (gameTempVars.isGameMaster) {
                TEXT_RENDERING(startMultiButton)->text = "Connected S";
            } else {
                TEXT_RENDERING(startMultiButton)->text = "Connected C";
            }
        }
    }
#endif
    gameTempVars.players = thePlayerSystem.RetrieveAllEntityWithComponent();
    if (gameTempVars.players.size() != gameTempVars.numPlayers) {
        // create both players
        if (gameTempVars.isGameMaster) {
            for (unsigned i=0; i<gameTempVars.numPlayers; i++) {
                Entity e = theEntityManager.CreateEntity();
                ADD_COMPONENT(e, Player);
                #ifdef SAC_NETWORK
                ADD_COMPONENT(e, Network);
                NETWORK(e)->systemUpdatePeriod[thePlayerSystem.getName()] = 0.1;
                #endif
                Entity run = addRunnerToPlayer(e, PLAYER(e), i);
                #ifdef SAC_NETWORK
                if (i != gameTempVars.playerIndex()) {
                    NETWORK(run)->newOwnerShipRequest = 1;
                }
                #endif
            }
            // Create coins for next game
            createCoins(20);
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
    LOGI("Change state");
    // store a few entities to avoid permanent lookups
    gameTempVars.syncCoins();
    gameTempVars.syncRunners();

    //TEXT_RENDERING(scoreText)->hide = true;
    TEXT_RENDERING(startSingleButton)->hide = true;
    TEXT_RENDERING(startSplitButton)->hide = true;
    RENDERING(startSingleButton)->hide = true;
    RENDERING(startSplitButton)->hide = true;
#ifdef SAC_NETWORK
    TEXT_RENDERING(startMultiButton)->hide = true;
    RENDERING(startMultiButton)->hide = true;
#endif
    for (unsigned i=0; i<gameTempVars.numPlayers; i++) {
        theRenderingSystem.cameras[1 + i].worldPosition.X = 
            TRANSFORM(gameTempVars.currentRunner[i])->position.X + PlacementHelper::ScreenWidth * 0.5;
    }
}

static void transitionWaitingPlayersMenu() {
    // bah
}

static GameState updatePlaying(float dt) {
    gameTempVars.syncRunners();

    // Manage player's current runner
    for (unsigned i=0; i<gameTempVars.numPlayers; i++) {
        CAM_TARGET(gameTempVars.currentRunner[i])->enabled = true;
        CAM_TARGET(gameTempVars.currentRunner[i])->offset = Vector2(
            ((RUNNER(gameTempVars.currentRunner[i])->speed > 0) ? 1 :-1) * 0.4 * PlacementHelper::ScreenWidth, 
            0 - TRANSFORM(gameTempVars.currentRunner[i])->position.Y);
        
        // If current runner has reached the edge of the screen
        if (RUNNER(gameTempVars.currentRunner[i])->finished) {
            std::cout << gameTempVars.currentRunner[i] << " finished, add runner or end game" << std::endl;
            CAM_TARGET(gameTempVars.currentRunner[i])->enabled = false;
            // return Runner control to master
            #ifdef SAC_NETWORK
            if (!gameTempVars.isGameMaster) {
                std::cout << "Give back ownership of " << gameTempVars.currentRunner[i] << " to server" << std::endl;
                NETWORK(gameTempVars.currentRunner[i])->newOwnerShipRequest = 0;
            }
            #endif
            if (PLAYER(gameTempVars.players[i])->runnersCount == 10) {
                theRenderingSystem.cameras[0].worldPosition = Vector2::Zero;
                // end of game
                // resetGame();
                return Menu;
            } else {
                std::cout << "Create runner" << std::endl;
                // add a new one
                gameTempVars.currentRunner[i] = addRunnerToPlayer(gameTempVars.players[i], PLAYER(gameTempVars.players[i]), i);
            }
        }

        // Input (jump) handling
        for (int j=0; j<2; j++) {
            if (theTouchInputManager.isTouched(j)) {
                if (gameTempVars.numPlayers == 2) {
                    const Vector2& ppp = theTouchInputManager.getTouchLastPosition(j);
                    if (i == 0 && ppp.Y < 0)
                        continue;
                    if (i == 1 && ppp.Y > 0)
                        continue;
                }
                PhysicsComponent* pc = PHYSICS(gameTempVars.currentRunner[i]);
                if (pc->gravity.Y >= 0) {
                    RunnerComponent* rc = RUNNER(gameTempVars.currentRunner[i]);
                    
                    if (!theTouchInputManager.wasTouched(j)) {
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
                break;
            }
        }

        TransformationComponent* tc = TRANSFORM(gameTempVars.currentRunner[i]);
        CAM_TARGET(gameTempVars.currentRunner[i])->offset.Y = 0 - tc->position.Y;
    }

    if (gameTempVars.isGameMaster) { // maybe do it for non master too (but do not delete entities, maybe only hide ?)
        std::vector<TransformationComponent*> actives;
        std::vector<int> direction;
        // check for collisions for non-ghost runners
        for (unsigned i=0; i<gameTempVars.numPlayers; i++) {
            for (unsigned j=0; j<gameTempVars.runners[i].size(); j++) {
                const Entity r = gameTempVars.runners[i][j];
                const RunnerComponent* rc = RUNNER(r);
                if (rc->ghost)
                    continue;
                actives.push_back(TRANSFORM(r));
                direction.push_back(rc->speed > 0 ? 1 : -1);
            }
        }
        const unsigned count = actives.size();
        for (unsigned i=0; i<gameTempVars.numPlayers; i++) {
            for (unsigned j=0; j<gameTempVars.runners[i].size(); j++) {
                Entity ghost = gameTempVars.runners[i][j];
                RunnerComponent* rc = RUNNER(ghost);
                if (!rc->ghost || rc->killed)
                    continue;
                TransformationComponent* ghostTc = TRANSFORM(ghost);
                for (unsigned k=0; k<count; k++) {
                    // we can only hit guys with opposite direction
                    if (rc->speed * direction[k] > 0)
                        continue;
                    if (rc->elapsed < 0.25)
                        continue;
                    const Vector2 pos = actives[k]->position + actives[k]->size * Vector2(0, 0.2);
                    const Vector2 size = actives[k]->size * Vector2(0.25, 0.6);
                    if (IntersectionUtil::rectangleRectangle(pos, size, 0, ghostTc->position, ghostTc->size * Vector2(0.25, 0.6), 0)) {
                        rc->killed = true;
                        break;
                    }
                }
            }
        }
    }

    for (unsigned i=0; i<gameTempVars.numPlayers; i++) {
        PlayerComponent* player = PLAYER(gameTempVars.players[i]);
        //std::cout << i << " -> " << gameTempVars.runners[i].size() << std::endl;
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
                    ANIMATION(e)->name = "jumptorunL2R";
                    RENDERING(e)->mirrorH = (rc->speed < 0);
                }
            }
            const Vector2 pos = tc->position + tc->size * Vector2(0, 0.2);
            const Vector2 size = tc->size * Vector2(0.25, 0.6);
            // check coins
            int end = gameTempVars.coins.size();
            Entity prev = 0;
            for(int idx=0; idx<end; idx++) {
                Entity coin = rc->speed > 0 ? gameTempVars.coins[idx] : gameTempVars.coins[end - idx - 1];
                if (std::find(rc->coins.begin(), rc->coins.end(), coin) == rc->coins.end()) {
                    if (IntersectionUtil::rectangleRectangle(pos, size, 0, TRANSFORM(coin)->position, TRANSFORM(coin)->size, 0)) {
                        if (!rc->coins.empty()) {
                            if (rc->coins.back() == prev) {
                                rc->coinSequenceBonus++;
                            } else {
                                rc->coinSequenceBonus = 1;
                            }
                        }
                        rc->coins.push_back(coin);
                        int gain = ((coin == goldCoin) ? 30 : 10) * pow(2.0f, rc->oldNessBonus) * rc->coinSequenceBonus;
                        player->score += gain;
                        spawnGainEntity(gain, TRANSFORM(coin)->position);
                    }
                }
                prev = coin;
            }
        }
    }

    for (unsigned i=0; i<gameTempVars.players.size(); i++) {
        std::stringstream a;
        a << "  " << PLAYER(gameTempVars.players[i])->score << " pts";
        TEXT_RENDERING(scoreText[i])->text = a.str();
    }
    

    thePlayerSystem.Update(dt);
    theRunnerSystem.Update(dt);
    theCameraTargetSystem.Update(dt);
#ifdef SAC_NETWORK
    {
        std::stringstream a;
        a << (int)theNetworkSystem.ulRate/1024 << "kops, " << theNetworkSystem.bytesSent / 1024 << " ko"; 
        TEXT_RENDERING(networkUL)->text = a.str();
        TRANSFORM(networkUL)->position = theRenderingSystem.cameraPosition + 
            Vector2(-PlacementHelper::ScreenWidth * 0.5, 0.46 * PlacementHelper::ScreenHeight);
    }
    {
        std::stringstream a;
        a << (int)theNetworkSystem.dlRate/1024 << "kops, " << theNetworkSystem.bytesReceived / 1024 << " ko"; 
        TEXT_RENDERING(networkDL)->text = a.str();
        TRANSFORM(networkDL)->position = theRenderingSystem.cameraPosition + 
            Vector2(-PlacementHelper::ScreenWidth * 0.5, 0.43 * PlacementHelper::ScreenHeight);
    }
#endif
    return Playing;
}

static void transitionPlayingMenu() {
    LOGI("Change state");
    setupCamera(CameraModeMenu);
    // Restore camera position
    for (unsigned i=0; i<theRenderingSystem.cameras.size(); i++) {
        theRenderingSystem.cameras[i].worldPosition = Vector2::Zero;
    }
    // Show menu UI
    TEXT_RENDERING(startSingleButton)->hide = false;
    CONTAINER(startSingleButton)->enable = true;
    RENDERING(startSingleButton)->hide = false;
    BUTTON(startSingleButton)->enabled = true;
    TEXT_RENDERING(startSplitButton)->hide = false;
    CONTAINER(startSplitButton)->enable = true;
    RENDERING(startSplitButton)->hide = false;
    BUTTON(startSplitButton)->enabled = true;
#ifdef SAC_NETWORK
    TEXT_RENDERING(startMultiButton)->hide = false;
    CONTAINER(startMultiButton)->enable = true;
    RENDERING(startMultiButton)->hide = false;
    BUTTON(startMultiButton)->enabled = true;
    TEXT_RENDERING(startMultiButton)->text = "Jouer multi";
#endif
    // TEXT_RENDERING(scoreText)->hide = false;
    // Cleanup previous game variables
    gameTempVars.cleanup();
}


void GameTempVar::cleanup() {
    for (unsigned i=0; i<coins.size(); i++) {
        theEntityManager.DeleteEntity(coins[i]);
    }
    coins.clear();
    std::vector<Entity> r = theRunnerSystem.RetrieveAllEntityWithComponent();
    for (unsigned j=0; j<r.size(); j++) {
        theEntityManager.DeleteEntity(r[j]);
    }

    for (unsigned i=0; i<players.size(); i++) {
        runners[i].clear();
        theEntityManager.DeleteEntity(players[i]);
    }
}

void GameTempVar::syncRunners() {
    std::vector<Entity> r = theRunnerSystem.RetrieveAllEntityWithComponent();
    for (unsigned i=0; i<players.size(); i++) {
        runners[i].clear();
        for (unsigned j=0; j<r.size(); j++) {
            RunnerComponent* rc = RUNNER(r[j]);
            if (rc->killed)
                continue;
            if (rc->playerOwner == gameTempVars.players[i]) {
                runners[i].push_back(r[j]);
                if (!rc->ghost) {
                        currentRunner[i] = r[j];
                }
            }
        }
    }
    if (currentRunner == 0) {
        LOGE("No current runner => bug. Nb players=%lu, nb runners=%lu",players.size(), r.size());
        for (unsigned i=0; i<players.size(); i++)
            LOGE("    runners[%d] = %lu", i, runners[i].size());
    }
}

static bool sortFromLeftToRight(Entity c1, Entity c2) {
    return TRANSFORM(c1)->position.X < TRANSFORM(c2)->position.X;
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
    std::sort(coins.begin(), coins.end(), sortFromLeftToRight);
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
        RENDERING(e)->cameraBitMask = (0x3 << 1);
        RENDERING(e)->hide = false;        
        if (MathUtil::Abs(TRANSFORM(e)->position.X) < min) {
            goldCoin = e;
            min = MathUtil::Abs(TRANSFORM(e)->position.X);
        }
        #ifdef SAC_NETWORK
        ADD_COMPONENT(e, Network);
        NETWORK(e)->systemUpdatePeriod[theTransformationSystem.getName()] = 0;
        NETWORK(e)->systemUpdatePeriod[theRenderingSystem.getName()] = 0;
        #endif
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
    TEXT_RENDERING(e)->cameraBitMask = (0x3 << 1);
    ADD_COMPONENT(e, Physics);
    PHYSICS(e)->mass = 1;
    PHYSICS(e)->gravity = Vector2(0, 6);
    ADD_COMPONENT(e, AutoDestroy);
    AUTO_DESTROY(e)->type = AutoDestroyComponent::LIFETIME;
    AUTO_DESTROY(e)->params.lifetime.value = 1;
    AUTO_DESTROY(e)->params.lifetime.map2AlphaTextRendering = true;
    AUTO_DESTROY(e)->hasTextRendering = true;
}

static Entity addRunnerToPlayer(Entity player, PlayerComponent* p, int playerIndex) {
    int direction = ((p->runnersCount + playerIndex) % 2) ? -1 : 1;
    Entity e = theEntityManager.CreateEntity();
    ADD_COMPONENT(e, Transformation);
    TRANSFORM(e)->position = Vector2(-9, 2);
    TRANSFORM(e)->size = Vector2(0.85, 1) * 3;//0.4,1);//0.572173, 0.815538);
    std::cout << PlacementHelper::ScreenHeight << std::endl;
    TRANSFORM(e)->rotation = 0;
    TRANSFORM(e)->z = 0.8 + 0.01 * p->runnersCount;
    ADD_COMPONENT(e, Rendering);
    RENDERING(e)->color = Color(1 - playerIndex, playerIndex, 1);
    RENDERING(e)->hide = false;
    RENDERING(e)->cameraBitMask = (0x3 << 1);
    ADD_COMPONENT(e, Runner);
    TRANSFORM(e)->position = RUNNER(e)->startPoint = Vector2(
        direction * -LEVEL_SIZE * 0.5 * PlacementHelper::ScreenWidth,
        -0.5 * PlacementHelper::ScreenHeight + TRANSFORM(e)->size.Y * 0.5);
    RUNNER(e)->endPoint = RUNNER(e)->startPoint + Vector2(direction * LEVEL_SIZE * PlacementHelper::ScreenWidth, 0);
    RUNNER(e)->speed = direction * playerSpeed * (1 + 0.05 * p->runnersCount);
    RUNNER(e)->startTime = 0;//MathUtil::RandomFloatInRange(1,3);
    RUNNER(e)->playerOwner = player;
    ADD_COMPONENT(e, CameraTarget);
    CAM_TARGET(e)->cameraIndex = 1 + playerIndex;
    CAM_TARGET(e)->maxCameraSpeed = direction * RUNNER(e)->speed;
    ADD_COMPONENT(e, Physics);
    PHYSICS(e)->mass = 1;
    ADD_COMPONENT(e, Animation);
    ANIMATION(e)->name = "runL2R";
    RENDERING(e)->mirrorH = (direction < 0);
#ifdef SAC_NETWORK
    ADD_COMPONENT(e, Network);
    NETWORK(e)->systemUpdatePeriod[theTransformationSystem.getName()] = 0.116;
    NETWORK(e)->systemUpdatePeriod[theRunnerSystem.getName()] = 0.016;
    NETWORK(e)->systemUpdatePeriod[thePhysicsSystem.getName()] = 0.116;
    NETWORK(e)->systemUpdatePeriod[theRenderingSystem.getName()] = 0;
    NETWORK(e)->systemUpdatePeriod[theAnimationSystem.getName()] = 0.1;
    NETWORK(e)->systemUpdatePeriod[theCameraTargetSystem.getName()] = 0.016;
#endif
	LOGI("Add runner %lu at pos : {%.2f, %.2f}, speed: %.2f (player=%lu)", e, TRANSFORM(e)->position.X, TRANSFORM(e)->position.Y, RUNNER(e)->speed, player);
    return e;
}

static void setupCamera(CameraMode mode) {
    switch (mode) {
        case CameraModeSingle:
            theRenderingSystem.cameras[0].enable = false;
            theRenderingSystem.cameras[1].enable = true;
            theRenderingSystem.cameras[2].enable = false;
            theRenderingSystem.cameras[1].worldSize.Y = PlacementHelper::ScreenHeight;
            theRenderingSystem.cameras[1].worldPosition.Y = 0;
            theRenderingSystem.cameras[1].screenSize.Y = 1;
            theRenderingSystem.cameras[1].screenPosition.Y  = 0;
            theRenderingSystem.cameras[1].mirrorY = false;
            TRANSFORM(scoreText[0])->position = Vector2(0, 0.35 * PlacementHelper::ScreenHeight);
            TEXT_RENDERING(scoreText[0])->hide = false;
            TEXT_RENDERING(scoreText[0])->positioning = TextRenderingComponent::CENTER;
            TEXT_RENDERING(scoreText[1])->hide = true;
            break;
        case CameraModeSplit:
            theRenderingSystem.cameras[0].enable = false;
            theRenderingSystem.cameras[1].enable = true;
            theRenderingSystem.cameras[2].enable = true;
            theRenderingSystem.cameras[1].worldSize.Y = PlacementHelper::ScreenHeight * 0.5;
            theRenderingSystem.cameras[1].worldPosition.Y = -PlacementHelper::ScreenHeight * 0.25;
            theRenderingSystem.cameras[1].screenSize.Y = 0.5;
            theRenderingSystem.cameras[1].screenPosition.Y  = 0.25;
            theRenderingSystem.cameras[1].mirrorY = true;
            theRenderingSystem.cameras[2].worldSize.Y = PlacementHelper::ScreenHeight * 0.5;
            theRenderingSystem.cameras[2].worldPosition.Y = -PlacementHelper::ScreenHeight * 0.25;
            theRenderingSystem.cameras[2].screenSize.Y = 0.5;
            theRenderingSystem.cameras[2].screenPosition.Y  = -0.25;
            TRANSFORM(scoreText[0])->position = Vector2(-PlacementHelper::ScreenWidth * 0.5, -0.1 * PlacementHelper::ScreenHeight);
            TRANSFORM(scoreText[1])->position = Vector2(PlacementHelper::ScreenWidth * 0.5, -0.1 * PlacementHelper::ScreenHeight);
            TEXT_RENDERING(scoreText[0])->hide = false;
            TEXT_RENDERING(scoreText[0])->positioning = TextRenderingComponent::LEFT;
            TEXT_RENDERING(scoreText[1])->hide = false;
            TEXT_RENDERING(scoreText[1])->positioning = TextRenderingComponent::RIGHT;
            break;
        case CameraModeMenu:
            theRenderingSystem.cameras[0].enable = true;
            theRenderingSystem.cameras[1].enable = false;
            theRenderingSystem.cameras[2].enable = false;
            break;
    }
}
