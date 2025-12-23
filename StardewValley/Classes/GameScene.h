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
    ~BackgroundLayer(); // Destructor
    static BackgroundLayer* create(BackgroundType type);
    virtual bool initWithType(BackgroundType type);

    cocos2d::Vec2 getFacingTile() const;

    // Helper for ForageSystem
    cocos2d::TMXTiledMap* getMap() const { return _map; }
    cocos2d::TMXLayer* getGroundLayer() const { return _groundLayer; }
    BackgroundType getType() const { return _type; }

    bool isValidSpawnPosition(int x, int y);

private:
    void onMouseDown(cocos2d::Event* event); // New mouse handling

    // Obstacle management
    struct Obstacle {
        int type; // 0: Wood, 1: Stone, 2: Weed
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

    std::unordered_map<int, Obstacle> _obstacles; // Key: tileIndex y * width + x
    void initObstacles();
    void spawnObstacles(int count); // Spawn a specific number of obstacles
    void removeObstacle(const cocos2d::Vec2& tileIndex);
    
public:
    bool hasObstacle(const cocos2d::Vec2& tileIndex);
    int getObstacleType(const cocos2d::Vec2& tileIndex);
    bool tryEatGrass(const cocos2d::Vec2& tileIndex); // For Animals
    bool isColliding(const cocos2d::Rect& box); // Check collisions with buildings/walls/obstacles

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
    cocos2d::Rect _henhouseRect; // Added: collision for henhouse building
    cocos2d::Rect _henhouseDoorRect;
    cocos2d::Rect _bedRect;
    std::vector<cocos2d::Rect> _poolRects;
    std::vector<cocos2d::Rect> _houseRects;
    cocos2d::Rect _townHomewayRect;
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
    bool _hasHouseRect;
    bool _hasHenhouseRect; // Added
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

    bool _sleepDialogActive;
    bool _isSleeping;

    // Seasonal filter support
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
    CropType _currentFishType; // Track which fish is on the line

    // Sleep/Faint input handling
    bool _waitingForSleepInput;
    cocos2d::Label* _sleepLabel;
    
    // Pause Menu
    PauseLayer* _pauseMenu;

    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
    void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
 
    virtual void update(float dt) override;
    virtual void onExit() override;
    
    // Force rebuild
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
    static bool sSpawnAtFarmStart;
    static bool sStartAtHomeBed;

    static Inventory* sInventory; // Shared inventory
    static class GameClock* sClock;
    static class Wallet* sWallet;
    static class HudLayer* sHud;
    static bool sDebugMode;
    static bool sMidnightWarned;
    static bool sWasFainted; // Track if player fainted (exhaustion or late night)

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
    static class Basket* sBasket;
    
    Inventory* _inventory; // Member inventory pointer

    virtual void update(float dt) override;
};

#endif // __GAME_SCENE_H__
