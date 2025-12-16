#include "HudLayer.h"
#include "GameClock.h"
#include "Wallet.h"
#include "Inventory.h"

USING_NS_CC;

HudLayer* HudLayer::create(GameClock* clock, Wallet* wallet, Inventory* inventory)
{
    HudLayer* ret = new (std::nothrow) HudLayer();
    if (ret && ret->initWithSystems(clock, wallet, inventory))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool HudLayer::initWithSystems(GameClock* clock, Wallet* wallet, Inventory* inventory)
{
    if (!Layer::init())
    {
        return false;
    }

    _clock = clock;
    _wallet = wallet;
    _inventory = inventory;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // Toolbar
    _toolbar = Sprite::create("Toolbar.png");
    if (_toolbar)
    {
        float targetWidth = visibleSize.width * 0.5f;
        float scale = targetWidth / _toolbar->getContentSize().width;
        _toolbar->setScale(scale);
        _toolbar->setAnchorPoint(Vec2(0.5f, 0.0f));
        _toolbar->setPosition(Vec2(visibleSize.width * 0.5f + origin.x, origin.y));
        addChild(_toolbar, 1);
    }

    // Backpack
    _backpack = Sprite::create("backpack_test.png");
    if (_backpack)
    {
        float targetWidth = visibleSize.width * 0.5f;
        float scale = targetWidth / _backpack->getContentSize().width;
        _backpack->setScale(scale);
        _backpack->setPosition(Vec2(visibleSize.width * 0.5f + origin.x, visibleSize.height * 0.5f + origin.y));
        _backpack->setVisible(false);
        addChild(_backpack, 2);
    }

    // Selector
    _selector = Sprite::create("CloseSelected.png"); // Using CloseSelected as a placeholder for selector frame, or create a simple rect
    if (!_selector)
    {
         // If texture not found, create a red rect
         _selector = Sprite::create();
         _selector->setTextureRect(Rect(0, 0, 40, 40));
         _selector->setColor(Color3B::RED);
         _selector->setOpacity(128);
    }
    if (_toolbar && _selector) {
        _selector->setAnchorPoint(Vec2(0.5f, 0.5f));
        _selector->retain(); // Keep it to add/remove or just toggle visibility/position
        addChild(_selector, 3);
    }

    float margin = 10.0f;
    Vec2 topRight(origin.x + visibleSize.width - margin, origin.y + visibleSize.height - margin);

    _timeLabel = Label::createWithSystemFont("", "Arial", 24);
    _dateLabel = Label::createWithSystemFont("", "Arial", 24);
    _weekLabel = Label::createWithSystemFont("", "Arial", 24);
    _moneyLabel = Label::createWithSystemFont("", "Arial", 24);

    if (_timeLabel)
    {
        _timeLabel->setAnchorPoint(Vec2(1.0f, 1.0f));
        _timeLabel->setPosition(topRight);
        addChild(_timeLabel, 1000);
    }
    if (_dateLabel)
    {
        _dateLabel->setAnchorPoint(Vec2(1.0f, 1.0f));
        _dateLabel->setPosition(Vec2(topRight.x, topRight.y - 28.0f));
        addChild(_dateLabel, 1000);
    }
    if (_weekLabel)
    {
        _weekLabel->setAnchorPoint(Vec2(1.0f, 1.0f));
        _weekLabel->setPosition(Vec2(topRight.x, topRight.y - 56.0f));
        addChild(_weekLabel, 1000);
    }
    if (_moneyLabel)
    {
        _moneyLabel->setAnchorPoint(Vec2(1.0f, 1.0f));
        _moneyLabel->setPosition(Vec2(topRight.x, topRight.y - 84.0f));
        addChild(_moneyLabel, 1000);
    }

    auto listener = EventListenerKeyboard::create();
    listener->onKeyPressed = CC_CALLBACK_2(HudLayer::onKeyPressed, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    auto mouseListener = EventListenerMouse::create();
    mouseListener->onMouseScroll = CC_CALLBACK_1(HudLayer::onScroll, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);

    auto updateListener = EventListenerCustom::create("INVENTORY_UPDATED", [this](EventCustom* event) {
        this->updateInventoryUI();
    });
    _eventDispatcher->addEventListenerWithSceneGraphPriority(updateListener, this);

    refresh();
    updateInventoryUI();
    
    return true;
}

void HudLayer::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
    if (keyCode == EventKeyboard::KeyCode::KEY_E)
    {
        if (_backpack)
        {
            _backpack->setVisible(!_backpack->isVisible());
        }
    }
    
    // Number keys for toolbar selection
    int slot = -1;
    if (keyCode >= EventKeyboard::KeyCode::KEY_1 && keyCode <= EventKeyboard::KeyCode::KEY_9)
    {
        slot = (int)keyCode - (int)EventKeyboard::KeyCode::KEY_1;
    }
    else if (keyCode == EventKeyboard::KeyCode::KEY_0)
    {
        slot = 9;
    }
    else if (keyCode == EventKeyboard::KeyCode::KEY_MINUS)
    {
        slot = 10;
    }
    else if (keyCode == EventKeyboard::KeyCode::KEY_EQUAL) // often +
    {
        slot = 11;
    }

    if (slot != -1 && _inventory)
    {
        _inventory->setSelectedSlot(slot);
        updateInventoryUI();
    }
}

void HudLayer::onScroll(Event* event)
{
    EventMouse* e = (EventMouse*)event;
    float scrollY = e->getScrollY();
    if (scrollY != 0 && _inventory)
    {
        int current = _inventory->getSelectedSlot();
        if (scrollY > 0)
        {
            current--;
            if (current < 0) current = Inventory::TOOLBAR_SIZE - 1;
        }
        else
        {
            current++;
            if (current >= Inventory::TOOLBAR_SIZE) current = 0;
        }
        _inventory->setSelectedSlot(current);
        updateInventoryUI();
    }
}

void HudLayer::updateInventoryUI()
{
    if (!_inventory || !_toolbar) return;

    // Clear old sprites
    for (auto sp : _itemSprites) sp->removeFromParent();
    _itemSprites.clear();
    for (auto lbl : _quantityLabels) lbl->removeFromParent();
    _quantityLabels.clear();

    Size toolbarSize = _toolbar->getContentSize();
    float scale = _toolbar->getScale();
    Vec2 toolbarPos = _toolbar->getPosition();
    
    // Toolbar has 12 slots.
    // Assuming Toolbar.png is a single row of 12 squares.
    // We need to calculate the position of each slot relative to the toolbar sprite.
    // Let's assume standard Stardew toolbar layout.
    // If we don't know the exact pixel offsets, we can approximate.
    // Toolbar width is toolbarSize.width. Each slot is roughly width / 12.
    
    float slotWidth = toolbarSize.width / 12.0f;
    float slotHeight = toolbarSize.height; // Assuming single row
    
    // Starting X (left side of the toolbar in world space)
    // Anchor is (0.5, 0.0), so left is pos.x - (width * scale)/2
    float startX = toolbarPos.x - (toolbarSize.width * scale * 0.5f);
    float startY = toolbarPos.y; // Bottom aligned
    
    float scaledSlotWidth = slotWidth * scale;
    float scaledSlotHeight = slotHeight * scale;
    
    // Update items
    for (int i = 0; i < Inventory::TOOLBAR_SIZE; i++)
    {
        if (_inventory->hasItem(i))
        {
            const Item& item = _inventory->getItem(i);
            auto sprite = Sprite::create(item.iconPath);
            if (sprite)
            {
                // Fit into slot
                float iconScale = (scaledSlotWidth * 0.7f) / sprite->getContentSize().width;
                sprite->setScale(iconScale);
                
                // Center of slot
                float cx = startX + (i * scaledSlotWidth) + (scaledSlotWidth * 0.5f);
                float cy = startY + (scaledSlotHeight * 0.5f);
                
                sprite->setPosition(Vec2(cx, cy));
                addChild(sprite, 10); // Above toolbar
                _itemSprites.push_back(sprite);

                // Quantity
                if (item.maxStack > 1 && item.quantity > 1)
                {
                    auto label = Label::createWithSystemFont(std::to_string(item.quantity), "Arial", 16);
                    if (label)
                    {
                        label->setAnchorPoint(Vec2(1.0f, 0.0f));
                        label->setPosition(Vec2(cx + scaledSlotWidth * 0.4f, cy - scaledSlotHeight * 0.4f));
                        addChild(label, 11);
                        _quantityLabels.push_back(label);
                    }
                }
            }
        }
    }

    // Update Selector position
    if (_selector)
    {
        int sel = _inventory->getSelectedSlot();
        float cx = startX + (sel * scaledSlotWidth) + (scaledSlotWidth * 0.5f);
        float cy = startY + (scaledSlotHeight * 0.5f);
        _selector->setPosition(Vec2(cx, cy));
        
        // Scale selector to fit slot
        // Assuming CloseSelected.png or placeholder is roughly slot size
        Size selSize = _selector->getContentSize();
        if (selSize.width > 0) {
            _selector->setScale((scaledSlotWidth * 1.1f) / selSize.width); // Slightly larger than slot
        }
    }
}

void HudLayer::refresh()
{
    if (_clock && _timeLabel)
    {
        _timeLabel->setString(_clock->getTimeString());
    }
    if (_clock && _dateLabel)
    {
        const char* s =
            (_clock->getSeason() == GameClock::Season::Spring) ? "Spring" :
            (_clock->getSeason() == GameClock::Season::Summer) ? "Summer" :
            (_clock->getSeason() == GameClock::Season::Fall)   ? "Fall" : "Winter";
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%s Day %d Year %d", s, _clock->getDay(), _clock->getYear());
        _dateLabel->setString(buf);
    }
    if (_clock && _weekLabel)
    {
        std::string w = _clock->getWeekdayString();
        int wk = _clock->getWeekOfSeason();
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%s, Week %d", w.c_str(), wk);
        _weekLabel->setString(buf);
    }
    if (_wallet && _moneyLabel)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Gold: %d", _wallet->getMoney());
        _moneyLabel->setString(buf);
    }
}
