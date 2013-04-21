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
#include "base/StateMachine.h"

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
    LogoStep7,
};

struct LogoTimeBasedStateHandler : public StateHandler<LogoStep> {
    LogoStep self;
    float elapsed, duration;
    std::function<void(void)> onExitC;
    LogoTimeBasedStateHandler(LogoStep pSelf, float pDuration,
        const std::function<void(void)>& pOnExit = [] () {} ) :
            self(pSelf), elapsed(0), duration(pDuration), onExitC(pOnExit) {}
    void setup() {}
    LogoStep update(float dt) {
        elapsed += dt;
        if (elapsed >= duration) {
            return (LogoStep)(self + 1);
        } else {
            return self;
        }
    }
    void onExit(LogoStep) {
        onExitC();
    }
};

class LogoScene : public StateHandler<Scene::Enum> {
    RecursiveRunnerGame* game;
    Entity logo, animLogo, logobg, logofade;
    StateMachine<LogoStep>* logoSM;

public:

    LogoScene(RecursiveRunnerGame* game) : StateHandler<Scene::Enum>() {
        this->game = game;
    }

    void setup() {
        DataFileParser dfp;
        FileBuffer fb = game->gameThreadContext->assetAPI->loadAsset("entities/logo.entity");
        if (!dfp.load(fb)) {
            LOGF("Unable to parse '" << "entities/logo.entity" << "'")
        }
        logo = theEntityManager.CreateEntity("logo", EntityType::Volatile, &dfp);
        logobg = theEntityManager.CreateEntity("logo_bg");
        logofade = theEntityManager.CreateEntity("logo_fade");
        animLogo = theEntityManager.CreateEntity("logo_anim");

        TRANSFORM(logo)->parent = game->cameraEntity;

        ADD_COMPONENT(logobg, Rendering);
        ADD_COMPONENT(logobg, Transformation);
        TRANSFORM(logobg)->position = glm::vec2(0.0);
        TRANSFORM(logobg)->size = glm::vec2(PlacementHelper::ScreenWidth, PlacementHelper::ScreenHeight);
        RENDERING(logobg)->color = Color(0,0,0);
        TRANSFORM(logobg)->z = DL_BehindLogo - TRANSFORM(game->cameraEntity)->z;
        TRANSFORM(logobg)->parent = game->cameraEntity;

        ADD_COMPONENT(logofade, Rendering);
        ADD_COMPONENT(logofade, Transformation);
        TRANSFORM(logofade)->position = glm::vec2(0.0);
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
    #define FADE 0.5f
    void onEnter(Scene::Enum) {
        RENDERING(logo)->show = RENDERING(logobg)->show = RENDERING(logofade)->show = true;
        // preload sound
        theSoundSystem.loadSoundFile("son_monte.ogg");

        // setup state machine
        logoSM = new StateMachine<LogoStep>();
        logoSM->registerState(LogoStep0,
            new LogoTimeBasedStateHandler(LogoStep0, FADE, [this] () {
                RENDERING(logofade)->show = false;
            }), "BlackToLogoFade");
        logoSM->registerState(LogoStep1,
            new LogoTimeBasedStateHandler(LogoStep1, 0.8, [this] () {
                RENDERING(animLogo)->show = true;
                SOUND(animLogo)->sound = theSoundSystem.loadSoundFile("son_monte.ogg");
            }), "WaitBeforeBlink");
        logoSM->registerState(LogoStep2,
            new LogoTimeBasedStateHandler(LogoStep2, 0.05, [this] () {
                RENDERING(animLogo)->texture = theRenderingSystem.loadTextureFile("soupe_logo3_365_331");
            }), "LogoStep2");
        logoSM->registerState(LogoStep3,
            new LogoTimeBasedStateHandler(LogoStep3, 0.25, [this] () {
                RENDERING(animLogo)->texture = theRenderingSystem.loadTextureFile("soupe_logo2_365_331");
            }), "LogoStep3");
        logoSM->registerState(LogoStep4,
            new LogoTimeBasedStateHandler(LogoStep4, 0.05, [this] () {
                RENDERING(animLogo)->show = false;
            }), "LogoStep4");
        logoSM->registerState(LogoStep5,
            new LogoTimeBasedStateHandler(LogoStep5, 0.6, [this] () {
                RENDERING(logofade)->show = true;
            }), "LogoStep5");
        logoSM->registerState(LogoStep6,
            new LogoTimeBasedStateHandler(LogoStep6, FADE),
            "FadeToBlack");
        logoSM->registerState(LogoStep7,
            new LogoTimeBasedStateHandler(LogoStep7, 10),
            "LogoStep7");
        logoSM->setup(LogoStep0);
    }


    ///----------------------------------------------------------------------------//
    ///--------------------- UPDATE SECTION ---------------------------------------//
    ///----------------------------------------------------------------------------//
    Scene::Enum update(float dt) {
        logoSM->update(dt);

        const float elapsed = (static_cast<LogoTimeBasedStateHandler*> (logoSM->getCurrentHandler()))->elapsed;

        switch (logoSM->getCurrentState()) {
            case LogoStep0:
                RENDERING(logofade)->color.a = 1 - elapsed / FADE;
                break;
            case LogoStep6:
                RENDERING(logofade)->color.a = elapsed / FADE;
                break;
            case LogoStep7:
                return Scene::Menu;
            default:
                break;
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
        delete logoSM;
    }
};

namespace Scene {
    StateHandler<Scene::Enum>* CreateLogoSceneHandler(RecursiveRunnerGame* game) {
        return new LogoScene(game);
    }
}
