#include "HomeScene.h"
#include "GameClock.h"
#include "Wallet.h"
#include "HudLayer.h"
#include "CropSystem.h"
#include "Basket.h"
#include "ShopLayer.h" 

USING_NS_CC;

HudLayer* HomeScene::sHud = nullptr;
bool HomeScene::sDebugMode = false;
GameClock* HomeScene::sClock = nullptr;
Wallet* HomeScene::sWallet = nullptr;
Inventory* HomeScene::sInventory = nullptr;
Basket* HomeScene::sBasket = nullptr;
Vec2 HomeScene::sLastFarmPlayerPos = Vec2::ZERO;
bool HomeScene::sHasLastFarmPlayerPos = false;

Scene* HomeScene::createScene(BackgroundType type)
{
    auto scene = Scene::create();
    auto layer = HomeScene::create();
    layer->setStartType(type);
    scene->addChild(layer);
    return scene;
}

void HomeScene::setStartType(BackgroundType type)
{
    _startType = type;
    // We defer the creation of BackgroundLayer to init or onEnter, 
    // but since init is called first, we might need to handle it there or add it here.
    // However, create() calls init(). So when we are here, init() is done.
    // Let's add the background layer here if it wasn't added in init() properly 
    // or if we want to override.
    // For simplicity, let's remove the default farm layer from init if we want to use this type.
    // But since we are rewriting the file, we can control this logic.
    
    // Clear existing children to be safe if we are switching
    removeAllChildren();

    auto backgroundLayer = BackgroundLayer::create(type);
    if (backgroundLayer)
    {
        addChild(backgroundLayer, 0);
    }
    
    // Re-add HUD
    if (HomeScene::sHud)
    {
        // If HUD is already created and we just want to re-add it?
        // Usually HUD is persistent or recreated.
        // Let's recreate it to be safe or re-add if it has no parent.
        if (HomeScene::sHud->getParent()) HomeScene::sHud->removeFromParent();
        addChild(HomeScene::sHud, 1000);
    }
}

bool HomeScene::init()
{
    if (!Scene::init())
    {
        return false;
    }

    if (!HomeScene::sClock) { HomeScene::sClock = new GameClock(); }
    if (!HomeScene::sWallet) { HomeScene::sWallet = new Wallet(); }
    if (!HomeScene::sInventory) { HomeScene::sInventory = new Inventory(); }
    if (!HomeScene::sBasket) { HomeScene::sBasket = new Basket(); }

    _clock = HomeScene::sClock;
    _wallet = HomeScene::sWallet;
    _inventory = HomeScene::sInventory;

    // HUD
    if (!HomeScene::sHud) {
        _hud = HudLayer::create(_clock, _wallet, _inventory);
        _hud->retain(); // Keep it alive
        HomeScene::sHud = _hud;
    } else {
        _hud = HomeScene::sHud;
    }
    
    // Default Background (will be overridden by setStartType usually)
    // auto backgroundLayer = BackgroundLayer::create(BackgroundType::Farm);
    // addChild(backgroundLayer, 0);
    // addChild(_hud, 1000);
    
    // CropSystem init
    // We need the map for CropSystem init, but map is in BackgroundLayer.
    // BackgroundLayer calls CropSystem init.
    
    CropSystem::getInstance()->setBasket(HomeScene::sBasket);
    
    scheduleUpdate();

    return true;
}

void HomeScene::update(float dt)
{
    if (_clock) _clock->update(dt);
    if (_hud) _hud->refresh();
    CropSystem::getInstance()->updateDailyGrowth();
}

void HomeScene::switchTo(BackgroundType type, float duration)
{
    // Create new scene
    auto scene = HomeScene::createScene(type);
    
    // Transition
    Director::getInstance()->replaceScene(TransitionFade::create(duration, scene, Color3B::BLACK));
}

void HomeScene::switchViaRightExit(float duration)
{
    switchTo(BackgroundType::Town, duration);
}

HomeScene::~HomeScene()
{
    // Clean up if needed
}

// BackgroundLayer Implementation

BackgroundLayer* BackgroundLayer::create(BackgroundType type)
{
    BackgroundLayer *pRet = new(std::nothrow) BackgroundLayer();
    if (pRet && pRet->initWithType(type))
    {
        pRet->autorelease();
        return pRet;
    }
    else
    {
        delete pRet;
        pRet = nullptr;
        return nullptr;
    }
}

bool BackgroundLayer::initWithType(BackgroundType type)
{
    if (!Layer::init()) return false;

    _type = type;
    std::string mapFile = "map/outdoors_spring.tmx"; // Default (Farm)
    if (type == BackgroundType::Home) mapFile = "map/home.tmx";
    else if (type == BackgroundType::Town) mapFile = "map/town.tmx";
    
    // Check if file exists, if not use default
    if (!FileUtils::getInstance()->isFileExist(mapFile)) {
        cocos2d::log("Map file not found: %s, fallback to map/outdoors_spring.tmx", mapFile.c_str());
        mapFile = "map/outdoors_spring.tmx";
    }

    _map = TMXTiledMap::create(mapFile);
    if (!_map) {
        cocos2d::log("Error loading map: %s", mapFile.c_str());
        return false;
    }
    addChild(_map);
    
    _groundLayer = _map->getLayer("Ground");

    // Init Player
    _player = Player::create();
    // Default position
    Vec2 spawnPos = Vec2(100, 100);
    if (type == BackgroundType::Farm && HomeScene::sHasLastFarmPlayerPos) {
        spawnPos = HomeScene::sLastFarmPlayerPos;
    } else {
        // Find spawn point from map objects if available
        auto objectGroup = _map->getObjectGroup("Objects");
        if (objectGroup) {
            auto spawnPoint = objectGroup->getObject("SpawnPoint");
            if (!spawnPoint.empty()) {
                spawnPos = Vec2(spawnPoint["x"].asFloat(), spawnPoint["y"].asFloat());
            }
        }
    }
    _player->setPosition(spawnPos);
    _map->addChild(_player, 10);

    // Init Systems with Map
    bool isFarm = (type == BackgroundType::Farm);
    CropSystem::getInstance()->init(_map, HomeScene::sClock, HomeScene::sWallet, HomeScene::sInventory, isFarm);

    // Obstacles
    if (type == BackgroundType::Farm) {
        initObstacles();
    }

    // Input
    auto listener = EventListenerKeyboard::create();
    listener->onKeyPressed = CC_CALLBACK_2(BackgroundLayer::onKeyPressed, this);
    listener->onKeyReleased = CC_CALLBACK_2(BackgroundLayer::onKeyReleased, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    auto mouseListener = EventListenerMouse::create();
    mouseListener->onMouseDown = CC_CALLBACK_1(BackgroundLayer::onMouseDown, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);

    scheduleUpdate();
    
    // Init Season Filter
    _seasonOverlay = LayerColor::create(Color4B(0, 0, 0, 0));
    addChild(_seasonOverlay, 100);
    updateSeasonFilter();

    return true;
}

void BackgroundLayer::initObstacles()
{
    // Simple obstacle generation for demo
    // Ideally load from map or save file
}

void BackgroundLayer::updateSeasonFilter()
{
    if (!HomeScene::sClock) return;
    auto season = HomeScene::sClock->getSeason();
    if (season == _lastSeason) return;
    _lastSeason = season;

    Color4B color = Color4B(0, 0, 0, 0);
    switch (season) {
        case GameClock::Season::Spring: color = Color4B(0, 255, 0, 20); break;
        case GameClock::Season::Summer: color = Color4B(255, 255, 0, 20); break;
        case GameClock::Season::Fall: color = Color4B(255, 165, 0, 40); break;
        case GameClock::Season::Winter: color = Color4B(200, 200, 255, 50); break;
    }
    _seasonOverlay->setColor(Color3B(color.r, color.g, color.b));
    _seasonOverlay->setOpacity(color.a);
}

void BackgroundLayer::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
    if (!_player) return;

    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_W:
    case EventKeyboard::KeyCode::KEY_UP_ARROW:
        _player->startMove(Vec2(0, 1));
        break;
    case EventKeyboard::KeyCode::KEY_S:
    case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
        _player->startMove(Vec2(0, -1));
        break;
    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
        _player->startMove(Vec2(-1, 0));
        break;
    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        _player->startMove(Vec2(1, 0));
        break;
    case EventKeyboard::KeyCode::KEY_ESCAPE:
        // Exit Game
        Director::getInstance()->end();
        break;
    case EventKeyboard::KeyCode::KEY_T:
    {
        // Open Shop
        if (Director::getInstance()->getRunningScene()->getChildByName("ShopLayer")) return;
        
        auto shop = ShopLayer::create(HomeScene::sWallet, HomeScene::sBasket, CropSystem::getInstance(), HomeScene::sInventory);
        if (shop)
        {
            shop->setName("ShopLayer");
            Director::getInstance()->getRunningScene()->addChild(shop, 2000);
        }
        break;
    }
    case EventKeyboard::KeyCode::KEY_X:
        // Sell Basket Content if at Top Right of Farm
        if (_type == BackgroundType::Farm && _map)
        {
            Size mapSize = _map->getMapSize();
            Size tileSize = _map->getTileSize();
            float mapW = mapSize.width * tileSize.width;
            float mapH = mapSize.height * tileSize.height;
            // Define Basket Area (Top Right Corner, say 3x3 tiles area)
            Rect basketRect(mapW - tileSize.width * 4, mapH - tileSize.height * 4, tileSize.width * 4, tileSize.height * 4);
            Rect playerBox = _player->getBoundingBox();
            
            // Adjust player box to world space if needed (player is child of map)
            // Map is at (0,0) usually unless scrolled.
            // If map scrolls, player position is relative to map. basketRect is relative to map.
            // So comparison is valid.
            
            if (playerBox.intersectsRect(basketRect))
            {
                int sold = CropSystem::getInstance()->sellBasket();
                
                // Visual feedback
                std::string msg = (sold > 0) ? "Sold! +" + std::to_string(sold) : "Basket Empty";
                auto label = Label::createWithSystemFont(msg, "Arial", 24);
                label->setPosition(_player->getPosition() + Vec2(0, 60));
                label->setColor(Color3B::YELLOW);
                label->runAction(Sequence::create(MoveBy::create(1.0f, Vec2(0, 50)), FadeOut::create(0.5f), RemoveSelf::create(), nullptr));
                _map->addChild(label, 2000);
            }
        }
        break;
    }
}

void BackgroundLayer::onKeyReleased(EventKeyboard::KeyCode keyCode, Event* event)
{
    if (!_player) return;
    
    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_W:
    case EventKeyboard::KeyCode::KEY_UP_ARROW:
    case EventKeyboard::KeyCode::KEY_S:
    case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
        _player->stopMove(true); // Stop Y
        break;
    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        _player->stopMove(false); // Stop X
        break;
    }
}

void BackgroundLayer::onMouseDown(Event* event)
{
    // Prevent interaction if Shop is open
    if (Director::getInstance()->getRunningScene()->getChildByName("ShopLayer")) return;

    EventMouse* e = (EventMouse*)event;
    if (e->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT)
    {
        // Convert screen to map space
        Vec2 clickPos = e->getLocationInView();
        clickPos = Director::getInstance()->convertToGL(clickPos);
        
        // Adjust for map position (scrolling)
        Vec2 nodePos = _map->convertToNodeSpace(clickPos);
        
        Size tileSize = _map->getTileSize();
        Size mapSize = _map->getMapSize();
        
        int x = nodePos.x / tileSize.width;
        int y = mapSize.height - (nodePos.y / tileSize.height) - 1; // TMX coords are inverted Y
        
        // Handle Crop interaction
        CropSystem::getInstance()->onTileClicked(Vec2(x, y));
        
        // Handle Obstacle removal (if we had logic)
    }
}

void BackgroundLayer::update(float dt)
{
    if (_player)
    {
        _player->update(dt);
        
        // Basic map boundary check
        Vec2 pos = _player->getPosition();
        Size mapSize = _map->getMapSize();
        Size tileSize = _map->getTileSize();
        float mapW = mapSize.width * tileSize.width;
        float mapH = mapSize.height * tileSize.height;
        
        // Scene Switching Logic
        if (_type == BackgroundType::Farm)
        {
            // Right Exit -> Town
            if (pos.x > mapW - 10)
            {
                // Save position but offset it left so we don't immediately re-trigger exit upon return
                HomeScene::sLastFarmPlayerPos = pos - Vec2(50, 0); 
                HomeScene::sHasLastFarmPlayerPos = true;
                HomeScene::switchTo(BackgroundType::Town);
                return; // Stop update to avoid multiple calls
            }
        }
        else if (_type == BackgroundType::Town)
        {
            // Left Exit -> Farm
            if (pos.x < 10)
            {
                HomeScene::switchTo(BackgroundType::Farm);
                return;
            }
            
            // Shop Entrance Logic
            bool enteredShop = false;
            
            // Debug Log for Player Pos in Town
            // static int logCounter = 0;
            // if (logCounter++ % 60 == 0) cocos2d::log("Player Town Pos: (%f, %f)", pos.x, pos.y);

            // 1. Try "shop_door" object group (from TMX)
            if (_map)
            {
                // Try "shop_door" (lowercase as seen in TMX)
                auto objectGroup = _map->getObjectGroup("shop_door");
                if (objectGroup)
                {
                    auto objects = objectGroup->getObjects();
                    if (!objects.empty())
                    {
                        auto shopObj = objects[0].asValueMap();
                        float x = shopObj["x"].asFloat();
                        float y = shopObj["y"].asFloat();
                        float w = shopObj["width"].asFloat();
                        float h = shopObj["height"].asFloat();
                        
                        // Fix Y coordinate: TMX y is from top, Cocos is from bottom.
                        // Map Height in pixels
                        Size mapSize = _map->getMapSize();
                        Size tileSize = _map->getTileSize();
                        float mapPixelH = mapSize.height * tileSize.height;
                        
                        // TMX Rect (x, y) is Top-Left. 
                        // Cocos Rect (x, y) is Bottom-Left.
                        // So cocosY = mapPixelH - y - h. (Wait, verify if Cocos auto-converts? 
                        // Usually TMXTiledMap does NOT auto-convert objects coordinates to Cocos space fully unless used in specific ways.
                        // But let's assume we need to flip.)
                        
                        // Let's broaden the search area significantly to ensure it works.
                        float centerX = x + w / 2;
                        float centerY_flipped = mapPixelH - y - h / 2;
                        
                        // Create a hit box around center
                        Rect shopRect(centerX - 40, centerY_flipped - 40, 80, 80);
                        
                        if (shopRect.containsPoint(pos))
                        {
                            enteredShop = true;
                        }
                    }
                }
            }

            // Fallback: Fixed Rect if object layer missing or failed
            // Based on TMX inspection: x=608, y=320 (Top-Down).
            // Map Height = 60 * 16 = 960.
            // Y (Bottom-Up) = 960 - 320 = 640.
            // So target is around (608, 640).
            if (!enteredShop)
            {
                 Rect mockShop(580, 580, 100, 120); // Center around (630, 640)
                 if (mockShop.containsPoint(pos))
                 {
                     enteredShop = true;
                 }
            }

            if (enteredShop)
             {
                  // Open Shop Layer
                  if (!Director::getInstance()->getRunningScene()->getChildByName("ShopLayer"))
                  {
                      _player->stopMove(true);
                      _player->stopMove(false);
                      
                      auto shop = ShopLayer::create(HomeScene::sWallet, HomeScene::sBasket, CropSystem::getInstance(), HomeScene::sInventory);
                      if (shop)
                      {
                          shop->setName("ShopLayer");
                          Director::getInstance()->getRunningScene()->addChild(shop, 2000);
                          
                          // Move player back a bit so they don't re-trigger immediately on close
                          // Shop Rect is roughly 80x80 or 100x120. Half height is 40-60.
                          // Move back 60 pixels to be safe.
                          _player->setPosition(pos + Vec2(0, -60)); 
                      }
                  }
             }
        }
        
        if (pos.x < 0) pos.x = 0;
        if (pos.x > mapW) pos.x = mapW;
        if (pos.y < 0) pos.y = 0;
        if (pos.y > mapH) pos.y = mapH;
        
        _player->setPosition(pos);
        
        // Camera follow
        auto winSize = Director::getInstance()->getWinSize();
        float viewX = winSize.width / 2 - pos.x;
        float viewY = winSize.height / 2 - pos.y;
        
        // Clamp camera
        if (viewX > 0) viewX = 0;
        if (viewX < winSize.width - mapW) viewX = winSize.width - mapW;
        if (viewY > 0) viewY = 0;
        if (viewY < winSize.height - mapH) viewY = winSize.height - mapH;
        
        _map->setPosition(Vec2(viewX, viewY));
    }
    
    updateSeasonFilter();
}

// Stub methods for missing implementations
cocos2d::Vec2 BackgroundLayer::getFacingTile() const { return Vec2::ZERO; }
void BackgroundLayer::spawnObstacles(int count) {}
void BackgroundLayer::removeObstacle(const cocos2d::Vec2& tileIndex) {}
bool BackgroundLayer::hasObstacle(const cocos2d::Vec2& tileIndex) { return false; }
int BackgroundLayer::getObstacleType(const cocos2d::Vec2& tileIndex) { return 0; }
bool BackgroundLayer::checkCollisionWithObstacles(const cocos2d::Rect& box) { return false; }
cocos2d::Vec2 FarmMapUtils::gridToWorld(const cocos2d::Vec2& gridIndex, cocos2d::Sprite* mapSprite, int cols, int rows) { return Vec2::ZERO; }
cocos2d::Vec2 FarmMapUtils::worldToGrid(const cocos2d::Vec2& worldPos, cocos2d::Sprite* mapSprite, int cols, int rows) { return Vec2::ZERO; }
void HomeScene::onStartGameClicked(cocos2d::Ref* sender) {}
void HomeScene::onExitClicked(cocos2d::Ref* sender) {}
