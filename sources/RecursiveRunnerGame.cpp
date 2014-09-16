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
#include "RecursiveRunnerGame.h"

#include "Parameters.h"

#include <sstream>

#include "base/Log.h"
#include "base/TouchInputManager.h"
#include "base/EntityManager.h"
#include "base/TimeUtil.h"
#include "base/PlacementHelper.h"
#include "base/ObjectSerializer.h"
#include "base/StateMachine.inl"

#include "systems/ADSRSystem.h"
#include "systems/AnchorSystem.h"
#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/ButtonSystem.h"
#include "systems/TextSystem.h"
#include "systems/SoundSystem.h"
#include "systems/MusicSystem.h"
#include "systems/AnimationSystem.h"
#include "systems/ParticuleSystem.h"
#include "systems/AutoDestroySystem.h"
#include "systems/CameraSystem.h"

#include "systems/RunnerSystem.h"
#include "systems/CameraTargetSystem.h"
#include "systems/PlayerSystem.h"
#include "systems/RangeFollowerSystem.h"
#include "systems/SessionSystem.h"
#include "systems/PlatformerSystem.h"

#include "api/LocalizeAPI.h"
#include "api/StorageAPI.h"
#include "api/InAppPurchaseAPI.h"
#include "util/ScoreStorageProxy.h"

#include "util/RecursiveRunnerDebugConsole.h"
#include "util/Random.h"

#include <glm/gtc/random.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <iomanip>


#include "systems/AutonomousAgentSystem.h"
#include "systems/CollisionSystem.h"
#include "systems/ContainerSystem.h"
#include "systems/GraphSystem.h"
#include "systems/GridSystem.h"
#include "systems/MorphingSystem.h"
#include "systems/ScrollingSystem.h"
#include "systems/ZSQDSystem.h"
#include "systems/SpotSystem.h"
#include "systems/SwypeButtonSystem.h"

extern std::map<TextureRef, CollisionZone> texture2Collision;

float RecursiveRunnerGame::nextRunnerStartTime[100];
int RecursiveRunnerGame::nextRunnerStartTimeIndex;

RecursiveRunnerGame::RecursiveRunnerGame(): Game() {
}


RecursiveRunnerGame::~RecursiveRunnerGame() {
    LOGW("Delete game instance " << this << " " << &theEntityManager);
    theEntityManager.deleteAllEntities();

    RunnerSystem::DestroyInstance();
    CameraTargetSystem::DestroyInstance();
    PlayerSystem::DestroyInstance();
    RangeFollowerSystem::DestroyInstance();
    SessionSystem::DestroyInstance();
    PlatformerSystem::DestroyInstance();
}

bool RecursiveRunnerGame::wantsAPI(ContextAPI::Enum api) const {
    switch (api) {
        case ContextAPI::Asset:
        case ContextAPI::Communication:
        case ContextAPI::Exit:
        #if SAC_RESTRICTIVE_PLUGINS
        case ContextAPI::GameCenter:
        // case ContextAPI::InAppPurchase:
        #endif
        // case ContextAPI::InAppPurchase:
        case ContextAPI::Localize:
        case ContextAPI::Music:
        case ContextAPI::OpenURL:
        case ContextAPI::Sound:
        case ContextAPI::Storage:
        case ContextAPI::Vibrate:
            return true;
        default:
            return false;
    }
}

void RecursiveRunnerGame::sacInit(int windowW, int windowH) {
    LOGI("SAC engine initialisation begins:");
    Game::sacInit(windowW, windowH);

    LOGI("\t- Create RecursiveRunner specific systems...");
    RunnerSystem::CreateInstance();
    CameraTargetSystem::CreateInstance();
    PlayerSystem::CreateInstance();
    RangeFollowerSystem::CreateInstance();
    SessionSystem::CreateInstance();
    PlatformerSystem::CreateInstance();

    LOGI("\t- Init sceneStateMachine...");
    sceneStateMachine.registerState(Scene::Logo, Scene::CreateLogoSceneHandler(this));
    sceneStateMachine.registerState(Scene::Menu, Scene::CreateMenuSceneHandler(this));
    sceneStateMachine.registerState(Scene::Pause, Scene::CreatePauseSceneHandler(this));
    sceneStateMachine.registerState(Scene::Rate, Scene::CreateRateSceneHandler(this));
    sceneStateMachine.registerState(Scene::Game, Scene::CreateGameSceneHandler(this));
    sceneStateMachine.registerState(Scene::RestartGame, Scene::CreateRestartGameSceneHandler(this));
    sceneStateMachine.registerState(SCENE_TUTORIAL, Scene::CreateTutorialSceneHandler(this));
    sceneStateMachine.registerState(Scene::About, Scene::CreateAboutSceneHandler(this));

    // load anim files
    LOGI("\t- Load animations...");
    theAnimationSystem.loadAnim(renderThreadContext->assetAPI, "disappear2", "disappear2");
    theAnimationSystem.loadAnim(renderThreadContext->assetAPI, "fumee_start", "fumee_start");
    theAnimationSystem.loadAnim(renderThreadContext->assetAPI, "fumee_loop", "fumee_loop");
    theAnimationSystem.loadAnim(renderThreadContext->assetAPI, "fumee_end", "fumee_end");
    theAnimationSystem.loadAnim(renderThreadContext->assetAPI, "jumpL2R_down", "jumpL2R_down");
    theAnimationSystem.loadAnim(renderThreadContext->assetAPI, "jumpL2R_up", "jumpL2R_up");
    theAnimationSystem.loadAnim(renderThreadContext->assetAPI, "jumptorunL2R", "jumptorunL2R");
    theAnimationSystem.loadAnim(renderThreadContext->assetAPI, "piano", "piano");
    theAnimationSystem.loadAnim(renderThreadContext->assetAPI, "piano2", "piano2");
    theAnimationSystem.loadAnim(renderThreadContext->assetAPI, "pianojournal", "pianojournal");
    theAnimationSystem.loadAnim(renderThreadContext->assetAPI, "runL2R", "runL2R");

    Game::buildOrderedSystemsToUpdateList();

    LOGI("SAC engine initialisation done.");
    PlacementHelper::GimpSize = glm::vec2(1280, 800);
}

void fumee(Entity building) {
    const glm::vec2 possible[] = {
        glm::vec2(-445 / 942.0, 292 / 594.0),
        glm::vec2(-310 / 942.0, 260 / 594.0),
        glm::vec2(-52 / 942.0, 255 / 594.0),
        glm::vec2(147 / 942.0, 239 / 594.0),
        glm::vec2(269 / 942.0, 218 / 594.0),
        glm::vec2(442 / 942.0, 239 / 594.0)
    };

    unsigned count = 6;//MathUtil::RandomIntInRange(1, 4);
    std::vector<int> indexes;
    do {
        int idx = Random::Int(0, 6);
        if (std::find(indexes.begin(), indexes.end(), idx) == indexes.end()) {
            indexes.push_back(idx);
        }
    } while (indexes.size() != count);

    for (unsigned i=0; i<indexes.size(); i++) {
        int idx = indexes[i];
        Entity fumee = theEntityManager.CreateEntityFromTemplate("background/fumee");

        glm::vec2 position = possible[idx] * TRANSFORM(building)->size + glm::vec2(0, TRANSFORM(fumee)->size.y * 0.5);
        if (RENDERING(building)->flags & RenderingFlags::MirrorHorizontal)
            position.x = -ANCHOR(fumee)->position.x;
        ANCHOR(fumee)->parent = building;
        ANCHOR(fumee)->z = -0.1;
        ANCHOR(fumee)->position = position;
        ANIMATION(fumee)->waitAccum = Random::Float(0.0f, 10.f);
    }
}

void RecursiveRunnerGame::decor() {
    LOGT("Position not handled in .entity file yet!");
    PlacementHelper::ScreenSize.x *= 3;
    PlacementHelper::GimpSize = glm::vec2(1280 * 3, 800);


    silhouette = theEntityManager.CreateEntity(HASH("silhouette", 0x1be67565),
        EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("background/silhouette"));
    TRANSFORM(silhouette)->position = AnchorSystem::adjustPositionWithCardinal(
        glm::vec2(0, PlacementHelper::GimpYToScreen(0)), TRANSFORM(silhouette)->size, Cardinal::N);

    route = theEntityManager.CreateEntity(HASH("road", 0x4875b6d4),
        EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("background/road"));
    TRANSFORM(route)->position = AnchorSystem::adjustPositionWithCardinal(
        glm::vec2(0, PlacementHelper::GimpYToScreen(800)), TRANSFORM(route)->size, Cardinal::S);

    Entity buildings = theEntityManager.CreateEntity(HASH("buildings", 0x240377a9),
        EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("background/city-object"));

    Entity trees = theEntityManager.CreateEntity(HASH("trees", 0xe84e02fd),
        EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("background/city-object"));

    int count = 33;
    struct Decor {
        float x, y, z;
        Cardinal::Enum ref;
        const char* texture;
        bool mirrorUV;
        Entity parent;
        Decor(float _x=0, float _y=0, float _z=0, Cardinal::Enum _ref=Cardinal::C, const char* _texture="", bool _mirrorUV=false, Entity _parent=0) :
            x(_x), y(_y), z(_z), ref(_ref), texture(_texture), mirrorUV(_mirrorUV), parent(_parent) {}
    };

    Decor def[] = {
        // buildings
        Decor(554, 149, 0.2, Cardinal::NE, "immeuble", false, buildings),
        Decor(1690, 149, 0.2, Cardinal::NE, "immeuble", false, buildings),
        Decor(3173, 139, 0.2, Cardinal::NW, "immeuble", false, buildings),
        Decor(358, 404, 0.25, Cardinal::NW, "maison", true, buildings),
        Decor(2097, 400, 0.25, Cardinal::NE, "maison", false, buildings),
        Decor(2053, 244, 0.29, Cardinal::NW, "usine_desaf", false, buildings),
        Decor(3185, 298, 0.22, Cardinal::NE, "usine2", true, buildings),
        // trees
        Decor(152, 780, 0.5, Cardinal::S, "arbre3", false, trees),
        Decor(522, 780, 0.5, Cardinal::S, "arbre2", false, trees),
        Decor(812, 774, 0.45, Cardinal::S, "arbre5", false, trees),
        Decor(1162, 792, 0.5, Cardinal::S, "arbre4", false, trees),
        Decor(1418, 790, 0.45, Cardinal::S, "arbre2", false, trees),
        Decor(1600, 768, 0.42, Cardinal::S, "arbre1", false, trees),
        Decor(1958, 782, 0.5, Cardinal::S, "arbre4", true, trees),
        Decor(2396, 774, 0.44, Cardinal::S, "arbre5", false, trees),
        Decor(2684, 784, 0.45, Cardinal::S, "arbre3", false, trees),
        Decor(3022, 764, 0.42, Cardinal::S, "arbre1", false, trees),
        Decor(3290, 764, 0.41, Cardinal::S, "arbre1", true, trees),
        Decor(3538, 768, 0.44, Cardinal::S, "arbre2", false, trees),
        Decor(3820, 772, 0.5, Cardinal::S, "arbre4", false, trees),
        // benchs
        Decor(672, 768, 0.35, Cardinal::S, "bench_cat", false, trees),
        Decor(1090, 764, 0.35, Cardinal::S, "bench", false, trees),
        Decor(2082, 760, 0.35, Cardinal::S, "bench", true, trees),
        Decor(2526, 762, 0.35, Cardinal::S, "bench", false, trees),
        Decor(3464, 758, 0.35, Cardinal::S, "bench_cat", false, trees),
        Decor(3612, 762, 0.6, Cardinal::S, "bench", false, trees),
        // lampadaire
        Decor(472, 748, 0.3, Cardinal::S, "lampadaire2", false, trees),
        Decor(970, 748, 0.3, Cardinal::S, "lampadaire3", false, trees),
        Decor(1740, 748, 0.3, Cardinal::S, "lampadaire2", false, trees),
        Decor(2208, 748, 0.3, Cardinal::S, "lampadaire1", false, trees),
        Decor(2620, 748, 0.3, Cardinal::S, "lampadaire3", false, trees),
        Decor(3182, 748, 0.3, Cardinal::S, "lampadaire1", false, trees),
        Decor(3732, 748, 0.3, Cardinal::S, "lampadaire3", false, trees),
    };
    // pour les arbres
    glm::vec2 v[5][4] = {
        {glm::vec2(71, 123), glm::vec2(73, 114), glm::vec2(125, 126), glm::vec2(92, 216)},
        {glm::vec2(116, 153), glm::vec2(78, 155), glm::vec2(100, 168), glm::vec2(44, 46)},
        {glm::vec2(69, 178), glm::vec2(155, 125), glm::vec2(163, 116), glm::vec2(206, 93)},
        {glm::vec2(152, 160), glm::vec2(85, 138), glm::vec2(84, 111), glm::vec2(168, 171)},
        {glm::vec2(115, 67), glm::vec2(146, 69), glm::vec2(132, 107), glm::vec2(124, 130)}
        };
    glm::vec2 o[5][4] = {
        {glm::vec2(0, 0), glm::vec2(313, 0), glm::vec2(0, 498), glm::vec2(293, 624-216)},
        {glm::vec2(0, 250), glm::vec2(0, 0), glm::vec2(137, 244), glm::vec2(193, 0)},
        {glm::vec2(0, 0), glm::vec2(308, 0), glm::vec2(300, 510), glm::vec2(0, 499)},
        {glm::vec2(0, 460), glm::vec2(0, 0), glm::vec2(438, 0), glm::vec2(354, 449)},
        {glm::vec2(0, 0), glm::vec2(234, 0), glm::vec2(248, 259), glm::vec2(0, 236)}
        };
    // pour les batiments
    glm::vec2 vBat[][4] = {
        {glm::vec2(901, 44), glm::vec2(215, 35), glm::vec2(174, 41), glm::vec2(265, 37)},
        {glm::vec2(143, 55), glm::vec2(83, 51), glm::vec2(82, 47), glm::vec2(0.0f)},
        {glm::vec2(324, 15), glm::vec2(0.0f), glm::vec2(0.0f), glm::vec2(0.0f)},
        {glm::vec2(124, 233), glm::vec2(630, 69), glm::vec2(245, 71), glm::vec2(0.0f)},
    };
    glm::vec2 oBat[][4] = {
        {glm::vec2(43, 0), glm::vec2(172, 44), glm::vec2(424, 44), glm::vec2(629, 44)},
        {glm::vec2(12, 0), glm::vec2(169, 0), glm::vec2(344, 0), glm::vec2(0.0f)},
        {glm::vec2(8, 0), glm::vec2(0.0f), glm::vec2(0.0f), glm::vec2(0.0f)},
        {glm::vec2(8, 0), glm::vec2(249, 0), glm::vec2(573, 69), glm::vec2(0.0f)},

    };

    char* fullname = (char*)alloca(256);
    for (int i=0; i<count; i++) {
        const Decor& bdef = def[i];
        strcpy(fullname, "decor/");
        strcat(fullname, bdef.texture);

        Entity b = theEntityManager.CreateEntity(Murmur::RuntimeHash(fullname));
        {
            ADD_COMPONENT(b, Transformation);
            auto tb = TRANSFORM(b);
            tb->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize(bdef.texture));
            TRANSFORM(b)->position =
                AnchorSystem::adjustPositionWithCardinal(
                glm::vec2(PlacementHelper::GimpXToScreen(bdef.x), PlacementHelper::GimpYToScreen(bdef.y)), tb->size, bdef.ref);
            TRANSFORM(b)->z = bdef.z;
            ADD_COMPONENT(b, Rendering);
            RENDERING(b)->texture = theRenderingSystem.loadTextureFile(bdef.texture);
            RENDERING(b)->show = true;
            RENDERING(b)->flags = RenderingFlags::NonOpaque;
            if (bdef.mirrorUV)
                RENDERING(b)->flags |= RenderingFlags::MirrorHorizontal;
            if (bdef.parent) {
                ADD_COMPONENT(b, Anchor);
                ANCHOR(b)->parent = bdef.parent;
                ANCHOR(b)->position = TRANSFORM(b)->position;
                ANCHOR(b)->z = TRANSFORM(b)->z;
            } else {
                RENDERING(b)->flags |= RenderingFlags::Constant;
            }

        }
        decorEntities.push_back(b);

        if (i < 3) {
            fumee(b);
        }
        glm::vec2* zPrepassSize = 0, *zPrepassOffset = 0;

        if (strncmp(bdef.texture, "arbre", 5) == 0) {
            char c = bdef.texture[5];
            int idx = (int)c - (int)'0' - 1;
            zPrepassSize = v[idx];
            zPrepassOffset = o[idx];
        } else if (!strcmp(bdef.texture, "immeuble")) {
            zPrepassSize = vBat[0];
            zPrepassOffset = oBat[0];
        } else if (!strcmp(bdef.texture, "maison")) {
            zPrepassSize = vBat[1];
            zPrepassOffset = oBat[1];
        } else if (!strcmp(bdef.texture, "usine2")) {
            zPrepassSize = vBat[2];
            zPrepassOffset = oBat[2];
        } else if (!strcmp(bdef.texture, "usine_desaf")) {
            zPrepassSize = vBat[3];
            zPrepassOffset = oBat[3];
        }
        if (zPrepassSize) {
            const glm::vec2 size = TRANSFORM(b)->size;
            #if 0
            Color color = Color::random();
            color.a = 0.6;
            #endif
            for (int j=0; j<4; j++) {
                if (zPrepassSize[j] == glm::vec2(0.0f))
                    break;

                strcpy(fullname, "decor/");
                strcat(fullname, bdef.texture);
                strcat(fullname, "_z_pre-pass");
                Entity bb = theEntityManager.CreateEntity(Murmur::RuntimeHash(fullname));
                ADD_COMPONENT(bb, Transformation);
                TRANSFORM(bb)->size = PlacementHelper::GimpSizeToScreen(zPrepassSize[j]);
                glm::vec2 ratio(zPrepassOffset[j] / theRenderingSystem.getTextureSize(bdef.texture));
                ratio.y = 1 - ratio.y;
                glm::vec2 pos =
                    size * (glm::vec2(-0.5) + ratio)+ TRANSFORM(bb)->size * glm::vec2(0.5, -0.5);
                if (bdef.mirrorUV)
                    pos.x = -pos.x;
                TRANSFORM(bb)->z = TRANSFORM(b)->z;
                TRANSFORM(bb)->position = TRANSFORM(b)->position + pos;
                ADD_COMPONENT(bb, Rendering);
                *RENDERING(bb) = *RENDERING(b);
                RENDERING(bb)->flags = RenderingFlags::ZPrePass;
            }
        }
    }

    theEntityManager.CreateEntity(HASH("background/banderolle", 0xa00038cb),
        EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("background/banderolle"));

    bestScore = theEntityManager.CreateEntity(HASH("background/best_score", 0x99691578),
        EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("background/best_score"));

    const bool muted =
#if SAC_EMSCRIPTEN
            false;
#else
            gameThreadContext->storageAPI->isOption("sound", "off");
#endif
    pianist = theEntityManager.CreateEntity(HASH("background/pianist", 0x136d34ba),
        EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("background/pianist"));
    ANIMATION(pianist)->name = muted ? HASH("pianojournal", 0xfd122906) : HASH("piano", 0x62205ad5);
    ANCHOR(pianist)->parent = trees;

    ADD_COMPONENT(cameraEntity, RangeFollower);
    RANGE_FOLLOWER(cameraEntity)->range = Interval<float>(
        leftMostCameraPos.x, -leftMostCameraPos.x);

    ADD_COMPONENT(route, RangeFollower);
    RANGE_FOLLOWER(route)->range = RANGE_FOLLOWER(cameraEntity)->range;
    RANGE_FOLLOWER(route)->parent = cameraEntity;

    ADD_COMPONENT(silhouette, RangeFollower);
    RANGE_FOLLOWER(silhouette)->range = Interval<float>(-6, 6);
    /*RANGE_FOLLOWER(silhouette)->range = Interval<float>(
        -PlacementHelper::ScreenSize.x * (LEVEL_SIZE * 0.45 - 0.5), PlacementHelper::ScreenSize.x * (LEVEL_SIZE * 0.45 - 0.5));*/
    RANGE_FOLLOWER(silhouette)->parent = cameraEntity;

    ADD_COMPONENT(buildings, RangeFollower);
    RANGE_FOLLOWER(buildings)->range = Interval<float>(-2, 2);
    RANGE_FOLLOWER(buildings)->parent = cameraEntity;

    ADD_COMPONENT(trees, RangeFollower);
    RANGE_FOLLOWER(trees)->range = Interval<float>(-1, 1);
    RANGE_FOLLOWER(trees)->parent = cameraEntity;

    PlacementHelper::GimpSize = glm::vec2(1280, 800);
    PlacementHelper::ScreenSize.x /= 3;

    buttonSpacing.H = PlacementHelper::GimpWidthToScreen(94);
    buttonSpacing.V = PlacementHelper::GimpHeightToScreen(76);

    muteBtn = theEntityManager.CreateEntity(HASH("mute_button", 0x8b1ba537),
        EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("menu/button"));
    ANCHOR(muteBtn)->parent = cameraEntity;
    ANCHOR(muteBtn)->position = TRANSFORM(cameraEntity)->size * glm::vec2(-0.5, 0.5)
        + glm::vec2(buttonSpacing.H, -buttonSpacing.V);
    RENDERING(muteBtn)->texture = theRenderingSystem.loadTextureFile(muted ? "son-off" : "son-on");
    BUTTON(muteBtn)->enabled = true;

    theSoundSystem.mute = muted;
    theMusicSystem.toggleMute(muted);
}

void RecursiveRunnerGame::initGame() {
    Color::nameColor(Color(0.8, 0.8, 0.8), "gray");
    baseLine = PlacementHelper::GimpYToScreen(800);
    leftMostCameraPos =
        glm::vec2(-PlacementHelper::ScreenSize.x * (param::LevelSize * 0.5 - 0.5),
        baseLine + theRenderingSystem.screenH * 0.5);

    LOGI("Creating cameras");
    Entity camera = cameraEntity = theEntityManager.CreateEntity(HASH("camera1", 0xc6993429),
        EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("camera"));
    TRANSFORM(camera)->position = leftMostCameraPos;
    theTouchInputManager.setCamera(camera);

    decor();

    scorePanel = theEntityManager.CreateEntityFromTemplate("background/score_panel");
    TRANSFORM(scorePanel)->position = glm::vec2(0, 0);
    scoreText = theEntityManager.CreateEntity(HASH("score_text", 0xa032241b),
        EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("background/score_text"));

    const auto& players = thePlayerSystem.RetrieveAllEntityWithComponent();
    TEXT(scoreText)->text = players.empty() ?
        "12345"
        : ObjectSerializer<int>::object2string(PLAYER(players.front())->points);

    texture2Collision[HASH("jump_l2r_0000", 0x955adbfa)] =  CollisionZone(90,52,28,84,-0.1);
    texture2Collision[HASH("jump_l2r_0001", 0x2737e4c3)] =  CollisionZone(91,62,27,78,-0.1);
    texture2Collision[HASH("jump_l2r_0002", 0xcfec26f2)] =  CollisionZone(95,74,23,72, -0.1);
    texture2Collision[HASH("jump_l2r_0003", 0x11fae020)] =  CollisionZone(95,74,23,70, -0.1);
    texture2Collision[HASH("jump_l2r_0004", 0x947eb14)] =  CollisionZone(111,95,24,75, -0.3);
    texture2Collision[HASH("jump_l2r_0005", 0x5a2307d8)] =  CollisionZone(114,94,15,84, -0.5);
    texture2Collision[HASH("jump_l2r_0006", 0xdf42bc08)] =  CollisionZone(109,100,20,81, -0.5);
    texture2Collision[HASH("jump_l2r_0007", 0x223d3232)] =  CollisionZone(101, 96,24,85,-0.2);
    texture2Collision[HASH("jump_l2r_0008", 0x70ba05a2)] =  CollisionZone(100, 98,25,74,-0.15);
    texture2Collision[HASH("jump_l2r_0009", 0x4d5e5e36)] =  CollisionZone(95,95,25, 76, 0.0);
    texture2Collision[HASH("jump_l2r_0010", 0x750568e5)] =  CollisionZone(88,96,25,75, 0.);
    texture2Collision[HASH("jump_l2r_0011", 0x40c86c9)] =  CollisionZone(85,95,24,83, 0.4);
    texture2Collision[HASH("jump_l2r_0012", 0x903325ff)] =  CollisionZone(93,100,24,83, 0.2);
    texture2Collision[HASH("jump_l2r_0013", 0x693e3cfa)] =  CollisionZone(110,119,25,64,-0.6);
    texture2Collision[HASH("jump_l2r_0014", 0x6b426fdc)] =  CollisionZone(100, 120,21,60, -0.2);
    texture2Collision[HASH("jump_l2r_0015", 0xeb11f32d)] =  CollisionZone(105, 115,22,62, -0.15);
    texture2Collision[HASH("jump_l2r_0016", 0x79836412)] =  CollisionZone(103,103,24,66,-0.1);
    for (int i=0; i<12; i++) {
        std::stringstream a;
        a << "run_l2r_" << std::setfill('0') << std::setw(4) << i;
        texture2Collision[theRenderingSystem.loadTextureFile(a.str().c_str())] =  CollisionZone(118,103,35,88,-0.5);
    }

    //important! This must be called AFTER camera setup, since we are referencing it (anchor component)
    #if SAC_RESTRICTIVE_PLUGINS
        gamecenterAPIHelper.init(gameThreadContext->gameCenterAPI, true, true, true, [this] {
            gameThreadContext->gameCenterAPI->openSpecificLeaderboard(0);
        });
    #endif
}

void RecursiveRunnerGame::init(const uint8_t* in, int size) {
    LOGI("RecursiveRunnerGame initialisation begins...");

    LOGI("\t- Init database...");
    gameThreadContext->storageAPI->init(gameThreadContext->assetAPI, "RecursiveRunner");
    gameThreadContext->storageAPI->setOption("sound", std::string(), "on");
    gameThreadContext->storageAPI->setOption("gameCount", std::string(), "0");

    ScoreStorageProxy ssp;
    gameThreadContext->storageAPI->createTable(&ssp);

    LOGI("\t- Create camera...");

    successManager.init(this);


    level = Level::Level1;
    initGame();
    if (in == 0 || size == 0) {
        ground = theEntityManager.CreateEntity(HASH("ground", 0x6f0d41f5), EntityType::Persistent);
        ADD_COMPONENT(ground, Transformation);
        TRANSFORM(ground)->size = glm::vec2(PlacementHelper::ScreenSize.x * (param::LevelSize + 1), 0);
        TRANSFORM(ground)->position = glm::vec2(0, baseLine);
    }
    updateBestScore();

    sceneStateMachine.setup(gameThreadContext->assetAPI);

    //recover
    if (size > 0 && in) {
        sceneStateMachine.start(Scene::Pause);
    } else {
#if SAC_DEBUG
        sceneStateMachine.start(Scene::Logo);
#elif SAC_BENCHMARK_MODE
        sceneStateMachine.start(Scene::Game);
#else
        sceneStateMachine.start(Scene::Logo);
#endif
    }

#if SAC_INGAME_EDITORS && SAC_DEBUG
   RecursiveRunnerDebugConsole::init(this);
#endif

   LOGI("RecursiveRunnerGame initialisation done.");
}

void RecursiveRunnerGame::quickInit() {
    // we just need to make sure current state is properly initiated

}

bool RecursiveRunnerGame::willConsumeBackEvent() {
    switch (sceneStateMachine.getCurrentState()) {
        case Scene::Menu:
            return false;
        default:
            return true;
    }
}

void RecursiveRunnerGame::backPressed() {
    switch (sceneStateMachine.getCurrentState()) {
        case Scene::Game:
            sceneStateMachine.forceNewState(Scene::Pause);
            break;
        case Scene::Pause:
            sceneStateMachine.forceNewState(Scene::Menu);
            break;
        case SCENE_TUTORIAL:
            sceneStateMachine.forceNewState(Scene::Menu);
            break;
        default:
            sceneStateMachine.forceNewState(Scene::Menu);
            break;
    }
}

void RecursiveRunnerGame::togglePause(bool pause) {
    if (sceneStateMachine.getCurrentState() == Scene::Game && pause) {
        sceneStateMachine.forceNewState(Scene::Pause);
    }
}

void RecursiveRunnerGame::tick(float dt) {
    TRANSFORM(scorePanel)->position.y =
        AnchorSystem::adjustPositionWithCardinal(
            glm::vec2(0, baseLine + PlacementHelper::ScreenSize.y - ADSR(scorePanel)->value),
            TRANSFORM(scorePanel)->size,
            Cardinal::S).y;

    if (BUTTON(muteBtn)->clicked) {
        //retrieve current state
        bool muted =
#if SAC_EMSCRIPTEN
            false;
#else
            gameThreadContext->storageAPI->isOption("sound", "off");
#endif

        //and invert it
        muted = ! muted;

        //then save it
        gameThreadContext->storageAPI->setOption("sound", muted ? "off" : "on", "on");
        RENDERING(muteBtn)->texture = theRenderingSystem.loadTextureFile(muted ? "son-off" : "son-on");

        theSoundSystem.mute = muted;
        theMusicSystem.toggleMute(muted);

        ignoreClick = true;
    } else {
        ignoreClick = BUTTON(muteBtn)->mouseOver;
        RENDERING(muteBtn)->color = BUTTON(muteBtn)->mouseOver ? Color("gray") : Color();
    }

    {
        bool pianistPlaying = !theMusicSystem.isMuted();
        if (pianistPlaying) {
            pianistPlaying = false;
            theMusicSystem.forEachECDo([&pianistPlaying] (Entity, MusicComponent* mc) -> void {
                if (mc->opaque[0] && mc->control != MusicControl::Pause) {
                    pianistPlaying = true;
                }
            });
         }
         auto* ac = ANIMATION(pianist);

         if (pianistPlaying) {
            // only change if necessary, because pianist alternates between piano and piano2
            if (HASH("pianojournal", 0xfd122906) == ac->name)
                ac->name = HASH("piano", 0x62205ad5);
         } else {
            ANIMATION(pianist)->name = HASH("pianojournal", 0xfd122906);
         }
    }

    //disable "play" click if we are hitting (or trying to) the mute button
    if (theTouchInputManager.isTouched(0)) {
        auto touchPos = theTouchInputManager.getTouchLastPosition(0);
        ignoreClick |= touchPos.y >= (TRANSFORM(muteBtn)->position.y - TRANSFORM(muteBtn)->size.y * BUTTON(muteBtn)->overSize * 0.5);
    }

    sceneStateMachine.update(dt);

    // limit cam position
    if (sceneStateMachine.getCurrentState() != Scene::Menu) {
        for (unsigned i=1; i<2 /* theRenderingSystem.cameras.size()*/; i++) {
            float& camPosX = TRANSFORM(cameraEntity)->position.x;
            if (camPosX < - PlacementHelper::ScreenSize.x * (param::LevelSize * 0.5 - 0.5)) {
                camPosX = - PlacementHelper::ScreenSize.x * (param::LevelSize * 0.5 - 0.5);
            } else if (camPosX > PlacementHelper::ScreenSize.x * (param::LevelSize * 0.5 - 0.5)) {
                camPosX = PlacementHelper::ScreenSize.x * (param::LevelSize * 0.5 - 0.5);
            }
            // TRANSFORM(silhouette)->position.X = TRANSFORM(route)->position.X = camPosX;
            TRANSFORM(cameraEntity)->position.x = camPosX;
            TRANSFORM(cameraEntity)->position.y = baseLine + TRANSFORM(cameraEntity)->size.y * 0.5;
        }
    } else {
        // force left pos
        TRANSFORM(cameraEntity)->position.x = - PlacementHelper::ScreenSize.x * (param::LevelSize * 0.5 - 0.5);
        TRANSFORM(cameraEntity)->position.y = baseLine + TRANSFORM(cameraEntity)->size.y * 0.5;
    }
    theRangeFollowerSystem.Update(dt);
}

void RecursiveRunnerGame::setupCamera(CameraMode::Enum mode) {
    LOGT("setupCamera");
    switch (mode) {
        case CameraMode::Single:
            LOGI("Setup camera : Single");
            TRANSFORM(cameraEntity)->position.x = - PlacementHelper::ScreenSize.x * (param::LevelSize * 0.5 - 0.5);
            TRANSFORM(cameraEntity)->position.y = baseLine + TRANSFORM(cameraEntity)->size.y * 0.5;
            break;
        case CameraMode::Menu:
            LOGI("Setup camera : Menu");
            TRANSFORM(cameraEntity)->position = leftMostCameraPos;
            break;
         default:
            break;
    }
}

void RecursiveRunnerGame::updateBestScore() {
    ScoreStorageProxy ssp;
    gameThreadContext->storageAPI->loadEntries(&ssp, "points", "order by points desc limit 1");
    if (! ssp.isEmpty()) {
        TEXT(bestScore)->text = gameThreadContext->localizeAPI->text("Best") + ": " + ObjectSerializer<int>::object2string(ssp._queue.front().points);
    } else {
        LOGW("No best score found (?!)");
        TEXT(bestScore)->text = "";
    }
}

int RecursiveRunnerGame::saveState(uint8_t** out) {
    switch (sceneStateMachine.getCurrentState()) {
        case Scene::Game:
            sceneStateMachine.forceNewState(Scene::Pause);
        case Scene::Pause:
            break;
        default:
            return 0;
    }

    /* save all entities/components */
    uint8_t* entities = 0;
    int eSize = theEntityManager.serialize(&entities);

    if (eSize == 0)
        return 0;

    uint8_t* systems = 0;
    int sSize = theRenderingSystem.saveInternalState(&systems);

    int finalSize = eSize + sSize + 2 * sizeof(int);
    uint8_t* ptr = *out = new uint8_t[finalSize + 2 * sizeof(int)];

    /* save entity/system thingie */
    MEMPCPY(uint8_t*, ptr, &eSize, sizeof(eSize));
    MEMPCPY(uint8_t*, ptr, &sSize, sizeof(sSize));
    MEMPCPY(uint8_t*, ptr, entities, eSize);
    MEMPCPY(uint8_t*, ptr, systems, sSize);

    LOGV(1, eSize << ", " << sSize);
    return finalSize;
}

static std::vector<glm::vec2> generateCoinsCoordinates(int count, float heightMin, float heightMax);

void RecursiveRunnerGame::startGame(Level::Enum level, bool transition) {
    LOGF_IF(theSessionSystem.entityCount() != 0, "Incoherent state. " << theSessionSystem.entityCount() << " sessions existing");
    LOGF_IF(thePlayerSystem.entityCount() != 0, "Incoherent state. " << thePlayerSystem.entityCount() << " players existing");

    // Level2 => fixed srand
    if (level == Level::Level2) {
        time_t t = time(NULL);
        struct tm * timeinfo = localtime (&t);
        int base[3];
        base[0] = timeinfo->tm_mday;
        base[1] = timeinfo->tm_mon;
        base[2] = timeinfo->tm_year;
        hash_t seed = Murmur::RuntimeHash(&base, 3 * sizeof(int));
        LOGI("SEED: " << seed);
        Random::Init(seed);
    }


    for (int i=0; i<100; i++) {
        nextRunnerStartTime[i] = Random::Float(0.0f, 2.0f);
    }
    nextRunnerStartTimeIndex = 0;

    // Create session
    Entity session = theEntityManager.CreateEntity(HASH("session", 0xba9956b4), EntityType::Persistent);
    ADD_COMPONENT(session, Session);
    SessionComponent* sc = SESSION(session);
    sc->numPlayers = 1;
    // Create player
    Entity player = theEntityManager.CreateEntity(HASH("player", 0x9881cf14), EntityType::Persistent);
    ADD_COMPONENT(player, Player);
    sc->players.push_back(player);
    PlayerComponent* pc = PLAYER(player);
    #define COLOR(n) Color(((0x##n >> 16) & 0xff) / 255.0, ((0x##n >> 8) & 0xff) / 255.0, ((0x##n >> 0) & 0xff) / 255.0, 1.0)
    pc->colors.push_back(COLOR(c30101));
    pc->colors.push_back(COLOR(ea6c06));
    pc->colors.push_back(COLOR(f4cf00));
    pc->colors.push_back(COLOR(8ec301));
    pc->colors.push_back(COLOR(01c373));
    pc->colors.push_back(COLOR(12bbd9));
    pc->colors.push_back(COLOR(1e49d7));
    pc->colors.push_back(COLOR(5d00bd));
    pc->colors.push_back(COLOR(a910db));
    pc->colors.push_back(COLOR(dc52b0));

    switch (level) {
        case Level::Level1:
            createCoins(generateCoinsCoordinates(20, PlacementHelper::GimpYToScreen(700), PlacementHelper::GimpYToScreen(450)), sc, transition);
            break;
        case Level::Level2:
            createCoins(generateCoinsCoordinates(20, PlacementHelper::GimpYToScreen(700), PlacementHelper::GimpYToScreen(450)), sc, transition);
            break;
    }

    if (level == Level::Level2) {
        // restore whatever seed
        Random::Init(time(0));
    }
}

void RecursiveRunnerGame::endGame() {
    const auto& sessions = theSessionSystem.RetrieveAllEntityWithComponent();
    if (!sessions.empty()) {
        SessionComponent* sc = SESSION(sessions.front());
        for(unsigned i=0; i<sc->runners.size(); i++)
            theEntityManager.DeleteEntity(RUNNER(sc->runners[i])->collisionZone);
        std::for_each(sc->runners.begin(), sc->runners.end(), deleteEntityFunctor);
        std::for_each(sc->coins.begin(), sc->coins.end(), deleteEntityFunctor);
        std::for_each(sc->players.begin(), sc->players.end(), deleteEntityFunctor);
        std::for_each(sc->links.begin(), sc->links.end(), deleteEntityFunctor);
        std::for_each(sc->sparkling.begin(), sc->sparkling.end(), deleteEntityFunctor);
        theEntityManager.DeleteEntity(sessions.front());
    }
    // on supprime aussi tous les trucs temporaires (lumières, ...)
    const auto temp = theAutoDestroySystem.RetrieveAllEntityWithComponent();
    std::for_each(temp.begin(), temp.end(), deleteEntityFunctor);
}


static bool sortLeftToRight(Entity e, Entity f) {
    return TRANSFORM(e)->position.x < TRANSFORM(f)->position.x;
}

static std::vector<glm::vec2> generateCoinsCoordinates(int count, float heightMin, float heightMax) {
    std::vector<glm::vec2> positions;

    int available = 0;
    float* randomX = new float[count];
    float* randomY = new float[count];


    for (int i=0; i<count; i++) {
        glm::vec2 p;
        bool notFarEnough = true;

        do {
            if (available == 0) {
                // initialize random
                Random::N_Floats(count, randomX,
                    -param::LevelSize * 0.5 * PlacementHelper::ScreenSize.x + 1,
                    param::LevelSize * 0.5 * PlacementHelper::ScreenSize.x - 1);

                Random::N_Floats(count, randomY, heightMin, heightMax);

                available = count;
            }

            p = glm::vec2(randomX[available - 1], randomY[available - 1]);
            available--;

            notFarEnough = false;
            for (unsigned j = 0; j < positions.size() && !notFarEnough; j++) {
                if (glm::abs(positions[j].x - p.x) < 1) {
                    notFarEnough = true;
                }
            }
        } while (notFarEnough);
        positions.push_back(p);
    }

    delete[] randomX;
    delete[] randomY;

    return positions;
}

void RecursiveRunnerGame::createCoins(const std::vector<glm::vec2>& coordinates, SessionComponent* session, bool transition) {
    LOGI("Coins creation started");

    EntityTemplateRef coinTemplate = theEntityManager.entityTemplateLibrary.load("ingame/coin");
    EntityTemplateRef linkTemplate = theEntityManager.entityTemplateLibrary.load("ingame/link");
    EntityTemplateRef link3Template = theEntityManager.entityTemplateLibrary.load("ingame/link3");

    std::vector<Entity> coins;
    for (unsigned i=0; i<coordinates.size(); i++) {
        Entity e = theEntityManager.CreateEntity(HASH("coin", 0x844c8381),
            EntityType::Persistent, coinTemplate);

        TRANSFORM(e)->size *= param::CoinScale;
        TRANSFORM(e)->position = coordinates[i];

        RENDERING(e)->color.a = (transition ? 0 : 1);

        coins.push_back(e);
    }

    std::sort(coins.begin(), coins.end(), sortLeftToRight);
    const glm::vec2 offset = glm::vec2(0, PlacementHelper::GimpHeightToScreen(14));
    glm::vec2 previous = glm::vec2(-param::LevelSize * PlacementHelper::ScreenSize.x * 0.5, -PlacementHelper::ScreenSize.y * 0.2);
    for (unsigned i = 0; i <= coins.size(); i++) {
        glm::vec2 topI;

        if (i < coins.size())
            topI = TRANSFORM(coins[i])->position + glm::rotate(offset, TRANSFORM(coins[i])->rotation);
        else
            topI = glm::vec2(param::LevelSize * PlacementHelper::ScreenSize.x * 0.5, 0);

        Entity link = theEntityManager.CreateEntity(HASH("link", 0xcacec0a0),
            EntityType::Persistent, linkTemplate);
        TRANSFORM(link)->position = (topI + previous) * 0.5f;
        TRANSFORM(link)->size = glm::vec2(glm::length(topI - previous), PlacementHelper::GimpHeightToScreen(54));
        TRANSFORM(link)->rotation = -/*glm::radians*/(glm::orientedAngle(glm::normalize(topI - previous), glm::vec2(1.0f, 0.0f)));
        RENDERING(link)->color.a =  (transition ? 0 : 1);

        Entity link3 = theEntityManager.CreateEntity(HASH("link3", 0xba31ef20),
            EntityType::Persistent, link3Template);
        TRANSFORM(link3)->size = TRANSFORM(link)->size * glm::vec2(1, 0.1);
        ANCHOR(link3)->parent = link;
        ANCHOR(link3)->position = glm::vec2(0, TRANSFORM(link)->size.y * 0.4);
        PARTICULE(link3)->emissionRate = 100 * TRANSFORM(link)->size.x * TRANSFORM(link)->size.y;
        session->sparkling.push_back(link3);

        previous = topI;
        session->links.push_back(link);
    }

    for (unsigned i=0; i<coins.size(); i++) {
        session->coins.push_back(coins[i]);
    }
    LOGI("Coins creation finished");
}
