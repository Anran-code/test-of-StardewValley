#include "Animal.h"

USING_NS_CC;

Animal::~Animal()
{
    if (_aiStrategy)
    {
        delete _aiStrategy;
        _aiStrategy = nullptr;
    }
}

Animal* Animal::create(Type type, Age age)
{
    Animal* pRet = new (std::nothrow) Animal();
    if (pRet && pRet->init(type, age))
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

bool Animal::init(Type type, Age age)
{
    _type = type;
    _age = age;
    _location = Location::Inside; // Default to inside
    _currentDirection = Direction::Down;
    _currentState = State::Idle;
    _stateTimer = 0.0f;
    _moveSpeed = 30.0f;
    _daysAlive = 0;
    _isFed = false;
    _daysSinceLastProduct = 0;
    _aiStrategy = nullptr;
    
    if (type == Type::Cat) {
        _cellSize = Size(32, 32);
        _aiStrategy = new CatAI();
    } else {
        _cellSize = Size(16, 16);
        _aiStrategy = new Chicken_Rabbit_AI();
    }

    // Determine texture file
    std::string prefix = (_age == Age::Baby) ? "Baby" : "";
    std::string color;
    if (_type == Type::Blue) color = "Blue Chicken";
    else if (_type == Type::White) color = "White Chicken";
    else if (_type == Type::Rabbit) {
        color = "Rabbit";
        if (_age == Age::Baby) prefix = "Baby"; 
        else prefix = "";
    }
    
    // Construct filename
      if (_type == Type::Rabbit) {
         _textureFile = "animals/" + prefix + color + ".png";
      } else if (_type == Type::Cat) {
         _textureFile = "animals/cat.png";
      } else {
         std::string cPrefix = (_age == Age::Baby) ? "Baby" : "";
         std::string cColor = (_type == Type::Blue) ? "Blue" : "White";
         _textureFile = "animals/" + cPrefix + cColor + " Chicken.png";
      }

    if (!Sprite::initWithFile(_textureFile, Rect(0, 0, _cellSize.width, _cellSize.height)))
    {
        return false;
    }

    // Disable antialiasing for pixel art
    if (getTexture()) {
        getTexture()->setAliasTexParameters();
    }

    initAnimations();

    // Start AI loop
    scheduleUpdate();
    pickNewState();

    return true;
}

void Animal::growUp()
{
    if (_age == Age::Adult) return;

    _age = Age::Adult;

    // Determine new texture file
    if (_type == Type::Rabbit)
    {
        _textureFile = "animals/Rabbit.png";
    }
    else
    {
        std::string color = (_type == Type::Blue) ? "Blue" : "White";
        _textureFile = "animals/" + color + " Chicken.png"; 
    }

    // Reload texture
    setTexture(_textureFile);

    // Clear old animations and re-init
    _animations.clear();
    stopAllActions();
    initAnimations();

    // Restart logic
    pickNewState();
}


void Animal::initAnimations()
{
    auto texture = getTexture();
    if (!texture) return;
    auto createAnim = [&](const std::string& name, int row, int startCol, int count, float delay, bool loop) {
        Vector<SpriteFrame*> frames;
        for (int i = 0; i < count; ++i)
        {
            float x = (startCol + i) * _cellSize.width;
            float y = row * _cellSize.height;
            auto frame = SpriteFrame::createWithTexture(texture, Rect(x, y, _cellSize.width, _cellSize.height));
            frames.pushBack(frame);
        }
        auto animation = Animation::createWithSpriteFrames(frames, delay);
        if (loop) animation->setLoops(-1); // Infinite loop
        _animations.insert(name, animation);
        };

    // Row 0: Walk Down 
    createAnim("walk_down", 0, 0, 4, 0.15f, true);

    // Row 1: Walk Right
    createAnim("walk_right", 1, 0, 4, 0.15f, true);

    // Row 2: Walk Up
    createAnim("walk_up", 2, 0, 4, 0.15f, true);

    // Row 3: Walk Left 
    createAnim("walk_left", 3, 0, 4, 0.15f, true);

    // Col 0: Idle Down, Col 1: Sit Down
    // Col 2: Idle Right, Col 3: Sit Right
    createAnim("idle_down", 4, 0, 1, 1.0f, false);
    createAnim("sit_down", 4, 1, 1, 1.0f, false);
    createAnim("idle_right", 4, 2, 1, 1.0f, false);
    createAnim("sit_right", 4, 3, 1, 1.0f, false);

    // Col 0: Idle Up, Col 1: Sit Up
    // Col 2: Idle Left, Col 3: Sit Left
    createAnim("idle_up", 5, 0, 1, 1.0f, false);
    createAnim("sit_up", 5, 1, 1, 1.0f, false);
    createAnim("idle_left", 5, 2, 1, 1.0f, false);
    createAnim("sit_left", 5, 3, 1, 1.0f, false);

    // Row 6: Eat 
    createAnim("eat", 6, 0, 4, 0.2f, true);

    if (_type == Type::Cat)
    {
        // Cat specific animations
        // Row 4: Sit (4 frames)
        createAnim("sit", 4, 0, 4, 0.2f, false);
        
        // Row 5: Groom (4 frames)
        createAnim("groom", 5, 0, 4, 0.2f, true);

        // Row 6: Lie Down (4 frames)
        createAnim("lie_down", 6, 0, 4, 0.2f, false);

        // Row 7: Sleep (First 2 frames)
        createAnim("sleep", 7, 0, 2, 1.0f, true);
    }
}

void Animal::update(float dt)
{
    _stateTimer -= dt;

    if (_currentState == State::Walk)
    {
        Vec2 dir = getDirectionVector();
        Vec2 newPos = getPosition() + dir * _moveSpeed * dt;

        setPosition(newPos);
    }

    if (_stateTimer <= 0)
    {
        pickNewState();
    }
}

cocos2d::Vec2 Animal::getDirectionVector() const
{
    switch (_currentDirection)
    {
        case Direction::Down: return Vec2(0, -1);
        case Direction::Up: return Vec2(0, 1);
        case Direction::Left: return Vec2(-1, 0);
        case Direction::Right: return Vec2(1, 0);
        default: return Vec2::ZERO;
    }
}

void Animal::setDirection(Direction dir)
{
    if (_currentDirection != dir)
    {
        _currentDirection = dir;
        updateAnimation();
    }
}

void Animal::setState(State state)
{
    if (_currentState != state)
    {
        _currentState = state;
        updateAnimation();
    }
}

void Animal::updateAnimation()
{
    stopAllActions();

    std::string animName = "";

    if (_currentState == State::Eat)
    {
        animName = "eat";
    }
    else if (_currentState == State::Walk)
    {
        switch (_currentDirection)
        {
            case Direction::Down: animName = "walk_down"; break;
            case Direction::Up: animName = "walk_up"; break;
            case Direction::Left: animName = "walk_left"; break;
            case Direction::Right: animName = "walk_right"; break;
        }
    }
    else if (_currentState == State::Idle)
    {
        switch (_currentDirection)
        {
            case Direction::Down: animName = "idle_down"; break;
            case Direction::Up: animName = "idle_up"; break;
            case Direction::Left: animName = "idle_left"; break;
            case Direction::Right: animName = "idle_right"; break;
        }
    }
    else if (_currentState == State::Sit || _currentState == State::Sleep)
    {
        if (_type == Type::Cat)
        {
            if (_currentState == State::Sit) animName = "sit";
            else if (_currentState == State::Sleep) animName = "sleep";
        }
        else
        {
            switch (_currentDirection)
            {
                case Direction::Down: animName = "sit_down"; break;
                case Direction::Up: animName = "sit_up"; break;
                case Direction::Left: animName = "sit_left"; break;
                case Direction::Right: animName = "sit_right"; break;
            }
        }
    }
    else if (_currentState == State::Groom)
    {
        animName = "groom";
    }
    else if (_currentState == State::LieDown)
    {
        animName = "lie_down";
    }

    if (!animName.empty())
    {
        auto anim = _animations.at(animName);
        if (anim)
        {
            runAction(Animate::create(anim));
        }
    }
}

void Animal::startEating()
{
    setState(State::Eat);
    _stateTimer = 2.0f + (rand() % 20) / 10.0f; // 2-4 seconds
}

void Animal::pickNewState()
{
    if (_aiStrategy)
    {
        _aiStrategy->decideNextState(this);
    }
}
