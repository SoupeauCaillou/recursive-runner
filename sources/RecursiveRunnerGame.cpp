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

#include "api/StorageAPI.h"
#include "util/ScoreStorageProxy.h"

#include "util/RecursiveRunnerDebugConsole.h"

#include <glm/gtc/random.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/core/func_geometric.hpp>
#include <iomanip>


extern std::map<TextureRef, CollisionZone> texture2Collision;

extern std::map<TextureRef, CollisionZone> texture2Collision;

RecursiveRunnerGame::RecursiveRunnerGame() {

   RunnerSystem::CreateInstance();
   CameraTargetSystem::CreateInstance();
   PlayerSystem::CreateInstance();
   RangeFollowerSystem::CreateInstance();
   SessionSystem::CreateInstance();
   PlatformerSystem::CreateInstance();

   sceneStateMachine.registerState(Scene::Logo, Scene::CreateLogoSceneHandler(this), "Scene::Logo");
   sceneStateMachine.registerState(Scene::Menu, Scene::CreateMenuSceneHandler(this), "Scene::Menu");
   sceneStateMachine.registerState(Scene::Ad, Scene::CreateAdSceneHandler(this), "Scene::Ad");
   sceneStateMachine.registerState(Scene::Pause, Scene::CreatePauseSceneHandler(this), "Scene::Pause");
   sceneStateMachine.registerState(Scene::Rate, Scene::CreateRateSceneHandler(this), "Scene::Rate");
   sceneStateMachine.registerState(Scene::Game, Scene::CreateGameSceneHandler(this), "Scene::Game");
   sceneStateMachine.registerState(Scene::RestartGame, Scene::CreateRestartGameSceneHandler(this), "Scene::RestartGame");
   sceneStateMachine.registerState(Scene::Tutorial, Scene::CreateTutorialSceneHandler(this), "Scene::Tutorial");
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
        case ContextAPI::Ad:
        case ContextAPI::Asset:
        case ContextAPI::Communication:
        case ContextAPI::GameCenter:
        case ContextAPI::Exit:
        case ContextAPI::Localize:
        case ContextAPI::Music:
        case ContextAPI::Sound:
        case ContextAPI::Storage:
        case ContextAPI::Vibrate:
            return true;
        default:
            return false;
    }
}

void RecursiveRunnerGame::sacInit(int windowW, int windowH) {
    Game::sacInit(windowW, windowH);

	PlacementHelper::GimpSize = glm::vec2(1280, 800);

    // load anim files
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

    // init font
//    loadFont(renderThreadContext->assetAPI, "typo");
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
        int idx = (int) glm::linearRand(0.f, 6.f);
        if (std::find(indexes.begin(), indexes.end(), idx) == indexes.end()) {
            indexes.push_back(idx);
        }
    } while (indexes.size() != count);

    for (unsigned i=0; i<indexes.size(); i++) {
        int idx = indexes[i];
        Entity fumee = theEntityManager.CreateEntity("fumee",
            EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("background/fumee"));
        TRANSFORM(fumee)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("fumee0")) * glm::linearRand(0.5f, 0.8f);
        ANCHOR(fumee)->parent = building;
        ANCHOR(fumee)->position = possible[idx] * TRANSFORM(building)->size + glm::vec2(0, TRANSFORM(fumee)->size.y * 0.5);
        if (RENDERING(building)->mirrorH) 
            ANCHOR(fumee)->position.x = -ANCHOR(fumee)->position.x;
        ANIMATION(fumee)->waitAccum = glm::linearRand(0.0f, 10.f);
    }
}

void RecursiveRunnerGame::decor() {
    LOGT("Position not handled in .entity file yet!");
    PlacementHelper::ScreenSize.x *= 3;
    PlacementHelper::GimpSize = glm::vec2(1280 * 3, 800);


    silhouette = theEntityManager.CreateEntity("silhouette",
        EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("background/silhouette"));
    TRANSFORM(silhouette)->position = AnchorSystem::adjustPositionWithCardinal(
        glm::vec2(0, PlacementHelper::GimpYToScreen(0)), TRANSFORM(silhouette)->size, Cardinal::N);

    route = theEntityManager.CreateEntity("road",
        EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("background/road"));
    TRANSFORM(route)->position = AnchorSystem::adjustPositionWithCardinal(
        glm::vec2(0, PlacementHelper::GimpYToScreen(800)), TRANSFORM(route)->size, Cardinal::S);

    Entity buildings = theEntityManager.CreateEntity("buildings",
        EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("background/city-object"));

    Entity trees = theEntityManager.CreateEntity("trees",
        EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("background/city-object"));

    int count = 33;
    struct Decor {
    	float x, y, z;
    	Cardinal::Enum ref;
    	std::string texture;
    	bool mirrorUV;
        Entity parent;
    	Decor(float _x=0, float _y=0, float _z=0, Cardinal::Enum _ref=Cardinal::C, const std::string& _texture="", bool _mirrorUV=false, Entity _parent=0) :
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
    for (int i=0; i<count; i++) {
    	const Decor& bdef = def[i];

 	    Entity b = theEntityManager.CreateEntity(bdef.texture);
	    ADD_COMPONENT(b, Transformation);
        auto tb = TRANSFORM(b);
	    tb->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize(bdef.texture));
        ADD_COMPONENT(b, Anchor);
	    ANCHOR(b)->position = AnchorSystem::adjustPositionWithCardinal(
            glm::vec2(PlacementHelper::GimpXToScreen(bdef.x), PlacementHelper::GimpYToScreen(bdef.y)), tb->size, bdef.ref);
	    ANCHOR(b)->z = bdef.z;
        ANCHOR(b)->parent = bdef.parent;
	    ADD_COMPONENT(b, Rendering);
	    RENDERING(b)->texture = theRenderingSystem.loadTextureFile(bdef.texture);
	    RENDERING(b)->show = true;
	    RENDERING(b)->mirrorH = bdef.mirrorUV;
        RENDERING(b)->opaqueType = RenderingComponent::NON_OPAQUE;
	    decorEntities.push_back(b);

        if (i < 3) {
            fumee(b);
        }
        glm::vec2* zPrepassSize = 0, *zPrepassOffset = 0;

        if (bdef.texture.find("arbre") != std::string::npos) {
            int idx = atoi(bdef.texture.substr(5, 1).c_str()) - 1;
            zPrepassSize = v[idx];
            zPrepassOffset = o[idx];
        } else if (bdef.texture == "immeuble") {
            zPrepassSize = vBat[0];
            zPrepassOffset = oBat[0];
        } else if (bdef.texture == "maison") {
            zPrepassSize = vBat[1];
            zPrepassOffset = oBat[1];
        } else if (bdef.texture == "usine2") {
            zPrepassSize = vBat[2];
            zPrepassOffset = oBat[2];
        } else if (bdef.texture == "usine_desaf") {
            zPrepassSize = vBat[3];
            zPrepassOffset = oBat[3];
        }
        if (zPrepassSize) {
            const glm::vec2& size = tb->size;
            for (int j=0; j<4; j++) {
                if (zPrepassSize[j] == glm::vec2(0.0f))
                    break;
                Entity bb = theEntityManager.CreateEntity(bdef.texture + "_z_pre-pass");
                ADD_COMPONENT(bb, Transformation);
                TRANSFORM(bb)->size = PlacementHelper::GimpSizeToScreen(zPrepassSize[j]);
                glm::vec2 ratio(zPrepassOffset[j] / theRenderingSystem.getTextureSize(bdef.texture));
                ratio.y = 1 - ratio.y;
                ADD_COMPONENT(bb, Anchor);
                ANCHOR(bb)->position =
                    size * (glm::vec2(-0.5) + ratio) + TRANSFORM(bb)->size * glm::vec2(0.5, -0.5);
                if (bdef.mirrorUV)
                    ANCHOR(bb)->position.x = -ANCHOR(bb)->position.x;
                ANCHOR(bb)->z = 0;
                ANCHOR(bb)->parent = b;
                ADD_COMPONENT(bb, Rendering);
                *RENDERING(bb) = *RENDERING(b);
                #if 1
                RENDERING(bb)->zPrePass = true;
                #else
                RENDERING(bb)->texture = InvalidTextureRef;
                RENDERING(bb)->color = Color(1,1,0,0.6);
                #endif
            }
        }
	}

    theEntityManager.CreateEntity("background/banderolle", 
        EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("background/banderolle"));

    bestScore = theEntityManager.CreateEntity("background/best_score",
        EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("background/best_score"));

    const bool muted = gameThreadContext->storageAPI->isOption("sound", "off");
    pianist = theEntityManager.CreateEntity("background/pianist",
        EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("background/pianist"));
    RENDERING(pianist)->color.a = 0.8;
    ANIMATION(pianist)->name = (muted ? "pianojournal" : "piano");

    std::vector<Entity> cameras = theCameraSystem.RetrieveAllEntityWithComponent();
    cameraEntity = cameras[0];
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

    PlacementHelper::GimpSize = glm::vec2(1280, 800);
    PlacementHelper::ScreenSize.x /= 3;

    buttonSpacing.H = PlacementHelper::GimpWidthToScreen(94);
    buttonSpacing.V = PlacementHelper::GimpHeightToScreen(76);

    muteBtn = theEntityManager.CreateEntity("mute_button",
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
    Entity camera = cameraEntity = theEntityManager.CreateEntity("camera1",
        EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("camera"));
    TRANSFORM(camera)->position = leftMostCameraPos;

    decor();

    scorePanel = theEntityManager.CreateEntity("background/score_panel",
        EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("background/score_panel"));
    TRANSFORM(scorePanel)->position = AnchorSystem::adjustPositionWithCardinal(
        glm::vec2(0, baseLine + PlacementHelper::ScreenSize.y + PlacementHelper::GimpHeightToScreen(20)), TRANSFORM(scorePanel)->size, Cardinal::N);

    scoreText = theEntityManager.CreateEntity("score_text",
        EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("background/score_text"));

    std::vector<Entity> players = thePlayerSystem.RetrieveAllEntityWithComponent();
    TEXT(scoreText)->text = players.empty() ?
        "12345"
        : ObjectSerializer<int>::object2string(PLAYER(players[0])->points);

    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0000")] =  CollisionZone(90,52,28,84,-0.1);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0001")] =  CollisionZone(91,62,27,78,-0.1);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0002")] =  CollisionZone(95,74,23,72, -0.1);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0003")] =  CollisionZone(95,74,23,70, -0.1);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0004")] =  CollisionZone(111,95,24,75, -0.3);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0005")] =  CollisionZone(114,94,15,84, -0.5);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0006")] =  CollisionZone(109,100,20,81, -0.5);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0007")] =  CollisionZone(101, 96,24,85,-0.2);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0008")] =  CollisionZone(100, 98,25,74,-0.15);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0009")] =  CollisionZone(95,95,25, 76, 0.0);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0010")] =  CollisionZone(88,96,25,75, 0.);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0011")] =  CollisionZone(85,95,24,83, 0.4);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0012")] =  CollisionZone(93,100,24,83, 0.2);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0013")] =  CollisionZone(110,119,25,64,-0.6);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0014")] =  CollisionZone(100, 120,21,60, -0.2);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0015")] =  CollisionZone(105, 115,22,62, -0.15);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0016")] =  CollisionZone(103,103,24,66,-0.1);
    for (int i=0; i<12; i++) {
        std::stringstream a;
        a << "run_l2r_" << std::setfill('0') << std::setw(4) << i;
        texture2Collision[theRenderingSystem.loadTextureFile(a.str())] =  CollisionZone(118,103,35,88,-0.5);
    }

    //important! This must be called AFTER camera setup, since we are referencing it (anchor component)
    gamecenterAPIHelper.init(gameThreadContext->gameCenterAPI, true, true, true);
}

void RecursiveRunnerGame::init(const uint8_t* in, int size) {
    gameThreadContext->storageAPI->init(gameThreadContext->assetAPI, "RecursiveRunner");
    gameThreadContext->storageAPI->setOption("sound", std::string(), "on");
    gameThreadContext->storageAPI->setOption("gameCount", std::string(), "0");

    ScoreStorageProxy ssp;
    gameThreadContext->storageAPI->createTable(&ssp);
    
    if (size > 0 && in) {
        int eSize, sSize, index=0;
        memcpy(&eSize, &in[index], sizeof(eSize));
        index += sizeof(eSize);
        memcpy(&sSize, &in[index], sizeof(sSize));
        index += sizeof(sSize);
        /* restore entities */
        theEntityManager.deserialize(&in[index], eSize);
        index += eSize;
        /* restore systems */
        theRenderingSystem.restoreInternalState(&in[index], sSize);
        index += sSize;
        
        LOGI( index << "/" << size << "(" << eSize << ", " << sSize << ")" );

        std::vector<Entity> all = theTransformationSystem.RetrieveAllEntityWithComponent();
        for (unsigned i=0; i<all.size(); i++) {
            if (TRANSFORM(all[i])->size.x > PlacementHelper::ScreenSize.x * param::LevelSize) {
                ground = all[i];
                break;
            }
        }
    }


//    RecursiveRunnerDebugConsole::init(this);

    level = Level::Level1;
    initGame();
    if (in == 0 || size == 0) {
        ground = theEntityManager.CreateEntity("ground", EntityType::Persistent);
        ADD_COMPONENT(ground, Transformation);
        TRANSFORM(ground)->size = glm::vec2(PlacementHelper::ScreenSize.x * (param::LevelSize + 1), 0);
        TRANSFORM(ground)->position = glm::vec2(0, baseLine);
    }
    updateBestScore();

    //recover
    if (size > 0 && in) {
        sceneStateMachine.setup(Scene::Pause);
    } else {
        #if SAC_DEBUG
            sceneStateMachine.setup(Scene::Menu);
        #else
            sceneStateMachine.setup(Scene::Logo);
        #endif
    }
    
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
        case Scene::Tutorial:
            sceneStateMachine.forceNewState(Scene::Menu);
            break;
        default:
            break;
    }
}

void RecursiveRunnerGame::togglePause(bool pause) {
    if (sceneStateMachine.getCurrentState() == Scene::Game && pause) {
        sceneStateMachine.forceNewState(Scene::Pause);
    }
}

void RecursiveRunnerGame::tick(float dt) {
    if (BUTTON(muteBtn)->clicked) {
        //retrieve current state
        bool muted = gameThreadContext->storageAPI->isOption("sound", "off");
        
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
            std::vector<Entity> e = theMusicSystem.RetrieveAllEntityWithComponent();
            for (unsigned i=0; i<e.size(); i++) {
                if (MUSIC(e[i])->opaque[0] && MUSIC(e[i])->control != MusicControl::Pause) {
                    pianistPlaying = true;
                    break;
                }
            }
         }
         if (pianistPlaying) {
            if (ANIMATION(pianist)->name == "pianojournal") {
                ANIMATION(pianist)->name = "piano";
            }
         } else {
            ANIMATION(pianist)->name = "pianojournal";
         }
    }

    //disable "play" click if we are hitting the left zone of the screen (buttons zone)
    if (theTouchInputManager.isTouched(0)) {
        auto touchPos = theTouchInputManager.getTouchLastPosition(0);
        ignoreClick |= touchPos.x < TRANSFORM(muteBtn)->position.x + TRANSFORM(muteBtn)->size.x * BUTTON(muteBtn)->overSize * 0.5;
    }

    sceneStateMachine.update(dt);

    // limit cam pos
    for (unsigned i=2; i<2 /* theRenderingSystem.cameras.size()*/; i++) {
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

    theRangeFollowerSystem.Update(dt);
}

void RecursiveRunnerGame::setupCamera(CameraMode::Enum mode) {
    LOGT("setupCamera");
    switch (mode) {
        case CameraMode::Single:
            LOGI("Setup camera : Single");
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
        TEXT(bestScore)->text = "Best: " + ObjectSerializer<int>::object2string(ssp._queue.front().points);
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
    ptr = (uint8_t*)mempcpy(ptr, &eSize, sizeof(eSize));
    ptr = (uint8_t*)mempcpy(ptr, &sSize, sizeof(sSize));
    ptr = (uint8_t*)mempcpy(ptr, entities, eSize);
    ptr = (uint8_t*)mempcpy(ptr, systems, sSize);
    std::cout << eSize << ", " << sSize << std::endl;
    return finalSize;
}

static std::vector<glm::vec2> generateCoinsCoordinates(int count, float heightMin, float heightMax);

void RecursiveRunnerGame::startGame(Level::Enum level, bool transition) {
    assert(theSessionSystem.RetrieveAllEntityWithComponent().empty());
    assert(thePlayerSystem.RetrieveAllEntityWithComponent().empty());

    // Create session
    Entity session = theEntityManager.CreateEntity("session", EntityType::Persistent);
    ADD_COMPONENT(session, Session);
    SessionComponent* sc = SESSION(session);
    sc->numPlayers = 1;
    // Create player
    Entity player = theEntityManager.CreateEntity("player", EntityType::Persistent);
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
            createCoins(generateCoinsCoordinates(10, PlacementHelper::GimpYToScreen(700), PlacementHelper::GimpYToScreen(450)), sc, transition);
            int index[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
            // use created coins as platforms switches
            for (unsigned i=0; i<9; i++) {
                Platform pf;
                pf.switches[0].entity = sc->coins[index[i]];
                pf.switches[1].entity = sc->coins[index[i]+1];
                pf.platform = sc->links[index[i]+1];
                RENDERING(pf.platform)->texture = InvalidTextureRef;
                sc->platforms.push_back(pf);
                TRANSFORM(pf.switches[0].entity)->position.y =
                TRANSFORM(pf.switches[1].entity)->position.y =
                TRANSFORM(pf.platform)->position.y = PlacementHelper::GimpYToScreen(600);
                TRANSFORM(pf.platform)->rotation = 0;
            }
            createCoins(generateCoinsCoordinates(15, PlacementHelper::GimpYToScreen(400), PlacementHelper::GimpYToScreen(150)), sc, transition);

            break;
    }
}

void RecursiveRunnerGame::endGame() {
    std::vector<Entity> sessions = theSessionSystem.RetrieveAllEntityWithComponent();
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
        // on supprime aussi tous les trucs temporaires (lumières, ...)
        std::vector<Entity> temp = theAutoDestroySystem.RetrieveAllEntityWithComponent();
        std::for_each(temp.begin(), temp.end(), deleteEntityFunctor);
    }
}


static bool sortLeftToRight(Entity e, Entity f) {
    return TRANSFORM(e)->position.x < TRANSFORM(f)->position.x;
}

static std::vector<glm::vec2> generateCoinsCoordinates(int count, float heightMin, float heightMax) {
    std::vector<glm::vec2> positions;
    for (int i=0; i<count; i++) {
        glm::vec2 p;
        bool notFarEnough = true;
        do {
            p = glm::vec2(
                glm::linearRand(
                    -param::LevelSize * 0.5 * PlacementHelper::ScreenSize.x,
                    param::LevelSize * 0.5 * PlacementHelper::ScreenSize.x),
                glm::linearRand(heightMin, heightMax));
           notFarEnough = false;
           for (unsigned j = 0; j < positions.size() && !notFarEnough; j++) {
                if (glm::distance(positions[j], p) < 1) {
                    notFarEnough = true;
                }
           }
        } while (notFarEnough);
        positions.push_back(p);
    }
    return positions;
}

void RecursiveRunnerGame::createCoins(const std::vector<glm::vec2>& coordinates, SessionComponent* session, bool transition) {
    LOGI("Coins creation started");
    
    EntityTemplateRef coinTemplate = theEntityManager.entityTemplateLibrary.load("ingame/coin");
    EntityTemplateRef linkTemplate = theEntityManager.entityTemplateLibrary.load("ingame/link");
    EntityTemplateRef link3Template = theEntityManager.entityTemplateLibrary.load("ingame/link3");

    std::vector<Entity> coins;
    for (unsigned i=0; i<coordinates.size(); i++) {
        Entity e = theEntityManager.CreateEntity("coin", 
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

        Entity link = theEntityManager.CreateEntity("link",
            EntityType::Persistent, linkTemplate);
        TRANSFORM(link)->position = (topI + previous) * 0.5f;
        TRANSFORM(link)->size = glm::vec2(glm::length(topI - previous), PlacementHelper::GimpHeightToScreen(54));
        TRANSFORM(link)->rotation = -/*glm::radians*/(glm::orientedAngle(glm::normalize(topI - previous), glm::vec2(1.0f, 0.0f)));
        RENDERING(link)->color.a =  (transition ? 0 : 1);

        Entity link3 = theEntityManager.CreateEntity("link3", 
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
