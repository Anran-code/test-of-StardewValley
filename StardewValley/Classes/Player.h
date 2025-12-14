#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "cocos2d.h"

class Player : public cocos2d::Sprite
{
public:
    static Player* create(const std::string& filename, float tileHeight);

    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode);
    void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode);

    cocos2d::Vec2 getMoveVelocity() const;
    cocos2d::Vec2 getFacingOffset() const;

private:
    float _speed;
    cocos2d::Vec2 _moveDir;
    bool _movingUp;
    bool _movingDown;
    bool _movingLeft;
    bool _movingRight;

    enum class FacingDirection
    {
        Down,
        Left,
        Right,
        Up
    };

    FacingDirection _facing;
};

#endif

