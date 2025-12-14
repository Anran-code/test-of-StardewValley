#include "HomeScene.h"

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

                auto follow = Follow::create(_player);
                this->runAction(follow);

                setScale(_zoom);
                scheduleUpdate();

                auto listener = EventListenerKeyboard::create();
                listener->onKeyPressed = CC_CALLBACK_2(BackgroundLayer::onKeyPressed, this);
                listener->onKeyReleased = CC_CALLBACK_2(BackgroundLayer::onKeyReleased, this);
                _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

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
        Vec2 pos = _player->getPosition() + delta;

        pos.x = std::max(0.0f, std::min(pos.x, mapWidth));
        pos.y = std::max(0.0f, std::min(pos.y, mapHeight));

        _player->setPosition(pos);
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
    }
}

void BackgroundLayer::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
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

    return true;
}

void HomeScene::onExitClicked(Ref* sender)
{
    Director::getInstance()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    exit(0);
#endif
}
