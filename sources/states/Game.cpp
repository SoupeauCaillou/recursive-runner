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
#include "base/StateMachine.h"

#include "base/EntityManager.h"
#include "base/TouchInputManager.h"
#include "base/ObjectSerializer.h"
#include "base/PlacementHelper.h"
#include "systems/AnchorSystem.h"
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
#include "api/LocalizeAPI.h"

#include "../RecursiveRunnerGame.h"
#include "../Parameters.h"
#include <glm/gtx/compatibility.hpp>
#include <cmath>

static void spawnGainEntity(int gain, Entity t, const Color& c, bool isGhost);
static Entity addRunnerToPlayer(RecursiveRunnerGame* game, Entity player, PlayerComponent* p, int playerIndex, SessionComponent* sc);
static void updateSessionTransition(const SessionComponent* session, float progress);
static void checkCoinsPickupForRunner(PlayerComponent* player, Entity e, RunnerComponent* rc, const SessionComponent* sc);

class GameScene : public StateHandler<Scene::Enum> {
    RecursiveRunnerGame* game;
    Entity pauseButton;
    Entity session;
    Entity transition;

public:
        GameScene(RecursiveRunnerGame* game) : StateHandler<Scene::Enum>() {
            this->game = game;
        }

        void setup() {
            pauseButton = theEntityManager.CreateEntity("pause_buttton");
            ADD_COMPONENT(pauseButton, Transformation);
            TRANSFORM(pauseButton)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("pause"));
            ADD_COMPONENT(pauseButton, Anchor);
            ANCHOR(pauseButton)->parent = game->cameraEntity;
            ANCHOR(pauseButton)->position =
                TRANSFORM(game->cameraEntity)->size * glm::vec2(0.5, 0.5)
                - glm::vec2(game->buttonSpacing.H, game->buttonSpacing.V);
            ANCHOR(pauseButton)->z = 0.95;
            ADD_COMPONENT(pauseButton, Rendering);
            RENDERING(pauseButton)->texture = theRenderingSystem.loadTextureFile("pause");
            RENDERING(pauseButton)->show = false;
            ADD_COMPONENT(pauseButton, Button);
            BUTTON(pauseButton)->enabled = false;
            BUTTON(pauseButton)->overSize = 1.2;

            transition = theEntityManager.CreateEntity("transition_helper");
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
        void onPreEnter(Scene::Enum from) {
            ADSR(transition)->active = true;

            if (theSessionSystem.RetrieveAllEntityWithComponent().empty()) {
                RecursiveRunnerGame::startGame(game->level, true);
                MUSIC(transition)->fadeOut = 2;
                MUSIC(transition)->volume = 1;
                MUSIC(transition)->music = theMusicSystem.loadMusicFile("jeu.ogg");
                ADSR(transition)->value = ADSR(transition)->idleValue;
                ADSR(transition)->activationTime = 0;
            }
            if (theMusicSystem.isMuted()) {
                MUSIC(transition)->control = MusicControl::Stop;
            } else {
                MUSIC(transition)->control = MusicControl::Play;
            }
            if (from != Scene::Tutorial) {
                RENDERING(pauseButton)->show = true;
                RENDERING(pauseButton)->color = Color(1,1,1,0);
            }
        }

        bool updatePreEnter(Scene::Enum, float) {
            const SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());

            float progress = ADSR(transition)->value;
            updateSessionTransition(session, progress);
            RENDERING(pauseButton)->color.a = progress;
            PLAYER(session->players[0])->ready = true;

            return progress >= ADSR(transition)->sustainValue;
        }

        void onEnter(Scene::Enum from) {
            session = theSessionSystem.RetrieveAllEntityWithComponent().front();
            SessionComponent* sc = SESSION(session);
            // only do this on first enter (ie: not when unpausing)
            if (from != Scene::Pause) {
                for (unsigned i=0; i<sc->numPlayers; i++) {
                    assert (sc->numPlayers == 1);
                    Entity r = addRunnerToPlayer(game, sc->players[i], PLAYER(sc->players[i]), i, sc);
                    sc->runners.push_back(r);
                    sc->currentRunner = r;
                }
                game->setupCamera(CameraMode::Single);
            }
            if (from != Scene::Tutorial)
                BUTTON(pauseButton)->enabled = true;
        }


        ///----------------------------------------------------------------------------//
        ///--------------------- UPDATE SECTION ---------------------------------------//
        ///----------------------------------------------------------------------------//
        Scene::Enum update(float dt) {
            SessionComponent* sc = SESSION(session);

            if (BUTTON(pauseButton)->clicked) {
                return Scene::Pause;
            }
            RENDERING(pauseButton)->color = BUTTON(pauseButton)->mouseOver ? Color("gray") : Color();

            // Manage piano's volume depending on the distance from the current runner to the piano
            double distanceAbs = glm::abs(TRANSFORM(sc->currentRunner)->position.x -
            TRANSFORM(game->pianist)->position.x) / (PlacementHelper::ScreenWidth * param::LevelSize);
            MUSIC(transition)->volume = 0.2 + 0.8 * (1 - distanceAbs);

            // Manage player's current runner
            for (unsigned i=0; i<sc->numPlayers; i++) {
                CAM_TARGET(sc->currentRunner)->enabled = true;
                CAM_TARGET(sc->currentRunner)->offset = glm::vec2(
                    ((RUNNER(sc->currentRunner)->speed > 0) ? 1 :-1) * 0.4 * PlacementHelper::ScreenWidth,
                    0 - TRANSFORM(sc->currentRunner)->position.y);

                // If current runner has reached the edge of the screen
                if (RUNNER(sc->currentRunner)->finished) {
                    LOGI(sc->currentRunner << " finished, add runner or end game");
                    CAM_TARGET(sc->currentRunner)->enabled = false;

                    if (PLAYER(sc->players[i])->runnersCount == param::runner) {
                        // Game is finished, show either Rate Menu or Main Menu
                          if (0 && game->gameThreadContext->communicationAPI->mustShowRateDialog()) {
                             return Scene::Rate;
                          } else {
                           return Scene::Menu;
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
                                const glm::vec2& ppp = theTouchInputManager.getTouchLastPosition(j);
                                if (i == 0 && ppp.y < 0)
                                    continue;
                                if (i == 1 && ppp.y > 0)
                                    continue;
                            }
                            PhysicsComponent* pc = PHYSICS(sc->currentRunner);
                            RunnerComponent* rc = RUNNER(sc->currentRunner);
                            if (!theTouchInputManager.wasTouched(j)) {
                                if (rc->jumpingSince <= 0 && pc->linearVelocity.y == 0) {
                                    rc->jumpTimes.push_back(rc->elapsed);
                                    rc->jumpDurations.push_back(dt);
                                }
                            } else if (!rc->jumpTimes.empty()) {
                                float& d = *(rc->jumpDurations.rbegin());
                                d = glm::min(d + dt, RunnerSystem::MaxJumpDuration);
                            }
                            break;
                        }
                    }
                }

                TransformationComponent* tc = TRANSFORM(sc->currentRunner);
                CAM_TARGET(sc->currentRunner)->offset.y = 0 - tc->position.y;
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
                TEXT_RENDERING(game->scoreText)->text = ObjectSerializer<int>::object2string(PLAYER(sc->players[i])->points);
            }

            thePlatformerSystem.Update(dt);
            thePlayerSystem.Update(dt);
            theRunnerSystem.Update(dt);
            theCameraTargetSystem.Update(dt);

            return Scene::Game;
        }

        ///----------------------------------------------------------------------------//
        ///--------------------- EXIT SECTION -----------------------------------------//
        ///----------------------------------------------------------------------------//
        void onPreExit(Scene::Enum to) {
            BUTTON(pauseButton)->enabled = false;
            if (to != Scene::Pause) {
                ADSR(transition)->active = false;
            }
        }

        bool updatePreExit(Scene::Enum to, float) {
            if (to == Scene::Pause) {
                return true;
            }
            const SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());

            float progress = ADSR(transition)->value;
            updateSessionTransition(session, progress);
            RENDERING(pauseButton)->color.a = progress;

            return progress <= ADSR(transition)->idleValue;
        }

        void onExit(Scene::Enum) {
            RENDERING(pauseButton)->show = false;
        }
};

namespace Scene {
    StateHandler<Scene::Enum>* CreateGameSceneHandler(RecursiveRunnerGame* game) {
        return new GameScene(game);
    }
}



static void spawnGainEntity(int, Entity parent, const Color& color, bool isGhost) {
    Entity e = theEntityManager.CreateEntity("gain");
    ADD_COMPONENT(e, Transformation);
    TRANSFORM(e)->position = TRANSFORM(parent)->position;
    TRANSFORM(e)->rotation = TRANSFORM(parent)->rotation;
    TRANSFORM(e)->size = TRANSFORM(parent)->size;
    TRANSFORM(e)->z = TRANSFORM(parent)->z + (isGhost ? 0.1 : 0.11);
    ADD_COMPONENT(e, Rendering);
    RENDERING(e)->texture = theRenderingSystem.loadTextureFile("lumiere");
    RENDERING(e)->color = color;
    RENDERING(e)->show = true;

    PARTICULE(parent)->duration = 0.1;
    PARTICULE(parent)->initialColor = PARTICULE(parent)->finalColor = Interval<Color> (color, color);
    ADD_COMPONENT(e, AutoDestroy);
    AUTO_DESTROY(e)->type = AutoDestroyComponent::LIFETIME;
    AUTO_DESTROY(e)->params.lifetime.freq.value = 3.5;
    AUTO_DESTROY(e)->params.lifetime.map2AlphaRendering = true;
    // AUTO_DESTROY(e)->hasTextRendering = true;
}

static Entity addRunnerToPlayer(RecursiveRunnerGame* game, Entity player, PlayerComponent* p, int playerIndex, SessionComponent* sc) {
    int direction = ((p->runnersCount + playerIndex) % 2) ? -1 : 1;
    Entity e = theEntityManager.CreateEntity("runner", EntityType::Persistent);
    ADD_COMPONENT(e, Transformation);
    TRANSFORM(e)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("run_l2r_0000")) * 0.68f;

    TRANSFORM(e)->rotation = 0;
    TRANSFORM(e)->z = 0.8 + 0.01 * p->runnersCount;
    ADD_COMPONENT(e, Rendering);
    RENDERING(e)->show = true;
    RENDERING(e)->color = Color(12.0/255, 4.0/255, 41.0/255);
    ADD_COMPONENT(e, Runner);

    TRANSFORM(e)->position = AnchorSystem::adjustPositionWithCardinal(
        glm::vec2(direction * -(param::LevelSize * PlacementHelper::ScreenWidth + TRANSFORM(e)->size.x) * 0.5, game->baseLine),
        TRANSFORM(e)->size,
        Cardinal::S);
    RUNNER(e)->startPoint = TRANSFORM(e)->position;
    RUNNER(e)->endPoint = glm::vec2(direction * (param::LevelSize * PlacementHelper::ScreenWidth + TRANSFORM(e)->size.x) * 0.5, 0);
    RUNNER(e)->speed = direction * (param::speedConst + param::speedCoeff * p->runnersCount);
    RUNNER(e)->startTime = 0;//MathUtil::RandomFloatInRange(1,3);
    RUNNER(e)->playerOwner = player;
    ADD_COMPONENT(e, Platformer);
    PLATFORMER(e)->offset = glm::vec2(0, TRANSFORM(e)->size.y * -0.5);
    PLATFORMER(e)->platforms.insert(std::make_pair(game->ground, true));
    for (unsigned i=0; i<sc->platforms.size(); i++) {
        PLATFORMER(e)->platforms.insert(std::make_pair(sc->platforms[i].platform, sc->platforms[i].active));
    }
    std::cout <<" add runner: " << e << " - " << "(pos: " << TRANSFORM(e)->position.x << ',' << TRANSFORM(e)->position.y << ")" << RUNNER(e)->speed << std::endl;
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
    int idx = (int)glm::linearRand(0.0f, (float)p->colors.size());
    RUNNER(e)->color = p->colors[idx];
    p->colors.erase(p->colors.begin() + idx);
    #endif
    ADD_COMPONENT(e, CameraTarget);
    CAM_TARGET(e)->camera = game->cameraEntity;
    CAM_TARGET(e)->maxCameraSpeed = direction * RUNNER(e)->speed;
    ADD_COMPONENT(e, Physics);
    PHYSICS(e)->mass = 1;
    ADD_COMPONENT(e, Animation);
    ANIMATION(e)->name = "runL2R";
    ANIMATION(e)->playbackSpeed = 1.1;
    RENDERING(e)->mirrorH = (direction < 0);

    Entity collisionZone = theEntityManager.CreateEntity("collision_zone", EntityType::Persistent);
    ADD_COMPONENT(collisionZone, Transformation);
    ADD_COMPONENT(collisionZone, Anchor);
    ANCHOR(collisionZone)->parent = e;
    ANCHOR(collisionZone)->z = 0.01;
    #if 0
    ADD_COMPONENT(collisionZone, Rendering);
    RENDERING(collisionZone)->show = true;
    RENDERING(collisionZone)->color = Color(1,0,0,1);
    #endif
    RUNNER(e)->collisionZone = collisionZone;
    p->runnersCount++;
    LOGI("Add runner " << e << " at pos : {" << TRANSFORM(e)->position.x << ", " << TRANSFORM(e)->position.y << "}, speed: " << RUNNER(e)->speed << " (player=" << player << ')');
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
    const auto* collisionZone = TRANSFORM(rc->collisionZone);
    const int end = sc->coins.size();
    Entity prev = 0;

    for(int idx=0; idx<end; idx++) {
        Entity coin = rc->speed > 0 ? sc->coins[idx] : sc->coins[end - idx - 1];
        if (std::find(rc->coins.begin(), rc->coins.end(), coin) == rc->coins.end()) {
            const TransformationComponent* tCoin = TRANSFORM(coin);
            if (IntersectionUtil::rectangleRectangle(
                collisionZone->position, collisionZone->size, collisionZone->rotation,
                tCoin->position, tCoin->size * glm::vec2(0.5, 0.6), tCoin->rotation)) {
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
                player->points += gain;

                //coins++ only for player, not his ghosts
                if (sc->currentRunner == e)
                    player->coins++;

                spawnGainEntity(gain, coin, rc->color, rc->ghost);
            }
        }
        prev = coin;
    }
}

#include "Tutorial.h"

namespace Scene {
    StateHandler<Scene::Enum>* CreateTutorialSceneHandler(RecursiveRunnerGame* game) {
        return new TutorialScene(game);
    }
}
