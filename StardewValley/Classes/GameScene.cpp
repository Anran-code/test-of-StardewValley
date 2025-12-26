#include "GameScene.h"
#include "GameClock.h"
#include "Wallet.h"
#include "HudLayer.h"
#include "CropSystem.h"
#include "ExperienceSystem.h"
#include "EnergySystem.h"
#include "ForageSystem.h"
#include "AnimalSystem.h"
#include "SimpleAudioEngine.h"
#include "Basket.h"
// #include "ShopLayer.h" // Ignore Shop related includes

USING_NS_CC;

HudLayer* GameScene::sHud = nullptr;
bool GameScene::sDebugMode = false;
bool GameScene::sMidnightWarned = false;
bool GameScene::sWasFainted = false;
std::string g_CurrentBgmPath = "";

BackgroundLayer::~BackgroundLayer()
{
    // Clear ForageSystem layer reference if this is Farm
    if (_type == BackgroundType::Farm)
    {
        ForageSystem::getInstance()->detachLayer();
    }
}

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
    _hasHouseRect = false;
    _hasHenhouseRect = false; // Added
    _hasTownHomewayRect = false;
    _hasBucket = false;
    _exitedHomeDoor = false;
    _enteredHome = false;
    _hasRightExit = false;
    _exitedRight = false;
    _hasBoundary = false;
    _isDebugMode = GameScene::sDebugMode;
    _canEnterHomeDoor = false;
    _canExitHomeDoor = false;
    _canEnterTownFromRight = false;
    _canReturnFarmFromTown = false;
    _canEnterHenhouse = false;
    _canEnterTownFromRight = false;
    _canReturnFarmFromTown = false;
    _hasHenhouseDoor = false;
    _canEnterHenhouse = false;
    _enteredHenhouse = false;

    _sleepDialogActive = false;
    _isSleeping = false;

    _backgroundNode = nullptr;
    _seasonOverlay = nullptr;
    _sleepOverlay = nullptr;
    _confirmationOverlay = nullptr;
    _fishingOverlay = nullptr;
    _fishingLabel = nullptr;
    _fishingGame = nullptr;
    _isFishing = false;
    _fishBite = false;
    _fishingElapsed = 0.0f;
    _biteTime = 0.0f;
    _biteWindow = 0.0f;
    
    _waitingForSleepInput = false;
    _waitingForEarningsInput = false;
    _sleepLabel = nullptr;
    _pauseMenu = nullptr;
    
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
                    }
                }

                auto townwayGroup = _map->getObjectGroup("townway");
                if (townwayGroup)
                {
                    const auto& objs = townwayGroup->getObjects();
                    if (!objs.empty())
                    {
                        const auto& dict = objs.front().asValueMap();
                        float tx = dict.at("x").asFloat();
                        float ty = dict.at("y").asFloat();
                        float tw = dict.at("width").asFloat();
                        float th = dict.at("height").asFloat();
                        _rightExitRect = Rect(tx, ty, tw, th);
                        _hasRightExit = true;
                        GameScene::sFarmTownwayPos = Vec2(tx + tw * 0.5f - tileSize.width, ty + th * 0.5f);
                        GameScene::sHasFarmTownwayPos = true;
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

                auto henhouseGroup = _map->getObjectGroup("henhouse");
                if (henhouseGroup)
                {
                    const auto& objs = henhouseGroup->getObjects();
                    bool first = true;
                    float minX = 0.0f;
                    float minY = 0.0f;
                    float maxX = 0.0f;
                    float maxY = 0.0f;

                    for (const auto& obj : objs)
                    {
                        const auto& dict = obj.asValueMap();

                        auto itX = dict.find("x");
                        auto itY = dict.find("y");
                        if (itX == dict.end() || itY == dict.end())
                        {
                            continue;
                        }

                        float hx = itX->second.asFloat();
                        float hy = itY->second.asFloat();

                        float hw = 0.0f;
                        float hh = 0.0f;

                        auto itW = dict.find("width");
                        if (itW != dict.end())
                        {
                            hw = itW->second.asFloat();
                        }
                        auto itH = dict.find("height");
                        if (itH != dict.end())
                        {
                            hh = itH->second.asFloat();
                        }

                        float left = hx;
                        float bottom = hy;
                        float right = hx + hw;
                        float top = hy + hh;

                        if (first)
                        {
                            minX = left;
                            maxX = right;
                            minY = bottom;
                            maxY = top;
                            first = false;
                        }
                        else
                        {
                            if (left < minX) minX = left;
                            if (right > maxX) maxX = right;
                            if (bottom < minY) minY = bottom;
                            if (top > maxY) maxY = top;
                        }
                    }

                    if (!first && maxX > minX && maxY > minY)
                    {
                        _henhouseRect = Rect(minX, minY, maxX - minX, maxY - minY);
                        _hasHenhouseRect = true;
                    }
                }

                auto henhouseOutGroup = _map->getObjectGroup("henhouse out");
                if (henhouseOutGroup)
                {
                    const auto& objs = henhouseOutGroup->getObjects();
                    if (!objs.empty())
                    {
                        const auto& dict = objs.front().asValueMap();
                        float ox = dict.at("x").asFloat();
                        float oy = dict.at("y").asFloat();
                        float ow = dict.at("width").asFloat();
                        float oh = dict.at("height").asFloat();
                        GameScene::sFarmHenhouseOutPos = Vec2(ox + ow * 0.5f, oy + oh * 0.5f);
                        GameScene::sHasFarmHenhouseOutPos = true;
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

                auto henhouseDoorGroup = _map->getObjectGroup("henhouse door");
                if (!henhouseDoorGroup) henhouseDoorGroup = _map->getObjectGroup("Henhouse Door");
                if (!henhouseDoorGroup) henhouseDoorGroup = _map->getObjectGroup("Buildings");
                
                if (henhouseDoorGroup)
                {
                    bool found = false;
                    ValueMap dict;
                    
                    // If strictly "henhouse door" group, take first object
                    std::string groupName = henhouseDoorGroup->getGroupName();
                    if (groupName == "henhouse door" || groupName == "Henhouse Door")
                    {
                        const auto& objs = henhouseDoorGroup->getObjects();
                        if (!objs.empty()) {
                            dict = objs.front().asValueMap();
                            found = true;
                        }
                    }
                    else if (groupName == "Buildings")
                    {
                        // Look for "Hen House" or "Coop"
                         for (const auto& obj : henhouseDoorGroup->getObjects()) {
                            ValueMap d = obj.asValueMap();
                            std::string name = d["name"].asString();
                            if (name == "Hen House" || name == "Coop" || name == "HenHouse") {
                                dict = d;
                                found = true;
                                break;
                            }
                        }
                    }

                    if (found)
                    {
                        float hx = dict.at("x").asFloat();
                        float hy = dict.at("y").asFloat();
                        float hw = dict.at("width").asFloat();
                        float hh = dict.at("height").asFloat();
                        
                        if (groupName == "Buildings")
                        {
                            _henhouseRect = Rect(hx, hy, hw, hh);
                            _hasHenhouseRect = true;

                            _henhouseDoorRect = Rect(hx + hw / 2 - 16, hy, 32, 48);
                            _hasHenhouseDoor = true;
                            GameScene::sFarmHenhouseDoorPos = Vec2(hx + hw * 0.5f, hy + 24);
                            GameScene::sHasFarmHenhouseDoorPos = true;
                        }
                        else
                        {
                            _henhouseDoorRect = Rect(hx, hy, hw, hh);
                            _hasHenhouseDoor = true;
                            GameScene::sFarmHenhouseDoorPos = Vec2(hx + hw * 0.5f, hy + hh * 0.5f);
                            GameScene::sHasFarmHenhouseDoorPos = true;

                            if (!_hasHenhouseRect)
                            {
                                float buildW = tileSize.width * 6.0f;
                                float buildH = tileSize.height * 3.0f;
                                float doorCenterX = hx + hw * 0.5f;
                                float bodyX = doorCenterX - buildW * 0.5f;
                                float bodyY = hy;
                                _henhouseRect = Rect(bodyX, bodyY, buildW, buildH);
                                _hasHenhouseRect = true;
                            }
                        }
                    }
                }

                auto bucketGroup = _map->getObjectGroup("bucket");
                if (bucketGroup)
                {
                    const auto& objs = bucketGroup->getObjects();
                    if (!objs.empty())
                    {
                        const auto& dict = objs.front().asValueMap();
                        float bx = dict.at("x").asFloat();
                        float by = dict.at("y").asFloat();
                        float bw = dict.at("width").asFloat();
                        float bh = dict.at("height").asFloat();
                        _bucketRect = Rect(bx, by, bw, bh);
                        _hasBucket = true;
                    }
                }
        
        CropSystem::getInstance()->init(_map, GameScene::sClock, GameScene::sWallet, GameScene::sInventory);
        // Explicitly set map (init does it, but redundant call is safe)
        CropSystem::getInstance()->setMap(_map);

        initObstacles(); // Init obstacles for Farm type
        ForageSystem::getInstance()->init(this);
        AnimalSystem::getInstance()->init(this, map);
        
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

    if (_type == BackgroundType::Town)
    {
        auto map = TMXTiledMap::create("map/town.tmx");
        if (map)
        {
            map->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);
            map->setPosition(Vec2::ZERO);
            addChild(map, 0);

            _map = map;
            _backgroundNode = map;

            Size mapSizeTiles = map->getMapSize();
            Size tileSize = map->getTileSize();
            float mapWidth = mapSizeTiles.width * tileSize.width;
            float mapHeight = mapSizeTiles.height * tileSize.height;

            _seasonOverlay = LayerColor::create(Color4B(0, 0, 0, 0), mapWidth, mapHeight);
            if (_seasonOverlay)
            {
                addChild(_seasonOverlay, 2);
            }

            _groundLayer = _map->getLayer("ground");
            if (!_groundLayer)
            {
                if (_map->getChildrenCount() > 0)
                {
                    _groundLayer = dynamic_cast<TMXLayer*>(_map->getChildren().at(0));
                }
            }

            auto housesGroup = _map->getObjectGroup("houses");
            if (housesGroup)
            {
                const auto& objs = housesGroup->getObjects();
                _houseRects.clear();
                for (const auto& obj : objs)
                {
                    const auto& dict = obj.asValueMap();
                    float baseX = dict.at("x").asFloat();
                    float baseY = dict.at("y").asFloat();

                    float hw = 0.0f;
                    float hh = 0.0f;
                    auto itW = dict.find("width");
                    if (itW != dict.end())
                    {
                        hw = itW->second.asFloat();
                    }
                    auto itH = dict.find("height");
                    if (itH != dict.end())
                    {
                        hh = itH->second.asFloat();
                    }

                    if (hw > 0.0f && hh > 0.0f)
                    {
                        Rect r(baseX, baseY, hw, hh);
                        _houseRects.push_back(r);
                    }

                    auto itPoints = dict.find("points");
                    if (itPoints != dict.end())
                    {
                        const auto& points = itPoints->second.asValueVector();
                        if (!points.empty())
                        {
                            bool first = true;
                            float minX = 0.0f;
                            float maxX = 0.0f;
                            float minY = 0.0f;
                            float maxY = 0.0f;

                            for (const auto& v : points)
                            {
                                const auto& pDict = v.asValueMap();
                                auto itPX = pDict.find("x");
                                auto itPY = pDict.find("y");
                                if (itPX == pDict.end() || itPY == pDict.end())
                                {
                                    continue;
                                }

                                float localX = itPX->second.asFloat();
                                float localY = itPY->second.asFloat();

                                float worldX = baseX + localX;
                                float worldY = baseY - localY;

                                if (first)
                                {
                                    minX = maxX = worldX;
                                    minY = maxY = worldY;
                                    first = false;
                                }
                                else
                                {
                                    if (worldX < minX) minX = worldX;
                                    if (worldX > maxX) maxX = worldX;
                                    if (worldY < minY) minY = worldY;
                                    if (worldY > maxY) maxY = worldY;
                                }
                            }

                            if (!first && maxX > minX && maxY > minY)
                            {
                                Rect r(minX, minY, maxX - minX, maxY - minY);
                                _houseRects.push_back(r);
                            }
                        }
                    }
                }
                _hasHouseRect = !_houseRects.empty();
            }

            Vec2 spawnPos(mapWidth * 0.5f, mapHeight * 0.5f);
            auto homewayGroup = _map->getObjectGroup("homeway");
            if (homewayGroup)
            {
                const auto& objs = homewayGroup->getObjects();
                if (!objs.empty())
                {
                    const auto& dict = objs.front().asValueMap();
                    float hx = dict.at("x").asFloat();
                    float hy = dict.at("y").asFloat();
                    float hw = dict.at("width").asFloat();
                    float hh = dict.at("height").asFloat();
                    spawnPos = Vec2(hx + hw * 0.5f, hy + hh * 0.5f);
                    _townHomewayRect = Rect(hx, hy, hw, hh);
                    _hasTownHomewayRect = true;
                }
            }

            auto player = Player::create("player.png", tileSize.height);
            if (!player)
            {
                player = Player::create("HelloWorld.png", tileSize.height);
            }
            if (player)
            {
                player->setPosition(spawnPos);
                addChild(player, 1);
                _player = player;

                _facingDebug = DrawNode::create();
                _map->addChild(_facingDebug, 100);

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

                updateSeasonFilter();

                return true;
            }
        }
    }

    if (_type == BackgroundType::Henhouse)
    {
        auto map = TMXTiledMap::create("map/henhouse.tmx");
        if (map)
        {
            map->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);
            map->setPosition(Vec2::ZERO);
            addChild(map, 0);

            _map = map;
            _backgroundNode = map;
            
            // Init Animal System
            AnimalSystem::getInstance()->init(this, map);

            Size mapSizeTiles = map->getMapSize();
            Size tileSize = map->getTileSize();
            float mapWidth = mapSizeTiles.width * tileSize.width;
            float mapHeight = mapSizeTiles.height * tileSize.height;

            auto visibleSize = Director::getInstance()->getVisibleSize();
            Vec2 origin = Director::getInstance()->getVisibleOrigin();
            float offsetX = origin.x + (visibleSize.width - mapWidth) * 0.5f;
            float offsetY = origin.y + (visibleSize.height - mapHeight) * 0.5f;

            setPosition(Vec2(offsetX, offsetY));

            _groundLayer = _map->getLayer("图块层 1");
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
                    _henhouseDoorRect = Rect(dx, dy, dw, dh);
                    _hasHenhouseDoor = true;
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
                if (_hasHenhouseDoor)
                {
                    float spawnX = _henhouseDoorRect.getMidX();
                    float spawnY = _henhouseDoorRect.getMidY() + tileSize.height;
                    spawnPos = Vec2(spawnX, spawnY);
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
                _boundaryTopRect = Rect(0.0f, mapHeight, mapWidth, 0.0f);
                _hasBoundary = false;

                updateSeasonFilter();

                return true;
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
                            GameScene::sStartAtHomeBed = false;
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
    case BackgroundType::Henhouse:
        imageFile = "res/farm_bg.png";
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

    _facingDebug = DrawNode::create();
    addChild(_facingDebug, 100);

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
    std::string bgmPath = "";

    switch (season)
    {
    case GameClock::Season::Spring:
        // Default
        bgmPath = "bgm/Spring.mp3";
        tintColor = Color3B(255, 255, 255);
        overlayColor = Color4B(0, 0, 0, 0);
        break;
    case GameClock::Season::Summer:
        // Brighter / Sunny
        bgmPath = "bgm/Summer.mp3";
        tintColor = Color3B(255, 255, 255);
        // Additive yellow/white for brightness
        overlayColor = Color4B(255, 255, 200, 40); 
        overlayBlend = BlendFunc::ADDITIVE;
        break;
    case GameClock::Season::Fall:
        // Brownish / Autumn / Deep
        // Darker and deeper brown
        bgmPath = "bgm/Fall.mp3";
        tintColor = Color3B(200, 150, 110);
        overlayColor = Color4B(0, 0, 0, 0);
        break;
    case GameClock::Season::Winter:
        // Gray / White / Cold / Dim
        // Lower brightness for "dim" feel
        bgmPath = "bgm/Winter.mp3";
        tintColor = Color3B(180, 180, 190);
        // Normal blending (not additive) to make it look like a white fog/snow cover
        // This will reduce contrast and make it look "whiter" but not "brighter"
        overlayColor = Color4B(220, 230, 240, 50);
        overlayBlend = BlendFunc::ALPHA_NON_PREMULTIPLIED;
        break;
    }

    if (!bgmPath.empty())
    {
        if (g_CurrentBgmPath != bgmPath)
        {
            CocosDenshion::SimpleAudioEngine::getInstance()->playBackgroundMusic(bgmPath.c_str(), true);
            g_CurrentBgmPath = bgmPath;
        }
        else
        {
            if (!CocosDenshion::SimpleAudioEngine::getInstance()->isBackgroundMusicPlaying())
            {
                CocosDenshion::SimpleAudioEngine::getInstance()->playBackgroundMusic(bgmPath.c_str(), true);
            }
        }
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

bool BackgroundLayer::isWater(const cocos2d::Vec2& worldPos)
{
    if (!_hasPoolRect || _poolRects.empty()) return false;
    
    for (const auto& r : _poolRects)
    {
        if (r.containsPoint(worldPos))
        {
            return true;
        }
    }
    return false;
}

void BackgroundLayer::showConfirmationDialog(const std::string& message, std::function<void()> onYes)
{
    if (_confirmationOverlay) return;

    if (_player)
    {
        // Stop player movement to prevent "ghost walking" while dialog is open
        _player->onKeyReleased(EventKeyboard::KeyCode::KEY_W);
        _player->onKeyReleased(EventKeyboard::KeyCode::KEY_S);
        _player->onKeyReleased(EventKeyboard::KeyCode::KEY_A);
        _player->onKeyReleased(EventKeyboard::KeyCode::KEY_D);
        _player->onKeyReleased(EventKeyboard::KeyCode::KEY_UP_ARROW);
        _player->onKeyReleased(EventKeyboard::KeyCode::KEY_DOWN_ARROW);
        _player->onKeyReleased(EventKeyboard::KeyCode::KEY_LEFT_ARROW);
        _player->onKeyReleased(EventKeyboard::KeyCode::KEY_RIGHT_ARROW);
    }

    auto director = Director::getInstance();
    Size visibleSize = director->getVisibleSize();
    Vec2 origin = director->getVisibleOrigin();

    _confirmationOverlay = LayerColor::create(Color4B(0, 0, 0, 160));
    _confirmationOverlay->setContentSize(visibleSize);
    _confirmationOverlay->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);
    
    // Position fixed to screen (add to Scene or Parent, not BackgroundLayer which moves)
    Node* parent = this->getParent();
    if (!parent) parent = this;
    
    if (parent == this)
    {
         // Fallback if no parent (should not happen in game), manual positioning
         Vec2 layerWorldPos = this->getPosition();
         _confirmationOverlay->setPosition(origin - layerWorldPos);
         this->addChild(_confirmationOverlay, 6000);
    }
    else
    {
         // Add to Scene/Parent directly so it doesn't move with camera
         _confirmationOverlay->setPosition(Vec2::ZERO);
         parent->addChild(_confirmationOverlay, 6000);
    }

    auto label = Label::createWithSystemFont(message, "Arial", 32);
    label->setPosition(Vec2(visibleSize.width * 0.5f, visibleSize.height * 0.6f));
    _confirmationOverlay->addChild(label);

    auto yesLabel = Label::createWithSystemFont("Yes", "Arial", 32);
    auto yesItem = MenuItemLabel::create(yesLabel, [this, onYes](Ref* sender) {
        if (onYes) onYes();
        if (this->_confirmationOverlay) {
            this->_confirmationOverlay->removeFromParent();
            this->_confirmationOverlay = nullptr;
        }
    });

    auto noLabel = Label::createWithSystemFont("No", "Arial", 32);
    auto noItem = MenuItemLabel::create(noLabel, [this](Ref* sender) {
        if (this->_confirmationOverlay) {
            this->_confirmationOverlay->removeFromParent();
            this->_confirmationOverlay = nullptr;
        }
    });

    auto menu = Menu::create(yesItem, noItem, nullptr);
    menu->setPosition(Vec2(visibleSize.width * 0.5f, visibleSize.height * 0.4f));
    menu->alignItemsHorizontallyWithPadding(100.0f);
    _confirmationOverlay->addChild(menu);

    // Block touches
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [](Touch* touch, Event* event) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, _confirmationOverlay);

    if (parent == this)
        addChild(_confirmationOverlay, 6000);
}

void BackgroundLayer::update(float dt)
{
    if (PauseLayer::isGamePaused()) return;
    if (_confirmationOverlay) return; // Pause game when confirmation dialog is active

    // Keep sleep overlay centered on screen
    if (_sleepOverlay && _sleepOverlay->isVisible())
    {
        auto director = Director::getInstance();
        Vec2 origin = director->getVisibleOrigin();
        Vec2 layerWorldPos = this->getPosition();
        _sleepOverlay->setPosition(origin - layerWorldPos);
    }
    
    // Time-based checks moved to GameScene::update to ensure they run even if layer is blocked

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
            if (!blockX && _hasHenhouseRect && boxX.intersectsRect(_henhouseRect))
            {
                // Allow entering door zone
                if (!boxX.intersectsRect(_henhouseDoorRect))
                    blockX = true;
            }
            
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
            if (!blockX && _hasHouseRect && !_houseRects.empty())
            {
                for (const auto& r : _houseRects)
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
            if (!blockY && _hasHenhouseRect && boxY.intersectsRect(_henhouseRect))
            {
                if (!boxY.intersectsRect(_henhouseDoorRect))
                    blockY = true;
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
            if (!blockY && _hasHouseRect && !_houseRects.empty())
            {
                for (const auto& r : _houseRects)
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
            if (!blockX && _hasHouseRect && !_houseRects.empty())
            {
                for (const auto& r : _houseRects)
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
            if (!blockY && _hasHouseRect && !_houseRects.empty())
            {
                for (const auto& r : _houseRects)
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

    // Reset per-frame interaction flags
    _canEnterHomeDoor = false;
    _canExitHomeDoor = false;
    _canEnterTownFromRight = false;
    _canReturnFarmFromTown = false;
    _canEnterHenhouse = false;

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
    if (_type == BackgroundType::Farm && _hasRightExit && !_exitedRight && _player)
    {
        Rect box = _player->getBoundingBox();
        if (box.intersectsRect(_rightExitRect))
        {
            _canEnterTownFromRight = true;
        }
    }

    if (_type == BackgroundType::Farm && _hasHenhouseDoor && !_enteredHenhouse && _player)
    {
        Rect box = _player->getBoundingBox();
        if (box.intersectsRect(_henhouseDoorRect))
        {
            _canEnterHenhouse = true;
        }
    }

    if (_type == BackgroundType::Town && _hasTownHomewayRect)
    {
        Rect box = _player->getBoundingBox();
        if (box.intersectsRect(_townHomewayRect))
        {
            _canReturnFarmFromTown = true;
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
    if (_type == BackgroundType::Henhouse && _hasHenhouseDoor && _player)
    {
        Rect box = _player->getBoundingBox();
        if (box.intersectsRect(_henhouseDoorRect))
        {
            _canEnterHenhouse = true;
        }
    }

    if (_facingDebug && _groundLayer)
    {
        _facingDebug->clear();

        // 1. Always draw facing tile cursor (User requirement: always visible)
        if (_groundLayer && _map)
        {
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
                
                if (CropSystem::getInstance()->canHarvest(tileIndex) || 
                    (_type == BackgroundType::Farm && ForageSystem::getInstance()->hasItem(tileIndex)))
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
                                    isValid = CropSystem::getInstance()->canWater(tileIndex) || inPool;
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

            if (_type == BackgroundType::Farm && _hasRightExit)
            {
                Vec2 r1(_rightExitRect.getMinX(), _rightExitRect.getMinY());
                Vec2 r2(_rightExitRect.getMaxX(), _rightExitRect.getMaxY());
                _facingDebug->drawSolidRect(r1, r2, Color4F(1.0f, 1.0f, 0.0f, 0.2f));
                _facingDebug->drawRect(r1, r2, Color4F(1.0f, 1.0f, 0.0f, 1.0f));
            }

            if (_hasBedRect)
            {
                Vec2 r1(_bedRect.getMinX(), _bedRect.getMinY());
                Vec2 r2(_bedRect.getMaxX(), _bedRect.getMaxY());
                _facingDebug->drawSolidRect(r1, r2, Color4F(0.0f, 0.0f, 1.0f, 0.2f)); // Blue for bed
                _facingDebug->drawRect(r1, r2, Color4F(0.0f, 0.0f, 1.0f, 1.0f));
            }

            if (_hasHomeExitDoor)
            {
                Vec2 r1(_homeExitDoorRect.getMinX(), _homeExitDoorRect.getMinY());
                Vec2 r2(_homeExitDoorRect.getMaxX(), _homeExitDoorRect.getMaxY());
                _facingDebug->drawSolidRect(r1, r2, Color4F(0.0f, 1.0f, 0.0f, 0.2f)); // Green for exit
                _facingDebug->drawRect(r1, r2, Color4F(0.0f, 1.0f, 0.0f, 1.0f));
            }

            if (_hasTownHomewayRect)
            {
                Vec2 r1(_townHomewayRect.getMinX(), _townHomewayRect.getMinY());
                Vec2 r2(_townHomewayRect.getMaxX(), _townHomewayRect.getMaxY());
                _facingDebug->drawSolidRect(r1, r2, Color4F(1.0f, 1.0f, 0.0f, 0.2f)); // Yellow for way back
                _facingDebug->drawRect(r1, r2, Color4F(1.0f, 1.0f, 0.0f, 1.0f));
            }

            if (_hasHouseRect && !_houseRects.empty())
            {
                for (const auto& r : _houseRects)
                {
                    Vec2 p1(r.getMinX(), r.getMinY());
                    Vec2 p2(r.getMaxX(), r.getMaxY());
                    _facingDebug->drawSolidRect(p1, p2, Color4F(1.0f, 0.0f, 0.0f, 0.2f)); // Red tint for houses
                    _facingDebug->drawRect(p1, p2, Color4F(1.0f, 0.0f, 0.0f, 1.0f));
                }
            }

            if (_player)
            {
                Rect spriteBox = _player->getBoundingBox();
                float footW = spriteBox.size.width * 0.5f;
                float footH = spriteBox.size.height * 0.25f;
                Rect box(spriteBox.getMidX() - footW * 0.5f, spriteBox.getMinY(), footW, footH);
            
                _facingDebug->drawRect(
                    Vec2(box.getMinX(), box.getMinY()), 
                    Vec2(box.getMaxX(), box.getMaxY()), 
                    Color4F(0.0f, 0.0f, 1.0f, 1.0f)
                );
            }

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

                // Debug draw for Forage Items
                const auto& forageItems = ForageSystem::getInstance()->getItems();
                for (const auto& item : forageItems)
                {
                    // Only draw items for the current map
                    if (item.mapType == _type)
                    {
                        float cx = (item.tilePosition.x + 0.5f) * tileSize.width;
                        float cy = mapHeight - (item.tilePosition.y + 0.5f) * tileSize.height;

                        float shrinkFactor = 0.85f; 
                        float w = tileSize.width * shrinkFactor;
                        float h = tileSize.height * shrinkFactor;

                        Rect itemRect(cx - w * 0.5f, cy - h * 0.5f, w, h);
                    
                        _facingDebug->drawRect(
                            Vec2(itemRect.getMinX(), itemRect.getMinY()),
                            Vec2(itemRect.getMaxX(), itemRect.getMaxY()),
                            Color4F(1.0f, 0.5f, 0.0f, 1.0f) // Orange for forage items
                        );
                    }
                }

                // Debug draw for Henhouse specific layers
                if (_type == BackgroundType::Henhouse)
                {
                    std::vector<std::string> debugLayers = { "door", "bucket", "eggbucket", "feedinghopper" };
                    for (const auto& layerName : debugLayers)
                    {
                        auto group = _map->getObjectGroup(layerName);
                        if (group)
                        {
                            const auto& objects = group->getObjects();
                            for (const auto& obj : objects)
                            {
                                ValueMap dict = obj.asValueMap();
                                float x = dict["x"].asFloat();
                                float y = dict["y"].asFloat();
                                float w = dict["width"].asFloat();
                                float h = dict["height"].asFloat();
                                
                                Vec2 p1(x, y);
                                Vec2 p2(x + w, y + h);
                                
                                // Draw solid rect with low opacity (Magenta)
                                _facingDebug->drawSolidRect(p1, p2, Color4F(1.0f, 0.0f, 1.0f, 0.2f)); 
                                // Draw border
                                _facingDebug->drawRect(p1, p2, Color4F(1.0f, 0.0f, 1.0f, 1.0f));
                            }
                        }
                    }
                }
            }
        }
    }
}

cocos2d::Vec2 BackgroundLayer::getFacingTile() const
{
    if (!_player)
    {
        return Vec2(-1, -1);
    }

    // 1. If we have a TMX map, use tile coordinates
    if (_map)
    {
        Size mapSizeTiles = _map->getMapSize();
        Size tileSize = _map->getTileSize();

        float mapWidth = mapSizeTiles.width * tileSize.width;
        float mapHeight = mapSizeTiles.height * tileSize.height;

        Rect spriteBox = _player->getBoundingBox();
        float footH = spriteBox.size.height * 0.25f;
        Vec2 feetPos(spriteBox.getMidX(), spriteBox.getMinY() + footH * 0.5f);

        float clampedX = std::max(0.0f, std::min(feetPos.x, mapWidth - 1.0f));
        float clampedY = std::max(0.0f, std::min(feetPos.y, mapHeight - 1.0f));

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
    // 2. If no map (static background), calculate pseudo-grid
    // Assuming static backgrounds (Path, Shop) cover the screen or have a main sprite
    // For now, we return (-1, -1) or a "screen grid" if needed.
    // However, the debug drawing relies on this. 
    // Let's implement a virtual grid for static scenes if needed, or just return (-1, -1) and handle debug drawing separately.
    // But wait, the user wants to see collision boxes.
    // Collision boxes (rects) don't rely on tiles. They rely on world coordinates.
    // The tile cursor (yellow dot) relies on tiles.
    // If we want to show a cursor on static scenes, we need a virtual grid.
    
    // For static scenes, let's use a virtual 16x16 grid based on visible size or the background sprite size.
    // Find the background sprite (usually child 0 or by tag)
    // But for now, let's just return (-1, -1) and fix the debug drawing to not crash or skip rects.
    
    return Vec2(-1, -1);
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
    AnimalSystem::getInstance()->cleanupVisuals(this);
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
Vec2 GameScene::sFarmTownwayPos = Vec2::ZERO;
bool GameScene::sHasFarmTownwayPos = false;
Vec2 GameScene::sFarmHenhouseDoorPos = Vec2::ZERO;
bool GameScene::sHasFarmHenhouseDoorPos = false;
Vec2 GameScene::sFarmHenhouseOutPos = Vec2::ZERO;
bool GameScene::sHasFarmHenhouseOutPos = false;
bool GameScene::sSpawnAtFarmStart = false;
bool GameScene::sStartAtHomeBed = false;

Inventory* GameScene::sInventory = nullptr; // Define sInventory
Basket* GameScene::sBasket = nullptr;
int GameScene::sTodayEarnings = 0;

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
    auto next = GameScene::createScene(BackgroundType::Town);
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

    // Default to Home for new game
    GameScene::sStartAtHomeBed = true;
    auto backgroundLayer = BackgroundLayer::create(BackgroundType::Home);
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
    if (!GameScene::sBasket) { GameScene::sBasket = new Basket(); }

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
    CropSystem::getInstance()->setBasket(GameScene::sBasket);
    scheduleUpdate();

    GameScene::sStartAtHomeBed = false;

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
    if (!GameScene::sBasket) { GameScene::sBasket = new Basket(); }

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
    CropSystem::getInstance()->setBasket(GameScene::sBasket);
    scheduleUpdate();

    GameScene::sStartAtHomeBed = false;

    return true;
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
    if (PauseLayer::isGamePaused()) return;

    if (_clock)
    {
        _clock->update(dt);
        ForageSystem::getInstance()->update(_clock);
    }
    CropSystem::getInstance()->updateDailyGrowth();
    AnimalSystem::getInstance()->update(dt); // Update animals

    // Time-based checks (Midnight Warning & 2:00 AM Fainting)
    // Moved here to ensure it runs even if BackgroundLayer is blocked (e.g. by confirmation dialog)
    if (_clock && !GameScene::sWasFainted)
    {
        int hour = _clock->getHour();

        // Midnight Warning (0:00 to 1:50)
        if (hour >= 0 && hour < 2)
        {
            if (!GameScene::sMidnightWarned)
            {
                std::string msg = "It's getting late...";
                Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("SHOW_NOTIFICATION", &msg);
                GameScene::sMidnightWarned = true;
            }
        }
        // 2:00 AM Fainting
        else if (hour == 2)
        {
             std::string msg = "You passed out...";
             Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("SHOW_NOTIFICATION", &msg);
             GameScene::sWasFainted = true;
             
             // Trigger sleep via BackgroundLayer
             for (auto child : getChildren())
             {
                 auto bg = dynamic_cast<BackgroundLayer*>(child);
                 if (bg)
                 {
                     bg->beginSleep();
                     break;
                 }
             }
        }
    }

    // Check for fainting (Energy)
    if (EnergySystem::getInstance()->isExhausted())
    {
        // Set to minimal energy to prevent re-triggering loop, actual restore happens in sleep
        EnergySystem::getInstance()->setEnergy(1.0f);
        
        int fee = 100; // Medical fee
        int currentMoney = _wallet ? _wallet->getMoney() : 0;
        int actualFee = fee;
        
        if (currentMoney < fee)
        {
            actualFee = currentMoney;
            if (_wallet) _wallet->setMoney(0);
        }
        else
        {
            if (_wallet) _wallet->spendMoney(fee);
        }

        // Show notification
        std::string msg = "You fainted! Medical fee: " + std::to_string(actualFee) + "g";
        Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("SHOW_NOTIFICATION", &msg);

        GameScene::sWasFainted = true;

        // Trigger sleep via BackgroundLayer
        for (auto child : getChildren())
        {
            auto bg = dynamic_cast<BackgroundLayer*>(child);
            if (bg)
            {
                bg->beginSleep();
                break;
            }
        }
    }

    if (_hud)
    {
        _hud->refresh();
    }
}

