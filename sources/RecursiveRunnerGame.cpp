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

#include "Parameters.h"

#include <sstream>

#include <base/Log.h>
#include <base/TouchInputManager.h>
#include <base/EntityManager.h>
#include <base/TimeUtil.h>
#include <base/PlacementHelper.h>

#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/ButtonSystem.h"
#include "systems/TextRenderingSystem.h"
#include "systems/SoundSystem.h"
#include "systems/MusicSystem.h"
#include "systems/AnimationSystem.h"

#include "systems/RunnerSystem.h"
#include "systems/CameraTargetSystem.h"
#include "systems/PlayerSystem.h"
#include "systems/RangeFollowerSystem.h"

#include <iomanip>

static void updateFps(float dt);

extern std::map<TextureRef, CollisionZone> texture2Collision;

RecursiveRunnerGame::RecursiveRunnerGame(AssetAPI* ast, StorageAPI* storage, NameInputAPI* nameInput, AdAPI* ad __attribute__((unused)), ExitAPI* exAPI, CommunicationAPI* comAPI) : Game() {
	assetAPI = ast;
	storageAPI = storage;
    nameInputAPI = nameInput;
	exitAPI = exAPI;
    communicationAPI = comAPI;


    RunnerSystem::CreateInstance();
    CameraTargetSystem::CreateInstance();
    PlayerSystem::CreateInstance();
    RangeFollowerSystem::CreateInstance();

    currentState = State::Logo;
    state2manager.insert(std::make_pair(State::Logo, new LogoStateManager(this)));
    state2manager.insert(std::make_pair(State::Menu, new MenuStateManager(this)));
    state2manager.insert(std::make_pair(State::Pause, new PauseStateManager(this)));
    state2manager.insert(std::make_pair(State::Game, new GameStateManager(this)));
    state2manager.insert(std::make_pair(State::Menu2Game,
        new TransitionStateManager(State::Menu2Game, state2manager[State::Menu], state2manager[State::Game], this)));
    state2manager.insert(std::make_pair(State::Game2Menu,
        new TransitionStateManager(State::Game2Menu, state2manager[State::Game], state2manager[State::Menu], this)));
    state2manager.insert(std::make_pair(State::Logo2Menu,
        new TransitionStateManager(State::Logo2Menu, state2manager[State::Logo], state2manager[State::Menu], this)));
}

RecursiveRunnerGame::~RecursiveRunnerGame() {
    LOGW("Delete game instance %p %p", this, &theEntityManager);
    theEntityManager.deleteAllEntities();
    
    RunnerSystem::DestroyInstance();
    CameraTargetSystem::DestroyInstance();
    PlayerSystem::DestroyInstance();
    RangeFollowerSystem::DestroyInstance();

    for(std::map<State::Enum, StateManager*>::iterator it=state2manager.begin(); it!=state2manager.end(); ++it) {
        delete it->second;
    }
    state2manager.clear();
}

void RecursiveRunnerGame::sacInit(int windowW, int windowH) {
    Game::sacInit(windowW, windowH);
	PlacementHelper::GimpWidth = 1280;
    PlacementHelper::GimpHeight = 800;
    theRenderingSystem.loadAtlas("alphabet", true);
    theRenderingSystem.loadAtlas("dummy", false);
    theRenderingSystem.loadAtlas("decor", false);
    theRenderingSystem.loadAtlas("arbre", false);
    theRenderingSystem.loadAtlas("fumee", false);
    theRenderingSystem.loadAtlas("logo", true);
    
    // register 4 animations
    std::string runL2R[] = { "run_l2r_0002",
        "run_l2r_0003", "run_l2r_0004", "run_l2r_0005",
        "run_l2r_0006", "run_l2r_0007", "run_l2r_0008",
        "run_l2r_0009", "run_l2r_0010", "run_l2r_0000", "run_l2r_0011", "run_l2r_0001"};
    std::string jumpL2R[] = { "jump_l2r_0004", "jump_l2r_0005",
        "jump_l2r_0006", "jump_l2r_0007", "jump_l2r_0008",
        "jump_l2r_0009", "jump_l2r_0010", "jump_l2r_0011"};
    std::string jumpL2Rtojump[] = { "jump_l2r_0012", "jump_l2r_0013", "jump_l2r_0015", "jump_l2r_0016"};

    theAnimationSystem.registerAnim("runL2R", runL2R, 12, 15, Interval<int>(-1, -1));
    theAnimationSystem.registerAnim("jumpL2R_up", jumpL2R, 5, 20, Interval<int>(0, 0));
    theAnimationSystem.registerAnim("jumpL2R_down", &jumpL2R[5], 3, 15, Interval<int>(0, 0));
    theAnimationSystem.registerAnim("jumptorunL2R", jumpL2Rtojump, 4, 30, Interval<int>(0, 0), "runL2R");

    std::string fumeeStart[] = {"fumee0", "fumee1", "fumee2", "fumee3", "fumee4", "fumee5" };
    std::string fumeeLoop[] = {"fumee5b", "fumee5c", "fumee5" };
    std::string fumeeEnd[] = {"fumee6", "fumee7", "fumee8", "fumee9" };
    theAnimationSystem.registerAnim("fumee_start", fumeeStart, 6, 8, Interval<int>(0, 0), "fumee_loop");
    theAnimationSystem.registerAnim("fumee_loop", fumeeLoop, 3, 8, Interval<int>(2, 5), "fumee_end");
    theAnimationSystem.registerAnim("fumee_end", fumeeEnd, 4, 8, Interval<int>(0, 0), "fumee_start", Interval<float>(2, 10));
    
    glClearColor(148.0/255, 148.0/255, 148.0/255, 1.0);

    // init font
    loadFont(assetAPI, "typo");
}

void fumee(Entity building) {
    const Vector2 possible[] = {
        Vector2(-445 / 942.0, 292 / 594.0),
        Vector2(-310 / 942.0, 260 / 594.0),
        Vector2(-52 / 942.0, 255 / 594.0),
        Vector2(147 / 942.0, 239 / 594.0),
        Vector2(269 / 942.0, 218 / 594.0),
        Vector2(442 / 942.0, 239 / 594.0)
    };

    unsigned count = 6;//MathUtil::RandomIntInRange(1, 4);
    std::vector<int> indexes;
    do {
        int idx = MathUtil::RandomIntInRange(0, 6);
        if (std::find(indexes.begin(), indexes.end(), idx) == indexes.end()) {
            indexes.push_back(idx);
        }
    } while (indexes.size() != count);

    for (unsigned i=0; i<indexes.size(); i++) {
        int idx = indexes[i];
        Entity fumee = theEntityManager.CreateEntity();
        ADD_COMPONENT(fumee, Transformation);
        TRANSFORM(fumee)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("fumee0")) * (0.8 - MathUtil::RandomFloat() * 0.3);
        TRANSFORM(fumee)->parent = building;
        TRANSFORM(fumee)->position = possible[idx] * TRANSFORM(building)->size + Vector2(0, TRANSFORM(fumee)->size.Y * 0.5);
        if (RENDERING(building)->mirrorH) TRANSFORM(fumee)->position.X = -TRANSFORM(fumee)->position.X;
        TRANSFORM(fumee)->z = -0.1;
        ADD_COMPONENT(fumee, Rendering);
        RENDERING(fumee)->hide = true;
        RENDERING(fumee)->color = Color(1,1,1,1);
        RENDERING(fumee)->opaqueType = RenderingComponent::NON_OPAQUE;
        ADD_COMPONENT(fumee, Animation);
        ANIMATION(fumee)->name = "fumee_start";
        ANIMATION(fumee)->waitAccum = MathUtil::RandomFloat() * 10;
    }
}

void RecursiveRunnerGame::decor(StorageAPI* storageAPI) {
	silhouette = theEntityManager.CreateEntity();
    ADD_COMPONENT(silhouette, Transformation);
    TRANSFORM(silhouette)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("silhouette_ville")) * 4;
    // TRANSFORM(silhouette)->size.X *= 1.6;
    theTransformationSystem.setPosition(TRANSFORM(silhouette), Vector2(0, PlacementHelper::GimpYToScreen(0)), TransformationSystem::N);
    TRANSFORM(silhouette)->z = 0.01;
    ADD_COMPONENT(silhouette, Rendering);
    RENDERING(silhouette)->texture = theRenderingSystem.loadTextureFile("silhouette_ville");
    RENDERING(silhouette)->hide = false;
    RENDERING(silhouette)->opaqueType = RenderingComponent::FULL_OPAQUE;
    // RENDERING(silhouette)->cameraBitMask = (0x3 << 1);

	route = theEntityManager.CreateEntity();
    ADD_COMPONENT(route, Transformation);
    TRANSFORM(route)->size = Vector2(PlacementHelper::ScreenWidth, PlacementHelper::GimpHeightToScreen(109));
    theTransformationSystem.setPosition(TRANSFORM(route), Vector2(0, PlacementHelper::GimpYToScreen(800)), TransformationSystem::S);
    TRANSFORM(route)->z = 0.1;
    ADD_COMPONENT(route, Rendering);
    RENDERING(route)->texture = theRenderingSystem.loadTextureFile("route");
    RENDERING(route)->hide = false;
    RENDERING(route)->opaqueType = RenderingComponent::FULL_OPAQUE;
    // RENDERING(route)->cameraBitMask = (0x3 << 1);
    
    Entity buildings = theEntityManager.CreateEntity();
    ADD_COMPONENT(buildings, Transformation);
    Entity trees = theEntityManager.CreateEntity();
    ADD_COMPONENT(trees, Transformation);
    
    PlacementHelper::ScreenWidth *= 3;
    PlacementHelper::GimpWidth = 1280 * 3;
    PlacementHelper::GimpHeight = 800;
    int count = 26;
    struct Decor {
    	float x, y, z;
    	TransformationSystem::PositionReference ref;
    	std::string texture;
    	bool mirrorUV;
        Entity parent;
    	Decor(float _x=0, float _y=0, float _z=0, TransformationSystem::PositionReference _ref=TransformationSystem::C, const std::string& _texture="", bool _mirrorUV=false, Entity _parent=0) :
    		x(_x), y(_y), z(_z), ref(_ref), texture(_texture), mirrorUV(_mirrorUV), parent(_parent) {}
   	};
   	
	Decor def[] = {
		// buildings
		Decor(554, 149, 0.2, TransformationSystem::NE, "immeuble", false, buildings),
		Decor(1690, 149, 0.2, TransformationSystem::NE, "immeuble", false, buildings),
		Decor(3173, 139, 0.2, TransformationSystem::NW, "immeuble", false, buildings),
		Decor(358, 404, 0.25, TransformationSystem::NW, "maison", true, buildings),
		Decor(2097, 400, 0.25, TransformationSystem::NE, "maison", false, buildings),
		Decor(2053, 244, 0.29, TransformationSystem::NW, "usine_desaf", false, buildings),
		Decor(3185, 298, 0.22, TransformationSystem::NE, "usine2", true, buildings),
		// trees
		Decor(152, 780, 0.5, TransformationSystem::S, "arbre3", false, trees),
		Decor(522, 780, 0.5, TransformationSystem::S, "arbre2", false, trees),
		Decor(812, 774, 0.45, TransformationSystem::S, "arbre5", false, trees),
		Decor(1162, 792, 0.5, TransformationSystem::S, "arbre4", false, trees),
		Decor(1418, 790, 0.45, TransformationSystem::S, "arbre2", false, trees),
		Decor(1600, 768, 0.42, TransformationSystem::S, "arbre1", false, trees),
		Decor(1958, 782, 0.5, TransformationSystem::S, "arbre4", true, trees),
		Decor(2396, 774, 0.44, TransformationSystem::S, "arbre5", false, trees),
		Decor(2684, 784, 0.45, TransformationSystem::S, "arbre3", false, trees),
		Decor(3022, 764, 0.42, TransformationSystem::S, "arbre1", false, trees),
		Decor(3290, 764, 0.41, TransformationSystem::S, "arbre1", true, trees),
		Decor(3538, 768, 0.44, TransformationSystem::S, "arbre2", false, trees),
		Decor(3820, 772, 0.5, TransformationSystem::S, "arbre4", false, trees),
		// benchs
		Decor(672, 768, 0.35, TransformationSystem::S, "bench_cat", false, trees),
		Decor(1090, 764, 0.35, TransformationSystem::S, "bench", false, trees),
		Decor(2082, 760, 0.35, TransformationSystem::S, "bench", true, trees),
		Decor(2526, 762, 0.35, TransformationSystem::S, "bench", false, trees),
		Decor(3464, 758, 0.35, TransformationSystem::S, "bench_cat", false, trees),
		Decor(3612, 762, 0.6, TransformationSystem::S, "bench", false, trees),
    };
    for (int i=0; i<count; i++) {
    	const Decor& bdef = def[i];

 	    Entity b = theEntityManager.CreateEntity();
	    ADD_COMPONENT(b, Transformation);
	    TRANSFORM(b)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize(bdef.texture));
	    theTransformationSystem.setPosition(TRANSFORM(b), Vector2(PlacementHelper::GimpXToScreen(bdef.x), PlacementHelper::GimpYToScreen(bdef.y)), bdef.ref);
	    TRANSFORM(b)->z = bdef.z;
        TRANSFORM(b)->parent = bdef.parent;
	    ADD_COMPONENT(b, Rendering);
	    RENDERING(b)->texture = theRenderingSystem.loadTextureFile(bdef.texture);
	    RENDERING(b)->hide = false;
	    RENDERING(b)->mirrorH = bdef.mirrorUV;
        RENDERING(b)->opaqueType = RenderingComponent::NON_OPAQUE;
	    // RENDERING(b)->cameraBitMask = (0x3 << 1);
	    decorEntities.push_back(b);

        if (i < 3) {
            fumee(b);
        }
	}

    Entity banderolle = theEntityManager.CreateEntity();
    ADD_COMPONENT(banderolle, Transformation);
    TRANSFORM(banderolle)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("banderolle"));
    TRANSFORM(banderolle)->position = Vector2(PlacementHelper::GimpXToScreen(772), PlacementHelper::GimpYToScreen(415));
    TRANSFORM(banderolle)->z = 0.31;
    TRANSFORM(banderolle)->rotation = 0.1;
    ADD_COMPONENT(banderolle, Rendering);
    RENDERING(banderolle)->texture = theRenderingSystem.loadTextureFile("banderolle");
    RENDERING(banderolle)->hide = false;
    
    bestScore = theEntityManager.CreateEntity();
    bestScore = theEntityManager.CreateEntity();
    ADD_COMPONENT(bestScore, Transformation);
    TRANSFORM(bestScore)->parent = banderolle;
    TRANSFORM(bestScore)->z = 0.001;
    TRANSFORM(bestScore)->position = Vector2(0, -0.25);
    ADD_COMPONENT(bestScore, TextRendering);
    TEXT_RENDERING(bestScore)->text = "bla";
    TEXT_RENDERING(bestScore)->charHeight = 0.7;
    // TEXT_RENDERING(bestScore)->flags |= TextRenderingComponent::IsANumberBit;
    TEXT_RENDERING(bestScore)->hide = false;
    TEXT_RENDERING(bestScore)->color = Color(64.0 / 255, 62.0/255, 72.0/255);

	PlacementHelper::GimpWidth = 1280;
    PlacementHelper::GimpHeight = 800;
    PlacementHelper::ScreenWidth /= 3;

    cameraEntity = theEntityManager.CreateEntity();
    ADD_COMPONENT(cameraEntity, Transformation);
    ADD_COMPONENT(cameraEntity, RangeFollower);
    RANGE_FOLLOWER(cameraEntity)->range = Interval<float>(
        leftMostCameraPos.X, -leftMostCameraPos.X);
    
    ADD_COMPONENT(route, RangeFollower);
    RANGE_FOLLOWER(route)->range = RANGE_FOLLOWER(cameraEntity)->range;
    RANGE_FOLLOWER(route)->parent = cameraEntity;
    
    ADD_COMPONENT(silhouette, RangeFollower);
    RANGE_FOLLOWER(silhouette)->range = Interval<float>(-6, 6);
    /*RANGE_FOLLOWER(silhouette)->range = Interval<float>(
        -PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.45 - 0.5), PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.45 - 0.5));*/
    RANGE_FOLLOWER(silhouette)->parent = cameraEntity;

    ADD_COMPONENT(buildings, RangeFollower);
    RANGE_FOLLOWER(buildings)->range = Interval<float>(-2, 2);
    RANGE_FOLLOWER(buildings)->parent = cameraEntity;

    muteBtn = theEntityManager.CreateEntity();
    ADD_COMPONENT(muteBtn, Transformation);
    TRANSFORM(muteBtn)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("mute"));
    TRANSFORM(muteBtn)->parent = cameraEntity;
    TRANSFORM(muteBtn)->position = 
        theRenderingSystem.cameras[0].worldSize * Vector2(-0.5, 0.5)
        + TRANSFORM(muteBtn)->size * Vector2(0.5, -0.5)
        + Vector2(0, baseLine + theRenderingSystem.cameras[0].worldSize.Y * 0.5);
    TRANSFORM(muteBtn)->z = 0.95;
    ADD_COMPONENT(muteBtn, Rendering);
    bool muted = storageAPI->isMuted();
    RENDERING(muteBtn)->texture = theRenderingSystem.loadTextureFile(muted ? "unmute" : "mute");
    RENDERING(muteBtn)->hide = false;
    RENDERING(muteBtn)->cameraBitMask = 0x3;
    RENDERING(muteBtn)->color = Color(119.0 / 255, 119.0 / 255, 119.0 / 255);
    ADD_COMPONENT(muteBtn, Button);
    BUTTON(muteBtn)->enabled = true;
    BUTTON(muteBtn)->overSize = 1.2;

    theSoundSystem.mute = muted;
    theMusicSystem.toggleMute(muted);
}

void RecursiveRunnerGame::initGame(StorageAPI* storageAPI) {
    baseLine = PlacementHelper::GimpYToScreen(800);
    leftMostCameraPos = 
        Vector2(-PlacementHelper::ScreenWidth * (param::LevelSize * 0.5 - 0.5),
        baseLine + theRenderingSystem.cameras[0].worldSize.Y * 0.5);
    decor(storageAPI);

    scorePanel = theEntityManager.CreateEntity();
    ADD_COMPONENT(scorePanel, Transformation);
    TRANSFORM(scorePanel)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("score"));
    theTransformationSystem.setPosition(TRANSFORM(scorePanel), Vector2(0, baseLine + PlacementHelper::ScreenHeight), TransformationSystem::N);
    TRANSFORM(scorePanel)->z = 0.8;
    ADD_COMPONENT(scorePanel, Rendering);
    RENDERING(scorePanel)->texture = theRenderingSystem.loadTextureFile("score");
    RENDERING(scorePanel)->hide = false;
    // RENDERING(scorePanel)->color.a = 0.5;

    for (int i=0; i<2; i++) {
        scoreText[i] = theEntityManager.CreateEntity();
        ADD_COMPONENT(scoreText[i], Transformation);
        TRANSFORM(scoreText[i])->position = Vector2(-0.05, -0.3);
        TRANSFORM(scoreText[i])->z = 0.13;
        TRANSFORM(scoreText[i])->rotation = 0.06;
        TRANSFORM(scoreText[i])->parent = scorePanel;
        ADD_COMPONENT(scoreText[i], TextRendering);
        TEXT_RENDERING(scoreText[i])->text = "12345";
        TEXT_RENDERING(scoreText[i])->charHeight = 1.5;
        TEXT_RENDERING(scoreText[i])->hide = false;
        // TEXT_RENDERING(scoreText[i])->cameraBitMask = 0x3 << 1;
        TEXT_RENDERING(scoreText[i])->color = Color(13.0 / 255, 5.0/255, 42.0/255);
        TEXT_RENDERING(scoreText[i])->flags |= TextRenderingComponent::IsANumberBit;
    }

    // 3 cameras
    // Default camera (UI)
    RenderingSystem::Camera cam = theRenderingSystem.cameras[0];
    cam.enable = false;
    // 1st player
    theRenderingSystem.cameras.push_back(cam);
    // 2nd player
    theRenderingSystem.cameras.push_back(cam);

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
}

void RecursiveRunnerGame::init(const uint8_t* in __attribute__((unused)), int size __attribute__((unused))) {
    initGame(storageAPI);
    updateBestScore();

    for(std::map<State::Enum, StateManager*>::iterator it=state2manager.begin(); it!=state2manager.end(); ++it) {
        it->second->setup();
    }
    currentState = State::Logo;
    state2manager[currentState]->enter();
}

void RecursiveRunnerGame::changeState(State::Enum newState) {
    if (newState == currentState)
        return;
    state2manager[currentState]->exit();
    currentState = newState;
    state2manager[currentState]->enter();
}


void RecursiveRunnerGame::backPressed() {
    Game::backPressed();
}

void RecursiveRunnerGame::togglePause(bool activate __attribute__((unused))) {

}

void RecursiveRunnerGame::tick(float dt) {
    if (BUTTON(muteBtn)->clicked) {
        bool muted = !storageAPI->isMuted();
        storageAPI->setMuted(muted);
        RENDERING(muteBtn)->texture = theRenderingSystem.loadTextureFile(muted ? "unmute" : "mute");
        theSoundSystem.mute = muted;
        theMusicSystem.toggleMute(muted);
        ignoreClick = true;
    } else {
        ignoreClick = BUTTON(muteBtn)->mouseOver;
    }
    if (theTouchInputManager.isTouched(0)) {
        ignoreClick |= theTouchInputManager.getTouchLastPosition(0).Y 
            >= (TRANSFORM(muteBtn)->position.Y - TRANSFORM(muteBtn)->size.Y * BUTTON(muteBtn)->overSize * 0.5);
    }

    State::Enum newState = state2manager[currentState]->update(dt);
    for(std::map<State::Enum, StateManager*>::iterator it=state2manager.begin(); it!=state2manager.end(); ++it) {
        it->second->backgroundUpdate(dt);
    }
    
    if (newState != currentState) {
        changeState(newState);
    }

    // limit cam pos
    for (unsigned i=1; i<2 /* theRenderingSystem.cameras.size()*/; i++) {
        float& camPosX = theRenderingSystem.cameras[i].worldPosition.X;

        if (camPosX < - PlacementHelper::ScreenWidth * (param::LevelSize * 0.5 - 0.5)) {
            camPosX = - PlacementHelper::ScreenWidth * (param::LevelSize * 0.5 - 0.5);
        } else if (camPosX > PlacementHelper::ScreenWidth * (param::LevelSize * 0.5 - 0.5)) {
            camPosX = PlacementHelper::ScreenWidth * (param::LevelSize * 0.5 - 0.5);
        }
        // TRANSFORM(silhouette)->position.X = TRANSFORM(route)->position.X = camPosX;
        TRANSFORM(cameraEntity)->position.X = camPosX;
        theRenderingSystem.cameras[i].worldPosition.Y = baseLine + theRenderingSystem.cameras[i].worldSize.Y * 0.5;

        
    }

    theRangeFollowerSystem.Update(dt);

    updateFps(dt);
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


void RecursiveRunnerGame::setupCamera(CameraMode mode) {
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
            theRenderingSystem.cameras[0].worldPosition = leftMostCameraPos;
            for (unsigned i=1; i<theRenderingSystem.cameras.size(); i++) {
                theRenderingSystem.cameras[i].enable = false;
                theRenderingSystem.cameras[i].worldPosition = leftMostCameraPos;
            }
            break;
    }
}

void RecursiveRunnerGame::updateBestScore() {
    float f;
    std::vector<StorageAPI::Score> scores = storageAPI->getScores(f);
    if (!scores.empty()) {
        std::stringstream best;
        best << "Best: " << scores[0].points;
        TEXT_RENDERING(bestScore)->text = best.str();
    } else {
        LOGW("No best score found (?!)");
        TEXT_RENDERING(bestScore)->text = "";
    }
}
