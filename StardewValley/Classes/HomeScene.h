#ifndef __HOME_SCENE_H__
#define __HOME_SCENE_H__

#include "cocos2d.h"
#include "Player.h"
#include "Inventory.h"

enum class BackgroundType
{
    Home,
    Farm,
    Path,
    Town,
    Shop
};

class BackgroundLayer : public cocos2d::Layer
{
public:
    static BackgroundLayer* create(BackgroundType type);

    bool initWithType(BackgroundType type);

    cocos2d::Vec2 getFacingTile() const;

private:
    void handleToolUse();
    void onMouseDown(cocos2d::Event* event); // New mouse handling

    // Obstacle management
    struct Obstacle {
        int type; // 0: Wood, 1: Stone, 2: Weed
        cocos2d::Sprite* sprite;
        bool active;
    };
    std::unordered_map<int, Obstacle> _obstacles; // Key: tileIndex y * width + x
    void initObstacles();
    void removeObstacle(const cocos2d::Vec2& tileIndex);
    bool hasObstacle(const cocos2d::Vec2& tileIndex);
    int getObstacleType(const cocos2d::Vec2& tileIndex);

    BackgroundType _type;
    cocos2d::TMXTiledMap* _map;
    cocos2d::TMXLayer* _groundLayer;
    Player* _player;
    float _zoom;
    cocos2d::DrawNode* _facingDebug;
    cocos2d::Rect _homeRect;
    cocos2d::Rect _homeDoorRect;
    cocos2d::Rect _homeDoorTunnelRect;
    cocos2d::Rect _rightExitRect;
    cocos2d::Rect _boundaryLeftRect;
    cocos2d::Rect _boundaryRightRect;
    cocos2d::Rect _boundaryTopRect;
    cocos2d::Rect _boundaryBottomRect;
    bool _hasHomeRect;
    bool _hasRightExit;
    bool _hasBoundary;
    bool _enteredHome;
    bool _exitedRight;
 
    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
    void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
 
    virtual void update(float dt) override;
};

class FarmMapUtils
{
public:
    static cocos2d::Vec2 gridToWorld(const cocos2d::Vec2& gridIndex, cocos2d::Sprite* mapSprite, int cols, int rows);

    static cocos2d::Vec2 worldToGrid(const cocos2d::Vec2& worldPos, cocos2d::Sprite* mapSprite, int cols, int rows);
};

class HomeScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();
    static cocos2d::Scene* createScene(BackgroundType type);
    static void switchTo(BackgroundType type, float duration = 0.5f);
    static cocos2d::Scene* make(BackgroundType type);
    static void switchViaRightExit(float duration = 0.5f);
    static cocos2d::Vec2 sLastFarmPlayerPos;
    static bool sHasLastFarmPlayerPos;

    static Inventory* sInventory; // Shared inventory

    virtual bool init();
    bool initWithStartType(BackgroundType type);
    virtual ~HomeScene();

    void onStartGameClicked(cocos2d::Ref* sender);

    void onExitClicked(cocos2d::Ref* sender);

    CREATE_FUNC(HomeScene);

private:
    BackgroundType _startType;
    class GameClock* _clock;
    class Wallet* _wallet;

    class HudLayer* _hud;
    static class GameClock* sClock;
    static class Wallet* sWallet;
    static class Basket* sBasket;

    virtual void update(float dt) override;
};

#endif // __HOME_SCENE_H__
