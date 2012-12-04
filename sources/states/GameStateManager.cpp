/*
 This file is part of Recursive Runner.

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
#include "StateManager.h"

#include "base/PlacementHelper.h"
#include "systems/AnimationSystem.h"
#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/ButtonSystem.h"
#include "systems/ADSRSystem.h"
#include "systems/TextRenderingSystem.h"
#include "systems/SoundSystem.h"
#include "systems/MusicSystem.h"
#include "systems/PhysicsSystem.h"
#include "systems/ParticuleSystem.h"
#include "systems/AutoDestroySystem.h"
#include "util/IntersectionUtil.h"

#include "systems/PlayerSystem.h"
#include "systems/RunnerSystem.h"
#include "systems/CameraTargetSystem.h"

#include "../RecursiveRunnerGame.h"
#include "../Parameters.h"

#include <cmath>
#include <sstream>

void GameTempVar::cleanup() {
    for (unsigned i=0; i<coins.size(); i++) {
        theEntityManager.DeleteEntity(coins[i]);
    }
    coins.clear();
    std::vector<Entity> r = theRunnerSystem.RetrieveAllEntityWithComponent();
    for (unsigned j=0; j<r.size(); j++) {
        theEntityManager.DeleteEntity(RUNNER(r[j])->collisionZone);
        theEntityManager.DeleteEntity(r[j]);
    }
    while (!links.empty()) {
        theEntityManager.DeleteEntity(links.back());
        links.pop_back();
    }
    while (!sparkling.empty()) {
        theEntityManager.DeleteEntity(sparkling.back());
        sparkling.pop_back();
    }

    for (unsigned i=0; i<players.size(); i++) {
        runners[i].clear();
        theEntityManager.DeleteEntity(players[i]);
    }
    players.clear();
}

void GameTempVar::syncRunners() {
    std::vector<Entity> r = theRunnerSystem.RetrieveAllEntityWithComponent();
    for (unsigned i=0; i<players.size(); i++) {
        runners[i].clear();
        for (unsigned j=0; j<r.size(); j++) {
            RunnerComponent* rc = RUNNER(r[j]);
            if (rc->killed)
                continue;
            if (rc->playerOwner == players[i]) {
                runners[i].push_back(r[j]);
                if (!rc->ghost) {
                        currentRunner[i] = r[j];
                }
            }
        }
    }
    if (currentRunner[0] == 0) {
        LOGE("No current runner => bug. Nb players=%lu, nb runners=%lu",players.size(), r.size());
        for (unsigned i=0; i<players.size(); i++)
            LOGE("    runners[%d] = %lu", i, runners[i].size());
    }
}

static bool sortFromLeftToRight(Entity c1, Entity c2) {
    return TRANSFORM(c1)->position.X < TRANSFORM(c2)->position.X;
}

void GameTempVar::syncCoins() {
    coins.clear();
    const std::vector<Entity> t = theRenderingSystem.RetrieveAllEntityWithComponent();
    const TextureRef coinTexture = theRenderingSystem.loadTextureFile("ampoule");
    for (unsigned i=0; i<t.size(); i++) {
        //...
        if (RENDERING(t[i])->texture == coinTexture) {
            coins.push_back(t[i]);
        }
    }
    if (coins.size() != 20) {
        LOGE("Weird, we have %lu coins...", coins.size());
    }
    std::sort(coins.begin(), coins.end(), sortFromLeftToRight);
}

int GameTempVar::playerIndex() {
    return 0;
}

static void spawnGainEntity(int gain, Entity t, const Color& c, bool isGhost);
static Entity addRunnerToPlayer(RecursiveRunnerGame* game, Entity player, PlayerComponent* p, int playerIndex);


struct GameStateManager::GameStateManagerDatas {
    Entity pauseButton;
};

GameStateManager::GameStateManager(RecursiveRunnerGame* game) : StateManager(State::Game, game) {
    datas = new GameStateManagerDatas();
}

GameStateManager::~GameStateManager() {
    delete datas;
}

void GameStateManager::setup() {
    Entity pauseButton = datas->pauseButton = theEntityManager.CreateEntity();
    ADD_COMPONENT(pauseButton, Transformation);
    TRANSFORM(pauseButton)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("mute"));
    TRANSFORM(pauseButton)->parent = game->cameraEntity;
    TRANSFORM(pauseButton)->position = 
        theRenderingSystem.cameras[0].worldSize * Vector2(0.5, 0.5)
        + TRANSFORM(pauseButton)->size * Vector2(-0.5, -0.5)
        + Vector2(0, game->baseLine + theRenderingSystem.cameras[0].worldSize.Y * 0.5);
    TRANSFORM(pauseButton)->z = 0.95;
    ADD_COMPONENT(pauseButton, Rendering);
    RENDERING(pauseButton)->texture = theRenderingSystem.loadTextureFile("pause");
    RENDERING(pauseButton)->hide = true;
    RENDERING(pauseButton)->cameraBitMask = 0x2;
    RENDERING(pauseButton)->color = Color(119.0 / 255, 119.0 / 255, 119.0 / 255);
    ADD_COMPONENT(pauseButton, Button);
    BUTTON(pauseButton)->enabled = false;
    BUTTON(pauseButton)->overSize = 1.2;
}

void GameStateManager::earlyEnter() {
}

void GameStateManager::enter() {
    // only do this on first enter (ie: not when unpausing)
    if (game->gameTempVars.runners[0].empty()) {
        for (unsigned i=0; i<game->gameTempVars.numPlayers; i++) {
            addRunnerToPlayer(game, game->gameTempVars.players[i], PLAYER(game->gameTempVars.players[i]), i);
        }
    
        game->gameTempVars.syncRunners();
        for (unsigned i=0; i<game->gameTempVars.numPlayers; i++) {
            theRenderingSystem.cameras[1 + i].worldPosition.X = 
                TRANSFORM(game->gameTempVars.currentRunner[i])->position.X + PlacementHelper::ScreenWidth * 0.5;
        }
    }
    RENDERING(datas->pauseButton)->hide = false;
    BUTTON(datas->pauseButton)->enabled = true;
}

State::Enum GameStateManager::update(float dt) {
    GameTempVar& gameTempVars = game->gameTempVars;

    gameTempVars.syncRunners();

    if (BUTTON(datas->pauseButton)->clicked) {
        return State::Pause;
    }
    /*if (MUSIC(titleGroup)->loopNext == InvalidMusicRef) {
        MUSIC(titleGroup)->loopNext = theMusicSystem.loadMusicFile("432796_ragtime.ogg");
    }*/

    // Manage player's current runner
    for (unsigned i=0; i<gameTempVars.numPlayers; i++) {
        CAM_TARGET(gameTempVars.currentRunner[i])->enabled = true;
        CAM_TARGET(gameTempVars.currentRunner[i])->offset = Vector2(
            ((RUNNER(gameTempVars.currentRunner[i])->speed > 0) ? 1 :-1) * 0.4 * PlacementHelper::ScreenWidth, 
            0 - TRANSFORM(gameTempVars.currentRunner[i])->position.Y);
        
        // If current runner has reached the edge of the screen
        if (RUNNER(gameTempVars.currentRunner[i])->finished) {
            LOGI("%lu finished, add runner or end game", gameTempVars.currentRunner[i]);
            CAM_TARGET(gameTempVars.currentRunner[i])->enabled = false;
            // return Runner control to master
            if (PLAYER(gameTempVars.players[i])->runnersCount == param::runner) {
                theRenderingSystem.cameras[0].worldPosition = Vector2::Zero;
                // end of game
                // resetGame();
                return State::Game2Menu;
            } else {
                LOGI("Create runner");
                // add a new one
                gameTempVars.currentRunner[i] = addRunnerToPlayer(game, gameTempVars.players[i], PLAYER(gameTempVars.players[i]), i);
            }
        }
        if (!game->ignoreClick) {
            // Input (jump) handling
            for (int j=0; j<1; j++) {
                if (theTouchInputManager.isTouched(j)) {
                    if (gameTempVars.numPlayers == 2) {
                        const Vector2& ppp = theTouchInputManager.getTouchLastPosition(j);
                        if (i == 0 && ppp.Y < 0)
                            continue;
                        if (i == 1 && ppp.Y > 0)
                            continue;
                    }
                    PhysicsComponent* pc = PHYSICS(gameTempVars.currentRunner[i]);
                    RunnerComponent* rc = RUNNER(gameTempVars.currentRunner[i]);
                    if (!theTouchInputManager.wasTouched(j)) {
                        if (rc->jumpingSince <= 0 && pc->linearVelocity.Y == 0) {
                            rc->jumpTimes.push_back(rc->elapsed);
                            rc->jumpDurations.push_back(0.001);
                        }
                    } else if (!rc->jumpTimes.empty()) {
                        float& d = *(rc->jumpDurations.rbegin());
                        d = MathUtil::Min(d + dt, RunnerSystem::MaxJumpDuration);
                    }
                    break;
                }
            }
        }

        TransformationComponent* tc = TRANSFORM(gameTempVars.currentRunner[i]);
        CAM_TARGET(gameTempVars.currentRunner[i])->offset.Y = 0 - tc->position.Y;
    }

    { // maybe do it for non master too (but do not delete entities, maybe only hide ?)
        std::vector<TransformationComponent*> activesColl;
        std::vector<int> direction;
        // check for collisions for non-ghost runners
        for (unsigned i=0; i<gameTempVars.numPlayers; i++) {
            for (unsigned j=0; j<gameTempVars.runners[i].size(); j++) {
                const Entity r = gameTempVars.runners[i][j];
                const RunnerComponent* rc = RUNNER(r);
                if (rc->ghost)
                    continue;
                activesColl.push_back(TRANSFORM(rc->collisionZone));
                direction.push_back(rc->speed > 0 ? 1 : -1);
            }
        }
        const unsigned count = activesColl.size();
        for (unsigned i=0; i<gameTempVars.numPlayers; i++) {
            for (unsigned j=0; j<gameTempVars.runners[i].size(); j++) {
                Entity ghost = gameTempVars.runners[i][j];
                RunnerComponent* rc = RUNNER(ghost);
                if (!rc->ghost || rc->killed)
                    continue;
                TransformationComponent* ghostColl = TRANSFORM(rc->collisionZone);
                for (unsigned k=0; k<count; k++) {
                    // we can only hit guys with opposite direction
                    if (rc->speed * direction[k] > 0)
                        continue;
                    if (rc->elapsed < 0.25)
                        continue;
                    if (IntersectionUtil::rectangleRectangle(ghostColl, activesColl[k])) {
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
            PhysicsComponent* pc = PHYSICS(e);
    
            // check jumps
            if (pc->gravity.Y < 0) {
                TransformationComponent* tc = TRANSFORM(e);
                if ((tc->position.Y - tc->size.Y * 0.5) <= game->baseLine) {
                    pc->gravity.Y = 0;
                    pc->linearVelocity = Vector2::Zero;
                    tc->position.Y = game->baseLine + tc->size.Y * 0.5;
                    ANIMATION(e)->name = "jumptorunL2R";
                    RENDERING(e)->mirrorH = (rc->speed < 0);
                }
            }
            const TransformationComponent* collisionZone = TRANSFORM(rc->collisionZone);
            // check coins
            int end = gameTempVars.coins.size();
            Entity prev = 0;
            for(int idx=0; idx<end; idx++) {
                Entity coin = rc->speed > 0 ? gameTempVars.coins[idx] : gameTempVars.coins[end - idx - 1];
                if (std::find(rc->coins.begin(), rc->coins.end(), coin) == rc->coins.end()) {
                    const TransformationComponent* tCoin = TRANSFORM(coin);
                    if (IntersectionUtil::rectangleRectangle(
                        collisionZone->worldPosition, collisionZone->size, collisionZone->worldRotation,
                        tCoin->worldPosition, tCoin->size * Vector2(0.5, 0.6), tCoin->worldRotation)) {
                        if (!rc->coins.empty()) {
                         int linkIdx = (rc->speed > 0) ? idx : end - idx;
                            if (rc->coins.back() == prev) {
                                rc->coinSequenceBonus++;
                                #if 1
                                if (!rc->ghost) {
                                    if (rc->speed > 0) {
                                     for (int j=1; j<rc->coinSequenceBonus; j++) {
                                         float t = 1 * ((rc->coinSequenceBonus - (j - 1.0)) / (float)rc->coinSequenceBonus);
                                         PARTICULE(gameTempVars.sparkling[linkIdx - j + 1])->duration += t;
                                         /*PARTICULE(gameTempVars.sparkling[linkIdx - j + 1])->initialColor = 
                                            PARTICULE(gameTempVars.sparkling[linkIdx - j + 1])->finalColor =
                                                Interval<Color>(rc->color, rc->color);*/
                                     }
                                    } else {
                                     for (int j=1; j<rc->coinSequenceBonus; j++) {
                                         PARTICULE(gameTempVars.sparkling[linkIdx + j - 1])->duration += 
                                             1 * ((rc->coinSequenceBonus - (j - 1.0)) / (float)rc->coinSequenceBonus);
                                         /*PARTICULE(gameTempVars.sparkling[linkIdx + j - 1])->initialColor =
                                            PARTICULE(gameTempVars.sparkling[linkIdx + j - 1])->finalColor =
                                                Interval<Color>(rc->color, rc->color);*/
                                        }
                                    }
                                }
                                #endif
                                
                            } else {
                                rc->coinSequenceBonus = 1;
                            }
                        }
                        rc->coins.push_back(coin);
                        int gain = 10 * pow(2.0f, rc->oldNessBonus) * rc->coinSequenceBonus;
                        player->score += gain;
                        
                        //coins++ only for player, not his ghosts
                        if (j == gameTempVars.runners[i].size() - 1)
                            player->coins++;
                        
                        spawnGainEntity(gain, coin, rc->color, rc->ghost);
                    }
                }
                prev = coin;
            }
        }
    }

    for (unsigned i=0; i<gameTempVars.players.size(); i++) {
        std::stringstream a;
        a << PLAYER(gameTempVars.players[i])->score;
        TEXT_RENDERING(game->scoreText[i])->text = a.str();
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
    return State::Game;
}

void GameStateManager::backgroundUpdate(float dt __attribute__((unused))) {

}

void GameStateManager::exit() {
    BUTTON(datas->pauseButton)->enabled = false;
}

void GameStateManager::lateExit() {
    RENDERING(datas->pauseButton)->hide = true;
}

bool GameStateManager::transitionCanExit() {
    return true;
}

bool GameStateManager::transitionCanEnter() {
    return true;
}

static void spawnGainEntity(int gain __attribute__((unused)), Entity parent, const Color& color, bool isGhost) {
    Entity e = theEntityManager.CreateEntity();
    ADD_COMPONENT(e, Transformation);
    TRANSFORM(e)->position = TRANSFORM(parent)->position;
    TRANSFORM(e)->rotation = TRANSFORM(parent)->rotation;
    TRANSFORM(e)->size = TRANSFORM(parent)->size;
    TRANSFORM(e)->z = TRANSFORM(parent)->z + (isGhost ? 0.1 : 0.11);
    ADD_COMPONENT(e, Rendering);
    RENDERING(e)->texture = theRenderingSystem.loadTextureFile("lumiere");
    RENDERING(e)->color = color;
    RENDERING(e)->hide = false;
    
    PARTICULE(parent)->duration = 0.1;
    PARTICULE(parent)->initialColor = PARTICULE(parent)->finalColor = Interval<Color> (color, color);
#if 0
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
#endif
    ADD_COMPONENT(e, AutoDestroy);
    AUTO_DESTROY(e)->type = AutoDestroyComponent::LIFETIME;
    AUTO_DESTROY(e)->params.lifetime.value = 5;
    AUTO_DESTROY(e)->params.lifetime.map2AlphaRendering = true;
    // AUTO_DESTROY(e)->hasTextRendering = true;
}

static Entity addRunnerToPlayer(RecursiveRunnerGame* game, Entity player, PlayerComponent* p, int playerIndex) {
    int direction = ((p->runnersCount + playerIndex) % 2) ? -1 : 1;
    Entity e = theEntityManager.CreateEntity();
    ADD_COMPONENT(e, Transformation);
    // TRANSFORM(e)->position = Vector2(-9, 2);
    // TRANSFORM(e)->size = Vector2(0.85, 2 * 0.85) * .8;//0.4,1);//0.572173, 0.815538);
    TRANSFORM(e)->size = Vector2(0.85, 0.85) * 2.5;//0.4,1);//0.572173, 0.815538);
    TRANSFORM(e)->rotation = 0;
    TRANSFORM(e)->z = 0.8 + 0.01 * p->runnersCount;
    ADD_COMPONENT(e, Rendering);
    // RENDERING(e)->color = Color(1 - playerIndex, playerIndex, 1);
    RENDERING(e)->hide = false;
    RENDERING(e)->cameraBitMask = (0x3 << 1);
    RENDERING(e)->color = Color(12.0/255, 4.0/255, 41.0/255);
    ADD_COMPONENT(e, Runner);

    /*TRANSFORM(e)->position = RUNNER(e)->startPoint = Vector2(
        direction * -LEVEL_SIZE * 0.5 * PlacementHelper::ScreenWidth,
        -0.5 * PlacementHelper::ScreenHeight + TRANSFORM(e)->size.Y * 0.5);*/
    theTransformationSystem.setPosition(TRANSFORM(e), 
        Vector2(direction * -(param::LevelSize * PlacementHelper::ScreenWidth + TRANSFORM(e)->size.X) * 0.5, game->baseLine), TransformationSystem::S);
    RUNNER(e)->startPoint = TRANSFORM(e)->position;
    RUNNER(e)->endPoint = Vector2(direction * (param::LevelSize * PlacementHelper::ScreenWidth + TRANSFORM(e)->size.X) * 0.5, 0);
    RUNNER(e)->speed = direction * (param::speedConst + param::speedCoeff * p->runnersCount);
    RUNNER(e)->startTime = 0;//MathUtil::RandomFloatInRange(1,3);
    RUNNER(e)->playerOwner = player;

    std::cout <<" add runner: " << e << " - " << TimeUtil::getTime()  << "(pos: " << TRANSFORM(e)->position << ")" << RUNNER(e)->speed << std::endl;
    do {
        Color c(Color::random());
        c.a = 1;
        float sum = c.r + c.b + c.g;
        float maxDiff = MathUtil::Max(MathUtil::Max(MathUtil::Abs(c.r - c.g), MathUtil::Abs(c.r - c.b)), MathUtil::Abs(c.b - c.g));
        if ((sum > 1.5 || c.r > 0.7 || c.g > 0.7 || c.b > 0.7) && maxDiff > 0.5) {
            RUNNER(e)->color = c;
            break;
        }
    } while (true);
    ADD_COMPONENT(e, CameraTarget);
    CAM_TARGET(e)->cameraIndex = 1 + playerIndex;
    CAM_TARGET(e)->maxCameraSpeed = direction * RUNNER(e)->speed;
    ADD_COMPONENT(e, Physics);
    PHYSICS(e)->mass = 1;
    ADD_COMPONENT(e, Animation);
    ANIMATION(e)->name = "runL2R";
    ANIMATION(e)->playbackSpeed = 1.1;
    RENDERING(e)->mirrorH = (direction < 0);
    
    Entity collisionZone = theEntityManager.CreateEntity();
    ADD_COMPONENT(collisionZone, Transformation);
    TRANSFORM(collisionZone)->parent = e;
    TRANSFORM(collisionZone)->z = 0.01;
    #if 0
    ADD_COMPONENT(collisionZone, Rendering);
    RENDERING(collisionZone)->hide = false;
    RENDERING(collisionZone)->color = Color(1,0,0,1);
    #endif
    RUNNER(e)->collisionZone = collisionZone;

 LOGI("Add runner %lu at pos : {%.2f, %.2f}, speed: %.2f (player=%lu)", e, TRANSFORM(e)->position.X, TRANSFORM(e)->position.Y, RUNNER(e)->speed, player);
    return e;
}