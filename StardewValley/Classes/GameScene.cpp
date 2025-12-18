#include "GameScene.h"
#include "GameClock.h"
#include "Wallet.h"
#include "HudLayer.h"
#include "CropSystem.h"
// #include "Basket.h" // Ignore Shop related includes
// #include "ShopLayer.h" // Ignore Shop related includes

USING_NS_CC;

HudLayer* GameScene::sHud = nullptr;
bool GameScene::sDebugMode = false;

// Static member initialization for BackgroundLayer persistence
std::unordered_map<int, BackgroundLayer::ObstacleSaveData> BackgroundLayer::sSavedObstacles;
bool BackgroundLayer::sObstaclesInitialized = false;
int BackgroundLayer::sLastObstacleSeason = -1;

BackgroundLayer* BackgroundLayer::create(BackgroundType type)
{
    BackgroundLayer* ret = new (std::nothrow) BackgroundLayer();
    if (ret && ret->initWithType(type))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool BackgroundLayer::initWithType(BackgroundType type)
{
    if (!Layer::init())
    {
        return false;
    }

    _type = type;

    _map = nullptr;
    _groundLayer = nullptr;
    _player = nullptr;
    _zoom = 2.0f;
    _facingDebug = nullptr;
    _poolDebug = nullptr;
    _hasHomeRect = false;
    _hasHomeExitDoor = false;
    _hasBedRect = false;
    _hasPoolRect = false;
    _exitedHomeDoor = false;
    _enteredHome = false;
    _hasRightExit = false;
    _exitedRight = false;
    _hasBoundary = false;
    _isDebugMode = false;
    _canEnterHomeDoor = false;
    _canExitHomeDoor = false;

    _sleepDialogActive = false;
    _isSleeping = false;

    _backgroundNode = nullptr;
    _seasonOverlay = nullptr;
    _sleepOverlay = nullptr;
    _fishingOverlay = nullptr;
    _fishingLabel = nullptr;
    _fishingGame = nullptr;
    _isFishing = false;
    _fishBite = false;
    _fishingElapsed = 0.0f;
    _biteTime = 0.0f;
    _biteWindow = 0.0f;
    // Force update on first frame
    _lastSeason = (GameClock::Season)-1;

    if (_type == BackgroundType::Farm)
    {
        auto map = TMXTiledMap::create("map/outdoors_spring.tmx");
        if (map)
        {
            map->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);
            map->setPosition(Vec2::ZERO);
            addChild(map, 0);

            _map = map;
            _backgroundNode = map;

            // Create season overlay
            Size mapSizeTiles = map->getMapSize();
            Size tileSize = map->getTileSize();
            float mapWidth = mapSizeTiles.width * tileSize.width;
            float mapHeight = mapSizeTiles.height * tileSize.height;
            
            _seasonOverlay = LayerColor::create(Color4B(0, 0, 0, 0), mapWidth, mapHeight);
            if (_seasonOverlay)
            {
                addChild(_seasonOverlay, 2); // Above player (Z=1)
            }

            _groundLayer = _map->getLayer("ground");
            if (!_groundLayer)
            {
                if (_map->getChildrenCount() > 0)
                {
                    _groundLayer = dynamic_cast<TMXLayer*>(_map->getChildren().at(0));
                }
            }

            auto startGroup = _map->getObjectGroup("start");
            if (startGroup)
            {
                const auto& startObjs = startGroup->getObjects();
                if (!startObjs.empty())
                {
                    const auto& dict = startObjs.front().asValueMap();
                    float sx = dict.at("x").asFloat();
                    float sy = dict.at("y").asFloat();
                    float sw = dict.at("width").asFloat();
                    float sh = dict.at("height").asFloat();
                    GameScene::sFarmStartPos = Vec2(sx + sw * 0.5f, sy + sh * 0.5f);
                    GameScene::sHasFarmStartPos = true;
                }
            }

            auto player = Player::create("player.png", tileSize.height);
            if (!player)
            {
                player = Player::create("HelloWorld.png", tileSize.height);
            }
            if (player)
            {
                Vec2 spawnPos(mapWidth * 0.5f, mapHeight * 0.5f);
                if (GameScene::sSpawnAtFarmStart && GameScene::sHasFarmStartPos)
                {
                    spawnPos = GameScene::sFarmStartPos;
                }
                else if (GameScene::sHasLastFarmPlayerPos)
                {
                    spawnPos = GameScene::sLastFarmPlayerPos;
                }

                player->setPosition(spawnPos);
                addChild(player, 1);

                _player = player;

                _facingDebug = DrawNode::create();
                _map->addChild(_facingDebug, 100);

                GameScene::sSpawnAtFarmStart = false;

                auto follow = Follow::create(_player);
                this->runAction(follow);

                setScale(_zoom);
                scheduleUpdate();

                auto listener = EventListenerKeyboard::create();
                listener->onKeyPressed = CC_CALLBACK_2(BackgroundLayer::onKeyPressed, this);
        listener->onKeyReleased = CC_CALLBACK_2(BackgroundLayer::onKeyReleased, this);
        _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

        auto mouseListener = EventListenerMouse::create();
        mouseListener->onMouseDown = CC_CALLBACK_1(BackgroundLayer::onMouseDown, this);
        _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);

        _boundaryLeftRect = Rect(tileSize.width, 0.0f, tileSize.width, mapHeight);
        _boundaryRightRect = Rect(mapWidth - tileSize.width * 7.0f, 0.0f, tileSize.width, mapHeight);
                _boundaryBottomRect = Rect(0.0f, tileSize.height, mapWidth, tileSize.height);
                _boundaryTopRect = Rect(0.0f, mapHeight - tileSize.height * 15.0f, mapWidth, tileSize.height);
                _hasBoundary = true;

                auto homeGroup = _map->getObjectGroup("home");
                if (homeGroup)
                {
                    const auto& objs = homeGroup->getObjects();
                    if (!objs.empty())
                    {
                        const auto& dict = objs.front().asValueMap();
                        float hx = dict.at("x").asFloat();
                        float hy = dict.at("y").asFloat();
                        float hw = dict.at("width").asFloat();
                        float hh = dict.at("height").asFloat();
                        _homeRect = Rect(hx, hy, hw, hh);
                        _hasHomeRect = true;

                        float exitW = tileSize.width * 2.0f;
                        float exitH = tileSize.height * 6.0f;
                        float exitX = _boundaryRightRect.getMinX() - exitW;
                        float exitY = (mapHeight - exitH) * 0.5f - tileSize.height * 7.0f;
                        _rightExitRect = Rect(exitX, exitY, exitW, exitH);
                        _hasRightExit = true;
                    }
                }

                auto poolGroup = _map->getObjectGroup("pool");
                if (poolGroup)
                {
                    const auto& objs = poolGroup->getObjects();
                    cocos2d::log("Pool object group found, object count = %d", static_cast<int>(objs.size()));
                    _poolRects.clear();
                    for (const auto& obj : objs)
                    {
                        const auto& dict = obj.asValueMap();
                        float px = dict.at("x").asFloat();
                        float pyTop = dict.at("y").asFloat();

                        float pw = 0.0f;
                        float ph = 0.0f;

                        auto itW = dict.find("width");
                        if (itW != dict.end())
                        {
                            pw = itW->second.asFloat();
                        }
                        auto itH = dict.find("height");
                        if (itH != dict.end())
                        {
                            ph = itH->second.asFloat();
                        }

                        if (pw <= 0.0f || ph <= 0.0f)
                        {
                            cocos2d::log("Pool object skipped: x=%f y=%f w=%f h=%f", px, pyTop, pw, ph);
                            continue;
                        }

                        float py = pyTop;
                        Rect r(px, py, pw, ph);
                        _poolRects.push_back(r);
                        cocos2d::log("Pool rect added: x=%f y=%f w=%f h=%f", r.origin.x, r.origin.y, r.size.width, r.size.height);
                    }
                    _hasPoolRect = !_poolRects.empty();
                    cocos2d::log("Total pool rects stored = %d", static_cast<int>(_poolRects.size()));
                }
                else
                {
                    cocos2d::log("Pool object group not found in TMX map.");
                }

                auto doorGroup = _map->getObjectGroup("door");
                if (doorGroup)
                {
                    const auto& objs = doorGroup->getObjects();
                    if (!objs.empty())
                    {
                        const auto& dict = objs.front().asValueMap();
                        float dx = dict.at("x").asFloat();
                        float dy = dict.at("y").asFloat();
                        float dw = dict.at("width").asFloat();
                        float dh = dict.at("height").asFloat();
                        _homeDoorRect = Rect(dx, dy, dw, dh);
                        float extendDown = tileSize.height * 0.3f;
                        _homeDoorTunnelRect = Rect(dx, dy - extendDown, dw, dh + extendDown);
                        if (!_hasHomeRect)
                        {
                            _homeRect = _homeDoorRect;
                            _hasHomeRect = true;
                        }
                    }
                }
        
        CropSystem::getInstance()->init(_map, GameScene::sClock, GameScene::sWallet, GameScene::sInventory);
        // Explicitly set map (init does it, but redundant call is safe)
        CropSystem::getInstance()->setMap(_map);

        initObstacles(); // Init obstacles for Farm type
        
        // Initial update for Farm
        updateSeasonFilter();

        return true;
    }
            else
            {
                 cocos2d::log("Player creation failed completely.");
                 // Fallback to allow map to show at least? 
                 // But if we return false, map won't show.
                 // Let's return true but without player control?
                 // No, that confuses user.
                 // Let's create a placeholder player
                 auto fallbackPlayer = Player::create("HelloWorld.png", tileSize.height);
                 if (fallbackPlayer)
                 {
                     _player = fallbackPlayer;
                     float mapWidth = mapSizeTiles.width * tileSize.width;
                     float mapHeight = mapSizeTiles.height * tileSize.height;
                     _player->setPosition(Vec2(mapWidth * 0.5f, mapHeight * 0.5f));
                     addChild(_player, 1);
                     _facingDebug = DrawNode::create();
                     _map->addChild(_facingDebug, 100);
                     
                     setScale(_zoom);
                     scheduleUpdate();

                     auto listener = EventListenerKeyboard::create();
                     listener->onKeyPressed = CC_CALLBACK_2(BackgroundLayer::onKeyPressed, this);
                     listener->onKeyReleased = CC_CALLBACK_2(BackgroundLayer::onKeyReleased, this);
                     _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
                     
                     updateSeasonFilter();

                     return true;
                 }
            }
        }
    }

    if (_type == BackgroundType::Home)
    {
        auto map = TMXTiledMap::create("map/home.tmx");
        if (map)
        {
            Size mapSizeTiles = map->getMapSize();
            Size tileSize = map->getTileSize();
            float mapWidth = mapSizeTiles.width * tileSize.width;
            float mapHeight = mapSizeTiles.height * tileSize.height;

            auto visibleSize = Director::getInstance()->getVisibleSize();
            Vec2 origin = Director::getInstance()->getVisibleOrigin();
            float offsetX = origin.x + (visibleSize.width - mapWidth) * 0.5f;
            float offsetY = origin.y + (visibleSize.height - mapHeight) * 0.5f;

            map->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);
            map->setPosition(Vec2::ZERO);
            addChild(map, 0);

            setPosition(Vec2(offsetX, offsetY));

            _map = map;
            _backgroundNode = map;

            _seasonOverlay = LayerColor::create(Color4B(0, 0, 0, 0), mapWidth, mapHeight);
            if (_seasonOverlay)
            {
                _map->addChild(_seasonOverlay, 2);
            }

            _groundLayer = _map->getLayer("floor");
            if (!_groundLayer)
            {
                if (_map->getChildrenCount() > 0)
                {
                    _groundLayer = dynamic_cast<TMXLayer*>(_map->getChildren().at(0));
                }
            }

            auto doorGroup = _map->getObjectGroup("door");
            if (doorGroup)
            {
                const auto& objs = doorGroup->getObjects();
                if (!objs.empty())
                {
                    const auto& dict = objs.front().asValueMap();
                    float dx = dict.at("x").asFloat();
                    float dy = dict.at("y").asFloat();
                    float dw = dict.at("width").asFloat();
                    float dh = dict.at("height").asFloat();
                    _homeExitDoorRect = Rect(dx, dy, dw, dh);
                    _hasHomeExitDoor = true;
                }
            }

            auto player = Player::create("player.png", tileSize.height);
            if (!player)
            {
                player = Player::create("HelloWorld.png", tileSize.height);
            }
            if (player)
            {
                Vec2 spawnPos(mapWidth * 0.5f, mapHeight * 0.5f);
                auto bedGroup = _map->getObjectGroup("bed");
                if (bedGroup)
                {
                    const auto& objs = bedGroup->getObjects();
                    if (!objs.empty())
                    {
                        const auto& dict = objs.front().asValueMap();
                        float bx = dict.at("x").asFloat();
                        float by = dict.at("y").asFloat();
                        float bw = dict.at("width").asFloat();
                        float bh = dict.at("height").asFloat();
                        _bedRect = Rect(bx, by, bw, bh);
                        _hasBedRect = true;
                        if (GameScene::sStartAtHomeBed)
                        {
                            spawnPos = Vec2(bx + bw * 0.5f, by + bh * 0.5f);
                        }
                    }
                }
                player->setPosition(spawnPos);
                addChild(player, 1);

                _player = player;

                _facingDebug = DrawNode::create();
                _map->addChild(_facingDebug, 100);

                scheduleUpdate();

                auto listener = EventListenerKeyboard::create();
                listener->onKeyPressed = CC_CALLBACK_2(BackgroundLayer::onKeyPressed, this);
                listener->onKeyReleased = CC_CALLBACK_2(BackgroundLayer::onKeyReleased, this);
                _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

                auto mouseListener = EventListenerMouse::create();
                mouseListener->onMouseDown = CC_CALLBACK_1(BackgroundLayer::onMouseDown, this);
                _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);

                _boundaryLeftRect = Rect(tileSize.width, 0.0f, tileSize.width, mapHeight);
                _boundaryRightRect = Rect(mapWidth - tileSize.width * 2.0f, 0.0f, tileSize.width, mapHeight);
                _boundaryBottomRect = Rect(0.0f, tileSize.height, mapWidth, tileSize.height);
                float topLimitY = mapHeight - tileSize.height * 5.0f;
                _boundaryTopRect = Rect(0.0f, topLimitY, mapWidth, mapHeight - topLimitY);
                _hasBoundary = true;

                updateSeasonFilter();

                return true;
            }
        }
    }

    std::string imageFile;

    switch (_type)
    {
    case BackgroundType::Home:
        imageFile = "res/home_bg.png";
        break;
    case BackgroundType::Farm:
        imageFile = "res/farm_bg.png";
        break;
    case BackgroundType::Path:
        imageFile = "res/path_bg.png";
        break;
    case BackgroundType::Town:
        imageFile = "res/town_bg.png";
        break;
    case BackgroundType::Shop:
        imageFile = "res/shop_bg.png";
        break;
    default:
        return false;
    }

    auto sprite = Sprite::create(imageFile);
    if (!sprite)
    {
        return false;
    }
    
    _backgroundNode = sprite;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    Size targetSize = visibleSize;
    Size imageSize = sprite->getContentSize();
    float scaleX = targetSize.width / imageSize.width;
    float scaleY = targetSize.height / imageSize.height;
    float scale = std::min(scaleX, scaleY);
    sprite->setScale(scale);

    sprite->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    sprite->setPosition(Vec2(visibleSize.width / 2 + origin.x, visibleSize.height / 2 + origin.y));

    addChild(sprite, 0);

    // Create season overlay for static backgrounds
    // It should cover the whole screen or the sprite?
    // Let's cover the screen.
    _seasonOverlay = LayerColor::create(Color4B(0, 0, 0, 0));
    if (_seasonOverlay)
    {
        addChild(_seasonOverlay, 2);
    }

    // Initial update
    updateSeasonFilter();

    return true;
}

void BackgroundLayer::updateSeasonFilter()
{
    if (!GameScene::sClock) return;

    auto season = GameScene::sClock->getSeason();

    // Check for season change to spawn new obstacles (Dynamic update)
    if (_type == BackgroundType::Farm)
    {
        int currentSeasonInt = (int)season;
        if (sLastObstacleSeason != -1 && sLastObstacleSeason != currentSeasonInt)
        {
            // Season changed! Spawn more obstacles
            if (_map)
            {
                Size mapSize = _map->getMapSize();
                int newObstacleCount = (mapSize.width * mapSize.height) / 20;
                spawnObstacles(newObstacleCount);
            }
            sLastObstacleSeason = currentSeasonInt;
        }
    }

    Color3B tintColor = Color3B::WHITE;
    Color4B overlayColor = Color4B(0, 0, 0, 0);
    BlendFunc overlayBlend = BlendFunc::ALPHA_NON_PREMULTIPLIED;

    switch (season)
    {
    case GameClock::Season::Spring:
        // Default
        tintColor = Color3B(255, 255, 255);
        overlayColor = Color4B(0, 0, 0, 0);
        break;
    case GameClock::Season::Summer:
        // Brighter / Sunny
        tintColor = Color3B(255, 255, 255);
        // Additive yellow/white for brightness
        overlayColor = Color4B(255, 255, 200, 40); 
        overlayBlend = BlendFunc::ADDITIVE;
        break;
    case GameClock::Season::Fall:
        // Brownish / Autumn / Deep
        // Darker and deeper brown
        tintColor = Color3B(200, 150, 110);
        overlayColor = Color4B(0, 0, 0, 0);
        break;
    case GameClock::Season::Winter:
        // Gray / White / Cold / Dim
        // Lower brightness for "dim" feel
        tintColor = Color3B(180, 180, 190);
        // Normal blending (not additive) to make it look like a white fog/snow cover
        // This will reduce contrast and make it look "whiter" but not "brighter"
        overlayColor = Color4B(220, 230, 240, 50);
        overlayBlend = BlendFunc::ALPHA_NON_PREMULTIPLIED;
        break;
    }

    if (_backgroundNode)
    {
        _backgroundNode->setColor(tintColor);
    }

    if (_seasonOverlay)
    {
        _seasonOverlay->setColor(Color3B(overlayColor));
        _seasonOverlay->setOpacity(overlayColor.a);
        _seasonOverlay->setBlendFunc(overlayBlend);
    }
    
    _lastSeason = season;
}

void BackgroundLayer::showSleepDialog()
{
    if (_sleepDialogActive || _isSleeping) return;
    _sleepDialogActive = true;
    auto director = Director::getInstance();
    Size visibleSize = director->getVisibleSize();
    Vec2 origin = director->getVisibleOrigin();
    if (!_sleepOverlay)
    {
        _sleepOverlay = LayerColor::create(Color4B(0, 0, 0, 160));
        _sleepOverlay->setContentSize(visibleSize);
        _sleepOverlay->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);
        Vec2 layerWorldPos = this->getPosition();
        _sleepOverlay->setPosition(origin - layerWorldPos);
        auto label = Label::createWithSystemFont("Do you want to sleep? press Y/N", "Arial", 32.0f);
        if (label)
        {
            label->setPosition(Vec2(visibleSize.width * 0.5f, visibleSize.height * 0.5f));
            _sleepOverlay->addChild(label);
        }
        addChild(_sleepOverlay, 5000);
    }
    else
    {
        _sleepOverlay->setVisible(true);
        _sleepOverlay->setOpacity(160);
    }
}

void BackgroundLayer::beginSleep()
{
    if (_isSleeping) return;
    _isSleeping = true;
    if (_sleepOverlay)
    {
        _sleepOverlay->setVisible(true);
        _sleepOverlay->stopAllActions();
        _sleepOverlay->setOpacity(160);
        auto fadeIn = FadeTo::create(0.7f, 255);
        auto apply = CallFunc::create([this]() {
            if (GameScene::sClock)
            {
                GameScene::sClock->setHour(6);
                GameScene::sClock->setMinute(0);
                GameScene::sClock->addDay(1);
            }
            CropSystem::getInstance()->updateDailyGrowth();
            if (_player && _hasBedRect)
            {
                Vec2 pos(_bedRect.getMidX(), _bedRect.getMidY());
                _player->setPosition(pos);
            }
        });
        auto wait = DelayTime::create(0.7f);
        auto fadeOut = FadeTo::create(0.7f, 0);
        auto finish = CallFunc::create([this]() {
            if (_sleepOverlay)
            {
                _sleepOverlay->setVisible(false);
            }
            _isSleeping = false;
        });
        _sleepOverlay->runAction(Sequence::create(fadeIn, apply, wait, fadeOut, finish, nullptr));
    }
    else
    {
        if (GameScene::sClock)
        {
            GameScene::sClock->setHour(6);
            GameScene::sClock->setMinute(0);
            GameScene::sClock->addDay(1);
        }
        CropSystem::getInstance()->updateDailyGrowth();
        if (_player && _hasBedRect)
        {
            Vec2 pos(_bedRect.getMidX(), _bedRect.getMidY());
            _player->setPosition(pos);
        }
        _isSleeping = false;
    }
}

void BackgroundLayer::cancelSleepDialog()
{
    _sleepDialogActive = false;
    if (_sleepOverlay)
    {
        _sleepOverlay->setVisible(false);
        _sleepOverlay->stopAllActions();
    }
}

void BackgroundLayer::startFishing()
{
    if (_isFishing) return;
    if (!_map || !_player) return;

    _isFishing = true;
    _fishBite = false;
    _fishingElapsed = 0.0f;
    _biteTime = cocos2d::RandomHelper::random_real(1.0f, 3.0f);
    _biteWindow = 1.0f;

    auto director = Director::getInstance();
    Size visibleSize = director->getVisibleSize();
    Vec2 origin = director->getVisibleOrigin();

    if (!_fishingOverlay)
    {
        _fishingOverlay = LayerColor::create(Color4B(0, 0, 0, 0));
        _fishingOverlay->setContentSize(visibleSize);
        _fishingOverlay->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);
        // Position relative to screen (not moving with BackgroundLayer)
        _fishingOverlay->setPosition(origin);

        auto label = Label::createWithSystemFont("Fishing... Wait for a bite", "Arial", 16.0f);
        if (label)
        {
            _fishingOverlay->addChild(label);
            _fishingLabel = label;
        }

        // Add to GameScene (Parent of BackgroundLayer) to be above HudLayer (z=1000)
        if (this->getParent())
        {
            this->getParent()->addChild(_fishingOverlay, 6000);
        }
        else
        {
            addChild(_fishingOverlay, 6000);
        }
        
        // Stardew Valley style: Narrow vertical bar
        float gameWidth = 80.0f;
        float gameHeight = visibleSize.height * 0.5f; 
        cocos2d::Size gameSize(gameWidth, gameHeight);
        
        _fishingGame = FishingMiniGame::create(gameSize);
        
        // Position on left side (15% of screen width)
        float gameX = visibleSize.width * 0.15f;

        // Select Random Fish
        int r = cocos2d::RandomHelper::random_int(0, 2);
        if (r == 0) _currentFishType = CropType::Anchovy;
        else if (r == 1) _currentFishType = CropType::Bream;
        else _currentFishType = CropType::LargemouthBass;

        if (_fishingGame)
        {
            // Apply difficulty based on fish type
            if (_currentFishType == CropType::Anchovy) {
                _fishingGame->setupDifficulty(80.0f, 0.8f, 1.5f); // Easy
            } else if (_currentFishType == CropType::Bream) {
                _fishingGame->setupDifficulty(120.0f, 0.5f, 1.0f); // Medium
            } else {
                _fishingGame->setupDifficulty(180.0f, 0.3f, 0.7f); // Hard
            }

            // Center horizontally at gameX, but move up vertically to avoid toolbar
            // visibleSize.height * 0.6f puts center at 60% height
            float centerY = visibleSize.height * 0.55f; 
            _fishingGame->setPosition(Vec2(gameX - gameSize.width * 0.5f, centerY - gameSize.height * 0.5f));
            _fishingOverlay->addChild(_fishingGame, 1);
        }

        if (label)
        {
            // Position label above the fishing game
            // Using same centerY logic
            float centerY = visibleSize.height * 0.55f;
            float labelY = centerY + (gameHeight * 0.5f) + 20.0f;
            label->setPosition(Vec2(gameX, labelY));
        }
    }
    else
    {
        _fishingOverlay->setVisible(true);
        _fishingOverlay->setOpacity(0);
        // Ensure position is fixed to screen
        _fishingOverlay->setPosition(origin);
        
        if (_fishingLabel)
        {
            _fishingLabel->setString("Press space to control the green block");
        }
        
        // Select Random Fish for next attempt
        int r = cocos2d::RandomHelper::random_int(0, 2);
        if (r == 0) _currentFishType = CropType::Anchovy;
        else if (r == 1) _currentFishType = CropType::Bream;
        else _currentFishType = CropType::LargemouthBass;

        if (_fishingGame)
        {
            _fishingGame->restart();
            // Apply difficulty based on fish type
            if (_currentFishType == CropType::Anchovy) {
                _fishingGame->setupDifficulty(80.0f, 0.8f, 1.5f); // Easy
            } else if (_currentFishType == CropType::Bream) {
                _fishingGame->setupDifficulty(120.0f, 0.5f, 1.0f); // Medium
            } else {
                _fishingGame->setupDifficulty(180.0f, 0.3f, 0.7f); // Hard
            }
        }
    }
}

void BackgroundLayer::endFishing(bool success)
{
    _isFishing = false;
    _fishBite = false;
    _fishingElapsed = 0.0f;

    if (_fishingOverlay)
    {
        _fishingOverlay->stopAllActions();
        _fishingOverlay->setOpacity(0);
    }

    if (success)
    {
        if (GameScene::sInventory)
        {
            const CropData* data = CropSystem::getInstance()->getCropData(_currentFishType);
            if (data)
            {
                Item fish;
                fish.type = ItemType::Crop;
                fish.cropType = _currentFishType;
                fish.name = data->itemName;
                fish.iconPath = data->itemIcon;
                fish.quantity = 1;
                fish.maxStack = 999;
                GameScene::sInventory->addItem(fish);
                Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("INVENTORY_UPDATED");
                
                if (_fishingLabel)
                {
                    _fishingLabel->setString("You caught a " + data->itemName + "!");
                }
            }
        }
    }
    else
    {
        if (_fishingLabel)
        {
            _fishingLabel->setString("you failed");
        }
    }

    if (_fishingOverlay)
    {
        auto fadeOut = FadeTo::create(0.8f, 0);
        auto hide = CallFunc::create([this]() {
            if (_fishingOverlay)
            {
                _fishingOverlay->setVisible(false);
            }
        });
        _fishingOverlay->runAction(Sequence::create(fadeOut, hide, nullptr));
    }
}

void BackgroundLayer::update(float dt)
{
    if (_isFishing)
    {
        if (_fishingGame && _fishingGame->isFinished())
        {
            bool success = _fishingGame->isSuccess();
            endFishing(success);
        }
    }

    if (GameScene::sClock && GameScene::sClock->getSeason() != _lastSeason)
    {
        updateSeasonFilter();
    }

    if (!_map || !_player)
    {
        return;
    }

    _canEnterHomeDoor = false;
    _canExitHomeDoor = false;

    Size mapSizeTiles = _map->getMapSize();
    Size tileSize = _map->getTileSize();
    float mapWidth = mapSizeTiles.width * tileSize.width;
    float mapHeight = mapSizeTiles.height * tileSize.height;

    Vec2 velocity = _player->getMoveVelocity();
    if (velocity.lengthSquared() > 0.0f)
    {
        Vec2 delta = velocity * dt;
        Vec2 pos = _player->getPosition();

        // Use a smaller footprint box for collision (feet only)
        // This prevents the head/body from getting stuck on obstacles
        Rect spriteBox = _player->getBoundingBox();
        float footW = spriteBox.size.width * 0.5f; // 50% width
        float footH = spriteBox.size.height * 0.25f; // 25% height (bottom)
        Rect box(spriteBox.getMidX() - footW * 0.5f, spriteBox.getMinY(), footW, footH);

        if (_hasHomeRect)
        {
            Rect boxX = box;
            boxX.origin.x += delta.x;
            bool blockX = boxX.intersectsRect(_homeRect) || (_hasBoundary && (boxX.intersectsRect(_boundaryLeftRect) || boxX.intersectsRect(_boundaryRightRect))) || checkCollisionWithObstacles(boxX);
            if (!blockX && _hasPoolRect && !_poolRects.empty())
            {
                for (const auto& r : _poolRects)
                {
                    if (boxX.intersectsRect(r))
                    {
                        blockX = true;
                        break;
                    }
                }
            }
            if (!blockX)
            {
                pos.x += delta.x;
            }
            Rect boxY = box;
            boxY.origin.y += delta.y;
            bool blockY = false;
            if (boxY.intersectsRect(_homeRect))
            {
                bool currentlyInHome = box.intersectsRect(_homeRect);
                Size tileSize2 = _map->getTileSize();
                float stripH = std::min(tileSize2.height * 0.3f, _homeDoorRect.size.height);
                Rect bottomStrip(_homeDoorRect.getMinX(), _homeDoorRect.getMinY(), _homeDoorRect.size.width, stripH);
                bool allowY = false;
                if (!currentlyInHome)
                {
                    bool hitBottom = boxY.intersectsRect(bottomStrip);
                    float centerX = (box.origin.x + box.getMaxX()) * 0.5f;
                    bool inDoorHoriz = centerX >= _homeDoorRect.getMinX() && centerX <= _homeDoorRect.getMaxX();
                    allowY = hitBottom && inDoorHoriz;
                }
                else
                {
                    allowY = boxY.intersectsRect(_homeDoorTunnelRect);
                }
                blockY = !allowY;
            }
            if (_hasBoundary)
            {
                if (boxY.intersectsRect(_boundaryTopRect) || boxY.intersectsRect(_boundaryBottomRect))
                {
                    blockY = true;
                }
            }
            if (!blockY && checkCollisionWithObstacles(boxY))
            {
                blockY = true;
            }
            if (!blockY && _hasPoolRect && !_poolRects.empty())
            {
                for (const auto& r : _poolRects)
                {
                    if (boxY.intersectsRect(r))
                    {
                        blockY = true;
                        break;
                    }
                }
            }
            if (!blockY)
            {
                pos.y += delta.y;
            }
        }
        else
        {
            Rect boxX = box;
            boxX.origin.x += delta.x;
            bool blockX = (_hasBoundary && (boxX.intersectsRect(_boundaryLeftRect) || boxX.intersectsRect(_boundaryRightRect))) || checkCollisionWithObstacles(boxX);
            if (!blockX && _hasPoolRect && !_poolRects.empty())
            {
                for (const auto& r : _poolRects)
                {
                    if (boxX.intersectsRect(r))
                    {
                        blockX = true;
                        break;
                    }
                }
            }
            if (!blockX)
            {
                pos.x += delta.x;
            }
            Rect boxY = box;
            boxY.origin.y += delta.y;
            bool blockY = (_hasBoundary && (boxY.intersectsRect(_boundaryTopRect) || boxY.intersectsRect(_boundaryBottomRect))) || checkCollisionWithObstacles(boxY);
            if (!blockY && _hasPoolRect && !_poolRects.empty())
            {
                for (const auto& r : _poolRects)
                {
                    if (boxY.intersectsRect(r))
                    {
                        blockY = true;
                        break;
                    }
                }
            }
            if (!blockY)
            {
                pos.y += delta.y;
            }
        }

        Rect boxAfter = _player->getBoundingBox();
        float halfW = boxAfter.size.width * 0.5f;
        float halfH = boxAfter.size.height * 0.5f;
        pos.x = std::max(halfW, std::min(pos.x, mapWidth - halfW));
        pos.y = std::max(halfH, std::min(pos.y, mapHeight - halfH));

        _player->setPosition(pos);
    }

    // Dynamic Z-Order for Player (Depth Sorting)
    if (_player)
    {
        _player->setLocalZOrder(static_cast<int>(mapHeight - _player->getPositionY()));
    }

    if (_type == BackgroundType::Farm && _hasHomeRect && !_enteredHome && _map && _groundLayer)
    {
        Vec2 tileIndex = getFacingTile();
        if (tileIndex.x >= 0 && tileIndex.y >= 0)
        {
            Size tileSize2 = _map->getTileSize();
            Vec2 tilePos = _groundLayer->getPositionAt(tileIndex);
            Rect facingRect(tilePos.x, tilePos.y, tileSize2.width, tileSize2.height);
            if (facingRect.intersectsRect(_homeDoorRect))
            {
                _canEnterHomeDoor = true;
            }
        }
    }
    if (_hasRightExit && !_exitedRight)
    {
        Rect box = _player->getBoundingBox();
        float stripW = std::min(tileSize.width * 0.3f, _rightExitRect.size.width);
        Rect rightStrip(_rightExitRect.getMaxX() - stripW, _rightExitRect.getMinY(), stripW, _rightExitRect.size.height);
        if (box.intersectsRect(rightStrip))
        {
            float cy = (box.origin.y + box.getMaxY()) * 0.5f;
            bool inVert = cy >= _rightExitRect.getMinY() && cy <= _rightExitRect.getMaxY();
            if (inVert)
                {
                    _exitedRight = true;
                    GameScene::sLastFarmPlayerPos = _player->getPosition();
                    GameScene::sHasLastFarmPlayerPos = true;
                    GameScene::switchViaRightExit(0.5f);
            }
        }
    }

    if (_type == BackgroundType::Home && _hasHomeExitDoor && !_exitedHomeDoor && _map && _groundLayer)
    {
        Vec2 tileIndex = getFacingTile();
        if (tileIndex.x >= 0 && tileIndex.y >= 0)
        {
            Size tileSize2 = _map->getTileSize();
            Vec2 tilePos = _groundLayer->getPositionAt(tileIndex);
            Rect facingRect(tilePos.x, tilePos.y, tileSize2.width, tileSize2.height);
            if (facingRect.intersectsRect(_homeExitDoorRect))
            {
                _canExitHomeDoor = true;
            }
        }
    }

    if (_facingDebug && _groundLayer)
    {
        _facingDebug->clear();

        // 1. Always draw facing tile cursor (User requirement: always visible)
        Vec2 tileIndex = getFacingTile();
        if (tileIndex.x >= 0 && tileIndex.y >= 0)
        {
            if (!(_type == BackgroundType::Home && tileIndex.y < 3))
            {
                Size tileSize2 = _map->getTileSize();

                Vec2 tilePos = _groundLayer->getPositionAt(tileIndex);

                Vec2 centerPos(tilePos.x + tileSize2.width * 0.5f,
                    tilePos.y + tileSize2.height * 0.5f);
                _facingDebug->drawDot(centerPos, 4.0f, Color4F::YELLOW);

                Vec2 p1(tilePos.x, tilePos.y);
                Vec2 p2(tilePos.x + tileSize2.width, tilePos.y + tileSize2.height);

                Rect facingRect(tilePos.x, tilePos.y, tileSize2.width, tileSize2.height);
                bool inPool = false;
                if (_hasPoolRect && !_poolRects.empty())
                {
                    for (const auto& r : _poolRects)
                    {
                        if (facingRect.intersectsRect(r))
                        {
                            inPool = true;
                            break;
                        }
                    }
                }

                bool isValid = false;
                
                if (CropSystem::getInstance()->canHarvest(tileIndex))
                {
                    isValid = true;
                }
                else
                {
                    if (hasObstacle(tileIndex))
                    {
                        const Item* item = GameScene::sInventory ? GameScene::sInventory->getSelectedItem() : nullptr;
                        if (item && item->type == ItemType::Tool)
                        {
                            int obsType = getObstacleType(tileIndex);
                            if (obsType == 0 && item->toolType == ToolType::Axe) isValid = true;
                            else if (obsType == 1 && item->toolType == ToolType::Pickaxe) isValid = true;
                            else if (obsType == 2 && item->toolType == ToolType::Scythe) isValid = true;
                        }
                    }
                    else
                    {
                        const Item* item = GameScene::sInventory ? GameScene::sInventory->getSelectedItem() : nullptr;
                        if (item)
                        {
                            if (item->type == ItemType::Tool)
                            {
                                switch (item->toolType)
                                {
                                case ToolType::Hoe:
                                    if (!inPool)
                                    {
                                        isValid = CropSystem::getInstance()->canTill(tileIndex);
                                    }
                                    break;
                                case ToolType::WateringCan:
                                    isValid = CropSystem::getInstance()->canWater(tileIndex);
                                    break;
                                case ToolType::Scythe:
                                    isValid = CropSystem::getInstance()->canClearWithered(tileIndex);
                                    break;
                                case ToolType::Pickaxe:
                                    isValid = CropSystem::getInstance()->canDestroy(tileIndex);
                                    break;
                                case ToolType::FishingRod:
                                    isValid = inPool;
                                    break;
                                default:
                                    break;
                                }
                            }
                            else if (item->type == ItemType::Seed)
                            {
                                isValid = CropSystem::getInstance()->canPlant(tileIndex, item->cropType);
                            }
                        }
                    }
                }

                Color4F boxColor = isValid ? Color4F(0.0f, 1.0f, 0.0f, 0.3f) : Color4F(1.0f, 0.0f, 0.0f, 0.3f);
                Color4F borderColor = isValid ? Color4F(0.0f, 1.0f, 0.0f, 1.0f) : Color4F(1.0f, 0.0f, 0.0f, 1.0f);

                _facingDebug->drawSolidRect(p1, p2, boxColor);
                _facingDebug->drawRect(p1, p2, borderColor);
            }
        }

        if (_hasRightExit)
        {
            Vec2 e1(_rightExitRect.getMinX(), _rightExitRect.getMinY());
            Vec2 e2(_rightExitRect.getMaxX(), _rightExitRect.getMaxY());
            _facingDebug->drawSolidRect(e1, e2, Color4F(0.0f, 1.0f, 0.0f, 0.2f));
            _facingDebug->drawRect(e1, e2, Color4F(0.0f, 1.0f, 0.0f, 1.0f));
        }

        if (_isDebugMode)
        {
            if (_hasHomeRect)
            {
                Vec2 h1(_homeRect.getMinX(), _homeRect.getMinY());
                Vec2 h2(_homeRect.getMaxX(), _homeRect.getMaxY());
                _facingDebug->drawRect(h1, h2, Color4F(1.0f, 0.0f, 0.0f, 1.0f));
                Vec2 d1(_homeDoorRect.getMinX(), _homeDoorRect.getMinY());
                Vec2 d2(_homeDoorRect.getMaxX(), _homeDoorRect.getMaxY());
                _facingDebug->drawSolidRect(d1, d2, Color4F(0.0f, 1.0f, 0.0f, 0.2f));
                _facingDebug->drawRect(d1, d2, Color4F(0.0f, 1.0f, 0.0f, 1.0f));
            }
            if (_hasBoundary)
            {
                _facingDebug->drawRect(Vec2(_boundaryLeftRect.getMinX(), _boundaryLeftRect.getMinY()), Vec2(_boundaryLeftRect.getMaxX(), _boundaryLeftRect.getMaxY()), Color4F(1.0f, 0.0f, 0.0f, 1.0f));
                _facingDebug->drawRect(Vec2(_boundaryRightRect.getMinX(), _boundaryRightRect.getMinY()), Vec2(_boundaryRightRect.getMaxX(), _boundaryRightRect.getMaxY()), Color4F(1.0f, 0.0f, 0.0f, 1.0f));
                _facingDebug->drawRect(Vec2(_boundaryTopRect.getMinX(), _boundaryTopRect.getMinY()), Vec2(_boundaryTopRect.getMaxX(), _boundaryTopRect.getMaxY()), Color4F(1.0f, 0.0f, 0.0f, 1.0f));
                _facingDebug->drawRect(Vec2(_boundaryBottomRect.getMinX(), _boundaryBottomRect.getMinY()), Vec2(_boundaryBottomRect.getMaxX(), _boundaryBottomRect.getMaxY()), Color4F(1.0f, 0.0f, 0.0f, 1.0f));
            }

            if (_hasPoolRect && !_poolRects.empty())
            {
                for (const auto& r : _poolRects)
                {
                    Vec2 p1(r.getMinX(), r.getMinY());
                    Vec2 p2(r.getMaxX(), r.getMaxY());
                    _facingDebug->drawSolidRect(p1, p2, Color4F(0.0f, 0.5f, 1.0f, 0.2f));
                    _facingDebug->drawRect(p1, p2, Color4F(0.0f, 0.5f, 1.0f, 1.0f));
                }
            }

            // --- DEBUG: Visualize Player Collision Box (Footprint) ---
            if (_player)
            {
                Rect spriteBox = _player->getBoundingBox();
                float footW = spriteBox.size.width * 0.5f;
                float footH = spriteBox.size.height * 0.25f;
                Rect box(spriteBox.getMidX() - footW * 0.5f, spriteBox.getMinY(), footW, footH);
            
                _facingDebug->drawRect(
                    Vec2(box.getMinX(), box.getMinY()), 
                    Vec2(box.getMaxX(), box.getMaxY()), 
                    Color4F(0.0f, 0.0f, 1.0f, 1.0f) // Blue for player footprint
                );
            }

            // --- DEBUG: Visualize Obstacle Collision Boxes ---
            if (_map)
            {
                Size tileSize = _map->getTileSize();
                Size mapSize = _map->getMapSize();
                float mapHeight = mapSize.height * tileSize.height;

                // Iterate all tiles to find obstacles (inefficient but okay for debug)
                for (const auto& pair : _obstacles)
                {
                    int key = pair.first;
                    int x = key % (int)mapSize.width;
                    int y = key / (int)mapSize.width;

                    float cx = (x + 0.5f) * tileSize.width;
                    float cy = mapHeight - (y + 0.5f) * tileSize.height;

                    float shrinkFactor = 0.85f; 
                    float w = tileSize.width * shrinkFactor;
                    float h = tileSize.height * shrinkFactor;

                    Rect obsRect(cx - w * 0.5f, cy - h * 0.5f, w, h);
                
                    _facingDebug->drawRect(
                        Vec2(obsRect.getMinX(), obsRect.getMinY()),
                        Vec2(obsRect.getMaxX(), obsRect.getMaxY()),
                        Color4F(1.0f, 0.0f, 0.0f, 1.0f) // Red for obstacles
                    );
                }
            }
        }
    }
}

void BackgroundLayer::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
    if (_isFishing)
    {
        if (_fishingGame)
        {
            _fishingGame->onKeyPressed(keyCode);
        }
        if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE)
        {
            endFishing(false);
        }
        return;
    }

    if (_sleepDialogActive)
    {
        if (keyCode == EventKeyboard::KeyCode::KEY_Y)
        {
            _sleepDialogActive = false;
            beginSleep();
        }
        else if (keyCode == EventKeyboard::KeyCode::KEY_N || keyCode == EventKeyboard::KeyCode::KEY_ESCAPE)
        {
            cancelSleepDialog();
        }
        return;
    }
    if (_isSleeping)
    {
        return;
    }
    if (_type == BackgroundType::Farm)
    {
        Vec2 tile = getFacingTile();
        switch (keyCode)
        {
        case EventKeyboard::KeyCode::KEY_GRAVE: // Tilde key (~)
            _isDebugMode = !_isDebugMode;
            GameScene::sDebugMode = _isDebugMode;
            if (GameScene::sHud) { GameScene::sHud->updateInventoryUI(); }
            Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("DEBUG_MODE_CHANGED");
            break;
        case EventKeyboard::KeyCode::KEY_1:
            CropSystem::getInstance()->setSelectedCrop(CropType::Parsnip);
            break;
        case EventKeyboard::KeyCode::KEY_2:
            CropSystem::getInstance()->setSelectedCrop(CropType::Cauliflower);
            break;
        case EventKeyboard::KeyCode::KEY_3:
            CropSystem::getInstance()->setSelectedCrop(CropType::Potato);
            break;
        case EventKeyboard::KeyCode::KEY_T:
            CropSystem::getInstance()->tillTile(tile);
            break;
        case EventKeyboard::KeyCode::KEY_G:
            CropSystem::getInstance()->waterTile(tile);
            break;
        case EventKeyboard::KeyCode::KEY_H:
            CropSystem::getInstance()->harvestTile(tile);
            break;
        case EventKeyboard::KeyCode::KEY_UP_ARROW:
            if (_isDebugMode && GameScene::sClock)
            {
                GameScene::sClock->addDay(1);
                CropSystem::getInstance()->updateDailyGrowth();
            }
            break;
        case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
            if (_isDebugMode && GameScene::sClock)
            {
                GameScene::sClock->addDay(-1);
                CropSystem::getInstance()->updateDailyGrowth();
            }
            break;
        case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
            if (_isDebugMode && GameScene::sClock)
            {
                GameScene::sClock->addHour(-1);
                CropSystem::getInstance()->updateDailyGrowth();
            }
            break;
        case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
            if (_isDebugMode && GameScene::sClock)
            {
                GameScene::sClock->addHour(1);
                CropSystem::getInstance()->updateDailyGrowth();
            }
            break;
        default:
            break;
        }
    }
    if (keyCode == EventKeyboard::KeyCode::KEY_X)
    {
        if (_type == BackgroundType::Home && _hasBedRect && _map && _groundLayer && _player)
        {
            Vec2 tileIndex = getFacingTile();
            if (tileIndex.x >= 0 && tileIndex.y >= 0)
            {
                Size tileSize2 = _map->getTileSize();
                Vec2 tilePos = _groundLayer->getPositionAt(tileIndex);
                Rect facingRect(tilePos.x, tilePos.y, tileSize2.width, tileSize2.height);
                if (facingRect.intersectsRect(_bedRect))
                {
                    showSleepDialog();
                    return;
                }
            }
        }
        if (_type == BackgroundType::Farm && _hasPoolRect && _map && _groundLayer && _player)
        {
            const Item* item = GameScene::sInventory ? GameScene::sInventory->getSelectedItem() : nullptr;
            if (item && item->type == ItemType::Tool && item->toolType == ToolType::FishingRod)
            {
                Vec2 tileIndex = getFacingTile();
                if (tileIndex.x >= 0 && tileIndex.y >= 0)
                {
                    Size tileSize2 = _map->getTileSize();
                    Vec2 tilePos = _groundLayer->getPositionAt(tileIndex);
                    Rect facingRect(tilePos.x, tilePos.y, tileSize2.width, tileSize2.height);

                    bool inPool = false;
                    for (const auto& r : _poolRects)
                    {
                        if (facingRect.intersectsRect(r))
                        {
                            inPool = true;
                            break;
                        }
                    }

                    if (inPool)
                    {
                        startFishing();
                        return;
                    }
                }
            }
        }
        if (_type == BackgroundType::Farm && _canEnterHomeDoor && !_enteredHome && _map && _groundLayer && _player)
        {
            _enteredHome = true;
            GameScene::sLastFarmPlayerPos = _player->getPosition();
            GameScene::sHasLastFarmPlayerPos = true;
            auto next = GameScene::createScene(BackgroundType::Home);
            if (next)
            {
                auto trans = TransitionFade::create(0.5f, next);
                Director::getInstance()->replaceScene(trans);
            }
        }
        else if (_type == BackgroundType::Home && _canExitHomeDoor && !_exitedHomeDoor && _map && _groundLayer)
        {
            _exitedHomeDoor = true;
            GameScene::sSpawnAtFarmStart = true;
            auto next = GameScene::createScene(BackgroundType::Farm);
            if (next)
            {
                auto trans = TransitionFade::create(0.5f, next);
                Director::getInstance()->replaceScene(trans);
            }
        }
    }
    if (_player)
    {
        // In debug mode, if arrow keys are used, do not pass them to the player (prevent movement)
        bool blockInput = false;
        if (_isDebugMode)
        {
            if (keyCode == EventKeyboard::KeyCode::KEY_UP_ARROW ||
                keyCode == EventKeyboard::KeyCode::KEY_DOWN_ARROW ||
                keyCode == EventKeyboard::KeyCode::KEY_LEFT_ARROW ||
                keyCode == EventKeyboard::KeyCode::KEY_RIGHT_ARROW)
            {
                blockInput = true;
            }
        }

        if (!blockInput)
        {
            _player->onKeyPressed(keyCode);
        }
    }
}

void BackgroundLayer::onKeyReleased(EventKeyboard::KeyCode keyCode, Event* event)
{
    if (_isFishing)
    {
        if (_fishingGame)
        {
            _fishingGame->onKeyReleased(keyCode);
        }
        return;
    }
    if (_player)
    {
        // In debug mode, if arrow keys are used, do not pass them to the player
        bool blockInput = false;
        if (_isDebugMode)
        {
            if (keyCode == EventKeyboard::KeyCode::KEY_UP_ARROW ||
                keyCode == EventKeyboard::KeyCode::KEY_DOWN_ARROW ||
                keyCode == EventKeyboard::KeyCode::KEY_LEFT_ARROW ||
                keyCode == EventKeyboard::KeyCode::KEY_RIGHT_ARROW)
            {
                blockInput = true;
            }
        }

        if (!blockInput)
        {
            _player->onKeyReleased(keyCode);
        }
    }
}

cocos2d::Vec2 BackgroundLayer::getFacingTile() const
{
    if (!_map || !_player)
    {
        return Vec2(-1, -1);
    }

    Size mapSizeTiles = _map->getMapSize();
    Size tileSize = _map->getTileSize();

    float mapWidth = mapSizeTiles.width * tileSize.width;
    float mapHeight = mapSizeTiles.height * tileSize.height;

    // Use the feet position (collision box center) for more accurate tile selection
    // instead of the anchor point which might be higher up.
    Rect spriteBox = _player->getBoundingBox();
    float footH = spriteBox.size.height * 0.25f; // Match collision logic
    Vec2 feetPos(spriteBox.getMidX(), spriteBox.getMinY() + footH * 0.5f);

    float clampedX = std::max(0.0f, std::min(feetPos.x, mapWidth - 1.0f));
    float clampedY = std::max(0.0f, std::min(feetPos.y, mapHeight - 1.0f));

    // 世界坐标 → 以左上角为原点的瓷砖坐标
    float tileXTop = clampedX / tileSize.width;
    float tileYTop = (mapHeight - clampedY) / tileSize.height;

    int ix = static_cast<int>(tileXTop);
    int iy = static_cast<int>(tileYTop);

    Vec2 offset = _player->getFacingOffset();
    int tx = ix + static_cast<int>(offset.x);
    int ty = iy + static_cast<int>(offset.y);

    int maxX = static_cast<int>(mapSizeTiles.width) - 1;
    int maxY = static_cast<int>(mapSizeTiles.height) - 1;

    tx = std::max(0, std::min(tx, maxX));
    ty = std::max(0, std::min(ty, maxY));

    return Vec2(tx, ty);
}

Vec2 FarmMapUtils::gridToWorld(const Vec2& gridIndex, Sprite* mapSprite, int cols, int rows)
{
    if (!mapSprite || cols <= 0 || rows <= 0)
    {
        return Vec2::ZERO;
    }

    Size size = mapSprite->getContentSize();
    float cellWidth = size.width / cols;
    float cellHeight = size.height / rows;

    float left = -size.width * 0.5f;
    float bottom = -size.height * 0.5f;

    float localX = left + (gridIndex.x + 0.5f) * cellWidth;
    float localY = bottom + (gridIndex.y + 0.5f) * cellHeight;

    Vec2 localPos(localX, localY);
    return mapSprite->convertToWorldSpace(localPos);
}

Vec2 FarmMapUtils::worldToGrid(const Vec2& worldPos, Sprite* mapSprite, int cols, int rows)
{
    if (!mapSprite || cols <= 0 || rows <= 0)
    {
        return Vec2(-1, -1);
    }

    Size size = mapSprite->getContentSize();
    float cellWidth = size.width / cols;
    float cellHeight = size.height / rows;

    float left = -size.width * 0.5f;
    float bottom = -size.height * 0.5f;

    Vec2 local = mapSprite->convertToNodeSpace(worldPos);

    float u = (local.x - left) / cellWidth;
    float v = (local.y - bottom) / cellHeight;

    int col = static_cast<int>(u);
    int row = static_cast<int>(v);

    return Vec2(col, row);
}

void BackgroundLayer::onExit()
{
    if (_type == BackgroundType::Farm)
    {
        CropSystem::getInstance()->setMap(nullptr);
    }
    Layer::onExit();
}

Scene* GameScene::createScene()
{
    return GameScene::create();
}

Scene* GameScene::createScene(BackgroundType type)
{
    auto ret = new (std::nothrow) GameScene();
    if (ret && ret->initWithStartType(type))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

Scene* GameScene::make(BackgroundType type)
{
    return GameScene::createScene(type);
}

GameClock* GameScene::sClock = nullptr;
Wallet* GameScene::sWallet = nullptr;
Vec2 GameScene::sLastFarmPlayerPos = Vec2::ZERO;
bool GameScene::sHasLastFarmPlayerPos = false;
Vec2 GameScene::sFarmStartPos = Vec2::ZERO;
bool GameScene::sHasFarmStartPos = false;
bool GameScene::sSpawnAtFarmStart = false;
bool GameScene::sStartAtHomeBed = false;

Inventory* GameScene::sInventory = nullptr; // Define sInventory

void GameScene::switchTo(BackgroundType type, float duration)
{
    auto next = GameScene::createScene(type);
    if (next)
    {
        auto trans = TransitionFade::create(duration, next);
        Director::getInstance()->replaceScene(trans);
    }
}

void GameScene::switchViaRightExit(float duration)
{
    auto next = GameScene::createScene(BackgroundType::Path);
    if (next)
    {
        auto trans = TransitionFade::create(duration, next);
        Director::getInstance()->replaceScene(trans);
    }
}

GameScene::~GameScene()
{
}

bool GameScene::init()
{
    if (!Scene::init())
    {
        return false;
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    auto backgroundLayer = BackgroundLayer::create(BackgroundType::Farm);
    if (backgroundLayer)
    {
        addChild(backgroundLayer, 0);
    }
    else
    {
        auto sprite = Sprite::create("res/home_bg.png");
        if (sprite)
        {
            sprite->setPosition(Vec2(visibleSize.width / 2 + origin.x, visibleSize.height / 2 + origin.y));
            addChild(sprite, 0);
        }
    }

    if (!GameScene::sClock) { GameScene::sClock = new GameClock(); }
    if (!GameScene::sWallet) { GameScene::sWallet = new Wallet(); }
    if (!GameScene::sInventory) { GameScene::sInventory = new Inventory(); }

    _clock = GameScene::sClock;
    _wallet = GameScene::sWallet;
    _inventory = GameScene::sInventory;

    _hud = HudLayer::create(_clock, _wallet, _inventory);
    if (_hud)
    {
        addChild(_hud, 1000);
        GameScene::sHud = _hud;
    }
    CropSystem::getInstance()->init(nullptr, _clock, _wallet, _inventory);
    scheduleUpdate();

    return true;
}

bool GameScene::initWithStartType(BackgroundType type)
{
    if (!Scene::init())
    {
        return false;
    }

    _startType = type;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    auto backgroundLayer = BackgroundLayer::create(_startType);
    if (backgroundLayer)
    {
        addChild(backgroundLayer, 0);
    }
    else
    {
        auto sprite = Sprite::create("res/home_bg.png");
        if (sprite)
        {
            sprite->setPosition(Vec2(visibleSize.width / 2 + origin.x, visibleSize.height / 2 + origin.y));
            addChild(sprite, 0);
        }
    }

    if (!GameScene::sClock) { GameScene::sClock = new GameClock(); }
    if (!GameScene::sWallet) { GameScene::sWallet = new Wallet(); }
    if (!GameScene::sInventory) { GameScene::sInventory = new Inventory(); }

    _clock = GameScene::sClock;
    _wallet = GameScene::sWallet;
    _inventory = GameScene::sInventory;

    _hud = HudLayer::create(_clock, _wallet, _inventory);
    if (_hud)
    {
        addChild(_hud, 1000);
        GameScene::sHud = _hud;
    }
    CropSystem::getInstance()->init(nullptr, _clock, _wallet, _inventory);
    scheduleUpdate();

    GameScene::sStartAtHomeBed = false;

    return true;
}

// BackgroundLayer implementation
void BackgroundLayer::onMouseDown(Event* event)
{
    if (_type != BackgroundType::Farm) return;
    
    EventMouse* e = (EventMouse*)event;
    if (e->getMouseButton() != EventMouse::MouseButton::BUTTON_LEFT) return;
    Vec2 clickPos = e->getLocation();
    if (GameScene::sHud && (GameScene::sHud->isPointInToolbarWorld(clickPos) || GameScene::sHud->isConsumingClick())) return;

    // Use the tile the player is currently facing (interaction block)
    Vec2 targetTile = getFacingTile();
    
    Size tileSize = _map->getTileSize();
    Size mapSize = _map->getMapSize();
    
    if (targetTile.x < 0 || targetTile.x >= mapSize.width || 
        targetTile.y < 0 || targetTile.y >= mapSize.height) return;

    // Visual feedback
    if (!_facingDebug) {
        _facingDebug = DrawNode::create();
        _map->addChild(_facingDebug, 1000);
    }
    _facingDebug->clear();
    
    float mapHeight = mapSize.height * tileSize.height;
    float cxPos = (targetTile.x + 0.5f) * tileSize.width;
    float cyPos = mapHeight - (targetTile.y + 0.5f) * tileSize.height;
    _facingDebug->drawDot(Vec2(cxPos, cyPos), 5.0f, Color4F::GREEN);
    
    // Now handle tool use on this tile
    // We can reuse logic from handleToolUse but pass the tile
    
    if (!GameScene::sClock) return;
    const Item* item = GameScene::sInventory->getSelectedItem();
    
    // Harvest logic (no tool needed, or click with any tool?)
    // Stardew usually harvest with click (empty hand or item).
    // If we have a crop that is ready, harvest it.
    // If we have a tool selected, use tool.
    
    // Priority: Harvest -> Obstacle -> Tool/Plant
    
    // 1. Harvest
    if (CropSystem::getInstance()->harvestTile(targetTile))
    {
        return; // Harvested
    }
    
    if (!item) return;

    // 2. Obstacles
    if (hasObstacle(targetTile))
    {
        int obsType = getObstacleType(targetTile);
        bool removed = false;
        if (item->type == ItemType::Tool)
        {
            if (obsType == 0 && item->toolType == ToolType::Axe) removed = true; // Wood
            else if (obsType == 1 && item->toolType == ToolType::Pickaxe) removed = true; // Stone
            else if (obsType == 2 && item->toolType == ToolType::Scythe) removed = true; // Weed
        }
        
        if (removed)
        {
            removeObstacle(targetTile);
            // Optional: Drop item?
            return;
        }
        else
        {
            // If obstacle exists and was not removed (wrong tool or no tool),
            // it blocks all other actions (tilling, watering, planting).
            return;
        }
    }

    // 3. Tool / Plant
    if (item->type == ItemType::Tool)
    {
        switch (item->toolType)
        {
        case ToolType::Hoe:
            CropSystem::getInstance()->tillTile(targetTile);
            break;
        case ToolType::WateringCan:
            CropSystem::getInstance()->waterTile(targetTile);
            break;
        case ToolType::Scythe:
            CropSystem::getInstance()->removeWithered(targetTile);
            break;
        case ToolType::Pickaxe:
            CropSystem::getInstance()->destroyTile(targetTile);
            break;
        default:
            break;
        }
    }
    else if (item->type == ItemType::Seed)
    {
        // Update selected crop
        CropSystem::getInstance()->setSelectedCrop(item->cropType);
        
        if (CropSystem::getInstance()->plantSelected(targetTile))
        {
            int slot = GameScene::sInventory->getSelectedSlot();
            GameScene::sInventory->removeItem(slot, 1);
            Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("INVENTORY_UPDATED");
        }
    }
}

void BackgroundLayer::spawnObstacles(int count)
{
    if (!_map) return;
    Size mapSize = _map->getMapSize();
    Size tileSize = _map->getTileSize();
    float mapHeight = mapSize.height * tileSize.height;

    for (int i = 0; i < count; ++i)
    {
        int x = RandomHelper::random_int(0, (int)mapSize.width - 1);
        int y = RandomHelper::random_int(0, (int)mapSize.height - 1);
        
        // Skip if occupied (home, etc.)
        // Simplified check: avoid center 5x5 area
        int cx = mapSize.width / 2;
        int cy = mapSize.height / 2;
        if (abs(x - cx) < 3 && abs(y - cy) < 3) continue;

        // Calculate tile position for boundary checks
        float cxPos = (x + 0.5f) * tileSize.width;
        float cyPos = mapHeight - (y + 0.5f) * tileSize.height;
        Vec2 pos(cxPos, cyPos);
        Rect tileRect(x * tileSize.width, mapHeight - (y + 1) * tileSize.height, tileSize.width, tileSize.height);

        // Check forbidden zones (Home, Exit, Pool, Boundaries)
        if (_hasHomeRect && (_homeRect.intersectsRect(tileRect) || _homeDoorRect.intersectsRect(tileRect) || _homeDoorTunnelRect.intersectsRect(tileRect))) continue;
        
        // --- NEW: Prevent spawning near Home Door (Entrance Buffer Zone) ---
        if (_hasHomeRect)
        {
            // Define a buffer zone around the door
            float bufferSize = tileSize.width * 3.0f;
            Rect doorBuffer = _homeDoorRect;
            doorBuffer.origin.x -= bufferSize;
            doorBuffer.origin.y -= bufferSize;
            doorBuffer.size.width += bufferSize * 2.0f;
            doorBuffer.size.height += bufferSize * 2.0f;
            
            if (doorBuffer.intersectsRect(tileRect)) continue;
        }
        // ------------------------------------------------------------------

        if (_hasRightExit && _rightExitRect.intersectsRect(tileRect)) continue;
        if (_hasPoolRect && !_poolRects.empty())
        {
            bool insidePool = false;
            for (const auto& r : _poolRects)
            {
                if (r.intersectsRect(tileRect))
                {
                    insidePool = true;
                    break;
                }
            }
            if (insidePool) continue;
        }
        
        if (_hasBoundary)
        {
            // Do not spawn inside or beyond the boundaries
            // Left Boundary
            if (tileRect.getMinX() < _boundaryLeftRect.getMaxX()) continue;
            // Right Boundary
            if (tileRect.getMaxX() > _boundaryRightRect.getMinX()) continue;
            // Bottom Boundary
            if (tileRect.getMinY() < _boundaryBottomRect.getMaxY()) continue;
            // Top Boundary
            if (tileRect.getMaxY() > _boundaryTopRect.getMinY()) continue;
        }
        
        // Check if already has obstacle
        if (hasObstacle(Vec2(x, y))) continue;

        // Check if occupied by crop or tilled soil
        if (!CropSystem::getInstance()->canTill(Vec2(x, y))) continue;
        
        int type = RandomHelper::random_int(0, 2); // 0, 1, 2
        
        int key = y * (int)mapSize.width + x;
        
        std::string file;
        if (type == 0) file = "block/Wood.png";
        else if (type == 1) file = "block/Stone.png";
        else if (type == 2) file = "block/Fiber.png"; 
        
        auto sprite = Sprite::create(file);
        if (sprite)
        {
            float cxPos = (x + 0.5f) * tileSize.width;
            float cyPos = mapHeight - (y + 0.5f) * tileSize.height;
            sprite->setPosition(Vec2(cxPos, cyPos));
            
            if (sprite->getContentSize().width > tileSize.width)
            {
                sprite->setScale(tileSize.width / sprite->getContentSize().width);
            }
            
            // Set Z-order based on Y position for proper occlusion (2.5D look)
            int zOrder = static_cast<int>(mapHeight - cyPos);
            addChild(sprite, zOrder);
            
            Obstacle obs;
            obs.type = type;
            obs.sprite = sprite;
            obs.active = true;
            _obstacles[key] = obs;

            // Save to static storage
            ObstacleSaveData data;
            data.type = type;
            data.active = true;
            sSavedObstacles[key] = data;
        }
    }
}

void BackgroundLayer::initObstacles()
{
    if (!_map) return;
    Size mapSize = _map->getMapSize();
    Size tileSize = _map->getTileSize();
    float mapHeight = mapSize.height * tileSize.height;

    int currentSeason = -1;
    if (GameScene::sClock)
    {
        currentSeason = (int)GameScene::sClock->getSeason();
    }

    if (sObstaclesInitialized)
    {
        // Restore from saved data
        for (const auto& pair : sSavedObstacles)
        {
            int key = pair.first;
            const auto& data = pair.second;
            
            if (!data.active) continue;

            int x = key % (int)mapSize.width;
            int y = key / (int)mapSize.width;
            
            std::string file;
            if (data.type == 0) file = "block/Wood.png";
            else if (data.type == 1) file = "block/Stone.png";
            else if (data.type == 2) file = "block/Fiber.png";
            
            auto sprite = Sprite::create(file);
            if (sprite)
            {
                float cxPos = (x + 0.5f) * tileSize.width;
                float cyPos = mapHeight - (y + 0.5f) * tileSize.height;
                sprite->setPosition(Vec2(cxPos, cyPos));
                
                if (sprite->getContentSize().width > tileSize.width)
                {
                    sprite->setScale(tileSize.width / sprite->getContentSize().width);
                }
                
                int zOrder = static_cast<int>(mapHeight - cyPos);
                addChild(sprite, zOrder);
                
                Obstacle obs;
                obs.type = data.type;
                obs.sprite = sprite;
                obs.active = true;
                _obstacles[key] = obs;
            }
        }

        // Check for season change to spawn new obstacles
        if (sLastObstacleSeason != -1 && currentSeason != -1 && sLastObstacleSeason != currentSeason)
        {
            // Season changed! Spawn more obstacles (e.g. 5% of map size)
            int newObstacleCount = (mapSize.width * mapSize.height) / 20; 
            spawnObstacles(newObstacleCount);
            sLastObstacleSeason = currentSeason;
        }
    }
    else
    {
        int obstacleCount = (mapSize.width * mapSize.height) / 30;
        spawnObstacles(obstacleCount);
        sObstaclesInitialized = true;
        sLastObstacleSeason = currentSeason;
    }
}

bool BackgroundLayer::hasObstacle(const Vec2& tileIndex)
{
    if (!_map) return false;
    int key = (int)tileIndex.y * (int)_map->getMapSize().width + (int)tileIndex.x;
    return _obstacles.find(key) != _obstacles.end();
}

int BackgroundLayer::getObstacleType(const Vec2& tileIndex)
{
    if (!_map) return -1;
    int key = (int)tileIndex.y * (int)_map->getMapSize().width + (int)tileIndex.x;
    auto it = _obstacles.find(key);
    if (it != _obstacles.end()) return it->second.type;
    return -1;
}

bool BackgroundLayer::checkCollisionWithObstacles(const Rect& box)
{
    if (!_map) return false;

    Size tileSize = _map->getTileSize();
    Size mapSize = _map->getMapSize();
    float mapHeight = mapSize.height * tileSize.height;

    // Convert box to tile range
    float minX = box.getMinX();
    float maxX = box.getMaxX();
    float minY = box.getMinY();
    float maxY = box.getMaxY();

    int tMinX = static_cast<int>(minX / tileSize.width);
    int tMaxX = static_cast<int>(maxX / tileSize.width);

    // Y conversion: top-most tile (smallest index) comes from maxY
    int tMinY = static_cast<int>((mapHeight - maxY) / tileSize.height);
    // bottom-most tile (largest index) comes from minY
    int tMaxY = static_cast<int>((mapHeight - minY) / tileSize.height);

    // Clamp to map bounds
    tMinX = std::max(0, std::min(tMinX, (int)mapSize.width - 1));
    tMaxX = std::max(0, std::min(tMaxX, (int)mapSize.width - 1));
    tMinY = std::max(0, std::min(tMinY, (int)mapSize.height - 1));
    tMaxY = std::max(0, std::min(tMaxY, (int)mapSize.height - 1));

    for (int x = tMinX; x <= tMaxX; ++x)
    {
        for (int y = tMinY; y <= tMaxY; ++y)
        {
            if (hasObstacle(Vec2(x, y)))
            {
                // Refined collision: Use a smaller box for the obstacle
                // to prevent getting stuck on edges ("sticky corners")
                float cx = (x + 0.5f) * tileSize.width;
                float cy = mapHeight - (y + 0.5f) * tileSize.height;

                // Reduce collision box size (e.g. 85% of tile size)
                float shrinkFactor = 0.85f; 
                float w = tileSize.width * shrinkFactor;
                float h = tileSize.height * shrinkFactor;

                Rect obsRect(cx - w * 0.5f, cy - h * 0.5f, w, h);

                if (box.intersectsRect(obsRect))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

void BackgroundLayer::removeObstacle(const Vec2& tileIndex)
{
    if (!_map) return;
    int key = (int)tileIndex.y * (int)_map->getMapSize().width + (int)tileIndex.x;
    auto it = _obstacles.find(key);
    if (it != _obstacles.end())
    {
        if (it->second.sprite) it->second.sprite->removeFromParent();
        _obstacles.erase(it);
        
        // Remove from persistence
        sSavedObstacles.erase(key);
        
        // TODO: Add item to inventory (Wood, Stone, Fiber)
    }
}

void GameScene::onExitClicked(Ref* sender)
{
    Director::getInstance()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    exit(0);
#endif
}

void GameScene::update(float dt)
{
    if (_clock)
    {
        _clock->update(dt);
    }
    CropSystem::getInstance()->updateDailyGrowth();
    if (_hud)
    {
        _hud->refresh();
    }
}
