/*
    This file is part of RecursiveRunner.

    @author Soupe au Caillou - Jordane Pelloux-Prayer
    @author Soupe au Caillou - Gautier Pelloux-Prayer
    @author Soupe au Caillou - Pierre-Eric Pelloux-Prayer

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
#include "systems/TextSystem.h"
#include "systems/SoundSystem.h"
#include "systems/MusicSystem.h"
#include "systems/PhysicsSystem.h"
#include "systems/ParticuleSystem.h"
#include "systems/AutoDestroySystem.h"
#include "util/IntersectionUtil.h"
#include "util/Random.h"
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

static Entity addRunnerToPlayer(RecursiveRunnerGame* game, Entity player, PlayerComponent* p, int playerIndex, SessionComponent* sc);
static void updateSessionTransition(const SessionComponent* session, float progress);
static void checkCoinsPickupForRunner(PlayerComponent* player, Entity e, RunnerComponent* rc, SessionComponent* sc);

class GameScene : public StateHandler<Scene::Enum> {
    RecursiveRunnerGame* game;
    Entity pauseButton;
    Entity session;
    Entity transition;

public:
        GameScene(RecursiveRunnerGame* game) : StateHandler<Scene::Enum>("game") {
            this->game = game;
        }

        void setup(AssetAPI*) override {
            pauseButton = theEntityManager.CreateEntity(HASH("pause_buttton", 0x62b4dbd4),
                EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("menu/button"));
            ANCHOR(pauseButton)->parent = game->muteBtn;
            ANCHOR(pauseButton)->z = 0;
            ANCHOR(pauseButton)->position =
                PlacementHelper::GimpSizeToScreen(glm::vec2(1092, 0));
            RENDERING(pauseButton)->texture = HASH("pause", 0xaf9ecc33);
            RENDERING(pauseButton)->show = false;

            transition = theEntityManager.CreateEntity(HASH("transition_helper", 0x91fede42),
                EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("transition_helper"));
        }


        ///----------------------------------------------------------------------------//
        ///--------------------- ENTER SECTION ----------------------------------------//
        ///----------------------------------------------------------------------------//
        void onPreEnter(Scene::Enum from) {
            ADSR(transition)->active = true;

            if (theSessionSystem.entityCount() == 0) {
                RecursiveRunnerGame::startGame(game->level, true);
                MUSIC(transition)->fadeOut = 2;
                MUSIC(transition)->volume = 1;
                MUSIC(transition)->music = theMusicSystem.loadMusicFile("sounds/jeu.ogg");
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

                game->successManager.gameStart(from == Scene::Tutorial);
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
            TRANSFORM(game->pianist)->position.x) / (PlacementHelper::ScreenSize.x * param::LevelSize);
            MUSIC(transition)->volume = 0.2 + 0.8 * (1 - distanceAbs);

            // Manage player's current runner
            for (unsigned i=0; i<sc->numPlayers; i++) {
                CAM_TARGET(sc->currentRunner)->enabled = true;
                CAM_TARGET(sc->currentRunner)->offset = glm::vec2(
                    ((RUNNER(sc->currentRunner)->speed > 0) ? 1 :-1) * 0.4 * PlacementHelper::ScreenSize.x,
                    0 - TRANSFORM(sc->currentRunner)->position.y);

                // If current runner has reached the edge of the screen
                if (RUNNER(sc->currentRunner)->finished) {
                    LOGI(sc->currentRunner << " finished, add runner or end game");
                    CAM_TARGET(sc->currentRunner)->enabled = false;
                    game->successManager.oneMoreRunner(RUNNER(sc->currentRunner)->totalCoinsEarned);

                    if (PLAYER(sc->players[i])->runnersCount == param::runner) {
                        // Game is finished, show either Rate Menu or Main Menu
                        LOGW("Apprater is disabled yet!");
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
#if SAC_BENCHMARK_MODE
                        if (true) {
#else
                        if (theTouchInputManager.isTouched(j)) {
#endif
                            if (sc->numPlayers == 2) {
                                const glm::vec2& ppp = theTouchInputManager.getTouchLastPosition(j);
                                if (i == 0 && ppp.y < 0)
                                    continue;
                                if (i == 1 && ppp.y > 0)
                                    continue;
                            }
                            PhysicsComponent* pc = PHYSICS(sc->currentRunner);
                            RunnerComponent* rc = RUNNER(sc->currentRunner);

#if SAC_BENCHMARK_MODE
                            if (Random::Float() > 0.9) {
#else
                            if (!theTouchInputManager.wasTouched(j)) {
#endif
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

                                game->successManager.oneLessRunner();
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
                    sc->stats.runner[j].lifetime += dt;

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
                const auto& platformers = thePlatformerSystem.RetrieveAllEntityWithComponent();
                for (unsigned i=0; i<sc->platforms.size(); i++) {
                    Platform& pt = sc->platforms[i];
                    bool active = pt.switches[1].state & pt.switches[1].state & (
                        pt.switches[1].owner == pt.switches[0].owner);
                    if (active != pt.active) {
                        pt.active = active;
                        std::cout << "platform #" << i << " is now : " << active << std::endl;
                        if (active) {
                            RENDERING(pt.platform)->texture = HASH("link", 0x0);
                        } else {
                            RENDERING(pt.platform)->texture = InvalidTextureRef;
                        }

                        for (auto pl: platformers) {
                            PLATFORMER(pl)->platforms[pt.platform] = active;
                        }
                    }
                }
            }

            // Show the score(s)
            for (unsigned i=0; i<sc->players.size(); i++) {
                TEXT(game->scoreText)->text = ObjectSerializer<int>::object2string(PLAYER(sc->players[i])->points);
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

                game->successManager.gameEnd(SESSION(session));
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

static Entity addRunnerToPlayer(RecursiveRunnerGame* game, Entity player, PlayerComponent* p, int playerIndex, SessionComponent* sc) {
    int direction = ((p->runnersCount + playerIndex) % 2) ? -1 : 1;
    Entity e = theEntityManager.CreateEntityFromTemplate("ingame/runner");
    TRANSFORM(e)->size *= .68f;
    TRANSFORM(e)->z += 0.01 * p->runnersCount;
    TRANSFORM(e)->position = AnchorSystem::adjustPositionWithCardinal(
        glm::vec2(direction * -(param::LevelSize * PlacementHelper::ScreenSize.x + TRANSFORM(e)->size.x) * 0.5, game->baseLine),
        TRANSFORM(e)->size,
        Cardinal::S);
    RUNNER(e)->startPoint = TRANSFORM(e)->position;
    RUNNER(e)->endPoint = glm::vec2(direction * (param::LevelSize * PlacementHelper::ScreenSize.x + TRANSFORM(e)->size.x) * 0.5, 0);
    RUNNER(e)->speed = direction * (param::speedConst + param::speedCoeff * p->runnersCount);
    RUNNER(e)->startTime = 0;//MathUtil::RandomFloatInRange(1,3);
    RUNNER(e)->playerOwner = player;

    PLATFORMER(e)->offset = glm::vec2(0, TRANSFORM(e)->size.y * -0.5);
    PLATFORMER(e)->platforms.insert(std::make_pair(game->ground, true));
    for (unsigned i=0; i<sc->platforms.size(); i++) {
        PLATFORMER(e)->platforms.insert(std::make_pair(sc->platforms[i].platform, sc->platforms[i].active));
    }

    int idx = (int)glm::linearRand(0.0f, (float)p->colors.size());
    RUNNER(e)->color = p->colors[idx];
    p->colors.erase(p->colors.begin() + idx);

    CAM_TARGET(e)->camera = game->cameraEntity;
    CAM_TARGET(e)->maxCameraSpeed = direction * RUNNER(e)->speed;

    if (direction < 0)
        RENDERING(e)->flags |= RenderingFlags::MirrorHorizontal;
    else
        RENDERING(e)->flags &= ~(RenderingFlags::MirrorHorizontal);

    Entity collisionZone = theEntityManager.CreateEntityFromTemplate("ingame/collision_zone");
    ANCHOR(collisionZone)->parent = e;
    RUNNER(e)->collisionZone = collisionZone;
    RUNNER(e)->index = p->runnersCount;

    p->runnersCount++;
    LOGI("Add runner " << e << " at pos : " << TRANSFORM(e)->position << "}, speed: " <<
        RUNNER(e)->speed << " (player=" << player << ')');

    return e;
}

static void updateSessionTransition(const SessionComponent* session, float progress) {
    for (unsigned i=0; i<session->coins.size(); i++) {
        RENDERING(session->coins[i])->color.a = progress;
    }
    for (unsigned i=0; i<session->links.size(); i++) {
        RENDERING(session->links[i])->color.a = progress;
    }
    theRunnerSystem.forEachEntityDo([progress] (Entity e) -> void {
        RENDERING(e)->color.a = progress;
    });
}

static void checkCoinsPickupForRunner(PlayerComponent* player, Entity e, RunnerComponent* rc, SessionComponent* sc) {
    const auto* collisionZone = TRANSFORM(rc->collisionZone);
    const int end = sc->coins.size();
    Entity prev = 0;

    for(int i=0; i<end; i++) {
        int idx = (rc->speed > 0) ? i : (end - i - 1);
        Entity coin = sc->coins[idx];
        /* lookup if runner has already picked up that coin */
        if (std::find(rc->coins.begin(), rc->coins.end(), coin) == rc->coins.end()) {
            /* if not, test for intersection */
            const TransformationComponent* tCoin = TRANSFORM(coin);
            if (IntersectionUtil::rectangleRectangle(
                collisionZone->position, collisionZone->size, collisionZone->rotation,
                tCoin->position, tCoin->size * glm::vec2(0.5, 0.6) /* why ? */, tCoin->rotation)) {
                /* if coin isn't the 1st one picked, check for consecutive pickup bonus */
                if (!rc->coins.empty()) {
                 int linkIdx = idx;
                    if (rc->coins.back() == prev) {
                        rc->coinSequenceBonus++;
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
                    } else {
                        rc->coinSequenceBonus = 1;
                    }
                }
                rc->coins.push_back(coin);
                int gain = 10 * pow(2.0f, rc->oldNessBonus) * rc->coinSequenceBonus;
                player->points += gain;

                /* update statistics */
                {
                    sc->stats.runner[rc->index].coinsCollected++;
                    sc->stats.runner[rc->index].pointScored += gain;
                }

                //coins++ only for player, not his ghosts
                if (sc->currentRunner == e)
                    player->coins++;

                /* reset lifetime of gain entity */
                AUTO_DESTROY(sc->gains[idx])->params.lifetime.freq.accum = 0;
                RENDERING(sc->gains[idx])->show = 1;
                RENDERING(sc->gains[idx])->color = rc->color;
            }
        }
        prev = coin;
    }
}

#define TUTORIAL_COMPILE_GUARD
#include "TutorialScene.cpp"
