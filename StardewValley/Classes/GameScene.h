#ifndef __GAME_SCENE_H__
#define __GAME_SCENE_H__

#include "cocos2d.h"
#include "Player.h"
#include "Inventory.h"
#include "GameClock.h"
#include "FishingMiniGame.h"
#include "PauseLayer.h"
#include <vector>

enum class BackgroundType
{
    Home,
    Farm,
    Path,
    Town,
    Shop,
    Henhouse
};

class BackgroundLayer : public cocos2d::Layer
{
public:
    ~BackgroundLayer(); // 析构函数，清理与当前地图相关的资源
    static BackgroundLayer* create(BackgroundType type);
    virtual bool initWithType(BackgroundType type);

    cocos2d::Vec2 getFacingTile() const;

    // 提供给 ForageSystem 使用的一些辅助接口
    cocos2d::TMXTiledMap* getMap() const { return _map; }
    cocos2d::TMXLayer* getGroundLayer() const { return _groundLayer; }
    BackgroundType getType() const { return _type; }

    bool isValidSpawnPosition(int x, int y);

private:
    void onMouseDown(cocos2d::Event* event); // 鼠标点击地图时的处理（用于交互、商店等）

    // 障碍物管理（木头、石头、杂草等）
    struct Obstacle {
        int type; // 0 表示木头，1 表示石头，2 表示杂草
        cocos2d::Sprite* sprite;
        bool active;
    };
    
    struct ObstacleSaveData {
        int type;
        bool active;
    };
    static std::unordered_map<int, ObstacleSaveData> sSavedObstacles;
    static bool sObstaclesInitialized;
    static int sLastObstacleSeason;

    std::unordered_map<int, Obstacle> _obstacles; // key 使用 tileIndex = y * width + x
    void initObstacles();
    void spawnObstacles(int count); // 在地图上生成指定数量的随机障碍物
    void removeObstacle(const cocos2d::Vec2& tileIndex);
    
public:
    bool hasObstacle(const cocos2d::Vec2& tileIndex);
    int getObstacleType(const cocos2d::Vec2& tileIndex);
    bool tryEatGrass(const cocos2d::Vec2& tileIndex); // 给动物系统调用，用于吃掉草地
    bool isColliding(const cocos2d::Rect& box); // 检查与建筑、墙体或障碍物的碰撞

private:
    bool checkCollisionWithObstacles(const cocos2d::Rect& box);

    BackgroundType _type;

public:
    void showSleepDialog();
    void beginSleep();
    void cancelSleepDialog();
    void startFishing();
    void endFishing(bool success);
    void updateSeasonFilter();
    void showConfirmationDialog(const std::string& message, std::function<void()> onYes);

    bool isWater(const cocos2d::Vec2& worldPos);

    static bool sMidnightWarned;

private:
    cocos2d::TMXTiledMap* _map;
    cocos2d::TMXLayer* _groundLayer;
    Player* _player;
    float _zoom;
    cocos2d::DrawNode* _facingDebug;
    cocos2d::DrawNode* _poolDebug;
    cocos2d::Rect _homeRect;
    cocos2d::Rect _homeDoorRect;
    cocos2d::Rect _homeDoorTunnelRect;
    cocos2d::Rect _homeExitDoorRect;
    cocos2d::Rect _henhouseRect; // 鸡舍建筑的碰撞区域
    cocos2d::Rect _henhouseDoorRect;
    cocos2d::Rect _bedRect;
    std::vector<cocos2d::Rect> _poolRects;
    std::vector<cocos2d::Rect> _houseRects;
    cocos2d::Rect _townHomewayRect;
    cocos2d::Rect _shopRect; // 商店建筑的触发区域
    cocos2d::Rect _rightExitRect;
    cocos2d::Rect _bucketRect;
    cocos2d::Rect _boundaryLeftRect;
    cocos2d::Rect _boundaryRightRect;
    cocos2d::Rect _boundaryTopRect;
    cocos2d::Rect _boundaryBottomRect;
    bool _hasHomeRect;
    bool _hasHomeExitDoor;
    bool _hasBedRect;
    bool _hasPoolRect;
    bool _hasTownHomewayRect;
    bool _hasShopRect; // 是否存在商店区域
    bool _hasHouseRect;
    bool _hasHenhouseRect; // 是否存在鸡舍区域
    bool _hasBucket;
    bool _exitedHomeDoor;
    bool _hasRightExit;
    bool _hasBoundary;
    bool _enteredHome;
    bool _exitedRight;
    bool _isDebugMode;
    bool _canEnterHomeDoor;
    bool _canExitHomeDoor;
    bool _canEnterTownFromRight;
    bool _canReturnFarmFromTown;
    bool _hasHenhouseDoor;
    bool _canEnterHenhouse;
    bool _enteredHenhouse;
    bool _enteredShop; // 是否已经进入商店触发区域

    bool _sleepDialogActive;
    bool _isSleeping;

    // 季节滤镜相关数据（用于根据季节调整颜色和 BGM）
    GameClock::Season _lastSeason;
    cocos2d::Node* _backgroundNode;
    cocos2d::LayerColor* _seasonOverlay;
    cocos2d::LayerColor* _sleepOverlay;
    cocos2d::LayerColor* _confirmationOverlay;
    cocos2d::LayerColor* _fishingOverlay;
    cocos2d::Label* _fishingLabel;
    FishingMiniGame* _fishingGame;
    bool _isFishing;
    bool _fishBite;
    float _fishingElapsed;
    float _biteTime;
    float _biteWindow;
    CropType _currentFishType; // 当前钓上的鱼的类型

    // 睡觉 / 晕倒相关的输入状态
    bool _waitingForSleepInput;
    bool _waitingForEarningsInput;
    cocos2d::Label* _sleepLabel;
    
    // 暂停菜单层（ESC 打开菜单）
    PauseLayer* _pauseMenu;

    bool handleSleepMouseDown(cocos2d::EventMouse* e);

    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
    void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
 
    virtual void update(float dt) override;
    virtual void onExit() override;
    
    // 预留字段，强制触发重新编译用（无实际逻辑）
};

class FarmMapUtils
{
public:
    static cocos2d::Vec2 gridToWorld(const cocos2d::Vec2& gridIndex, cocos2d::Sprite* mapSprite, int cols, int rows);

    static cocos2d::Vec2 worldToGrid(const cocos2d::Vec2& worldPos, cocos2d::Sprite* mapSprite, int cols, int rows);
};

class GameScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();
    static cocos2d::Scene* createScene(BackgroundType type);
    static void switchTo(BackgroundType type, float duration = 0.5f);
    static cocos2d::Scene* make(BackgroundType type);
    static void switchViaRightExit(float duration = 0.5f);
    static cocos2d::Vec2 sLastFarmPlayerPos;
    static bool sHasLastFarmPlayerPos;
    static cocos2d::Vec2 sFarmStartPos;
    static bool sHasFarmStartPos;
    static cocos2d::Vec2 sFarmTownwayPos;
    static bool sHasFarmTownwayPos;
    static cocos2d::Vec2 sFarmHenhouseDoorPos;
    static bool sHasFarmHenhouseDoorPos;
    static cocos2d::Vec2 sFarmHenhouseOutPos;
    static bool sHasFarmHenhouseOutPos;
    static bool sSpawnAtFarmStart;
    static bool sStartAtHomeBed;

    static Inventory* sInventory; // 共享的背包实例，在整个游戏场景中通用
    static class GameClock* sClock;
    static class Wallet* sWallet;
    static class HudLayer* sHud;
    static class Basket* sBasket;
    static bool sDebugMode;
    static bool sMidnightWarned;
    static bool sWasFainted; // 记录玩家当日是否因体力耗尽或太晚而晕倒
    static int sTodayEarnings;

    virtual bool init();
    bool initWithStartType(BackgroundType type);
    virtual ~GameScene();

    void onStartGameClicked(cocos2d::Ref* sender);

    void onExitClicked(cocos2d::Ref* sender);

    CREATE_FUNC(GameScene);

private:
    BackgroundType _startType;
    class GameClock* _clock;
    class Wallet* _wallet;

    class HudLayer* _hud;
    Inventory* _inventory; // 指向当前场景使用的背包实例

    virtual void update(float dt) override;
};

#endif // __GAME_SCENE_H__ 结束
