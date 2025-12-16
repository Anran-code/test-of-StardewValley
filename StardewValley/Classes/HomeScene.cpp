#include "HomeScene.h"
#include "GameClock.h"
#include "Wallet.h"
#include "HudLayer.h"
#include "CropSystem.h"

USING_NS_CC;

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

    if (_type == BackgroundType::Farm)
    {
        auto map = TMXTiledMap::create("map/outdoors_spring.tmx");
        if (map)
        {
            map->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);
            map->setPosition(Vec2::ZERO);
            addChild(map, 0);

            _map = map;
            _groundLayer = _map->getLayer("ground");
            if (!_groundLayer)
            {
                if (_map->getChildrenCount() > 0)
                {
                    _groundLayer = dynamic_cast<TMXLayer*>(_map->getChildren().at(0));
                }
            }

            Size mapSizeTiles = map->getMapSize();
            Size tileSize = map->getTileSize();

            auto player = Player::create("player.png", tileSize.height);
            if (!player)
            {
                player = Player::create("HelloWorld.png", tileSize.height);
            }
            if (player)
            {
                float mapWidth = mapSizeTiles.width * tileSize.width;
                float mapHeight = mapSizeTiles.height * tileSize.height;

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
                        float exitW = tileSize.width;
                        float exitH = tileSize.height * 4.0f;
                        float exitX = _boundaryRightRect.getMinX() - exitW;
                        float exitY = (mapHeight - exitH) * 0.5f;
                        _rightExitRect = Rect(exitX, exitY, exitW, exitH);
                        _hasRightExit = true;
                    }
                }

                CropSystem::getInstance()->init(_map, nullptr, nullptr);
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

    return true;
}

void BackgroundLayer::update(float dt)
{
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
        if (_hasHomeRect)
        {
            Rect box = _player->getBoundingBox();
            Rect boxX = box;
            boxX.origin.x += delta.x;
            bool blockX = boxX.intersectsRect(_homeRect) || (_hasBoundary && (boxX.intersectsRect(_boundaryLeftRect) || boxX.intersectsRect(_boundaryRightRect)));
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
            if (!blockY)
            {
                pos.y += delta.y;
            }
        }
        else
        {
            Rect box = _player->getBoundingBox();
            Rect boxX = box;
            boxX.origin.x += delta.x;
            bool blockX = _hasBoundary && (boxX.intersectsRect(_boundaryLeftRect) || boxX.intersectsRect(_boundaryRightRect));
            if (!blockX)
            {
                pos.x += delta.x;
            }
            Rect boxY = box;
            boxY.origin.y += delta.y;
            bool blockY = _hasBoundary && (boxY.intersectsRect(_boundaryTopRect) || boxY.intersectsRect(_boundaryBottomRect));
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

            _facingDebug->drawSolidRect(p1, p2, Color4F(1.0f, 0.0f, 0.3f, 0.3f));
            _facingDebug->drawRect(p1, p2, Color4F(1.0f, 1.0f, 1.0f, 1.0f));
        }
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
        if (_hasRightExit)
        {
            Vec2 e1(_rightExitRect.getMinX(), _rightExitRect.getMinY());
            Vec2 e2(_rightExitRect.getMaxX(), _rightExitRect.getMaxY());
            _facingDebug->drawSolidRect(e1, e2, Color4F(0.0f, 1.0f, 0.0f, 0.2f));
            _facingDebug->drawRect(e1, e2, Color4F(0.0f, 1.0f, 0.0f, 1.0f));
        }
        if (_hasBoundary)
        {
            _facingDebug->drawRect(Vec2(_boundaryLeftRect.getMinX(), _boundaryLeftRect.getMinY()), Vec2(_boundaryLeftRect.getMaxX(), _boundaryLeftRect.getMaxY()), Color4F(1.0f, 0.0f, 0.0f, 1.0f));
            _facingDebug->drawRect(Vec2(_boundaryRightRect.getMinX(), _boundaryRightRect.getMinY()), Vec2(_boundaryRightRect.getMaxX(), _boundaryRightRect.getMaxY()), Color4F(1.0f, 0.0f, 0.0f, 1.0f));
            _facingDebug->drawRect(Vec2(_boundaryTopRect.getMinX(), _boundaryTopRect.getMinY()), Vec2(_boundaryTopRect.getMaxX(), _boundaryTopRect.getMaxY()), Color4F(1.0f, 0.0f, 0.0f, 1.0f));
            _facingDebug->drawRect(Vec2(_boundaryBottomRect.getMinX(), _boundaryBottomRect.getMinY()), Vec2(_boundaryBottomRect.getMaxX(), _boundaryBottomRect.getMaxY()), Color4F(1.0f, 0.0f, 0.0f, 1.0f));
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
        case EventKeyboard::KeyCode::KEY_1:
            CropSystem::getInstance()->setSelectedCrop(CropType::Parsnip);
            break;
        case EventKeyboard::KeyCode::KEY_2:
            CropSystem::getInstance()->setSelectedCrop(CropType::Cauliflower);
            break;
        case EventKeyboard::KeyCode::KEY_3:
            CropSystem::getInstance()->setSelectedCrop(CropType::Potato);
            break;
        case EventKeyboard::KeyCode::KEY_E:
            CropSystem::getInstance()->tillTile(tile);
            break;
        case EventKeyboard::KeyCode::KEY_F:
            CropSystem::getInstance()->plantSelected(tile);
            break;
        case EventKeyboard::KeyCode::KEY_G:
            CropSystem::getInstance()->waterTile(tile);
            break;
        case EventKeyboard::KeyCode::KEY_H:
            CropSystem::getInstance()->harvestTile(tile);
            break;
        default:
            break;
        }
    }
    if (_player)
    {
        _player->onKeyPressed(keyCode);
    }
}

void BackgroundLayer::onKeyReleased(EventKeyboard::KeyCode keyCode, Event* event)
{
    if (_player)
    {
        _player->onKeyReleased(keyCode);
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

    Vec2 pos = _player->getPosition();

    float clampedX = std::max(0.0f, std::min(pos.x, mapWidth - 1.0f));
    float clampedY = std::max(0.0f, std::min(pos.y, mapHeight - 1.0f));

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
    _clock = HomeScene::sClock;
    _wallet = HomeScene::sWallet;
    _hud = HudLayer::create(_clock, _wallet);
    if (_hud)
    {
        addChild(_hud, 1000);
    }
    CropSystem::getInstance()->init(nullptr, _clock, _wallet);
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
    _clock = HomeScene::sClock;
    _wallet = HomeScene::sWallet;
    _hud = HudLayer::create(_clock, _wallet);
    if (_hud)
    {
        addChild(_hud, 1000);
    }
    CropSystem::getInstance()->init(nullptr, _clock, _wallet);
    scheduleUpdate();

    return true;
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
