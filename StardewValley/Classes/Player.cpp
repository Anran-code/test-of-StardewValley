#include "Player.h"
#include "GameScene.h"

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

        // Adjust anchor point to shift sprite and collision box upwards relative to logical position
        // Default is (0.5, 0.5). (0.5, 0.3) moves the pivot down, so the sprite sits higher.
        ret->setAnchorPoint(Vec2(0.5f, 0.3f));

        ret->_speed = 150.0f;
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

        // Create feedback sprite (hidden by default)
        ret->_feedbackSprite = Sprite::create();
        if (ret->_feedbackSprite)
        {
            ret->_feedbackSprite->setVisible(false);
            ret->addChild(ret->_feedbackSprite, 10);
        }

        Size size = ret->getContentSize();
        if (size.height > 0.0f && tileHeight > 0.0f)
        {
            // Shrink player more (was 1.2f)
            float desiredHeight = tileHeight * 1.0f;
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
        if (std::abs(velocity.x) > std::abs(velocity.y))
        {
            if (velocity.x > 0)
            {
                targetAction = _walkRightAction;
                _facing = FacingDirection::Right;
            }
            else
            {
                targetAction = _walkLeftAction;
                _facing = FacingDirection::Left;
            }
        }
        else
        {
            if (velocity.y > 0)
            {
                targetAction = _walkUpAction;
                _facing = FacingDirection::Up;
            }
            else
            {
                targetAction = _walkDownAction;
                _facing = FacingDirection::Down;
            }
        }
    }

    if (targetAction != _currentAction)
    {
        stopAllActions();
        _currentAction = targetAction;

        if (_currentAction)
        {
            runAction(_currentAction);
        }
        else
        {
            // Idle frame based on facing
            std::string frameName;
            switch (_facing)
            {
            case FacingDirection::Down: frameName = "player/Player_front_1.png"; break;
            case FacingDirection::Up:   frameName = "player/Player_back_1.png"; break;
            case FacingDirection::Left: frameName = "player/Player_left_1.png"; break;
            case FacingDirection::Right:frameName = "player/Player_right_1.png"; break;
            }
            setTexture(frameName);
        }
    }
    else if (!isMoving && _currentAction != nullptr)
    {
        stopAllActions();
        _currentAction = nullptr;
        // Idle frame based on facing
        std::string frameName;
        switch (_facing)
        {
        case FacingDirection::Down: frameName = "player/Player_front_1.png"; break;
        case FacingDirection::Up:   frameName = "player/Player_back_1.png"; break;
        case FacingDirection::Left: frameName = "player/Player_left_1.png"; break;
        case FacingDirection::Right:frameName = "player/Player_right_1.png"; break;
        }
        setTexture(frameName);
    }
}

void Player::onKeyPressed(EventKeyboard::KeyCode keyCode)
{
    if (GameScene::sWasFainted) return; // Block input if fainted

    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_W:
    case EventKeyboard::KeyCode::KEY_UP_ARROW:
        _movingUp = true;
        _moveDir.y += 1.0f;
        break;
    case EventKeyboard::KeyCode::KEY_S:
    case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
        _movingDown = true;
        _moveDir.y -= 1.0f;
        break;
    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
        _movingLeft = true;
        _moveDir.x -= 1.0f;
        break;
    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        _movingRight = true;
        _moveDir.x += 1.0f;
        break;
    default:
        break;
    }
    updateAnimationState();
}

void Player::onKeyReleased(EventKeyboard::KeyCode keyCode)
{
    if (GameScene::sWasFainted) return; // Block input if fainted
    
    switch (keyCode)
    {
    case EventKeyboard::KeyCode::KEY_W:
    case EventKeyboard::KeyCode::KEY_UP_ARROW:
        if (_movingUp) { _movingUp = false; _moveDir.y -= 1.0f; }
        break;
    case EventKeyboard::KeyCode::KEY_S:
    case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
        if (_movingDown) { _movingDown = false; _moveDir.y += 1.0f; }
        break;
    case EventKeyboard::KeyCode::KEY_A:
    case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
        if (_movingLeft) { _movingLeft = false; _moveDir.x += 1.0f; }
        break;
    case EventKeyboard::KeyCode::KEY_D:
    case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
        if (_movingRight) { _movingRight = false; _moveDir.x -= 1.0f; }
        break;
    default:
        break;
    }
    
    // Clamp small floating point errors
    if (std::abs(_moveDir.x) < 0.1f) _moveDir.x = 0.0f;
    if (std::abs(_moveDir.y) < 0.1f) _moveDir.y = 0.0f;

    updateAnimationState();
}

Vec2 Player::getMoveVelocity() const
{
    if (GameScene::sWasFainted) return Vec2::ZERO; // Force zero velocity if fainted

    Vec2 v = _moveDir;
    if (v.lengthSquared() > 0)
    {
        v.normalize();
        return v * _speed;
    }
    return Vec2::ZERO;
}

Vec2 Player::getFacingOffset() const
{
    switch (_facing)
    {
    case FacingDirection::Up: return Vec2(0, -1); // Up in TMX (y decreases going down, but wait. TMX y increases going down. 0,0 is top-left.)
        // Actually, our logic:
        // world Y increases going up.
        // TMX tile Y increases going down.
        // If we face UP (world +Y), we are decreasing tile Y. So (0, -1). Correct.
        
    case FacingDirection::Down: return Vec2(0, 1);
    case FacingDirection::Left: return Vec2(-1, 0);
    case FacingDirection::Right: return Vec2(1, 0);
    }
    return Vec2(0, 1);
}

void Player::showToolFeedback(const std::string& texturePath)
{
    if (!_feedbackSprite) return;

    // Check if texture exists
    auto texture = Director::getInstance()->getTextureCache()->addImage(texturePath);
    if (texture)
    {
        _feedbackSprite->setTexture(texture);
        _feedbackSprite->setTextureRect(Rect(0, 0, texture->getContentSize().width, texture->getContentSize().height));
        
        _feedbackSprite->setVisible(true);
        _feedbackSprite->setOpacity(255);
        _feedbackSprite->stopAllActions();

        // Position relative to player head
        // Player content size might be the size of the frame
        float headY = getContentSize().height;
        // Position slightly above the head
        _feedbackSprite->setPosition(Vec2(getContentSize().width * 0.5f, headY + 10));

        _feedbackSprite->setScale(0.0f);
        auto scaleUp = ScaleTo::create(0.15f, 1.2f);
        auto delay = DelayTime::create(0.1f);
        auto fadeOut = FadeOut::create(0.2f);
        auto scaleDown = ScaleTo::create(0.2f, 0.5f);
        auto hide = Hide::create();

        auto disappear = Spawn::create(fadeOut, scaleDown, nullptr);

        _feedbackSprite->runAction(Sequence::create(scaleUp, delay, disappear, hide, nullptr));
    }
}
