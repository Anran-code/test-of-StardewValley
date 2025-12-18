#include "HomeScene.h"
#include "GameClock.h"
#include "Wallet.h"
#include "HudLayer.h"
#include "CropSystem.h"
// #include "Basket.h" // Ignore Shop related includes
// #include "ShopLayer.h" // Ignore Shop related includes

USING_NS_CC;

HudLayer* HomeScene::sHud = nullptr;
bool HomeScene::sDebugMode = false;
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
    _hasHomeRect = false;
    _enteredHome = false;
    _hasRightExit = false;
    _exitedRight = false;
    _hasBoundary = false;
    _isDebugMode = false;

    _backgroundNode = nullptr;
    _seasonOverlay = nullptr;
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

            auto player = Player::create("player.png", tileSize.height);
            if (!player)
            {
                player = Player::create("HelloWorld.png", tileSize.height);
            }
            if (player)
            {
                player->setPosition(Vec2(mapWidth * 0.5f, mapHeight * 0.5f));
                addChild(player, 1);

                _player = player;

                _facingDebug = DrawNode::create();
                _map->addChild(_facingDebug, 100);

                if (HomeScene::sHasLastFarmPlayerPos)
                {
                    _player->setPosition(HomeScene::sLastFarmPlayerPos);
                }

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
                        float doorW = tileSize.width;
                        float doorH = tileSize.height * 2.0f;
                        float baseDoorX = _homeRect.getMinX() + (_homeRect.size.width - doorW) * 0.5f + tileSize.width * 1.35f;
                        float baseCenterX = baseDoorX + doorW * 0.5f;
                        float alignedCenterX = std::round(baseCenterX / tileSize.width) * tileSize.width + tileSize.width;
                        float fineOffset = -tileSize.width * 0.40f;
                        float doorX = alignedCenterX - doorW * 0.5f + fineOffset;
                        float doorY = std::floor(_homeRect.getMinY() / tileSize.height) * tileSize.height;
                        _homeDoorRect = Rect(doorX, doorY, doorW, doorH);
                        float extendDown = tileSize.height * 0.3f;
                        _homeDoorTunnelRect = Rect(doorX, doorY - extendDown, doorW, doorH + extendDown);
                        _hasHomeRect = true;
                        float exitW = tileSize.width * 2.0f;
                        float exitH = tileSize.height * 6.0f;
                        float exitX = _boundaryRightRect.getMinX() - exitW;
                        float exitY = (mapHeight - exitH) * 0.5f - tileSize.height * 7.0f;
                        _rightExitRect = Rect(exitX, exitY, exitW, exitH);
                        _hasRightExit = true;
            }
        }
        
        initObstacles(); // Init obstacles for Farm type

        CropSystem::getInstance()->init(_map, HomeScene::sClock, HomeScene::sWallet, HomeScene::sInventory);
        
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
    if (!HomeScene::sClock) return;

    auto season = HomeScene::sClock->getSeason();
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

void BackgroundLayer::update(float dt)
{
    if (HomeScene::sClock && HomeScene::sClock->getSeason() != _lastSeason)
    {
        if (_type == BackgroundType::Farm && _map)
        {
             Size mapSize = _map->getMapSize();
             int initialCount = (mapSize.width * mapSize.height) / 10;
             spawnObstacles(initialCount / 3);
        }
        updateSeasonFilter();
    }

    if (!_map || !_player)
    {
        return;
    }

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
            if (!blockX)
            {
                pos.x += delta.x;
            }
            Rect boxY = box;
            boxY.origin.y += delta.y;
            bool blockY = (_hasBoundary && (boxY.intersectsRect(_boundaryTopRect) || boxY.intersectsRect(_boundaryBottomRect))) || checkCollisionWithObstacles(boxY);
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

    if (_hasHomeRect && !_enteredHome)
    {
        Rect box = _player->getBoundingBox();
        Size tileSize2 = _map->getTileSize();
        float stripH = std::min(tileSize2.height * 0.3f, _homeDoorRect.size.height);
        Rect topStrip(_homeDoorRect.getMinX(), _homeDoorRect.getMaxY() - stripH, _homeDoorRect.size.width, stripH);
        if (box.intersectsRect(topStrip))
        {
            float centerX = (box.origin.x + box.getMaxX()) * 0.5f;
            if (centerX >= _homeDoorRect.getMinX() && centerX <= _homeDoorRect.getMaxX())
            {
                _enteredHome = true;
                HomeScene::sLastFarmPlayerPos = _player->getPosition();
                HomeScene::sHasLastFarmPlayerPos = true;
                auto next = HomeScene::createScene(BackgroundType::Home);
                if (next)
                {
                    auto trans = TransitionMoveInR::create(0.5f, next);
                    Director::getInstance()->replaceScene(trans);
                }
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
                HomeScene::sLastFarmPlayerPos = _player->getPosition();
                HomeScene::sHasLastFarmPlayerPos = true;
                HomeScene::switchViaRightExit(0.5f);
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
            Size tileSize2 = _map->getTileSize();

            Vec2 tilePos = _groundLayer->getPositionAt(tileIndex);

            Vec2 centerPos(tilePos.x + tileSize2.width * 0.5f,
                tilePos.y + tileSize2.height * 0.5f);
            _facingDebug->drawDot(centerPos, 4.0f, Color4F::YELLOW);

            Vec2 p1(tilePos.x, tilePos.y);
            Vec2 p2(tilePos.x + tileSize2.width, tilePos.y + tileSize2.height);

            // Determine if action is valid on this tile
            bool isValid = false;
            
            // 1. Check Harvest (High priority, works with any item)
            if (CropSystem::getInstance()->canHarvest(tileIndex))
            {
                isValid = true;
            }
            else
            {
                // Check Obstacles
                if (hasObstacle(tileIndex))
                {
                    const Item* item = HomeScene::sInventory ? HomeScene::sInventory->getSelectedItem() : nullptr;
                    if (item && item->type == ItemType::Tool)
                    {
                        int obsType = getObstacleType(tileIndex);
                        if (obsType == 0 && item->toolType == ToolType::Axe) isValid = true;      // Wood
                        else if (obsType == 1 && item->toolType == ToolType::Pickaxe) isValid = true; // Stone
                        else if (obsType == 2 && item->toolType == ToolType::Scythe) isValid = true;  // Weed
                    }
                }
                else
                {
                    // Check Tool / Plant (No obstacle)
                    const Item* item = HomeScene::sInventory ? HomeScene::sInventory->getSelectedItem() : nullptr;
                    if (item)
                    {
                        if (item->type == ItemType::Tool)
                        {
                            switch (item->toolType)
                            {
                            case ToolType::Hoe:
                                isValid = CropSystem::getInstance()->canTill(tileIndex);
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

        if (_hasRightExit)
        {
            Vec2 e1(_rightExitRect.getMinX(), _rightExitRect.getMinY());
            Vec2 e2(_rightExitRect.getMaxX(), _rightExitRect.getMaxY());
            _facingDebug->drawSolidRect(e1, e2, Color4F(0.0f, 1.0f, 0.0f, 0.2f));
            _facingDebug->drawRect(e1, e2, Color4F(0.0f, 1.0f, 0.0f, 1.0f));
        }

        // 2. Only draw debug collision boxes if debug mode is ON
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

                    // Match the shrink factor used in checkCollisionWithObstacles
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
    if (_type == BackgroundType::Farm)
    {
        Vec2 tile = getFacingTile();
        switch (keyCode)
        {
        case EventKeyboard::KeyCode::KEY_GRAVE: // Tilde key (~)
            _isDebugMode = !_isDebugMode;
            HomeScene::sDebugMode = _isDebugMode;
            if (HomeScene::sHud) { HomeScene::sHud->updateInventoryUI(); }
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
            if (_isDebugMode && HomeScene::sClock)
            {
                HomeScene::sClock->addDay(1);
                CropSystem::getInstance()->updateDailyGrowth();
            }
            break;
        case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
            if (_isDebugMode && HomeScene::sClock)
            {
                HomeScene::sClock->addDay(-1);
                CropSystem::getInstance()->updateDailyGrowth();
            }
            break;
        case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
            if (_isDebugMode && HomeScene::sClock)
            {
                HomeScene::sClock->addHour(-1);
                CropSystem::getInstance()->updateDailyGrowth();
            }
            break;
        case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
            if (_isDebugMode && HomeScene::sClock)
            {
                HomeScene::sClock->addHour(1);
                CropSystem::getInstance()->updateDailyGrowth();
            }
            break;
        default:
            break;
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

Scene* HomeScene::createScene()
{
    return HomeScene::create();
}

Scene* HomeScene::createScene(BackgroundType type)
{
    auto ret = new (std::nothrow) HomeScene();
    if (ret && ret->initWithStartType(type))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

Scene* HomeScene::make(BackgroundType type)
{
    return HomeScene::createScene(type);
}

GameClock* HomeScene::sClock = nullptr;
Wallet* HomeScene::sWallet = nullptr;
Vec2 HomeScene::sLastFarmPlayerPos = Vec2::ZERO;
bool HomeScene::sHasLastFarmPlayerPos = false;

Inventory* HomeScene::sInventory = nullptr; // Define sInventory

void HomeScene::switchTo(BackgroundType type, float duration)
{
    auto next = HomeScene::createScene(type);
    if (next)
    {
        auto trans = TransitionMoveInR::create(duration, next);
        Director::getInstance()->replaceScene(trans);
    }
}

void HomeScene::switchViaRightExit(float duration)
{
    auto next = HomeScene::createScene(BackgroundType::Path);
    if (next)
    {
        auto trans = TransitionMoveInR::create(duration, next);
        Director::getInstance()->replaceScene(trans);
    }
}

HomeScene::~HomeScene()
{
}

bool HomeScene::init()
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

    if (!HomeScene::sClock) { HomeScene::sClock = new GameClock(); }
    if (!HomeScene::sWallet) { HomeScene::sWallet = new Wallet(); }
    if (!HomeScene::sInventory) { HomeScene::sInventory = new Inventory(); }

    _clock = HomeScene::sClock;
    _wallet = HomeScene::sWallet;
    _inventory = HomeScene::sInventory;

    _hud = HudLayer::create(_clock, _wallet, _inventory);
    if (_hud)
    {
        addChild(_hud, 1000);
        HomeScene::sHud = _hud;
    }
    CropSystem::getInstance()->init(nullptr, _clock, _wallet, _inventory);
    scheduleUpdate();

    return true;
}

bool HomeScene::initWithStartType(BackgroundType type)
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

    if (!HomeScene::sClock) { HomeScene::sClock = new GameClock(); }
    if (!HomeScene::sWallet) { HomeScene::sWallet = new Wallet(); }
    if (!HomeScene::sInventory) { HomeScene::sInventory = new Inventory(); }

    _clock = HomeScene::sClock;
    _wallet = HomeScene::sWallet;
    _inventory = HomeScene::sInventory;

    _hud = HudLayer::create(_clock, _wallet, _inventory);
    if (_hud)
    {
        addChild(_hud, 1000);
        HomeScene::sHud = _hud;
    }
    CropSystem::getInstance()->init(nullptr, _clock, _wallet, _inventory);
    scheduleUpdate();

    return true;
}

// BackgroundLayer implementation
void BackgroundLayer::onMouseDown(Event* event)
{
    if (_type != BackgroundType::Farm) return;
    
    EventMouse* e = (EventMouse*)event;
    if (e->getMouseButton() != EventMouse::MouseButton::BUTTON_LEFT) return;
    Vec2 clickPos = e->getLocation();
    if (HomeScene::sHud && (HomeScene::sHud->isPointInToolbarWorld(clickPos) || HomeScene::sHud->isConsumingClick())) return;

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
    
    if (!HomeScene::sInventory) return;
    const Item* item = HomeScene::sInventory->getSelectedItem();
    
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
            int slot = HomeScene::sInventory->getSelectedSlot();
            HomeScene::sInventory->removeItem(slot, 1);
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

        // Check forbidden zones (Home, Exit, Boundaries)
        if (_hasHomeRect && (_homeRect.intersectsRect(tileRect) || _homeDoorRect.intersectsRect(tileRect) || _homeDoorTunnelRect.intersectsRect(tileRect))) continue;
        if (_hasRightExit && _rightExitRect.intersectsRect(tileRect)) continue;
        
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
        }
    }
}

void BackgroundLayer::initObstacles()
{
    if (!_map) return;
    Size mapSize = _map->getMapSize();
    int obstacleCount = (mapSize.width * mapSize.height) / 10; // 10% density
    spawnObstacles(obstacleCount);
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
        
        // TODO: Add item to inventory (Wood, Stone, Fiber)
    }
}

void HomeScene::onExitClicked(Ref* sender)
{
    Director::getInstance()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    exit(0);
#endif
}

void HomeScene::update(float dt)
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
