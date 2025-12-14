#ifndef __HOME_SCENE_H__
#define __HOME_SCENE_H__

#include "cocos2d.h"
#include "Player.h"

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
    BackgroundType _type;
    cocos2d::TMXTiledMap* _map;
    cocos2d::TMXLayer* _groundLayer;
    Player* _player;
    float _zoom;
    cocos2d::DrawNode* _facingDebug;

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

    virtual bool init();

    void onStartGameClicked(cocos2d::Ref* sender);

    void onExitClicked(cocos2d::Ref* sender);

    CREATE_FUNC(HomeScene);
};

#endif // __HOME_SCENE_H__
