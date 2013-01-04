/*
	This file is part of RecursiveRunner.

	@author Soupe au Caillou - Pierre-Eric Pelloux-Prayer
	@author Soupe au Caillou - Gautier Pelloux-Prayer

	RecursiveRunner is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, version 3.

	RecursiveRunner is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with RecursiveRunner.  If not, see <http://www.gnu.org/licenses/>.
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
#include "systems/SessionSystem.h"
#include "systems/PlatformerSystem.h"

#include "../RecursiveRunnerGame.h"
#include "../Parameters.h"

#include <cmath>
#include <sstream>

static void spawnGainEntity(int gain, Entity t, const Color& c, bool isGhost);
static Entity addRunnerToPlayer(RecursiveRunnerGame* game, Entity player, PlayerComponent* p, int playerIndex, SessionComponent* sc);
static void updateSessionTransition(const SessionComponent* session, float progress);
static void checkCoinsPickupForRunner(PlayerComponent* player, Entity e, RunnerComponent* rc, const SessionComponent* sc);

struct GameStateManager::GameStateManagerDatas {
    Entity pauseButton;
    Entity session;
    Entity transition;
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
    TRANSFORM(pauseButton)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("pause"));
    TRANSFORM(pauseButton)->parent = game->cameraEntity;
    TRANSFORM(pauseButton)->position =
        theRenderingSystem.cameras[0].worldSize * Vector2(0.5, 0.5)
        - Vector2(game->buttonSpacing.H, game->buttonSpacing.V);
    TRANSFORM(pauseButton)->z = 0.95;
    ADD_COMPONENT(pauseButton, Rendering);
    RENDERING(pauseButton)->texture = theRenderingSystem.loadTextureFile("pause");
    RENDERING(pauseButton)->hide = true;
    RENDERING(pauseButton)->cameraBitMask = 0x3;
    ADD_COMPONENT(pauseButton, Button);
    BUTTON(pauseButton)->enabled = false;
    BUTTON(pauseButton)->overSize = 1.2;

    Entity transition = datas->transition = theEntityManager.CreateEntity();
    ADD_COMPONENT(transition, ADSR);
    ADSR(transition)->idleValue = 0;
    ADSR(transition)->sustainValue = 1;
    ADSR(transition)->attackValue = 1;
    ADSR(transition)->attackTiming = 1;
    ADSR(transition)->decayTiming = 0.;
    ADSR(transition)->releaseTiming = 0.5;
    ADD_COMPONENT(transition, Music);
    MUSIC(transition)->fadeOut = 2;
    MUSIC(transition)->fadeIn = 1;
}


///----------------------------------------------------------------------------//
///--------------------- ENTER SECTION ----------------------------------------//
///----------------------------------------------------------------------------//
void GameStateManager::willEnter(State::Enum from) {
    ADSR(datas->transition)->active = true;

    if (theSessionSystem.RetrieveAllEntityWithComponent().empty()) {
        RecursiveRunnerGame::startGame(game->level, true);
        MUSIC(datas->transition)->fadeOut = 2;
        MUSIC(datas->transition)->volume = 1;
        MUSIC(datas->transition)->music = theMusicSystem.loadMusicFile("jeu.ogg");
        ADSR(datas->transition)->value = ADSR(datas->transition)->idleValue;
        ADSR(datas->transition)->activationTime = 0;
    }
    if (theMusicSystem.isMuted()) {
        MUSIC(datas->transition)->control = MusicControl::Stop;
    } else {
        MUSIC(datas->transition)->control = MusicControl::Play;
    }
    if (from != State::Tutorial) {
        RENDERING(datas->pauseButton)->hide = false;
        RENDERING(datas->pauseButton)->color = Color(1,1,1,0);
    }
}

bool GameStateManager::transitionCanEnter(State::Enum) {
    const SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());

    float progress = ADSR(datas->transition)->value;
    updateSessionTransition(session, progress);
    RENDERING(datas->pauseButton)->color.a = progress;
    PLAYER(session->players[0])->ready = true;

    return progress >= ADSR(datas->transition)->sustainValue;
}

void GameStateManager::enter(State::Enum from) {
    datas->session = theSessionSystem.RetrieveAllEntityWithComponent().front();
    SessionComponent* sc = SESSION(datas->session);
    // only do this on first enter (ie: not when unpausing)
    if (from != State::Pause) {
        for (unsigned i=0; i<sc->numPlayers; i++) {
            assert (sc->numPlayers == 1);
            Entity r = addRunnerToPlayer(game, sc->players[i], PLAYER(sc->players[i]), i, sc);
            sc->runners.push_back(r);
            sc->currentRunner = r;
        }
        game->setupCamera(CameraMode::Single);
    }
    if (from != State::Tutorial)
        BUTTON(datas->pauseButton)->enabled = true;
}


///----------------------------------------------------------------------------//
///--------------------- UPDATE SECTION ---------------------------------------//
///----------------------------------------------------------------------------//
State::Enum GameStateManager::update(float dt) {
    SessionComponent* sc = SESSION(datas->session);

    if (BUTTON(datas->pauseButton)->clicked) {
        return State::Pause;
    }
    RENDERING(datas->pauseButton)->color = BUTTON(datas->pauseButton)->mouseOver ? Color("gray") : Color();

    // Manage piano's volume depending on the distance from the current runner to the piano
    double distanceAbs = MathUtil::Abs(TRANSFORM(sc->currentRunner)->position.X -
    TRANSFORM(game->pianist)->position.X) / (PlacementHelper::ScreenWidth * param::LevelSize);
    MUSIC(datas->transition)->volume = 0.2 + 0.8 * (1 - distanceAbs);

    // Manage player's current runner
    for (unsigned i=0; i<sc->numPlayers; i++) {
        CAM_TARGET(sc->currentRunner)->enabled = true;
        CAM_TARGET(sc->currentRunner)->offset = Vector2(
            ((RUNNER(sc->currentRunner)->speed > 0) ? 1 :-1) * 0.4 * PlacementHelper::ScreenWidth,
            0 - TRANSFORM(sc->currentRunner)->position.Y);

        // If current runner has reached the edge of the screen
        if (RUNNER(sc->currentRunner)->finished) {
            LOGI("%lu finished, add runner or end game", sc->currentRunner);
            CAM_TARGET(sc->currentRunner)->enabled = false;

            if (PLAYER(sc->players[i])->runnersCount == param::runner) {
                // Game is finished, show either Rate Menu or Main Menu
                  if (0 && game->communicationAPI->mustShowRateDialog()) {
                     return State::Rate;
                  } else {
                   return State::Menu;
                }
            } else {
                LOGI("Create runner");
                // add a new runner
                sc->currentRunner = addRunnerToPlayer(game, sc->players[i], PLAYER(sc->players[i]), i, sc);
                sc->runners.push_back(sc->currentRunner);
            }
        }
        if (!game->ignoreClick && sc->userInputEnabled) {
            // Input (jump) handling
            for (int j=0; j<1; j++) {
                if (theTouchInputManager.isTouched(j)) {
                    if (sc->numPlayers == 2) {
                        const Vector2& ppp = theTouchInputManager.getTouchLastPosition(j);
                        if (i == 0 && ppp.Y < 0)
                            continue;
                        if (i == 1 && ppp.Y > 0)
                            continue;
                    }
                    PhysicsComponent* pc = PHYSICS(sc->currentRunner);
                    RunnerComponent* rc = RUNNER(sc->currentRunner);
                    if (!theTouchInputManager.wasTouched(j)) {
                        if (rc->jumpingSince <= 0 && pc->linearVelocity.Y == 0) {
                            rc->jumpTimes.push_back(rc->elapsed);
                            rc->jumpDurations.push_back(dt);
                        }
                    } else if (!rc->jumpTimes.empty()) {
                        float& d = *(rc->jumpDurations.rbegin());
                        d = MathUtil::Min(d + dt, RunnerSystem::MaxJumpDuration);
                    }
                    break;
                }
            }
        }

        TransformationComponent* tc = TRANSFORM(sc->currentRunner);
        CAM_TARGET(sc->currentRunner)->offset.Y = 0 - tc->position.Y;
    }

    // Manage runner-runner collisions
    {
        std::vector<TransformationComponent*> activesColl;
        std::vector<int> direction;

        for (unsigned i=0; i<sc->numPlayers; i++) {
            for (unsigned j=0; j<sc->runners.size(); j++) {
                const Entity r = sc->runners[j];
                const RunnerComponent* rc = RUNNER(r);
                if (rc->ghost)
                    continue;
                activesColl.push_back(TRANSFORM(rc->collisionZone));
                direction.push_back(rc->speed > 0 ? 1 : -1);
            }
        }
        const unsigned count = activesColl.size();
        for (unsigned i=0; i<sc->numPlayers; i++) {
            for (unsigned j=0; j<sc->runners.size(); j++) {
                Entity ghost = sc->runners[j];
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
                        sc->runners.erase(sc->runners.begin() + j);
                        j--;
                        break;
                    }
                }
            }
        }
    }

    for (unsigned i=0; i<sc->numPlayers; i++) {
        PlayerComponent* player = PLAYER(sc->players[i]);
        for (unsigned j=0; j<sc->runners.size(); j++) {
            Entity e = sc->runners[j];
            RunnerComponent* rc = RUNNER(e);
            if (rc->killed)
                continue;
#if 0
            PhysicsComponent* pc = PHYSICS(e);
            // check jumps
            if (pc->gravity.Y < 0) {
                TransformationComponent* tc = TRANSFORM(e);
                // landing management
                if ((tc->position.Y - tc->size.Y * 0.5) <= game->baseLine) {
                    pc->gravity.Y = 0;
                    pc->linearVelocity = Vector2::Zero;
                    tc->position.Y = game->baseLine + tc->size.Y * 0.5;
                    ANIMATION(e)->name = "jumptorunL2R";
                    RENDERING(e)->mirrorH = (rc->speed < 0);
                }
            }
#endif
            // check coins
            checkCoinsPickupForRunner(player, e, rc, sc);

            // check platform switch
            const TransformationComponent* collisionZone = TRANSFORM(rc->collisionZone);
            for (unsigned k=0; k<sc->platforms.size(); k++) {
                for (unsigned l=0; l<2; l++) {
                    if (IntersectionUtil::rectangleRectangle(
                        collisionZone, TRANSFORM(sc->platforms[k].switches[l].entity))) {
                        sc->platforms[k].switches[l].owner = e;
                        if (!sc->platforms[k].switches[l].state) {
                            sc->platforms[k].switches[l].state = true;
                            std::cout << e << " activated " << sc->platforms[k].switches[l].entity << std::endl;
                        }
                    }
                }
            }
        }
    }

    // handle platforms
    {
        std::vector<Entity> platformers = thePlatformerSystem.RetrieveAllEntityWithComponent();
        for (unsigned i=0; i<sc->platforms.size(); i++) {
            Platform& pt = sc->platforms[i];
            bool active = pt.switches[1].state & pt.switches[1].state & (
                pt.switches[1].owner == pt.switches[0].owner);
            if (active != pt.active) {
                pt.active = active;
                std::cout << "platform #" << i << " is now : " << active << std::endl;
                if (active) {
                    RENDERING(pt.platform)->texture = theRenderingSystem.loadTextureFile("link");
                } else {
                    RENDERING(pt.platform)->texture = InvalidTextureRef;
                }
                for (unsigned k=0; k<platformers.size(); k++) {
                    PLATFORMER(platformers[k])->platforms[pt.platform] = active;
                }
            }
        }
    }

    // Show the score(s)
    for (unsigned i=0; i<sc->players.size(); i++) {
        std::stringstream a;
        a << PLAYER(sc->players[i])->score;
        TEXT_RENDERING(game->scoreText)->text = a.str();
    }

    thePlatformerSystem.Update(dt);
    thePlayerSystem.Update(dt);
    theRunnerSystem.Update(dt);
    theCameraTargetSystem.Update(dt);

    return State::Game;
}

void GameStateManager::backgroundUpdate(float) {

}


///----------------------------------------------------------------------------//
///--------------------- EXIT SECTION -----------------------------------------//
///----------------------------------------------------------------------------//
void GameStateManager::willExit(State::Enum to) {
    BUTTON(datas->pauseButton)->enabled = false;
    if (to != State::Pause) {
        ADSR(datas->transition)->active = false;
    }
}

bool GameStateManager::transitionCanExit(State::Enum to) {
    if (to == State::Pause) {
        return true;
    }
    const SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());

    float progress = ADSR(datas->transition)->value;
    updateSessionTransition(session, progress);
    RENDERING(datas->pauseButton)->color.a = progress;

    return progress <= ADSR(datas->transition)->idleValue;
}

void GameStateManager::exit(State::Enum) {
    RENDERING(datas->pauseButton)->hide = true;
}


static void spawnGainEntity(int, Entity parent, const Color& color, bool isGhost) {
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
    AUTO_DESTROY(e)->params.lifetime.value = 3.5;
    AUTO_DESTROY(e)->params.lifetime.map2AlphaRendering = true;
    // AUTO_DESTROY(e)->hasTextRendering = true;
}

static Entity addRunnerToPlayer(RecursiveRunnerGame* game, Entity player, PlayerComponent* p, int playerIndex, SessionComponent* sc) {
    int direction = ((p->runnersCount + playerIndex) % 2) ? -1 : 1;
    Entity e = theEntityManager.CreateEntity(EntityType::Persistent);
    ADD_COMPONENT(e, Transformation);
    TRANSFORM(e)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("run_l2r_0000")) * 0.68;
    //Vector2(0.85, 0.85) * 2.5;
    
    TRANSFORM(e)->rotation = 0;
    TRANSFORM(e)->z = 0.8 + 0.01 * p->runnersCount;
    ADD_COMPONENT(e, Rendering);
    RENDERING(e)->hide = false;
    RENDERING(e)->cameraBitMask = (0x3 << 1);
    RENDERING(e)->color = Color(12.0/255, 4.0/255, 41.0/255);
    ADD_COMPONENT(e, Runner);
    theTransformationSystem.setPosition(TRANSFORM(e),
        Vector2(direction * -(param::LevelSize * PlacementHelper::ScreenWidth + TRANSFORM(e)->size.X) * 0.5, game->baseLine), TransformationSystem::S);
    RUNNER(e)->startPoint = TRANSFORM(e)->position;
    RUNNER(e)->endPoint = Vector2(direction * (param::LevelSize * PlacementHelper::ScreenWidth + TRANSFORM(e)->size.X) * 0.5, 0);
    RUNNER(e)->speed = direction * (param::speedConst + param::speedCoeff * p->runnersCount);
    RUNNER(e)->startTime = 0;//MathUtil::RandomFloatInRange(1,3);
    RUNNER(e)->playerOwner = player;
    ADD_COMPONENT(e, Platformer);
    PLATFORMER(e)->offset = Vector2(0, TRANSFORM(e)->size.Y * -0.5);
    PLATFORMER(e)->platforms.insert(std::make_pair(game->ground, true));
    for (unsigned i=0; i<sc->platforms.size(); i++) {
        PLATFORMER(e)->platforms.insert(std::make_pair(sc->platforms[i].platform, sc->platforms[i].active));
    }
    std::cout <<" add runner: " << e << " - " << TimeUtil::getTime()  << "(pos: " << TRANSFORM(e)->position << ")" << RUNNER(e)->speed << std::endl;
    #if 0
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
    #else
    int idx = MathUtil::RandomInt(p->colors.size());
    RUNNER(e)->color = p->colors[idx];
    p->colors.erase(p->colors.begin() + idx);
    #endif
    ADD_COMPONENT(e, CameraTarget);
    CAM_TARGET(e)->cameraIndex = 1 + playerIndex;
    CAM_TARGET(e)->maxCameraSpeed = direction * RUNNER(e)->speed;
    ADD_COMPONENT(e, Physics);
    PHYSICS(e)->mass = 1;
    ADD_COMPONENT(e, Animation);
    ANIMATION(e)->name = "runL2R";
    ANIMATION(e)->playbackSpeed = 1.1;
    RENDERING(e)->mirrorH = (direction < 0);

    Entity collisionZone = theEntityManager.CreateEntity(EntityType::Persistent);
    ADD_COMPONENT(collisionZone, Transformation);
    TRANSFORM(collisionZone)->parent = e;
    TRANSFORM(collisionZone)->z = 0.01;
    #if 0
    ADD_COMPONENT(collisionZone, Rendering);
    RENDERING(collisionZone)->hide = false;
    RENDERING(collisionZone)->color = Color(1,0,0,1);
    #endif
    RUNNER(e)->collisionZone = collisionZone;
    p->runnersCount++;
 LOGI("Add runner %lu at pos : {%.2f, %.2f}, speed: %.2f (player=%lu)", e, TRANSFORM(e)->position.X, TRANSFORM(e)->position.Y, RUNNER(e)->speed, player);
    return e;
}

static void updateSessionTransition(const SessionComponent* session, float progress) {
    for (unsigned i=0; i<session->coins.size(); i++) {
        RENDERING(session->coins[i])->color.a = progress;
    }
    for (unsigned i=0; i<session->links.size(); i++) {
        RENDERING(session->links[i])->color.a = progress * 0.65;
    }
}

static void checkCoinsPickupForRunner(PlayerComponent* player, Entity e, RunnerComponent* rc, const SessionComponent* sc) {
    const TransformationComponent* collisionZone = TRANSFORM(rc->collisionZone);
    const int end = sc->coins.size();
    Entity prev = 0;

    for(int idx=0; idx<end; idx++) {
        Entity coin = rc->speed > 0 ? sc->coins[idx] : sc->coins[end - idx - 1];
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
                                    PARTICULE(sc->sparkling[linkIdx - j + 1])->duration += t;
                                }
                            } else {
                                for (int j=1; j<rc->coinSequenceBonus; j++) {
                                    PARTICULE(sc->sparkling[linkIdx + j - 1])->duration +=
                                        1 * ((rc->coinSequenceBonus - (j - 1.0)) / (float)rc->coinSequenceBonus);
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
                if (sc->currentRunner == e)
                    player->coins++;

                spawnGainEntity(gain, coin, rc->color, rc->ghost);
            }
        }
        prev = coin;
    }
}
