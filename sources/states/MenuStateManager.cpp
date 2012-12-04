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
#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/ButtonSystem.h"
#include "systems/ADSRSystem.h"
#include "systems/TextRenderingSystem.h"
#include "systems/SoundSystem.h"
#include "systems/MusicSystem.h"
#include "systems/PlayerSystem.h"
#include "systems/ParticuleSystem.h"

#include "../RecursiveRunnerGame.h"
#include "../Parameters.h"

#include <sstream>
#include <vector>

static void createCoins(int count, GameTempVar& gameTempVars);

static void startMenuMusic(Entity title) {
    MUSIC(title)->music = theMusicSystem.loadMusicFile("intro-menu.ogg");
    MUSIC(title)->loopNext = theMusicSystem.loadMusicFile("boucle-menu.ogg");
    MUSIC(title)->loopAt = 4.54;
    MUSIC(title)->fadeOut = 2;
    MUSIC(title)->fadeIn = 1;
    MUSIC(title)->control = MusicControl::Play;
}

struct MenuStateManager::MenuStateManagerDatas {
    Entity titleGroup, title, subtitle, subtitleText, swarmBtn;
};

MenuStateManager::MenuStateManager(RecursiveRunnerGame* game) : StateManager(State::Menu, game) {
    datas = new MenuStateManagerDatas;
}

MenuStateManager::~MenuStateManager() {
    delete datas;
}

void MenuStateManager::setup() {
    Entity titleGroup = datas->titleGroup  = theEntityManager.CreateEntity();
    ADD_COMPONENT(titleGroup, Transformation);
    TRANSFORM(titleGroup)->z = 0.7;
    TRANSFORM(titleGroup)->rotation = 0.05;
    ADD_COMPONENT(titleGroup, ADSR);
    ADSR(titleGroup)->idleValue = PlacementHelper::ScreenHeight + PlacementHelper::GimpYToScreen(400);
    ADSR(titleGroup)->sustainValue = 
        game->baseLine + 
        PlacementHelper::ScreenHeight - PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("titre")).Y * 0.5
        + PlacementHelper::GimpHeightToScreen(20);
    ADSR(titleGroup)->attackValue = ADSR(titleGroup)->sustainValue - PlacementHelper::GimpHeightToScreen(5);
    ADSR(titleGroup)->attackTiming = 2;
    ADSR(titleGroup)->decayTiming = 0.2;
    ADSR(titleGroup)->releaseTiming = 1.5;
    TRANSFORM(titleGroup)->position = Vector2(game->leftMostCameraPos.X + TRANSFORM(titleGroup)->size.X * 0.5, ADSR(titleGroup)->idleValue);
    ADD_COMPONENT(titleGroup, Music);
    MUSIC(titleGroup)->fadeOut = 2;
    MUSIC(titleGroup)->fadeIn = 1;

    Entity title = datas->title = theEntityManager.CreateEntity();
    ADD_COMPONENT(title, Transformation);
    TRANSFORM(title)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("titre"));
    TRANSFORM(title)->parent = titleGroup;
    TRANSFORM(title)->position = Vector2::Zero;
    TRANSFORM(title)->z = 0.15;
    ADD_COMPONENT(title, Rendering);
    RENDERING(title)->texture = theRenderingSystem.loadTextureFile("titre");
    RENDERING(title)->hide = false;
    RENDERING(title)->cameraBitMask = 0x1;
    ADD_COMPONENT(title, Music);

    Entity subtitle = datas->subtitle = theEntityManager.CreateEntity();
    ADD_COMPONENT(subtitle, Transformation);
    TRANSFORM(subtitle)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("taptostart"));
    TRANSFORM(subtitle)->parent = titleGroup;
    TRANSFORM(subtitle)->position = Vector2(-PlacementHelper::GimpWidthToScreen(0), -PlacementHelper::GimpHeightToScreen(150));
    TRANSFORM(subtitle)->z = -0.1;
    ADD_COMPONENT(subtitle, Rendering);
    RENDERING(subtitle)->texture = theRenderingSystem.loadTextureFile("taptostart");
    RENDERING(subtitle)->hide = false;
    RENDERING(subtitle)->cameraBitMask = 0x1;
    ADD_COMPONENT(subtitle, ADSR);
    ADSR(subtitle)->idleValue = 0;
    ADSR(subtitle)->sustainValue = -PlacementHelper::GimpHeightToScreen(150);
    ADSR(subtitle)->attackValue = -PlacementHelper::GimpHeightToScreen(150);
    ADSR(subtitle)->attackTiming = 2;
    ADSR(subtitle)->decayTiming = 0.1;
    ADSR(subtitle)->releaseTiming = 1;

    Entity subtitleText = datas->subtitleText = theEntityManager.CreateEntity();
    ADD_COMPONENT(subtitleText, Transformation);
    TRANSFORM(subtitleText)->parent = subtitle;
    TRANSFORM(subtitleText)->position = Vector2(0, -PlacementHelper::GimpHeightToScreen(25));
    ADD_COMPONENT(subtitleText, TextRendering);
    TEXT_RENDERING(subtitleText)->text = "Tap screen to start";
    TEXT_RENDERING(subtitleText)->charHeight = PlacementHelper::GimpHeightToScreen(45);
    TEXT_RENDERING(subtitleText)->hide = false;
    TEXT_RENDERING(subtitleText)->cameraBitMask = 0x1;
    TEXT_RENDERING(subtitleText)->color = Color(40.0 / 255, 32.0/255, 30.0/255, 0.8);

    Entity swarmBtn = datas->swarmBtn = theEntityManager.CreateEntity();
    ADD_COMPONENT(swarmBtn, Transformation);
    TRANSFORM(swarmBtn)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("swarm_icon"));
    TRANSFORM(swarmBtn)->parent = game->cameraEntity;
    TRANSFORM(swarmBtn)->position =
        theRenderingSystem.cameras[0].worldSize * Vector2(-0.5, -0.5)
        + TRANSFORM(swarmBtn)->size * Vector2(0.5, 0.5)
        + Vector2(0, game->baseLine + theRenderingSystem.cameras[0].worldSize.Y * 0.5);
        
    TRANSFORM(swarmBtn)->z = 0.95;
    ADD_COMPONENT(swarmBtn, Rendering);
    RENDERING(swarmBtn)->texture = theRenderingSystem.loadTextureFile("swarm_icon");
    RENDERING(swarmBtn)->hide = false;
    RENDERING(swarmBtn)->cameraBitMask = 0x1;
    ADD_COMPONENT(swarmBtn, Button);
    BUTTON(swarmBtn)->overSize = 1.2;
}

void MenuStateManager::earlyEnter() {
    // Restore camera position
    for (unsigned i=0; i<theRenderingSystem.cameras.size(); i++) {
        theRenderingSystem.cameras[i].worldPosition = game->leftMostCameraPos;
    }
    game->setupCamera(CameraModeMenu);
    std::vector<Entity> players = thePlayerSystem.RetrieveAllEntityWithComponent();
    if (!players.empty()) {
        std::stringstream a;
        a << PLAYER(players[0])->score << " points - tap screen to restart";
        TEXT_RENDERING(datas->subtitleText)->text = a.str();
        game->storageAPI->submitScore(StorageAPI::Score(PLAYER(players[0])->score, PLAYER(players[0])->coins, "rzehtrtyBg"));
        game->updateBestScore();
    }
    if (!theMusicSystem.isMuted()) {
        startMenuMusic(datas->title);
    }
}

void MenuStateManager::enter() {
    game->gameTempVars.cleanup();
}

void MenuStateManager::backgroundUpdate(float dt) {
    TRANSFORM(datas->titleGroup)->position.Y = ADSR(datas->titleGroup)->value;
    TRANSFORM(datas->subtitle)->position.Y = ADSR(datas->subtitle)->value;
}

State::Enum MenuStateManager::update(float dt) {
    const Entity titleGroup = datas->titleGroup;
    const Entity title = datas->title;
    const Entity subtitle = datas->subtitle;

    if (!theMusicSystem.isMuted()) {
        MusicComponent* music = MUSIC(title);
        music->control = MusicControl::Play;

        if (music->music == InvalidMusicRef) {
            startMenuMusic(title);
        } else if (music->loopNext == InvalidMusicRef) {
            music->loopAt = 21.34;
            music->loopNext = theMusicSystem.loadMusicFile("boucle-menu.ogg");
        }
    }

    // Handle Swarm button
    if (!game->ignoreClick) {
        if (BUTTON(datas->swarmBtn)->clicked) {
            game->communicationAPI->swarmRegistering(-1,-1);
        }
        game->ignoreClick = BUTTON(datas->swarmBtn)->mouseOver;
    }

    // Restore music ?
    if (theTouchInputManager.isTouched(0)) {
        if (theMusicSystem.isMuted() != theSoundSystem.mute) {
            theMusicSystem.toggleMute(theSoundSystem.mute);
            game->ignoreClick = true;
        }
    }

    // Start game ?
    if (theTouchInputManager.isTouched(0) && theTouchInputManager.wasTouched(0) && !game->ignoreClick) {
        ADSR(titleGroup)->active = ADSR(subtitle)->active = false;
        MUSIC(titleGroup)->music = theMusicSystem.loadMusicFile("jeu.ogg");
        return State::Menu2Game;
    }
    return State::Menu;
}

void MenuStateManager::exit() {
    game->gameTempVars.numPlayers = 1;
    if (game->gameTempVars.players.empty()) {
        Entity e = theEntityManager.CreateEntity();
        ADD_COMPONENT(e, Player);
        game->gameTempVars.players.push_back(e);
    }
    MUSIC(datas->title)->control = MusicControl::Stop;
    BUTTON(datas->swarmBtn)->enabled = false;

    createCoins(20, game->gameTempVars);
    game->gameTempVars.syncCoins();
}

void MenuStateManager::lateExit() {
    game->setupCamera(CameraModeSingle);
}

float minipause;
bool MenuStateManager::transitionCanExit() {
    const ADSRComponent* adsr = ADSR(datas->titleGroup);
    {
        float progress = (adsr->value - adsr->attackValue) /
            (adsr->idleValue - adsr->attackValue);
        progress = MathUtil::Max(0.0f, MathUtil::Min(1.0f, progress));
        for (unsigned i=0; i<game->gameTempVars.coins.size(); i++) {
            RENDERING(game->gameTempVars.coins[i])->color.a = progress;
        }
        for (unsigned i=0; i<game->gameTempVars.links.size(); i++) {
            if (i % 2)
                RENDERING(game->gameTempVars.links[i])->color.a = progress * 0.2;
            else
                RENDERING(game->gameTempVars.links[i])->color.a = progress;
        }
    }
    PLAYER(game->gameTempVars.players[0])->ready = true;
    for (std::vector<Entity>::iterator it=game->gameTempVars.players.begin(); it!=game->gameTempVars.players.end(); ++it) {
        if (!PLAYER(*it)->ready) {
            return false;
        }
    }
    if (adsr->value < adsr->idleValue) {
        minipause = TimeUtil::getTime();
        return false;
    }
    MUSIC(datas->titleGroup)->control = MusicControl::Play;

    if (TimeUtil::getTime() - minipause >= 1) {
        return true;
    } else {
        return false;
    }
}

bool MenuStateManager::transitionCanEnter() {
    ADSRComponent* adsr = ADSR(datas->titleGroup);
    adsr->active = ADSR(datas->subtitle)->active = true;
    {
        float progress = (adsr->value - adsr->attackValue) /
            (adsr->idleValue - adsr->attackValue);
        progress = MathUtil::Max(0.0f, MathUtil::Min(1.0f, progress));
        for (unsigned i=0; i<game->gameTempVars.coins.size(); i++) {
            RENDERING(game->gameTempVars.coins[i])->color.a = progress;
        }
        for (unsigned i=0; i<game->gameTempVars.links.size(); i++) {
            if (i % 2)
                RENDERING(game->gameTempVars.links[i])->color.a = progress * 0.2;
            else
                RENDERING(game->gameTempVars.links[i])->color.a = progress;
        }
    }
    
    return (ADSR(datas->titleGroup)->value == ADSR(datas->titleGroup)->sustainValue);
}

static bool sortLeftToRight(Entity e, Entity f) {
 return TRANSFORM(e)->position.X < TRANSFORM(f)->position.X;
}

static void createCoins(int count, GameTempVar& gameTempVars) {
    LOGI("Coins creation started");
    std::vector<Entity> coins;
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
        RENDERING(e)->color.a = 0;
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
        RENDERING(link)->color.a = 0;

        Entity link2 = theEntityManager.CreateEntity(EntityType::Persistent);
         ADD_COMPONENT(link2, Transformation);
         TRANSFORM(link2)->parent = link;
         TRANSFORM(link2)->size = TRANSFORM(link)->size;
         TRANSFORM(link2)->z = 0.2;
         ADD_COMPONENT(link2, Rendering);
         RENDERING(link2)->texture = theRenderingSystem.loadTextureFile("link");
         RENDERING(link2)->color.a = 0;
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
        gameTempVars.sparkling.push_back(link3);
#endif
     previous = topI;
     gameTempVars.links.push_back(link);
        gameTempVars.links.push_back(link2);
    }
    #endif
    LOGI("Coins creation finished");
}
