#ifndef __HOME_SCENE_H__
#define __HOME_SCENE_H__

#include "cocos2d.h"
#include "GameClock.h"
#include "Wallet.h"
#include "Inventory.h"
#include "Basket.h"
#include "CropSystem.h"

class HudLayer;

enum class BackgroundType
{
    Home,
    Farm,
    Town,
    Shop,
    Path
};

class HomeScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene(BackgroundType type = BackgroundType::Farm);
    virtual bool init();
    
    void setStartType(BackgroundType type);
    static void switchTo(BackgroundType type, float duration = 0.5f);
    void switchViaRightExit(float duration);

    void update(float dt) override;
    
    static HudLayer* sHud;
    static bool sDebugMode;
    static GameClock* sClock;
    static Wallet* sWallet;
    static Inventory* sInventory;
    static Basket* sBasket;
    static cocos2d::Vec2 sLastFarmPlayerPos;
    static bool sHasLastFarmPlayerPos;
    
    // Callbacks
    void onStartGameClicked(cocos2d::Ref* sender);
    void onExitClicked(cocos2d::Ref* sender);
    
    CREATE_FUNC(HomeScene);

private:
    BackgroundType _startType;
    HudLayer* _hud;
    GameClock* _clock;
    Wallet* _wallet;
    Inventory* _inventory;
};

class BackgroundLayer : public cocos2d::Layer
{
public:
    static BackgroundLayer* create(BackgroundType type);
    bool initWithType(BackgroundType type);
    void update(float dt) override;
    
    // Input
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
    void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
    void onMouseDown(cocos2d::Event* event);
    
    // Helpers
    cocos2d::Vec2 getFacingTile() const;
    void spawnObstacles(int count);
    void removeObstacle(const cocos2d::Vec2& tileIndex);
    bool hasObstacle(const cocos2d::Vec2& tileIndex);
    int getObstacleType(const cocos2d::Vec2& tileIndex);
    bool checkCollisionWithObstacles(const cocos2d::Rect& box);
    
    void initObstacles();
    void updateSeasonFilter();

private:
    BackgroundType _type;
    cocos2d::TMXTiledMap* _map;
    cocos2d::TMXLayer* _groundLayer;
    class Player* _player;
    cocos2d::LayerColor* _seasonOverlay;
    GameClock::Season _lastSeason;
};

class FarmMapUtils
{
public:
    static cocos2d::Vec2 gridToWorld(const cocos2d::Vec2& gridIndex, cocos2d::Sprite* mapSprite, int cols, int rows);
    static cocos2d::Vec2 worldToGrid(const cocos2d::Vec2& worldPos, cocos2d::Sprite* mapSprite, int cols, int rows);
};

#endif // __HOME_SCENE_H__