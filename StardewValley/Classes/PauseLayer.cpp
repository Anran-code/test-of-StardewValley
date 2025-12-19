#include "PauseLayer.h"

USING_NS_CC;

bool PauseLayer::sIsPaused = false;

PauseLayer* PauseLayer::create()
{
    PauseLayer* ret = new (std::nothrow) PauseLayer();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool PauseLayer::init()
{
    // Init with semi-transparent black background
    if (!LayerColor::initWithColor(Color4B(0, 0, 0, 150)))
    {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    
    // Menu Items
    auto resumeItem = MenuItemFont::create("Resume Game", CC_CALLBACK_0(PauseLayer::onResume, this));
    auto exitItem = MenuItemFont::create("Exit Game", CC_CALLBACK_0(PauseLayer::onExitGame, this));
    
    resumeItem->setFontSize(24);
    exitItem->setFontSize(24);
    
    auto menu = Menu::create(resumeItem, exitItem, nullptr);
    menu->alignItemsVerticallyWithPadding(20);
    menu->setPosition(Vec2(visibleSize.width / 2, visibleSize.height / 2));
    
    this->addChild(menu);
    
    // Swallow touches
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = CC_CALLBACK_2(PauseLayer::onTouchBegan, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    return true;
}

void PauseLayer::show(Node* parent)
{
    if (this->getParent()) return;
    
    // Ensure it covers the whole screen by setting position to origin (0,0) relative to parent (Scene)
    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    
    this->setContentSize(visibleSize);
    this->setPosition(origin);
    
    parent->addChild(this, 9999); // High Z-order
    sIsPaused = true;
}

void PauseLayer::hide()
{
    this->removeFromParent();
    sIsPaused = false;
}

void PauseLayer::onResume()
{
    hide();
}

void PauseLayer::onExitGame()
{
    Director::getInstance()->end();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    exit(0);
#endif
}

bool PauseLayer::onTouchBegan(Touch* touch, Event* event)
{
    return true; // Swallow touch
}
