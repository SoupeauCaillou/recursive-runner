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

#include <base/EntityManager.h>
#include <base/TouchInputManager.h>
#include <base/PlacementHelper.h>

#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/SoundSystem.h"

#include "DepthLayer.h"

enum LogoStep {
    LogoStep0,
    LogoStep1,
    LogoStep2,
    LogoStep3,
    LogoStep4,
    LogoStep5,
    LogoStep6,
};

struct LogoStateManager::LogoStateManagerDatas {
    Entity logo, animLogo, logobg, logofade;
    float duration;

    LogoStep step;
};

LogoStateManager::LogoStateManager(RecursiveRunnerGame* game) : StateManager(State::Logo, game) {
    datas = new LogoStateManagerDatas();
}

LogoStateManager::~LogoStateManager() {
    delete datas;
}

void LogoStateManager::setup() {
    Entity logo = datas->logo = theEntityManager.CreateEntity();
    Entity logobg = datas->logobg = theEntityManager.CreateEntity();
    Entity logofade = datas->logofade = theEntityManager.CreateEntity();
    datas->animLogo = theEntityManager.CreateEntity();

    ADD_COMPONENT(logo, Rendering);
    ADD_COMPONENT(logo, Transformation);
    TRANSFORM(logo)->position = theRenderingSystem.cameras[0].worldPosition;
    TRANSFORM(logo)->size = Vector2(PlacementHelper::ScreenHeight * 0.8, PlacementHelper::ScreenHeight * 0.8);
    TRANSFORM(logo)->z = DL_Logo;
    RENDERING(logo)->texture = theRenderingSystem.loadTextureFile("soupe_logo");

    ADD_COMPONENT(logobg, Rendering);
    ADD_COMPONENT(logobg, Transformation);
    TRANSFORM(logobg)->position = theRenderingSystem.cameras[0].worldPosition;
    TRANSFORM(logobg)->size = Vector2(PlacementHelper::ScreenWidth, PlacementHelper::ScreenHeight);
    RENDERING(logobg)->color = Color(0,0,0);
    TRANSFORM(logobg)->z = DL_BehindLogo;

    ADD_COMPONENT(logofade, Rendering);
    ADD_COMPONENT(logofade, Transformation);
    TRANSFORM(logofade)->position = theRenderingSystem.cameras[0].worldPosition;
    TRANSFORM(logofade)->size = Vector2(PlacementHelper::ScreenWidth, PlacementHelper::ScreenHeight);
    RENDERING(logofade)->color = Color(0,0,0);
    TRANSFORM(logofade)->z = 1;

    ADD_COMPONENT(datas->animLogo, Transformation);
    TRANSFORM(datas->animLogo)->size = TRANSFORM(datas->logo)->size * theRenderingSystem.getTextureSize("soupe_logo2_365_331")
        * Vector2(1.0 / theRenderingSystem.getTextureSize("soupe_logo").X, 1.0 / theRenderingSystem.getTextureSize("soupe_logo").Y);
    Vector2 offset = Vector2(-10 / 800.0, 83/869.0) * TRANSFORM(datas->logo)->size;
    TRANSFORM(datas->animLogo)->position = TRANSFORM(datas->logo)->position + offset;
    TRANSFORM(datas->animLogo)->z = DL_LogoAnim;
    ADD_COMPONENT(datas->animLogo, Rendering);
    RENDERING(datas->animLogo)->texture = theRenderingSystem.loadTextureFile("soupe_logo2_365_331");
    RENDERING(datas->animLogo)->hide = true;
    ADD_COMPONENT(datas->animLogo, Sound);
}


///----------------------------------------------------------------------------//
///--------------------- ENTER SECTION ----------------------------------------//
///----------------------------------------------------------------------------//
void LogoStateManager::willEnter(State::Enum) {
}

bool LogoStateManager::transitionCanEnter(State::Enum) {
    return true;
}

#define FADE 0.5
void LogoStateManager::enter(State::Enum) {
    datas->duration = 0;
    RENDERING(datas->logo)->hide = RENDERING(datas->logobg)->hide = RENDERING(datas->logofade)->hide = false;
    // preload sound
    theSoundSystem.loadSoundFile("son_monte.ogg");
    datas->step = LogoStep0;
}


///----------------------------------------------------------------------------//
///--------------------- UPDATE SECTION ---------------------------------------//
///----------------------------------------------------------------------------//
State::Enum LogoStateManager::update(float dt) {
    float& duration = (datas->duration += dt);

    switch (datas->step) {
        case LogoStep0:
            RENDERING(datas->logofade)->color.a = 1 - (duration / FADE);
            if (duration > FADE) {
                duration = 0;
                RENDERING(datas->logofade)->hide = true;
                datas->step = LogoStep1;
            }
            break;
        case LogoStep1:
            if (duration > 0.8) {
                duration = 0;
                RENDERING(datas->animLogo)->hide = false;
                SOUND(datas->animLogo)->sound = theSoundSystem.loadSoundFile("son_monte.ogg");
                datas->step = LogoStep2;
            }
            break;
        case LogoStep2:
            if (duration > 0.05) {
                duration = 0;
                RENDERING(datas->animLogo)->texture = theRenderingSystem.loadTextureFile("soupe_logo3_365_331");
                datas->step = LogoStep3;
            }
            break;
        case LogoStep3:
            if (duration > 0.25) {
                duration = 0;
                RENDERING(datas->animLogo)->texture = theRenderingSystem.loadTextureFile("soupe_logo2_365_331");
                datas->step = LogoStep4;
            }
            break;
        case LogoStep4:
            if (duration > 0.05) {
                duration = 0;
                RENDERING(datas->animLogo)->hide = true;
                datas->step = LogoStep5;
            }
            break;
        case LogoStep5:
            if (duration > 0.6) {
                duration = 0;
                RENDERING(datas->logofade)->hide = false;
                datas->step = LogoStep6;
            }
            break;
        case LogoStep6:
            RENDERING(datas->logofade)->color.a = (duration / FADE);
            if (duration > FADE) {
                duration = 0;
                return State::Menu;
            }
    }
    return State::Logo;
}

void LogoStateManager::backgroundUpdate(float) {

}


///----------------------------------------------------------------------------//
///--------------------- EXIT SECTION -----------------------------------------//
///----------------------------------------------------------------------------//
void LogoStateManager::willExit(State::Enum) {
    theEntityManager.DeleteEntity(datas->logo);
    theEntityManager.DeleteEntity(datas->logobg);
    theEntityManager.DeleteEntity(datas->animLogo);
    theEntityManager.DeleteEntity(datas->logofade);
    theRenderingSystem.unloadAtlas("logo");
}

bool LogoStateManager::transitionCanExit(State::Enum) {
    return true;
}

void LogoStateManager::exit(State::Enum) {

}




