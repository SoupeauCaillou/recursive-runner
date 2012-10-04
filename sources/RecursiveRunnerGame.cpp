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

static void spawnGainEntity(int gain, const Vector2& pos);
static Entity addRunnerToPlayer(Entity player, PlayerComponent* p, int playerIndex);
static void updateFps(float dt);


// PURE LOCAL VARS
Entity background, startSingleButton;
#ifdef SAC_NETWORK
Entity startMultiButton;
Entity networkUL, networkDL;
#endif
Entity scoreText, goldCoin;


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
    scoreText = theEntityManager.CreateEntity();
    ADD_COMPONENT(scoreText, Transformation);
    TRANSFORM(scoreText)->position = Vector2(0, 0.35 * PlacementHelper::ScreenHeight);
    TRANSFORM(scoreText)->z = 0.9;
    ADD_COMPONENT(scoreText, TextRendering);
    TEXT_RENDERING(scoreText)->text = "";
    TEXT_RENDERING(scoreText)->charHeight = 1;
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
}


void RecursiveRunnerGame::backPressed() {

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
    if (theRenderingSystem.cameraPosition.X < - PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5)) {
        theRenderingSystem.cameraPosition.X = - PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5);
    } else if (theRenderingSystem.cameraPosition.X > PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5)) {
        theRenderingSystem.cameraPosition.X = PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5);
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

                Entity run = addRunnerToPlayer(e, PLAYER(e), i);
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

    TEXT_RENDERING(scoreText)->hide = false;
    TEXT_RENDERING(startSingleButton)->hide = true;
    RENDERING(startSingleButton)->hide = true;
#ifdef SAC_NETWORK
    TEXT_RENDERING(startMultiButton)->hide = true;
    RENDERING(startMultiButton)->hide = true;
#endif
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
        CAM_TARGET(gameTempVars.currentRunner)->enabled = true;
        CAM_TARGET(gameTempVars.currentRunner)->offset = Vector2(
            ((RUNNER(gameTempVars.currentRunner)->speed > 0) ? 1 :-1) * 0.4 * PlacementHelper::ScreenWidth, 
            0 - TRANSFORM(gameTempVars.currentRunner)->position.Y);
        
        // If current runner has reached the edge of the screen
        if (RUNNER(gameTempVars.currentRunner)->finished) {
            std::cout << gameTempVars.currentRunner << " finished, add runner or end game" << std::endl;
            CAM_TARGET(gameTempVars.currentRunner)->enabled = false;
            // return Runner control to master
            #ifdef SAC_NETWORK
            if (!gameTempVars.isGameMaster) {
                std::cout << "Give back ownership of " << gameTempVars.currentRunner << " to server" << std::endl;
                NETWORK(gameTempVars.currentRunner)->newOwnerShipRequest = 0;
            }
            #endif
            if (myPlayer->runnersCount == 10) {
                theRenderingSystem.cameraPosition = Vector2::Zero;
                // end of game
                // resetGame();
                return Menu;
            } else {
                std::cout << "Create runner" << std::endl;
                // add a new one
                gameTempVars.currentRunner = addRunnerToPlayer(gameTempVars.players[myPlayerIndex], myPlayer, myPlayerIndex);
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
        std::vector<int> direction;
        // check for collisions for non-ghost runners
        for (unsigned i=0; i<gameTempVars.numPlayers; i++) {
            for (unsigned j=0; j<gameTempVars.runners[i].size(); j++) {
                Entity r = gameTempVars.runners[i][j];
                if (RUNNER(r)->ghost)
                    continue;
                actives.push_back(TRANSFORM(r));
                direction.push_back(RUNNER(r)->speed > 0 ? 1 : -1);
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
                    if (IntersectionUtil::rectangleRectangle(ghostTc, actives[k])) {
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
                        player->score += gain;
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
    // Restore camera position
    theRenderingSystem.cameraPosition = Vector2::Zero;
    // Show menu UI
    TEXT_RENDERING(startSingleButton)->hide = false;
    CONTAINER(startSingleButton)->enable = true;
    RENDERING(startSingleButton)->hide = false;
    BUTTON(startSingleButton)->enabled = true;
#ifdef SAC_NETWORK
    TEXT_RENDERING(startMultiButton)->hide = false;
    CONTAINER(startMultiButton)->enable = true;
    RENDERING(startMultiButton)->hide = false;
    BUTTON(startMultiButton)->enabled = true;
    TEXT_RENDERING(startMultiButton)->text = "Jouer multi";
#endif
    TEXT_RENDERING(scoreText)->hide = false;
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
                if (i == playerIndex()) {
                    if (!rc->ghost)
                        currentRunner = r[j];
                }
            }
        }
    }
    if (currentRunner == 0) {
        LOGE("No current runner => bug. Nb players=%d, nb runners=%d",players.size(), r.size());
        for (unsigned i=0; i<players.size(); i++)
            LOGE("    runners[%d] = %d", i, runners[i].size());
    }
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
    TRANSFORM(e)->size = Vector2(0.85,1);//0.4,1);//0.572173, 0.815538);
    TRANSFORM(e)->rotation = 0;
    TRANSFORM(e)->z = 0.8;
    ADD_COMPONENT(e, Rendering);
    RENDERING(e)->color = Color(1 - playerIndex, playerIndex, 1);
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
    CAM_TARGET(e)->maxCameraSpeed = direction * RUNNER(e)->speed;
    ADD_COMPONENT(e, Physics);
    PHYSICS(e)->mass = 1;
    ADD_COMPONENT(e, Animation);
    ANIMATION(e)->name = (direction > 0) ? "runL2R" : "runR2L";
#ifdef SAC_NETWORK
    ADD_COMPONENT(e, Network);
    NETWORK(e)->systemUpdatePeriod[theTransformationSystem.getName()] = 0.116;
    NETWORK(e)->systemUpdatePeriod[theRunnerSystem.getName()] = 0.016;
    NETWORK(e)->systemUpdatePeriod[thePhysicsSystem.getName()] = 0.116;
    NETWORK(e)->systemUpdatePeriod[theRenderingSystem.getName()] = 0;
    NETWORK(e)->systemUpdatePeriod[theAnimationSystem.getName()] = 0.1;
    NETWORK(e)->systemUpdatePeriod[theCameraTargetSystem.getName()] = 0.016;
#endif
    std::cout << "Add player " << e << " at pos : " << TRANSFORM(e)->position << ", speed= " << RUNNER(e)->speed << "/" << player << std::endl;
    return e;
}