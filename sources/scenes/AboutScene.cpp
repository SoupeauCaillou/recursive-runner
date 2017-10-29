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
#include "base/StateMachine.h"
#include "base/ObjectSerializer.h"
#include "base/EntityManager.h"
#include "base/PlacementHelper.h"
#include "base/TouchInputManager.h"

#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/ButtonSystem.h"
#include "systems/ADSRSystem.h"
#include "systems/TextSystem.h"
#include "systems/SoundSystem.h"
#include "systems/MusicSystem.h"
#include "systems/PlayerSystem.h"
#include "systems/ParticuleSystem.h"
#include "systems/AutoDestroySystem.h"
#include "systems/AnchorSystem.h"

#include "../systems/SessionSystem.h"
#include "../systems/RunnerSystem.h"

#include "api/LocalizeAPI.h"
#include "api/StorageAPI.h"
#include "api/OpenURLAPI.h"
#include "api/CommunicationAPI.h"

#include "util/ScoreStorageProxy.h"

#include "../RecursiveRunnerGame.h"
#include "../Parameters.h"

#include <vector>
#include <mutex>

#if SAC_USE_PROPRIETARY_PLUGINS
#include "api/InAppPurchaseAPI.h"
#endif

namespace Image {
    enum Enum {
        Background = 0,
        Count
    };
}

namespace Button {
    enum Enum {
        Flattr = 0,
        Web,
        Back,
        Wolf,
        Count
    };
}

namespace Text {
    enum Enum {
        SupportUs = 0,
        AboutUs,
        Count
    };
}

class AboutScene : public StateHandler<Scene::Enum> {
    RecursiveRunnerGame* game;

    Entity images[Image::Count];
    Entity buttons[Button::Count];
    Entity texts[Text::Count];

public:
    AboutScene(RecursiveRunnerGame* game) : StateHandler<Scene::Enum>("about"), game(game) {
    }


    void setup(AssetAPI*) override {
        images[Image::Background] = theEntityManager.CreateEntityFromTemplate("menu/about/background");

        texts[Text::SupportUs] = theEntityManager.CreateEntityFromTemplate("menu/about/supportus_text");
        texts[Text::AboutUs] = theEntityManager.CreateEntityFromTemplate("menu/about/aboutus_text");

        buttons[Button::Flattr] = theEntityManager.CreateEntityFromTemplate("menu/about/flattr_button");
        buttons[Button::Web] = theEntityManager.CreateEntityFromTemplate("menu/about/web_button");
        buttons[Button::Back] = theEntityManager.CreateEntityFromTemplate("menu/about/back_button");
        buttons[Button::Wolf] = theEntityManager.CreateEntityFromTemplate("menu/about/wolf");

        ANCHOR(buttons[Button::Back])->parent = game->muteBtn;
        TRANSFORM(buttons[Button::Back])->position.y += game->baseLine + TRANSFORM(game->cameraEntity)->size.y * 0.5;
    }


    ///----------------------------------------------------------------------------//
    ///--------------------- ENTER SECTION ----------------------------------------//
    ///----------------------------------------------------------------------------//
    void onEnter(Scene::Enum) override{
        for (int i=0; i<Text::Count; i++) {
            TEXT(texts[i])->show = true;
        }
        for (int i=0; i<Button::Count; i++) {
            RENDERING(buttons[i])->show =
                BUTTON(buttons[i])->enabled = true;
        }
        for (int i=0; i<Image::Count; i++) {
            RENDERING(images[i])->show = true;
        }
    }

    ///----------------------------------------------------------------------------//
    ///--------------------- UPDATE SECTION ---------------------------------------//
    ///----------------------------------------------------------------------------//

    Scene::Enum update(float) override {
        RENDERING(buttons[Button::Back])->color = BUTTON(buttons[Button::Back])->mouseOver ? Color(HASH("gray", 0xd8a86c30)) : Color();
        RENDERING(buttons[Button::Flattr])->color = BUTTON(buttons[Button::Flattr])->mouseOver ? Color(HASH("gray", 0xd8a86c30)) : Color();
        RENDERING(buttons[Button::Web])->color = BUTTON(buttons[Button::Web])->mouseOver ? Color(HASH("gray", 0xd8a86c30)) : Color();

        if (BUTTON(buttons[Button::Flattr])->clicked) {
            std::string url = game->gameThreadContext->localizeAPI->text("donate_flattr_url");
            game->gameThreadContext->openURLAPI->openURL(url);
        } else if (BUTTON(buttons[Button::Back])->clicked) {
            return Scene::Menu;
        } else if (BUTTON(buttons[Button::Web])->clicked) {
            const std::string url = "http://soupeaucaillou.com";
            game->gameThreadContext->openURLAPI->openURL(url);
        } else if (BUTTON(buttons[Button::Wolf])->clicked) {
            // display version information
            game->gameThreadContext->communicationAPI->show("__version__");
        }

        return Scene::About;
    }


///----------------------------------------------------------------------------//
///--------------------- EXIT SECTION -----------------------------------------//
///----------------------------------------------------------------------------//
    void onPreExit(Scene::Enum ) override {
        for (int i=0; i<Text::Count; i++) {
            TEXT(texts[i])->show = false;
        }
        for (int i=0; i<Button::Count; i++) {
            RENDERING(buttons[i])->show =
                BUTTON(buttons[i])->enabled = false;
        }
        for (int i=0; i<Image::Count; i++) {
            RENDERING(images[i])->show = false;
        }
    }


    void onExit(Scene::Enum) override {

    }
};

namespace Scene {
    StateHandler<Scene::Enum>* CreateAboutSceneHandler(RecursiveRunnerGame* game) {
        return new AboutScene(game);
    }
}
