#include "Player.h"

USING_NS_CC;

Player* Player::create(const std::string& filename, float tileHeight)
{
    Player* ret = new (std::nothrow) Player();
    if (ret && ret->initWithFile("player.png"))
    {
        ret->autorelease();

        ret->_speed = 200.0f;
        ret->_moveDir = Vec2::ZERO;
        ret->_movingUp = false;
        ret->_movingDown = false;
        ret->_movingLeft = false;
        ret->_movingRight = false;
        ret->_facing = FacingDirection::Down;

        Size size = ret->getContentSize();
        if (size.height > 0.0f && tileHeight > 0.0f)
        {
            float desiredHeight = tileHeight * 3.0f;
            float scale = desiredHeight / size.height;
            if (scale < 0.5f)
            {
                scale = 0.5f;
            }
            ret->setScale(scale);
        }

        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

void Player::onKeyPressed(EventKeyboard::KeyCode keyCode)
{
    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_W:
    case EventKeyboard::KeyCode::KEY_UP_ARROW:
        _movingUp = true;
        _facing = FacingDirection::Up;
        break;
    case EventKeyboard::KeyCode::KEY_S:
    case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
        _movingDown = true;
        _facing = FacingDirection::Down;
        break;
    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
        _movingLeft = true;
        _facing = FacingDirection::Left;
        break;
    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        _movingRight = true;
        _facing = FacingDirection::Right;
        break;
    default:
        break;
    }
}

void Player::onKeyReleased(EventKeyboard::KeyCode keyCode)
{
    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_W:
    case EventKeyboard::KeyCode::KEY_UP_ARROW:
        _movingUp = false;
        break;
    case EventKeyboard::KeyCode::KEY_S:
    case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
        _movingDown = false;
        break;
    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
        _movingLeft = false;
        break;
    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        _movingRight = false;
        break;
    default:
        break;
    }
}

Vec2 Player::getMoveVelocity() const
{
    Vec2 dir(
        (_movingRight ? 1.0f : 0.0f) - (_movingLeft ? 1.0f : 0.0f),
        (_movingUp ? 1.0f : 0.0f) - (_movingDown ? 1.0f : 0.0f));
    if (dir.lengthSquared() == 0.0f)
    {
        return Vec2::ZERO;
    }
    return dir.getNormalized() * _speed;
}

Vec2 Player::getFacingOffset() const
{
    switch (_facing)
    {
    case FacingDirection::Up:
        return Vec2(0.0f, -1.0f);
    case FacingDirection::Down:
        return Vec2(0.0f, 1.0f);
    case FacingDirection::Left:
        return Vec2(-1.0f, 0.0f);
    case FacingDirection::Right:
        return Vec2(1.0f, 0.0f);
    }
    return Vec2::ZERO;
}
