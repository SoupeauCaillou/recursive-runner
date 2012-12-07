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
#include "systems/ParticuleSystem.h"
#include "systems/AutoDestroySystem.h"

#include "systems/RunnerSystem.h"
#include "systems/CameraTargetSystem.h"
#include "systems/PlayerSystem.h"
#include "systems/RangeFollowerSystem.h"
#include "systems/SessionSystem.h"

#include <iomanip>

static void updateFps(float dt);

extern std::map<TextureRef, CollisionZone> texture2Collision;

extern std::map<TextureRef, CollisionZone> texture2Collision;

static void createTransition(RecursiveRunnerGame* game,
std::map<State::Enum, StateManager*> & state2manager,
State::Enum Transition, State::Enum Past, State::Enum Future) {
   state2manager.insert(std::make_pair(Transition, new
   TransitionStateManager(Transition, state2manager[Past], state2manager[Future], game)));
}

RecursiveRunnerGame::RecursiveRunnerGame(AssetAPI* ast, StorageAPI* storage,
NameInputAPI* nameInput, AdAPI* ad, ExitAPI* exit, CommunicationAPI* communication) :
   Game() {

   assetAPI = ast;
   storageAPI = storage;
   nameInputAPI = nameInput;
   exitAPI = exit;
   communicationAPI = communication;
   adAPI = ad;

   RunnerSystem::CreateInstance();
   CameraTargetSystem::CreateInstance();
   PlayerSystem::CreateInstance();
   RangeFollowerSystem::CreateInstance();
   SessionSystem::CreateInstance();

   overrideNextState = State::Invalid;
   currentState = State::Logo;
   state2manager.insert(std::make_pair(State::Logo, new LogoStateManager(this)));
   state2manager.insert(std::make_pair(State::Menu, new MenuStateManager(this)));
   state2manager.insert(std::make_pair(State::Ad, new AdStateManager(this)));
   state2manager.insert(std::make_pair(State::Pause, new PauseStateManager(this)));
   state2manager.insert(std::make_pair(State::Rate, new RateStateManager(this)));
   state2manager.insert(std::make_pair(State::Game, new GameStateManager(this)));

   createTransition(this, state2manager, State::Logo2Menu, State::Logo, State::Menu);
   createTransition(this, state2manager, State::Game2Pause, State::Game, State::Pause);
   createTransition(this, state2manager, State::Game2Menu, State::Game, State::Menu);
   createTransition(this, state2manager, State::Game2Rate, State::Game, State::Rate);
   createTransition(this, state2manager, State::Pause2Game, State::Pause, State::Game);
   createTransition(this, state2manager, State::Ad2Game, State::Ad, State::Game);
   createTransition(this, state2manager, State::Rate2Menu, State::Rate, State::Menu);
}


RecursiveRunnerGame::~RecursiveRunnerGame() {
    LOGW("Delete game instance %p %p", this, &theEntityManager);
    theEntityManager.deleteAllEntities();

    RunnerSystem::DestroyInstance();
    CameraTargetSystem::DestroyInstance();
    PlayerSystem::DestroyInstance();
    RangeFollowerSystem::DestroyInstance();
    SessionSystem::DestroyInstance();

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
    ADD_COMPONENT(route, Music);
    MUSIC(route)->fadeOut = 2;
    MUSIC(route)->fadeIn = 1;

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

    scoreText = theEntityManager.CreateEntity();
    ADD_COMPONENT(scoreText, Transformation);
    TRANSFORM(scoreText)->position = Vector2(-0.05, -0.3);
    TRANSFORM(scoreText)->z = 0.13;
    TRANSFORM(scoreText)->rotation = 0.06;
    TRANSFORM(scoreText)->parent = scorePanel;
    ADD_COMPONENT(scoreText, TextRendering);
    std::vector<Entity> players = thePlayerSystem.RetrieveAllEntityWithComponent();
    if (!players.empty()) {
        std::stringstream a;
        a << PLAYER(players[0])->score;
        TEXT_RENDERING(scoreText)->text = a.str();
    } else {
        TEXT_RENDERING(scoreText)->text = "12345";
    }
    TEXT_RENDERING(scoreText)->charHeight = 1.5;
    TEXT_RENDERING(scoreText)->hide = false;
    // TEXT_RENDERING(scoreText[i])->cameraBitMask = 0x3 << 1;
    TEXT_RENDERING(scoreText)->color = Color(13.0 / 255, 5.0/255, 42.0/255);
    TEXT_RENDERING(scoreText)->flags |= TextRenderingComponent::IsANumberBit;

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

void RecursiveRunnerGame::init(const uint8_t* in, int size) {
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
        std::cout << index << "/" << size << "(" << eSize << ", " << sSize << ")" << std::endl;
    }

    initGame(storageAPI);
    updateBestScore();

    for(std::map<State::Enum, StateManager*>::iterator it=state2manager.begin(); it!=state2manager.end(); ++it) {
        it->second->setup();
    }
    if (size > 0 && in) {
        currentState = State::Pause;
    } else {
        currentState = State::Logo;
    }
    state2manager[currentState]->enter();
}

void RecursiveRunnerGame::changeState(State::Enum newState) {
    if (newState == currentState)
        return;
    state2manager[currentState]->exit();
    currentState = newState;
    state2manager[currentState]->enter();
}

bool RecursiveRunnerGame::willConsumeBackEvent() {
    if (currentState == State::Game)
        return true;
    return false;
}

void RecursiveRunnerGame::backPressed() {
    overrideNextState = State::Pause;
}

void RecursiveRunnerGame::togglePause(bool pause) {
    if (currentState == State::Game && pause) {
        changeState(State::Pause);
    }
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

    if (overrideNextState != State::Invalid) {
        changeState(overrideNextState);
        overrideNextState = State::Invalid;
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

            TEXT_RENDERING(scoreText)->hide = false;
            TEXT_RENDERING(scoreText)->positioning = TextRenderingComponent::CENTER;
            break;
#if 0
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
#endif
        case CameraModeMenu:
            theRenderingSystem.cameras[0].enable = true;
            theRenderingSystem.cameras[0].worldPosition = leftMostCameraPos;
            for (unsigned i=1; i<theRenderingSystem.cameras.size(); i++) {
                theRenderingSystem.cameras[i].enable = false;
                theRenderingSystem.cameras[i].worldPosition = leftMostCameraPos;
            }
            break;
         default:
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

int RecursiveRunnerGame::saveState(uint8_t** out) {
    if (currentState != State::Pause)
        return 0;
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

static void createCoins(int count, SessionComponent* session, bool transition);

void RecursiveRunnerGame::startGame(bool transition) {
    assert(theSessionSystem.RetrieveAllEntityWithComponent().empty());
    assert(thePlayerSystem.RetrieveAllEntityWithComponent().empty());

    // Create session
    Entity session = theEntityManager.CreateEntity(EntityType::Persistent);
    ADD_COMPONENT(session, Session);
    SessionComponent* sc = SESSION(session);
    sc->numPlayers = 1;
    // Create player
    Entity player = theEntityManager.CreateEntity(EntityType::Persistent);
    ADD_COMPONENT(player, Player);
    sc->players.push_back(player);

    createCoins(20, sc, transition);
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
    return TRANSFORM(e)->position.X < TRANSFORM(f)->position.X;
}

static void createCoins(int count, SessionComponent* session, bool transition) {
    LOGI("Coins creation started");
    std::vector<Entity>& coins = session->coins;
    for (int i=0; i<count; i++) {
        Entity e = theEntityManager.CreateEntity(EntityType::Persistent);
        ADD_COMPONENT(e, Transformation);
        TRANSFORM(e)->size = Vector2(0.3, 0.3) * param::CoinScale;
        Vector2 p;
        bool notFarEnough = true;
        do {
            p = Vector2(
                MathUtil::RandomFloatInRange(
                    -param::LevelSize * 0.5 * PlacementHelper::ScreenWidth,
                    param::LevelSize * 0.5 * PlacementHelper::ScreenWidth),
                MathUtil::RandomFloatInRange(
                    PlacementHelper::GimpYToScreen(700),
                    PlacementHelper::GimpYToScreen(450)));
                    //-0.3 * PlacementHelper::ScreenHeight,
                    // -0.05 * PlacementHelper::ScreenHeight));
           notFarEnough = false;
           for (unsigned j = 0; j < coins.size() && !notFarEnough; j++) {
                if (Vector2::Distance(TRANSFORM(coins[j])->position, p) < 1) {
                    notFarEnough = true;
                }
           }
        } while (notFarEnough);
        TRANSFORM(e)->position = p;
        TRANSFORM(e)->rotation = -0.1 + MathUtil::RandomFloat() * 0.2;
        TRANSFORM(e)->z = 0.75;
        ADD_COMPONENT(e, Rendering);
        RENDERING(e)->texture = theRenderingSystem.loadTextureFile("ampoule");
        // RENDERING(e)->cameraBitMask = (0x3 << 1);
        RENDERING(e)->hide = false;
        RENDERING(e)->color.a = (transition ? 0 : 1);
        #ifdef SAC_NETWORK
        ADD_COMPONENT(e, Network);
        NETWORK(e)->systemUpdatePeriod[theTransformationSystem.getName()] = 0;
        NETWORK(e)->systemUpdatePeriod[theRenderingSystem.getName()] = 0;
        #endif
        ADD_COMPONENT(e, Particule);
        PARTICULE(e)->emissionRate = 150;
         PARTICULE(e)->duration = 0;
         PARTICULE(e)->lifetime = 0.1 * 1;
         PARTICULE(e)->texture = InvalidTextureRef;
         PARTICULE(e)->initialColor = Interval<Color>(Color(135.0/255, 135.0/255, 135.0/255, 0.8), Color(145.0/255, 145.0/255, 145.0/255, 0.8));
         PARTICULE(e)->finalColor = PARTICULE(e)->initialColor;
         PARTICULE(e)->initialSize = Interval<float>(0.08, 0.16);
         PARTICULE(e)->finalSize = Interval<float>(0.0, 0.0);
         PARTICULE(e)->forceDirection = Interval<float> (0, 6.28);
         PARTICULE(e)->forceAmplitude = Interval<float>(5, 10);
         PARTICULE(e)->moment = Interval<float>(-5, 5);
         PARTICULE(e)->mass = 0.1;
        coins.push_back(e);
    }
    #if 1
    std::sort(coins.begin(), coins.end(), sortLeftToRight);
    const Vector2 offset = Vector2(0, PlacementHelper::GimpHeightToScreen(14));
    Vector2 previous = Vector2(-param::LevelSize * PlacementHelper::ScreenWidth * 0.5, -PlacementHelper::ScreenHeight * 0.2);
    for (unsigned i = 0; i <= coins.size(); i++) {
     Vector2 topI;
     if (i < coins.size()) topI = TRANSFORM(coins[i])->position + Vector2::Rotate(offset, TRANSFORM(coins[i])->rotation);
     else
         topI = Vector2(param::LevelSize * PlacementHelper::ScreenWidth * 0.5, 0);
     Entity link = theEntityManager.CreateEntity(EntityType::Persistent);
     ADD_COMPONENT(link, Transformation);
     TRANSFORM(link)->position = (topI + previous) * 0.5;
     TRANSFORM(link)->size = Vector2((topI - previous).Length(), PlacementHelper::GimpHeightToScreen(54));
     TRANSFORM(link)->z = 0.4;
     TRANSFORM(link)->rotation = MathUtil::AngleFromVector(topI - previous);
     ADD_COMPONENT(link, Rendering);
     RENDERING(link)->texture = theRenderingSystem.loadTextureFile("link");
     RENDERING(link)->hide = false;
        RENDERING(link)->color.a =  (transition ? 0 : 1);

        Entity link2 = theEntityManager.CreateEntity(EntityType::Persistent);
         ADD_COMPONENT(link2, Transformation);
         TRANSFORM(link2)->parent = link;
         TRANSFORM(link2)->size = TRANSFORM(link)->size;
         TRANSFORM(link2)->z = 0.2;
         ADD_COMPONENT(link2, Rendering);
         RENDERING(link2)->texture = theRenderingSystem.loadTextureFile("link");
         RENDERING(link2)->color.a = (transition ? 0 : 0.2);
         RENDERING(link2)->hide = false;

#if 1
        Entity link3 = theEntityManager.CreateEntity(EntityType::Persistent);
        ADD_COMPONENT(link3, Transformation);
        TRANSFORM(link3)->parent = link;
        TRANSFORM(link3)->position = Vector2(0, TRANSFORM(link)->size.Y * 0.4);
        TRANSFORM(link3)->size = TRANSFORM(link)->size * Vector2(1, 0.1);
        TRANSFORM(link3)->z = 0.2;
        ADD_COMPONENT(link3, Particule);
        PARTICULE(link3)->emissionRate = 100 * TRANSFORM(link)->size.X * TRANSFORM(link)->size.Y;
     PARTICULE(link3)->duration = 0;
     PARTICULE(link3)->lifetime = 0.1 * 1;
     PARTICULE(link3)->texture = InvalidTextureRef;
     PARTICULE(link3)->initialColor = Interval<Color>(Color(135.0/255, 135.0/255, 135.0/255, 0.8), Color(145.0/255, 145.0/255, 145.0/255, 0.8));
     PARTICULE(link3)->finalColor = PARTICULE(link3)->initialColor;
     PARTICULE(link3)->initialSize = Interval<float>(0.05, 0.1);
     PARTICULE(link3)->finalSize = Interval<float>(0.01, 0.03);
     PARTICULE(link3)->forceDirection = Interval<float> (0, 6.28);
     PARTICULE(link3)->forceAmplitude = Interval<float>(5 / 10, 10 / 10);
     PARTICULE(link3)->moment = Interval<float>(-5, 5);
     PARTICULE(link3)->mass = 0.01;
        session->sparkling.push_back(link3);
#endif
     previous = topI;
     session->links.push_back(link);
        session->links.push_back(link2);
    }
    #endif
    LOGI("Coins creation finished");
}
