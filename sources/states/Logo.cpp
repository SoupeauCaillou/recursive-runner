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
#include "base/PlacementHelper.h"

#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/SoundSystem.h"

#include "RecursiveRunnerGame.h"
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

class LogoScene : public StateHandler<Scene::Enum> {
    RecursiveRunnerGame* game;
    Entity logo, animLogo, logobg, logofade;
    float duration;

    LogoStep step;

public:

    LogoScene(RecursiveRunnerGame* game) : StateHandler<Scene::Enum>() {
        this->game = game;
    }

    void setup() {
        logo = theEntityManager.CreateEntity("logo");
        logobg = theEntityManager.CreateEntity("logo_bg");
        logofade = theEntityManager.CreateEntity("logo_fade");
        animLogo = theEntityManager.CreateEntity("logo_anim");

        ADD_COMPONENT(logo, Rendering);
        ADD_COMPONENT(logo, Transformation);
        TRANSFORM(logo)->position = TRANSFORM(game->cameraEntity)->worldPosition;
        TRANSFORM(logo)->size = glm::vec2(PlacementHelper::ScreenHeight * 0.8, PlacementHelper::ScreenHeight * 0.8);
        TRANSFORM(logo)->parent = game->cameraEntity;
        TRANSFORM(logo)->z = DL_Logo - TRANSFORM(game->cameraEntity)->z;
        RENDERING(logo)->texture = theRenderingSystem.loadTextureFile("soupe_logo");

        ADD_COMPONENT(logobg, Rendering);
        ADD_COMPONENT(logobg, Transformation);
        TRANSFORM(logobg)->position = TRANSFORM(game->cameraEntity)->worldPosition;
        TRANSFORM(logobg)->size = glm::vec2(PlacementHelper::ScreenWidth, PlacementHelper::ScreenHeight);
        RENDERING(logobg)->color = Color(0,0,0);
        TRANSFORM(logobg)->z = DL_BehindLogo - TRANSFORM(game->cameraEntity)->z;
        TRANSFORM(logobg)->parent = game->cameraEntity;

        ADD_COMPONENT(logofade, Rendering);
        ADD_COMPONENT(logofade, Transformation);
        TRANSFORM(logofade)->position = TRANSFORM(game->cameraEntity)->worldPosition;
        TRANSFORM(logofade)->size = glm::vec2(PlacementHelper::ScreenWidth, PlacementHelper::ScreenHeight);
        RENDERING(logofade)->color = Color(0,0,0);
        TRANSFORM(logofade)->z = 1 - TRANSFORM(game->cameraEntity)->z;
        TRANSFORM(logofade)->parent = game->cameraEntity;

        ADD_COMPONENT(animLogo, Transformation);
        TRANSFORM(animLogo)->size = TRANSFORM(logo)->size * theRenderingSystem.getTextureSize("soupe_logo2_365_331")
            * glm::vec2(1.0 / theRenderingSystem.getTextureSize("soupe_logo").x, 1.0 / theRenderingSystem.getTextureSize("soupe_logo").y);
        glm::vec2 offset = glm::vec2(-10 / 800.0, 83/869.0) * TRANSFORM(logo)->size;
        TRANSFORM(animLogo)->position = TRANSFORM(logo)->position + offset;
        TRANSFORM(animLogo)->z = DL_LogoAnim - TRANSFORM(game->cameraEntity)->z;
        TRANSFORM(animLogo)->parent = game->cameraEntity;
        ADD_COMPONENT(animLogo, Rendering);
        RENDERING(animLogo)->texture = theRenderingSystem.loadTextureFile("soupe_logo2_365_331");
        RENDERING(animLogo)->show = false;
        ADD_COMPONENT(animLogo, Sound);
    }


    ///----------------------------------------------------------------------------//
    ///--------------------- ENTER SECTION ----------------------------------------//
    ///----------------------------------------------------------------------------//
    #define FADE 0.5
    void onEnter(Scene::Enum) {
        duration = 0;
        RENDERING(logo)->show = RENDERING(logobg)->show = RENDERING(logofade)->show = true;
        // preload sound
        theSoundSystem.loadSoundFile("son_monte.ogg");
        step = LogoStep0;
    }


    ///----------------------------------------------------------------------------//
    ///--------------------- UPDATE SECTION ---------------------------------------//
    ///----------------------------------------------------------------------------//
    Scene::Enum update(float dt) {
        duration += dt;

        switch (step) {
            case LogoStep0:
                RENDERING(logofade)->color.a = 1 - (duration / FADE);
                if (duration > FADE) {
                    duration = 0;
                    RENDERING(logofade)->show = false;
                    step = LogoStep1;
                }
                break;
            case LogoStep1:
                if (duration > 0.8) {
                    duration = 0;
                    RENDERING(animLogo)->show = true;
                    SOUND(animLogo)->sound = theSoundSystem.loadSoundFile("son_monte.ogg");
                    step = LogoStep2;
                }
                break;
            case LogoStep2:
                if (duration > 0.05) {
                    duration = 0;
                    RENDERING(animLogo)->texture = theRenderingSystem.loadTextureFile("soupe_logo3_365_331");
                    step = LogoStep3;
                }
                break;
            case LogoStep3:
                if (duration > 0.25) {
                    duration = 0;
                    RENDERING(animLogo)->texture = theRenderingSystem.loadTextureFile("soupe_logo2_365_331");
                    step = LogoStep4;
                }
                break;
            case LogoStep4:
                if (duration > 0.05) {
                    duration = 0;
                    RENDERING(animLogo)->show = false;
                    step = LogoStep5;
                }
                break;
            case LogoStep5:
                if (duration > 0.6) {
                    duration = 0;
                    RENDERING(logofade)->show = true;
                    step = LogoStep6;
                }
                break;
            case LogoStep6:
                RENDERING(logofade)->color.a = (duration / FADE);
                if (duration > FADE) {
                    duration = 0;
                    return Scene::Menu;
                }
        }
        return Scene::Logo;
    }

    ///----------------------------------------------------------------------------//
    ///--------------------- EXIT SECTION -----------------------------------------//
    ///----------------------------------------------------------------------------//
    void onPreExit(Scene::Enum) {
        theEntityManager.DeleteEntity(logo);
        theEntityManager.DeleteEntity(logobg);
        theEntityManager.DeleteEntity(animLogo);
        theEntityManager.DeleteEntity(logofade);
        theRenderingSystem.unloadAtlas("logo");
    }
};

namespace Scene {
    StateHandler<Scene::Enum>* CreateLogoSceneHandler(RecursiveRunnerGame* game) {
        return new LogoScene(game);
    }
}