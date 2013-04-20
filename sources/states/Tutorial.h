
namespace Tutorial {
    enum Enum {
        Title,
        IntroduceHero,
        SmallJump,
        ScorePoints,
        BigJump,
        RunTilTheEdge,
        NewHero,
        MeetYourself,
        AvoidYourself,
        BestScore,
        TheEnd,
        Finished
    };
}

struct TutorialEntities {
    Entity text;
    Entity anim;
};

struct TutorialStep : public StateHandler<Tutorial::Enum> {
    GameScene* gameScene;
    LocalizeAPI* loc;
    SessionComponent* sc;
    TutorialEntities* entities;

    Tutorial::Enum self;
    std::function<bool(void)> enterCondition;
    std::function<void(void)> exitAction;
    std::string textId;
    int runnerPointedByArrow;
    glm::vec2 arrowOffset;

    TutorialStep(Tutorial::Enum pSelf, const std::function<bool(void)>& cond, const std::string& pTextId, int r = -1, const glm::vec2& arr = glm::vec2(0.0f)) {
        self = pSelf;
        enterCondition = cond;
        textId = pTextId;
        runnerPointedByArrow = r;
        arrowOffset = arr;
        exitAction = [] () {};
    }

    TutorialStep(Tutorial::Enum pSelf, const std::function<bool(void)>& cond, const std::function<void(void)>& pExitAction, const std::string& pTextId, int r = -1, const glm::vec2& arr = glm::vec2(0.0f)) {
        self = pSelf;
        enterCondition = cond;
        textId = pTextId;
        runnerPointedByArrow = r;
        arrowOffset = arr;
        exitAction = pExitAction;
    }

    void init(GameScene* pGameScene, LocalizeAPI* pLoc, SessionComponent* pSc, TutorialEntities* pEntities) {
        gameScene = pGameScene; loc = pLoc; sc = pSc; entities = pEntities;
    }

    void setup() {
    }

    // Update game until entering condition is met (e.g : runner is at x = 12)
    virtual bool updatePreEnter(Tutorial::Enum , float dt) {
        gameScene->update(dt);
        return enterCondition();
    }

    // Setup text/arrow and everything when entering the state
    virtual void onEnter(Tutorial::Enum ) {
        LOGV(1, "Entering tutorial state: " << self)
        // change text
        setupText(textId);
        // display arrow if needed
        if (runnerPointedByArrow >= 0) {
            RENDERING(entities->anim)->show = true;
            TRANSFORM(entities->anim)->position = TRANSFORM(sc->runners[runnerPointedByArrow])->worldPosition +
                TRANSFORM(sc->runners[runnerPointedByArrow])->size * arrowOffset;
            pointArrowTo(entities->anim, TRANSFORM(sc->runners[runnerPointedByArrow])->worldPosition);
        } else {
            RENDERING(entities->anim)->show = false;
            ANIMATION(entities->anim)->playbackSpeed = 0;
        }
        // pause animation
        for (unsigned i=0; i<sc->runners.size(); i++) {
            ANIMATION(sc->runners[i])->playbackSpeed = 0;
            PHYSICS(sc->runners[i])->mass = 0;
        }
    }

    // Loop until click occurs
    Tutorial::Enum update(float ) {
        if (theTouchInputManager.hasClicked()) {
            return (Tutorial::Enum)(self + 1);
        } else {
            return self;
        }
    }

    // Prepare intermediate state (blinking text, no arrow)
    virtual void onPreExit(Tutorial::Enum ) {
        LOGV(1, "Will exit tutorial state: " << self)
        // setup blinking text
        TEXT_RENDERING(entities->text)->show = true;
        TEXT_RENDERING(entities->text)->text = ". . .";
        TEXT_RENDERING(entities->text)->blink.offDuration =
        TEXT_RENDERING(entities->text)->blink.onDuration = 0.5;
        // hide arrow
        RENDERING(entities->anim)->show = false;
        ANIMATION(entities->anim)->playbackSpeed = 0;
        // restore anim
        for (unsigned i=0; i<sc->runners.size(); i++) {
            ANIMATION(sc->runners[i])->playbackSpeed = 1.1;
            PHYSICS(sc->runners[i])->mass = 1;
        }
        exitAction();
    }

    void pointArrowTo(Entity arrow, const glm::vec2& target) const {
        glm::vec2 v = glm::normalize(target - TRANSFORM(arrow)->position);
        TRANSFORM(arrow)->rotation = glm::atan2(v.y, v.x);
        ANIMATION(arrow)->accum = ANIMATION(arrow)->frameIndex = 0;
        ANIMATION(arrow)->playbackSpeed = 1.0;
    }

    void setupText(const std::string& textId) {
        TEXT_RENDERING(entities->text)->show = true;
        TEXT_RENDERING(entities->text)->text = loc->text(textId);
        TEXT_RENDERING(entities->text)->blink.offDuration =
            TEXT_RENDERING(entities->text)->blink.onDuration = 0;
    }
};

class TutorialScene : public StateHandler<Scene::Enum> {
    RecursiveRunnerGame* game;
    GameScene gameScene;
    float waitBeforeEnterExit;
    TutorialEntities entities;
    Entity titleGroup, title, hideText;
    bool waitingClick;
    StateMachine<Tutorial::Enum> tutorialStateMachine;

public:

    TutorialScene(RecursiveRunnerGame* game) : gameScene(game) {
       this->game = game;
    }

    void setup() {
        // setup game
        gameScene.setup();
        // setup tutorial
        titleGroup  = theEntityManager.CreateEntity("tuto_title_group");
        ADD_COMPONENT(titleGroup, Transformation);
        TRANSFORM(titleGroup)->z = 0.7;
        TRANSFORM(titleGroup)->rotation = 0.036;
        TRANSFORM(titleGroup)->parent = game->cameraEntity;
        ADD_COMPONENT(titleGroup, ADSR);
        ADSR(titleGroup)->idleValue = (PlacementHelper::ScreenHeight + PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("titre")).y) * 0.6;
        ADSR(titleGroup)->sustainValue = (PlacementHelper::ScreenHeight - PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("titre")).y) * 0.5
            + PlacementHelper::GimpHeightToScreen(35);
        ADSR(titleGroup)->attackValue = ADSR(titleGroup)->sustainValue - PlacementHelper::GimpHeightToScreen(5);
        ADSR(titleGroup)->attackTiming = 1;
        ADSR(titleGroup)->decayTiming = 0.1;
        ADSR(titleGroup)->releaseTiming = 0.3;
        TRANSFORM(titleGroup)->position = glm::vec2(0, ADSR(titleGroup)->idleValue);

        title = theEntityManager.CreateEntity("tuto_title");
        ADD_COMPONENT(title, Transformation);
        TRANSFORM(title)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("taptostart"));
        TRANSFORM(title)->parent = titleGroup;
        TRANSFORM(title)->position = glm::vec2(0.0f);
        TRANSFORM(title)->z = 0.15;
        ADD_COMPONENT(title, Rendering);
        RENDERING(title)->texture = theRenderingSystem.loadTextureFile("taptostart");

        hideText = theEntityManager.CreateEntity("tuto_hide_text");
        ADD_COMPONENT(hideText, Transformation);
        TRANSFORM(hideText)->size = PlacementHelper::GimpSizeToScreen(glm::vec2(776, 102));
        TRANSFORM(hideText)->parent = title;
        TRANSFORM(hideText)->position = (glm::vec2(-0.5, 0.5) + glm::vec2(41, -72) / theRenderingSystem.getTextureSize("titre")) *
            TRANSFORM(title)->size + TRANSFORM(hideText)->size * glm::vec2(0.5, -0.5);
        TRANSFORM(hideText)->z = 0.01;
        ADD_COMPONENT(hideText, Rendering);
        RENDERING(hideText)->color = Color(130.0/255, 116.0/255, 117.0/255);

        entities.text = theEntityManager.CreateEntity("tuto_text");
        ADD_COMPONENT(entities.text, Transformation);
        TRANSFORM(entities.text)->size = TRANSFORM(hideText)->size;
        TRANSFORM(entities.text)->parent = hideText;
        TRANSFORM(entities.text)->z = 0.02;
        TRANSFORM(entities.text)->rotation = 0.004;
        ADD_COMPONENT(entities.text, TextRendering);
        TEXT_RENDERING(entities.text)->charHeight = PlacementHelper::GimpHeightToScreen(50);
        TEXT_RENDERING(entities.text)->show = false;
        TEXT_RENDERING(entities.text)->color = Color(40.0 / 255, 32.0/255, 30.0/255, 0.8);
        TEXT_RENDERING(entities.text)->positioning = TextRenderingComponent::CENTER;
        TEXT_RENDERING(entities.text)->flags |= TextRenderingComponent::AdjustHeightToFillWidthBit;

        entities.anim = theEntityManager.CreateEntity("tuto_fleche");
        ADD_COMPONENT(entities.anim, Transformation);
        TRANSFORM(entities.anim)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("fleche"));
        TRANSFORM(entities.anim)->z = 0.9;
        ADD_COMPONENT(entities.anim, Rendering);
        RENDERING(entities.anim)->texture = theRenderingSystem.loadTextureFile("fleche");
        RENDERING(entities.anim)->show = false;
        ADD_COMPONENT(entities.anim, Animation);
        ANIMATION(entities.anim)->name = "arrow_tuto";
        theAnimationSystem.loadAnim(game->gameThreadContext->assetAPI, "arrow_tuto", "arrow_tuto");
    }


    ///----------------------------------------------------------------------------//
    ///--------------------- ENTER SECTION ----------------------------------------//
    ///----------------------------------------------------------------------------//
    void onPreEnter(Scene::Enum) {
        // TODO init state machine

        bool isMuted = theMusicSystem.isMuted();
        theMusicSystem.toggleMute(true);
        gameScene.onPreEnter(Scene::Tutorial);
        theMusicSystem.toggleMute(isMuted);

        waitBeforeEnterExit = TimeUtil::GetTime();
        RENDERING(game->scorePanel)->show = TEXT_RENDERING(game->scoreText)->show = false;

        // hack lights/links
        SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
        std::for_each(session->coins.begin(), session->coins.end(), deleteEntityFunctor);
        session->coins.clear();
        std::for_each(session->links.begin(), session->links.end(), deleteEntityFunctor);
        session->links.clear();
        std::for_each(session->sparkling.begin(), session->sparkling.end(), deleteEntityFunctor);
        session->sparkling.clear();

        PlacementHelper::ScreenWidth = 60;
        PlacementHelper::GimpWidth = 3840;
        glm::vec2 c[] = {
            PlacementHelper::GimpPositionToScreen(glm::vec2(184, 568)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(316, 448)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(432, 612)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(844, 600)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(910, 450)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(1046, 630)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(1244, 564)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(1340, 508)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(1568, 460)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(1820, 450)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(1872, 580)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(2096, 464)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(2380, 584)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(2540, 472)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(2692, 572)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(2916, 500)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(3044, 568)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(3560, 492)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(3684, 566)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(3776, 470))
        };
        std::vector<glm::vec2> coords;
        coords.resize(20);
        std::copy(c, &c[20], coords.begin());
        RecursiveRunnerGame::createCoins(coords, session, true);

        PlacementHelper::ScreenWidth = 20;
        PlacementHelper::GimpWidth = 1280;
        TEXT_RENDERING(entities.text)->text = game->gameThreadContext->localizeAPI->text("how_to_play");
        TEXT_RENDERING(entities.text)->show = false;

        BUTTON(game->muteBtn)->enabled = false;

        // Declare tutorial steps
        // 1. Title
        tutorialStateMachine.registerState(
            Tutorial::Title,
            new TutorialStep(
                Tutorial::Title,
                [] () { return true; },
                "how_to_play"),
            "Title");
        // 2. Hero
        tutorialStateMachine.registerState(
            Tutorial::IntroduceHero,
            new TutorialStep(
                Tutorial::IntroduceHero,
                [session] () { return (TRANSFORM(session->currentRunner)->position.x >= -23); },
                [] () {},
                "this_is_you",
                0,
                glm::vec2(0.5, 1)),
            "IntroduceHero");
        // 3. Small Jump
        tutorialStateMachine.registerState(
            Tutorial::SmallJump,
            new TutorialStep(
                Tutorial::SmallJump,
                [session] () { return (TRANSFORM(session->currentRunner)->position.x >= -15); },
                [session] () {
                    RunnerComponent* rc = RUNNER(session->currentRunner);
                    rc->jumpTimes.push_back(rc->elapsed);
                    rc->jumpDurations.push_back(0.06);
                },
                "tap_jump"),
            "SmallJump");
        // 4. ScorePoints
        tutorialStateMachine.registerState(
            Tutorial::ScorePoints,
            new TutorialStep(
                Tutorial::ScorePoints,
                [session] () { return (PHYSICS(session->currentRunner)->linearVelocity.y < 0); },
                "turn_on"),
            "ScorePoints");
        // 5. BigJump
        tutorialStateMachine.registerState(
            Tutorial::BigJump,
            new TutorialStep(
                Tutorial::BigJump,
                [session] () { return (TRANSFORM(session->currentRunner)->position.x >= 0); },
                [session] () {
                    RunnerComponent* rc = RUNNER(session->currentRunner);
                    rc->jumpTimes.push_back(rc->elapsed);
                    rc->jumpDurations.push_back(RunnerSystem::MaxJumpDuration);
                },
                "longer_press"),
            "BigJump");
        // 6. RunTilTheEdge
        tutorialStateMachine.registerState(
            Tutorial::RunTilTheEdge,
            new TutorialStep(
                Tutorial::RunTilTheEdge,
                [session] () { return (TRANSFORM(session->currentRunner)->position.x >= 29); },
                "run_edge"),
            "RunTilTheEdge");
        // 7. NewHero
        tutorialStateMachine.registerState(
            Tutorial::NewHero,
            new TutorialStep(
                Tutorial::NewHero,
                [session] () { return session->runners.size() == 2 && (TRANSFORM(session->runners[1])->position.x <= 29); },
                "new_hero",
                1,
                glm::vec2(-0.5, 1)),
            "NewHero");
        // 8. MeetYourself
        tutorialStateMachine.registerState(
            Tutorial::MeetYourself,
            new TutorialStep(
                Tutorial::MeetYourself,
                [session] () {
                    return (glm::distance(
                        TRANSFORM(session->runners[0])->position,
                        TRANSFORM(session->runners[1])->position) < theRenderingSystem.screenW * 0.7);
                },
                "previous_hero_replay",
                0,
                glm::vec2(0.5, 1)),
            "MeetYourself");
        // 9. AvoidYourself
        tutorialStateMachine.registerState(
            Tutorial::AvoidYourself,
            new TutorialStep(
                Tutorial::AvoidYourself,
                [session] () {
                    return (glm::distance(
                        TRANSFORM(session->runners[0])->position,
                        TRANSFORM(session->runners[1])->position) < 4);
                },
                [session] () {
                    RunnerComponent* rc = RUNNER(session->currentRunner);
                    rc->jumpTimes.push_back(rc->elapsed);
                    rc->jumpDurations.push_back(RunnerSystem::MaxJumpDuration * 0.8);
                },
                "avoid_him"),
            "AvoidYourself");
        // 10. BestScore
        tutorialStateMachine.registerState(
            Tutorial::BestScore,
            new TutorialStep(
                Tutorial::BestScore,
                [session] () { return (TRANSFORM(session->currentRunner)->position.x < -15); },
                "ten_heroes"),
            "BestScore");
        // 11. TheEnd
        tutorialStateMachine.registerState(
            Tutorial::TheEnd,
            new TutorialStep(
                Tutorial::TheEnd,
                [session] () { return (TRANSFORM(session->currentRunner)->position.x < -29); },
                "good_luck"),
            "TheEnd");
        // 12. Finished
        tutorialStateMachine.registerState(
            Tutorial::Finished,
            new TutorialStep(
                Tutorial::TheEnd, // <- not a c/p error
                [session] () { return true; },
                ""),
            "Finished");

        auto hdl = tutorialStateMachine.getHandlers();
        for(auto it = hdl.begin(); it!=hdl.end(); ++it) {
            (static_cast<TutorialStep*> (it->second))->init(&gameScene, game->gameThreadContext->localizeAPI, session, &entities);
        }
    }

    bool updatePreEnter(Scene::Enum from, float dt) {
        bool gameCanEnter = gameScene.updatePreEnter(from, dt);

        if (TimeUtil::GetTime() - waitBeforeEnterExit < ADSR(titleGroup)->attackTiming) {
            return false;
        }
        ADSR(titleGroup)->active = true;
        RENDERING(title)->show = true;

        ADSRComponent* adsr = ADSR(titleGroup);
        TRANSFORM(titleGroup)->position.y = adsr->value;
        RENDERING(game->muteBtn)->color.a = 1. - (adsr->value - adsr->idleValue) / (adsr->sustainValue - adsr->idleValue);

        return (adsr->value == adsr->sustainValue) && gameCanEnter;
    }

    void onEnter(Scene::Enum) {
        SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
        session->userInputEnabled = false;
        TEXT_RENDERING(entities.text)->show = true;

        gameScene.onEnter(Scene::Tutorial);
        waitingClick = true;

        tutorialStateMachine.setup(Tutorial::Title);
        tutorialStateMachine.reEnterCurrentState();

        RENDERING(game->muteBtn)->show = RENDERING(game->scorePanel)->show = TEXT_RENDERING(game->scoreText)->show = false;
    }


    ///----------------------------------------------------------------------------//
    ///--------------------- UPDATE SECTION ---------------------------------------//
    ///----------------------------------------------------------------------------//

    Scene::Enum update(float dt) {
        tutorialStateMachine.update(dt);
        if (tutorialStateMachine.getCurrentState() == Tutorial::Finished)
            return Scene::Menu;
        else
            return Scene::Tutorial;
    }

    ///----------------------------------------------------------------------------//
    ///--------------------- EXIT SECTION -----------------------------------------//
    ///----------------------------------------------------------------------------//
    void onPreExit(Scene::Enum to) {
        SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
        session->userInputEnabled = false;
        RENDERING(hideText)->show = false;
        TEXT_RENDERING(entities.text)->show = false;
        gameScene.onPreExit(to);


        RENDERING(game->muteBtn)->show = RENDERING(game->scorePanel)->show = TEXT_RENDERING(game->scoreText)->show = true;
    }

    bool updatePreExit(Scene::Enum to, float dt) {

        ADSRComponent* adsr = ADSR(titleGroup);
        adsr->active = false;

        TRANSFORM(titleGroup)->position.y = adsr->value;

        RENDERING(game->muteBtn)->color.a = (adsr->sustainValue - adsr->value) / (adsr->sustainValue - adsr->idleValue);

        return gameScene.updatePreExit(to, dt);
    }

    void onExit(Scene::Enum to) {
        RENDERING(title)->show = false;

        TEXT_RENDERING(entities.text)->show = false;
        RENDERING(entities.anim)->show = false;
        gameScene.onExit(to);
        BUTTON(game->muteBtn)->enabled = true;

        auto hdl = tutorialStateMachine.getHandlers();
        for(auto it = hdl.begin(); it!=hdl.end(); ++it) {
            delete it->second;
        }
    }
};