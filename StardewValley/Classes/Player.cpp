#include "Player.h"

USING_NS_CC;

Player::~Player()
{
    CC_SAFE_RELEASE(_walkUpAction);
    CC_SAFE_RELEASE(_walkDownAction);
    CC_SAFE_RELEASE(_walkLeftAction);
    CC_SAFE_RELEASE(_walkRightAction);
}

Player* Player::create(const std::string& filename, float tileHeight)
{
    Player* ret = new (std::nothrow) Player();
    // Default to front standing frame
    if (ret && ret->initWithFile("player/Player_front_1.png"))
    {
        ret->autorelease();

        ret->_speed = 200.0f;
        ret->_moveDir = Vec2::ZERO;
        ret->_movingUp = false;
        ret->_movingDown = false;
        ret->_movingLeft = false;
        ret->_movingRight = false;
        ret->_facing = FacingDirection::Down;
        
        ret->_walkUpAction = nullptr;
        ret->_walkDownAction = nullptr;
        ret->_walkLeftAction = nullptr;
        ret->_walkRightAction = nullptr;
        ret->_currentAction = nullptr;

        // Create Animations
        // Front (Down)
        auto animDown = ret->createAnimation("player/Player_front_", 1, 4, 0.15f);
        ret->_walkDownAction = RepeatForever::create(Animate::create(animDown));
        ret->_walkDownAction->retain();

        // Back (Up)
        auto animUp = ret->createAnimation("player/Player_back_", 1, 4, 0.15f);
        ret->_walkUpAction = RepeatForever::create(Animate::create(animUp));
        ret->_walkUpAction->retain();

        // Left
        auto animLeft = ret->createAnimation("player/Player_left_", 1, 4, 0.15f);
        ret->_walkLeftAction = RepeatForever::create(Animate::create(animLeft));
        ret->_walkLeftAction->retain();

        // Right
        auto animRight = ret->createAnimation("player/Player_right_", 1, 4, 0.15f);
        ret->_walkRightAction = RepeatForever::create(Animate::create(animRight));
        ret->_walkRightAction->retain();

        Size size = ret->getContentSize();
        if (size.height > 0.0f && tileHeight > 0.0f)
        {
            float desiredHeight = tileHeight * 2.0f;
            float scale = desiredHeight / size.height;
            if (scale < 0.3f)
            {
                scale = 0.3f;
            }
            ret->setScale(scale);
        }

        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

Animation* Player::createAnimation(std::string prefix, int start, int end, float delay)
{
    Vector<SpriteFrame*> animFrames;
    for (int i = start; i <= end; i++)
    {
        std::string frameName = prefix + std::to_string(i) + ".png";
        auto texture = Director::getInstance()->getTextureCache()->addImage(frameName);
        if (texture)
        {
            auto frame = SpriteFrame::createWithTexture(texture, Rect(0, 0, texture->getContentSize().width, texture->getContentSize().height));
            animFrames.pushBack(frame);
        }
    }
    return Animation::createWithSpriteFrames(animFrames, delay);
}

void Player::updateAnimationState()
{
    Vec2 velocity = getMoveVelocity();
    bool isMoving = velocity.lengthSquared() > 0;
    Action* targetAction = nullptr;

    if (isMoving)
    {
        // Update facing based on velocity
        if (std::abs(velocity.x) > std::abs(velocity.y))
        {
            if (velocity.x > 0) _facing = FacingDirection::Right;
            else _facing = FacingDirection::Left;
        }
        else
        {
            if (velocity.y > 0) _facing = FacingDirection::Up;
            else _facing = FacingDirection::Down;
        }

        switch (_facing)
        {
        case FacingDirection::Up: targetAction = _walkUpAction; break;
        case FacingDirection::Down: targetAction = _walkDownAction; break;
        case FacingDirection::Left: targetAction = _walkLeftAction; break;
        case FacingDirection::Right: targetAction = _walkRightAction; break;
        }

        if (_currentAction != targetAction)
        {
            stopAllActions();
            if (targetAction)
            {
                runAction(targetAction);
            }
            _currentAction = targetAction;
        }
    }
    else
    {
        if (_currentAction != nullptr)
        {
            stopAllActions();
            _currentAction = nullptr;

            // Set idle frame
            std::string idleFrameName;
            switch (_facing)
            {
            case FacingDirection::Up: idleFrameName = "player/Player_back_1.png"; break;
            case FacingDirection::Down: idleFrameName = "player/Player_front_1.png"; break;
            case FacingDirection::Left: idleFrameName = "player/Player_left_1.png"; break;
            case FacingDirection::Right: idleFrameName = "player/Player_right_1.png"; break;
            }
            
            auto texture = Director::getInstance()->getTextureCache()->addImage(idleFrameName);
            if (texture)
            {
                setTexture(texture);
                setTextureRect(Rect(0, 0, texture->getContentSize().width, texture->getContentSize().height));
            }
        }
    }
}

void Player::onKeyPressed(EventKeyboard::KeyCode keyCode)
{
    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_W:
    case EventKeyboard::KeyCode::KEY_UP_ARROW:
        _movingUp = true;
        break;
    case EventKeyboard::KeyCode::KEY_S:
    case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
        _movingDown = true;
        break;
    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
        _movingLeft = true;
        break;
    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        _movingRight = true;
        break;
    default:
        break;
    }
    updateAnimationState();
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
    updateAnimationState();
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
