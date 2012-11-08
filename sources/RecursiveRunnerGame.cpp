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
#include "api/linux/StorageAPILinuxImpl.h"
#include "api/linux/NameAPILinuxImpl.h"
#endif
#include "systems/RunnerSystem.h"
#include "systems/CameraTargetSystem.h"
#include "systems/PlayerSystem.h"
#include "systems/RangeFollowerSystem.h"

#include <cmath>
#include <vector>
#include <iomanip>


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
    

static void spawnGainEntity(int gain, Entity t, const Color& c);
static Entity addRunnerToPlayer(Entity player, PlayerComponent* p, int playerIndex);
static void updateFps(float dt);
static void setupCamera(CameraMode mode);

extern std::map<TextureRef, CollisionZone> texture2Collision;


#ifdef SAC_NETWORK
Entity networkUL, networkDL;
#endif
Entity scoreText[2], goldCoin, scorePanel, bestScore;
Entity titleGroup, title, subtitle;

StorageAPI* tmpStorageAPI;
NameInputAPI* tmpNameInputAPI;
Vector2 leftMostCameraPos;
std::string playerName;

enum GameOverState {
    NoGame,
    GameEnded,
    AskingPlayerName
} gameOverState;
        

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
    std::vector<Entity> runners[2], coins, players, links; 
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
float baseLine;


RecursiveRunnerGame::RecursiveRunnerGame(AssetAPI* ast, StorageAPI* storage, NameInputAPI* nameInput, AdAPI* ad __attribute__((unused)), ExitAPI* exAPI) : Game() {
	assetAPI = ast;
	storageAPI = storage;
    nameInputAPI = nameInput;
	exitAPI = exAPI;

    //to remove...
    tmpStorageAPI = storage;
    tmpNameInputAPI = nameInput;
}

void RecursiveRunnerGame::sacInit(int windowW, int windowH) {
    Game::sacInit(windowW, windowH);
	PlacementHelper::GimpWidth = 1280;
    PlacementHelper::GimpHeight = 800;
    theRenderingSystem.loadAtlas("alphabet", true);
    theRenderingSystem.loadAtlas("dummy", true);
    theRenderingSystem.loadAtlas("decor", true);
    theRenderingSystem.loadAtlas("arbre", true);
    theRenderingSystem.loadAtlas("fumee", true);
    
    // register 4 animations
    std::string runL2R[] = { "run_l2r_0002",
        "run_l2r_0003", "run_l2r_0004", "run_l2r_0005",
        "run_l2r_0006", "run_l2r_0007", "run_l2r_0008",
        "run_l2r_0009", "run_l2r_0010", "run_l2r_0011", "run_l2r_0000", "run_l2r_0001"};
    std::string jumpL2R[] = { "jump_l2r_0004", "jump_l2r_0005",
        "jump_l2r_0006", "jump_l2r_0007", "jump_l2r_0008",
        "jump_l2r_0009", "jump_l2r_0010", "jump_l2r_0011"};
    std::string jumpL2Rtojump[] = { "jump_l2r_0012", "jump_l2r_0013", "jump_l2r_0015"};

    theAnimationSystem.registerAnim("runL2R", runL2R, 12, 15, Interval<int>(-1, -1));
    theAnimationSystem.registerAnim("jumpL2R_up", jumpL2R, 6, 15, Interval<int>(0, 0));
    theAnimationSystem.registerAnim("jumpL2R_down", &jumpL2R[6], 2, 15, Interval<int>(0, 0));
    theAnimationSystem.registerAnim("jumptorunL2R", jumpL2Rtojump, 3, 30, Interval<int>(0, 0), "runL2R");

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

Entity silhouette, route, cameraEntity;
std::vector<Entity> decorEntities;

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

void decor() {
	silhouette = theEntityManager.CreateEntity();
    ADD_COMPONENT(silhouette, Transformation);
    TRANSFORM(silhouette)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("silhouette_ville")) * 4;
    // TRANSFORM(silhouette)->size.X *= 1.6;
    theTransformationSystem.setPosition(TRANSFORM(silhouette), Vector2(0, PlacementHelper::GimpYToScreen(0)), TransformationSystem::N);
    TRANSFORM(silhouette)->z = 0.1;
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
		Decor(1958, 782, 0.5, TransformationSystem::S, "arbre4", false, trees),
		Decor(2396, 774, 0.44, TransformationSystem::S, "arbre5", false, trees),
		Decor(2684, 784, 0.45, TransformationSystem::S, "arbre3", false, trees),
		Decor(3022, 764, 0.42, TransformationSystem::S, "arbre1", false, trees),
		Decor(3290, 764, 0.41, TransformationSystem::S, "arbre1", false, trees),
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


    titleGroup  = theEntityManager.CreateEntity();
    ADD_COMPONENT(titleGroup, Transformation);
    TRANSFORM(titleGroup)->z = 0.7;
    TRANSFORM(titleGroup)->rotation = 0.05;
    ADD_COMPONENT(titleGroup, ADSR);
    ADSR(titleGroup)->idleValue = PlacementHelper::ScreenHeight + PlacementHelper::GimpYToScreen(400);
    ADSR(titleGroup)->sustainValue = 
        baseLine + 
        PlacementHelper::ScreenHeight - PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("titre")).Y * 0.5
        + PlacementHelper::GimpHeightToScreen(20);
    ADSR(titleGroup)->attackValue = ADSR(titleGroup)->sustainValue - PlacementHelper::GimpHeightToScreen(5);
    ADSR(titleGroup)->attackTiming = 2;
    ADSR(titleGroup)->decayTiming = 0.2;
    ADSR(titleGroup)->releaseTiming = 1.5;
    TRANSFORM(titleGroup)->position = Vector2(PlacementHelper::GimpXToScreen(640), ADSR(titleGroup)->idleValue);

    title = theEntityManager.CreateEntity();
    ADD_COMPONENT(title, Transformation);
    TRANSFORM(title)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("titre"));
    TRANSFORM(title)->parent = titleGroup;
    TRANSFORM(title)->position = Vector2::Zero;
    TRANSFORM(title)->z = 0.15;
    ADD_COMPONENT(title, Rendering);
    RENDERING(title)->texture = theRenderingSystem.loadTextureFile("titre");
    RENDERING(title)->hide = false;
    RENDERING(title)->cameraBitMask = 0x1;

    subtitle = theEntityManager.CreateEntity();
    ADD_COMPONENT(subtitle, Transformation);
    TRANSFORM(subtitle)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("taptostart"));
    TRANSFORM(subtitle)->parent = titleGroup;
    TRANSFORM(subtitle)->position = Vector2(-PlacementHelper::GimpHeightToScreen(10), -PlacementHelper::GimpHeightToScreen(150));
    TRANSFORM(subtitle)->z = 0.1;
    ADD_COMPONENT(subtitle, Rendering);
    RENDERING(subtitle)->texture = theRenderingSystem.loadTextureFile("taptostart");
    RENDERING(subtitle)->hide = false;
    RENDERING(subtitle)->cameraBitMask = 0x1;

	PlacementHelper::GimpWidth = 1280;
    PlacementHelper::GimpHeight = 800;
    PlacementHelper::ScreenWidth /= 3;
    
    leftMostCameraPos = 
        Vector2(-PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5),
        baseLine + theRenderingSystem.cameras[0].worldSize.Y * 0.5);

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
}

void RecursiveRunnerGame::init(const uint8_t* in __attribute__((unused)), int size __attribute__((unused))) {
    RunnerSystem::CreateInstance();
    CameraTargetSystem::CreateInstance();
    PlayerSystem::CreateInstance();
    RangeFollowerSystem::CreateInstance();

    baseLine = PlacementHelper::GimpYToScreen(800);
    decor();

#if 0
    background = theEntityManager.CreateEntity();
    ADD_COMPONENT(background, Transformation);
    TRANSFORM(background)->size = Vector2(LEVEL_SIZE * PlacementHelper::ScreenWidth, PlacementHelper::ScreenHeight);
    TRANSFORM(background)->position.X = 0;
    TRANSFORM(background)->position.Y = -(PlacementHelper::ScreenHeight - TRANSFORM(background)->size.Y)*0.5;
    TRANSFORM(background)->z = 0.1;
    ADD_COMPONENT(background, Rendering);
    RENDERING(background)->texture = theRenderingSystem.loadTextureFile("decor-entier");
    RENDERING(background)->hide = false;
    RENDERING(background)->opaqueType = RenderingComponent::FULL_OPAQUE;
    RENDERING(background)->cameraBitMask = (0x3 << 1);
    #endif

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
    
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0000")] =  CollisionZone(50,50,25,77);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0001")] =  CollisionZone(50,50,25,77);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0002")] =  CollisionZone(49,64,22,67);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0003")] =  CollisionZone(49,64,22,67);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0004")] =  CollisionZone(50,56,27,73, -0.3);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0005")] =  CollisionZone(43,35,37,71, -0.5);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0006")] =  CollisionZone(45,30,37,65, -0.5);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0007")] =  CollisionZone(46,17,36,74);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0008")] =  CollisionZone(46,10,35,54);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0009")] =  CollisionZone(40,11,38,60, 0.4);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0010")] =  CollisionZone(39,23,36,62, 0.5);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0011")] =  CollisionZone(38,39,37,69, 0.4);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0012")] =  CollisionZone(45,56,35,73, 0.2);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0013")] =  CollisionZone(46,70,32,62);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0014")] =  CollisionZone(41,71,32,59);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0015")] =  CollisionZone(50,67,28,62);
    texture2Collision[theRenderingSystem.loadTextureFile("jump_l2r_0016")] =  CollisionZone(47,58,29,74);
    for (int i=0; i<12; i++) {
        std::stringstream a;
        a << "run_l2r_" << std::setfill('0') << std::setw(4) << i;
        texture2Collision[theRenderingSystem.loadTextureFile(a.str())] =  CollisionZone(58,51,28,65,-0.5);
    }
}


void RecursiveRunnerGame::backPressed() {
    Game::backPressed();
}

void RecursiveRunnerGame::togglePause(bool activate __attribute__((unused))) {

}

void RecursiveRunnerGame::tick(float dt) {
	theTouchInputManager.Update(dt);
    TRANSFORM(titleGroup)->position.Y = ADSR(titleGroup)->value;
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
    for (unsigned i=1; i<2 /* theRenderingSystem.cameras.size()*/; i++) {
        float& camPosX = theRenderingSystem.cameras[i].worldPosition.X;

        if (camPosX < - PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5)) {
            camPosX = - PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5);
        } else if (camPosX > PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5)) {
            camPosX = PlacementHelper::ScreenWidth * (LEVEL_SIZE * 0.5 - 0.5);
        }
        // TRANSFORM(silhouette)->position.X = TRANSFORM(route)->position.X = camPosX;
        TRANSFORM(cameraEntity)->position.X = camPosX;
        theRenderingSystem.cameras[i].worldPosition.Y = baseLine + theRenderingSystem.cameras[i].worldSize.Y * 0.5;

        
    }

    theRangeFollowerSystem.Update(dt);
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

static void updateBestScore() {
    float f;
    std::vector<StorageAPI::Score> scores = tmpStorageAPI->getScores(f);
    if (!scores.empty()) {
        std::stringstream best;
        best << scores[0].points;
        TEXT_RENDERING(bestScore)->text = best.str();
    } else {
        LOGW("No best score found (?!)");
        TEXT_RENDERING(bestScore)->text = "";
    }
}
static GameState updateMenu(float dt __attribute__((unused))) {
    switch (gameOverState) {
        case NoGame: {
			break;
        }
        case GameEnded: {
            //should test if its a good score
            if (1) {
                tmpNameInputAPI->show();
				gameOverState = AskingPlayerName;
            } else {
				gameOverState = NoGame;
                tmpStorageAPI->submitScore(StorageAPI::Score(PLAYER(gameTempVars.players[0])->score, PLAYER(gameTempVars.players[0])->coins, "rzehtrtyBg"));
                // Cleanup previous game variables
                gameTempVars.cleanup();
            }
        }
        case AskingPlayerName: {
            if (tmpNameInputAPI->done(playerName)) {
                tmpNameInputAPI->hide();
                
                tmpStorageAPI->submitScore(StorageAPI::Score(PLAYER(gameTempVars.players[0])->score, PLAYER(gameTempVars.players[0])->coins, playerName));
                gameOverState = NoGame;
                
                updateBestScore();
    
                // Cleanup previous game variables
                gameTempVars.cleanup();
            } else {
                return Menu;
            }
            break;
        }
    }
    
    if (ADSR(titleGroup)->value == ADSR(titleGroup)->sustainValue) {
        if (theTouchInputManager.isTouched(0)) {
            gameTempVars.numPlayers = 1;
            gameTempVars.isGameMaster = true;
            ADSR(titleGroup)->active = false;
            return WaitingPlayers;
        }
    }
    return Menu;
}

static void transitionMenuWaitingPlayers() {
    LOGI("Change state");
#ifdef SAC_NETWORK
    theNetworkSystem.deleteAllNonLocalEntities();
#endif
}

static GameState updateWaitingPlayers(float dt __attribute__((unused))) {
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
					 Entity run =
					 #endif
                addRunnerToPlayer(e, PLAYER(e), i);
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
    if (ADSR(titleGroup)->value < ADSR(titleGroup)->idleValue)
        return WaitingPlayers;

    setupCamera(CameraModeSingle);
    return Playing;
}

static void transitionWaitingPlayersPlaying() {
    LOGI("Change state");
    // store a few entities to avoid permanent lookups
    gameTempVars.syncCoins();
    gameTempVars.syncRunners();
#ifdef SAC_NETWORK
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
            if (PLAYER(gameTempVars.players[i])->runnersCount == param::runner) {
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
                if ((tc->position.Y - tc->size.Y * 0.5) <= baseLine) {
                    pc->gravity.Y = 0;
                    pc->linearVelocity = Vector2::Zero;
                    tc->position.Y = baseLine + tc->size.Y * 0.5;
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
                                #if 0
                                if (rc->speed > 0) {
                                	for (int j=1; j<rc->coinSequenceBonus; j++) {
                                		float t = 1 * ((rc->coinSequenceBonus - (j - 1.0)) / (float)rc->coinSequenceBonus);
                                		std::cout << "adding: " << t;
                                		PARTICULE(gameTempVars.links[linkIdx - j + 1])->duration += t;
                                		std::cout << " -> " << PARTICULE(gameTempVars.links[linkIdx - j + 1])->duration << std::endl;
                                			
                                	}
                                } else {
                                	for (int j=1; j<rc->coinSequenceBonus; j++) {
                                		PARTICULE(gameTempVars.links[linkIdx + j - 1])->duration += 
                                			1 * ((rc->coinSequenceBonus - (j - 1.0)) / (float)rc->coinSequenceBonus);
                                	}
                                }
                                #endif
                                
                            } else {
                                rc->coinSequenceBonus = 1;
                            }
                        }
                        rc->coins.push_back(coin);
                        int gain = ((coin == goldCoin) ? 30 : 10) * pow(2.0f, rc->oldNessBonus) * rc->coinSequenceBonus;
                        player->score += gain;
                        
                        //coins++ only for player, not his ghosts
                        if (j == gameTempVars.runners[i].size() - 1)
                            player->coins++;
                        
                        spawnGainEntity(gain, coin, rc->color);
                    }
                }
                prev = coin;
            }
        }
    }

    for (unsigned i=0; i<gameTempVars.players.size(); i++) {
        std::stringstream a;
        a << PLAYER(gameTempVars.players[i])->score;
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
        theRenderingSystem.cameras[i].worldPosition = leftMostCameraPos;
    }
    // Save score and coins earned
    if (gameTempVars.players.size()) {
        gameOverState = GameEnded;
    }

    // TEXT_RENDERING(scoreText)->hide = false;
    updateBestScore();
    ADSR(titleGroup)->active = true;
}

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
#define COIN_SCALE 3
void GameTempVar::syncCoins() {
    std::vector<Entity> t = theTransformationSystem.RetrieveAllEntityWithComponent();
    for (unsigned i=0; i<t.size(); i++) {
        //...
        float x = TRANSFORM(t[i])->size.X;
        if (MathUtil::Abs(x - 0.3 * COIN_SCALE) < 0.01) {
            coins.push_back(t[i]);
        }
    }
    std::sort(coins.begin(), coins.end(), sortFromLeftToRight);
}

int GameTempVar::playerIndex() {
    return (isGameMaster ? 0 : 1);
}

static bool sortLeftToRight(Entity e, Entity f) {
	return TRANSFORM(e)->position.X < TRANSFORM(f)->position.X;
}

static void createCoins(int count) {
    std::vector<Entity> coins;
    float min = LEVEL_SIZE * PlacementHelper::ScreenWidth;
    for (int i=0; i<count; i++) {
        Entity e = theEntityManager.CreateEntity();
        ADD_COMPONENT(e, Transformation);
        TRANSFORM(e)->size = Vector2(0.3, 0.3) * COIN_SCALE;
        Vector2 p;
        bool notFarEnough = true;
        do {
            p = Vector2(
                MathUtil::RandomFloatInRange(
                    -LEVEL_SIZE * 0.5 * PlacementHelper::ScreenWidth,
                    LEVEL_SIZE * 0.5 * PlacementHelper::ScreenWidth),
                MathUtil::RandomFloatInRange(
                    PlacementHelper::GimpYToScreen(650),
                    PlacementHelper::GimpYToScreen(450)));
                    //-0.3 * PlacementHelper::ScreenHeight,
                    // -0.05 * PlacementHelper::ScreenHeight));
           notFarEnough = false;
           for (unsigned j = 0; j < coins.size() && !notFarEnough; j++) {
                if (Vector2::Distance(TRANSFORM(coins[j])->position, p) < 2) {
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
        if (MathUtil::Abs(TRANSFORM(e)->position.X) < min) {
            goldCoin = e;
            min = MathUtil::Abs(TRANSFORM(e)->position.X);
        }
        #ifdef SAC_NETWORK
        ADD_COMPONENT(e, Network);
        NETWORK(e)->systemUpdatePeriod[theTransformationSystem.getName()] = 0;
        NETWORK(e)->systemUpdatePeriod[theRenderingSystem.getName()] = 0;
        #endif
        coins.push_back(e);
    }
    #if 1
    std::sort(coins.begin(), coins.end(), sortLeftToRight);
    const Vector2 offset = Vector2(0, PlacementHelper::GimpHeightToScreen(14));
    Vector2 previous = Vector2(-LEVEL_SIZE * PlacementHelper::ScreenWidth * 0.5, -PlacementHelper::ScreenHeight * 0.2);
    for (unsigned i = 0; i <= coins.size(); i++) {
    	Vector2 topI;
    	if (i < coins.size()) topI = TRANSFORM(coins[i])->position + Vector2::Rotate(offset, TRANSFORM(coins[i])->rotation);
    	else
    		topI = Vector2(LEVEL_SIZE * PlacementHelper::ScreenWidth * 0.5, 0);
    	Entity link = theEntityManager.CreateEntity();
    	ADD_COMPONENT(link, Transformation);
    	TRANSFORM(link)->position = (topI + previous) * 0.5;
    	TRANSFORM(link)->size = Vector2((topI - previous).Length(), PlacementHelper::GimpHeightToScreen(54));
    	TRANSFORM(link)->z = 0.4;
    	TRANSFORM(link)->rotation = MathUtil::AngleFromVector(topI - previous);
    	ADD_COMPONENT(link, Rendering);
    	RENDERING(link)->texture = theRenderingSystem.loadTextureFile("link");
    	RENDERING(link)->hide = false;

        Entity link2 = theEntityManager.CreateEntity();
         ADD_COMPONENT(link2, Transformation);
         TRANSFORM(link2)->parent = link;
         TRANSFORM(link2)->size = TRANSFORM(link)->size;
         TRANSFORM(link2)->z = 0.2;
         ADD_COMPONENT(link2, Rendering);
         RENDERING(link2)->texture = theRenderingSystem.loadTextureFile("link");
         RENDERING(link2)->color.a = 0.2;
         RENDERING(link2)->hide = false;

#if 0
        ADD_COMPONENT(link, Particule);
        PARTICULE(link)->emissionRate = 300 * TRANSFORM(link)->size.X * TRANSFORM(link)->size.Y;
    	PARTICULE(link)->duration = 0;
    	PARTICULE(link)->lifetime = 0.1;
    	PARTICULE(link)->texture = InvalidTextureRef;
    	PARTICULE(link)->initialColor = Interval<Color>(Color(1, 1, 0, 1), Color(1, 0.8, 0, 1));
    	PARTICULE(link)->finalColor = PARTICULE(link)->initialColor;
    	PARTICULE(link)->initialSize = Interval<float>(0.05, 0.1);
    	PARTICULE(link)->finalSize = Interval<float>(0.05, 0.1);
    	PARTICULE(link)->forceDirection = Interval<float> (0, 6.28);
    	PARTICULE(link)->forceAmplitude = Interval<float>(5, 10);
    	PARTICULE(link)->moment = Interval<float>(-5, 5);
    	PARTICULE(link)->mass = 0.1;
#endif
    	previous = topI;
    	gameTempVars.links.push_back(link);
        gameTempVars.links.push_back(link2);
    }
    #endif
    
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

static void spawnGainEntity(int gain __attribute__((unused)), Entity parent, const Color& color) {
    Entity e = theEntityManager.CreateEntity();
    ADD_COMPONENT(e, Transformation);
    TRANSFORM(e)->position = TRANSFORM(parent)->position;
    TRANSFORM(e)->rotation = TRANSFORM(parent)->rotation;
    TRANSFORM(e)->size = TRANSFORM(parent)->size;
    TRANSFORM(e)->z = TRANSFORM(parent)->z + 0.1;
    ADD_COMPONENT(e, Rendering);
    RENDERING(e)->texture = theRenderingSystem.loadTextureFile("lumiere");
    RENDERING(e)->color = color;
    RENDERING(e)->hide = false;
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
    AUTO_DESTROY(e)->params.lifetime.value = 3;
    AUTO_DESTROY(e)->params.lifetime.map2AlphaRendering = true;
    // AUTO_DESTROY(e)->hasTextRendering = true;
}

static Entity addRunnerToPlayer(Entity player, PlayerComponent* p, int playerIndex) {
    int direction = ((p->runnersCount + playerIndex) % 2) ? -1 : 1;
    Entity e = theEntityManager.CreateEntity();
    ADD_COMPONENT(e, Transformation);
    TRANSFORM(e)->position = Vector2(-9, 2);
    TRANSFORM(e)->size = Vector2(0.85, 0.85) * 3;//0.4,1);//0.572173, 0.815538);
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
        Vector2(direction * -LEVEL_SIZE * 0.5 * PlacementHelper::ScreenWidth, baseLine), TransformationSystem::S);
    RUNNER(e)->startPoint = TRANSFORM(e)->position;
    RUNNER(e)->endPoint = RUNNER(e)->startPoint + Vector2(direction * LEVEL_SIZE * PlacementHelper::ScreenWidth, 0);
    RUNNER(e)->speed = direction * playerSpeed * (param::speedConst + param::speedCoeff * p->runnersCount);
    RUNNER(e)->startTime = 0;//MathUtil::RandomFloatInRange(1,3);
    RUNNER(e)->playerOwner = player;
    do {
        Color c = Color::random();
        float sum = c.r + c.b + c.g;
        if (sum > 1.5 || c.r > 0.7 || c.g > 0.7 || c.b > 0.7) {
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
    ANIMATION(e)->playbackSpeed = 1 + 0.08 * p->runnersCount;
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
#ifdef SAC_NETWORK
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
            // TRANSFORM(scoreText[0])->position = Vector2(0, 0.35 * PlacementHelper::ScreenHeight);
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
