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


#include "RecursiveRunnerGame.h"
#include "base/SceneState.h"
#include "Scenes.h"

#include "base/EntityManager.h"
#include "base/TouchInputManager.h"
#include "base/PlacementHelper.h"

#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/SoundSystem.h"
#include "systems/AnchorSystem.h"

#include "util/FaderHelper.h"

class LogoScene : public SceneState<Scene::Enum> {
    RecursiveRunnerGame* game;
    FaderHelper faderHelper;

public:

    LogoScene(RecursiveRunnerGame* game) :
        SceneState<Scene::Enum>("logo", SceneEntityMode::DoNothing, SceneEntityMode::InstantaneousOnExit) {
        this->game = game;
    }

    ///----------------------------------------------------------------------------//
    ///--------------------- ENTER SECTION ----------------------------------------//
    ///----------------------------------------------------------------------------//
    #define FADE 0.7
    void onPreEnter(Scene::Enum f) override {
        SceneState<Scene::Enum>::onPreEnter(f);

        faderHelper.init(game->cameraEntity);
        faderHelper.start(Fading::In, FADE);

        // preload sound
        theSoundSystem.loadSoundFile("sounds/logo_blink.ogg");
    }

    bool updatePreEnter(Scene::Enum f, float dt) override {
        return SceneState<Scene::Enum>::updatePreEnter(f, dt) &&
            faderHelper.update(dt);
    }

    float timeAccum;
    bool soundPlayed;
    void onEnter(Scene::Enum f) override {
        SceneState<Scene::Enum>::onEnter(f);

        timeAccum = 0;
        soundPlayed = false;
    }


    ///----------------------------------------------------------------------------//
    ///--------------------- UPDATE SECTION ---------------------------------------//
    ///----------------------------------------------------------------------------//
    Scene::Enum update(float dt) override {
        Entity animLogo = e(HASH("logo/logo_anim", 0xed78c546));

        if (timeAccum > 0.8 + 0.05 + 0.25 + 0.05) {
            RENDERING(animLogo)->show = false;
            return Scene::Menu;
        } else if (timeAccum > 0.8 + 0.05 + 0.25) {
            RENDERING(animLogo)->texture = theRenderingSystem.loadTextureFile("soupe_logo2_365_331");
        }
        else if (timeAccum > 0.8 + 0.05) {
            if (!soundPlayed) {
                SOUND(animLogo)->sound = theSoundSystem.loadSoundFile("sounds/logo_blink.ogg");
                soundPlayed = true;
            }
            RENDERING(animLogo)->texture = theRenderingSystem.loadTextureFile("soupe_logo3_365_331");
        }
        else if (timeAccum > 0.8) {
            RENDERING(animLogo)->show = true;
        }

        timeAccum += dt;
        return Scene::Logo;
    }

    ///----------------------------------------------------------------------------//
    ///--------------------- EXIT SECTION -----------------------------------------//
    ///----------------------------------------------------------------------------//
    void onPreExit(Scene::Enum f) override{
        SceneState<Scene::Enum>::onPreExit(f);

        faderHelper.start(Fading::OutIn, 3 * FADE);
        faderHelper.registerFadingOutEntity(e(HASH("logo/logo", 0xbfef4b6c)));
        faderHelper.registerFadingOutEntity(e(HASH("logo/logo_bg", 0xf04967d8)));
    }

    bool updatePreExit(Scene::Enum f, float dt) override {
        return SceneState<Scene::Enum>::updatePreExit(f, dt) &&
            faderHelper.update(dt);
    }

    void onExit(Scene::Enum f) override {
        SceneState<Scene::Enum>::onExit(f);

        for (auto p: entities) {
            theEntityManager.DeleteEntity(p.second);
        }
        entities.clear();

        theRenderingSystem.unloadAtlas("logo");

        faderHelper.clearFadingEntities();
    }
};

namespace Scene {
    StateHandler<Scene::Enum>* CreateLogoSceneHandler(RecursiveRunnerGame* game) {
        return new LogoScene(game);
    }
}
