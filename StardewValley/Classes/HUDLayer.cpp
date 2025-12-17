#include "HudLayer.h"
#include "GameClock.h"
#include "Wallet.h"
#include "Inventory.h"
#include "HomeScene.h"

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
    _debugBounds = DrawNode::create();
    addChild(_debugBounds, 10000);

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

    // Mouse listener with FIXED PRIORITY so HUD receives events before scene graph listeners
    _mouseListener = EventListenerMouse::create();
    _mouseListener->onMouseScroll = CC_CALLBACK_1(HudLayer::onScroll, this);
    _mouseListener->onMouseDown = CC_CALLBACK_1(HudLayer::onMouseDown, this);
    _mouseListener->onMouseUp = CC_CALLBACK_1(HudLayer::onMouseUp, this);
    _eventDispatcher->addEventListenerWithFixedPriority(_mouseListener, -128);

    _touchListener = EventListenerTouchOneByOne::create();
    _touchListener->setSwallowTouches(true);
    _touchListener->onTouchBegan = CC_CALLBACK_2(HudLayer::onTouchBegan, this);
    _touchListener->onTouchEnded = CC_CALLBACK_2(HudLayer::onTouchEnded, this);
    _touchListener->onTouchCancelled = CC_CALLBACK_2(HudLayer::onTouchCancelled, this);
    _eventDispatcher->addEventListenerWithFixedPriority(_touchListener, -128);

    auto updateListener = EventListenerCustom::create("INVENTORY_UPDATED", [this](EventCustom* event) {
        this->updateInventoryUI();
    });
    _eventDispatcher->addEventListenerWithSceneGraphPriority(updateListener, this);

    auto debugModeListener = EventListenerCustom::create("DEBUG_MODE_CHANGED", [this](EventCustom* event) {
        this->updateInventoryUI();
    });
    _eventDispatcher->addEventListenerWithSceneGraphPriority(debugModeListener, this);

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
            current++;
            if (current >= Inventory::TOOLBAR_SIZE) current = 0;
        }
        else
        {
            current--;
            if (current < 0) current = Inventory::TOOLBAR_SIZE - 1;
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
    
    // Toolbar.png 像素信息:
    // 假设 Toolbar.png 的原始宽度对应这些像素值
    // 每个格子 198x190
    // 左边框到第一个格子左边缘: 54
    // 下边框到第一个格子下边缘: 75 (原点在左下角，所以Y轴偏移是75)
    // 格子间隙: 30
    // 
    // 我们需要将这些像素值转换为相对于 toolbar 缩放后的屏幕坐标。
    
    // 获取 toolbar 原始尺寸
    Size originalSize = _toolbar->getContentSize();
    
    // Pixel constants
    // These constants are defined in the header file, no need to redefine
    
    // 计算缩放后的值
    float scaledLeftMargin = RAW_LEFT_MARGIN * scale;
    float scaledBottomMargin = RAW_BOTTOM_MARGIN * scale;
    float scaledCellWidth = RAW_CELL_WIDTH * scale;
    float scaledCellHeight = RAW_CELL_HEIGHT * scale;
    float scaledGap = RAW_GAP * scale;
    
    // Toolbar 在屏幕上的左下角坐标
    // Anchor(0.5, 0.0), Pos(cx, y)
    // Left = Pos.x - (ContentW * Scale * 0.5)
    // Bottom = Pos.y
    float toolbarScreenLeft = toolbarPos.x - (originalSize.width * scale * 0.5f);
    float toolbarScreenBottom = toolbarPos.y;
    
    // Cache for hit testing
    _cachedToolbarLeft = toolbarScreenLeft;
    _cachedToolbarBottom = toolbarScreenBottom;
    _cachedScale = scale;
    if (_debugBounds)
    {
        _debugBounds->clear();
        if (HomeScene::sDebugMode)
        {
            float w = originalSize.width * scale;
            float h = originalSize.height * scale;
            _debugBounds->drawRect(
                Vec2(toolbarScreenLeft, toolbarScreenBottom),
                Vec2(toolbarScreenLeft + w, toolbarScreenBottom + h),
                Color4F(0.2f, 0.8f, 0.2f, 1.0f));
        }
    }


    // 第一个格子的中心点
    // CenterX = Left + Margin + CellW/2
    // CenterY = Bottom + Margin + CellH/2
    float startCenterX = toolbarScreenLeft + scaledLeftMargin + (scaledCellWidth * 0.5f);
    float startCenterY = toolbarScreenBottom + scaledBottomMargin + (scaledCellHeight * 0.5f);
    
    // 每个格子之间的步长
    float stepX = scaledCellWidth + scaledGap;

    // Update items
    for (int i = 0; i < Inventory::TOOLBAR_SIZE; i++)
    {
        if (_inventory->hasItem(i))
        {
            const Item& item = _inventory->getItem(i);
            auto sprite = Sprite::create(item.iconPath);
            if (sprite)
            {
                // Fit into slot (leave some padding, e.g. 10%)
                float maxIconW = scaledCellWidth * 0.8f;
                float maxIconH = scaledCellHeight * 0.8f;
                
                float sX = maxIconW / sprite->getContentSize().width;
                float sY = maxIconH / sprite->getContentSize().height;
                float iconScale = std::min(sX, sY);
                
                sprite->setScale(iconScale);
                
                // Calculate position based on index
                float cx = startCenterX + (i * stepX);
                float cy = startCenterY;
                
                sprite->setPosition(Vec2(cx, cy));
                addChild(sprite, 10); // Above toolbar
                _itemSprites.push_back(sprite);

                // Quantity
                if (item.maxStack > 1 && item.quantity > 1)
                {
                    auto label = Label::createWithSystemFont(std::to_string(item.quantity), "Arial", 16); // Font size might need scaling
                    if (label)
                    {
                        label->setColor(Color3B::BLACK);
                        label->enableBold(); // Make text bold
                        label->setAnchorPoint(Vec2(1.0f, 0.0f));
                        // Position at bottom-right of the cell content area
                        // Move slightly more to bottom-right (from 0.4f to 0.48f)
                        // User requested slightly more down (0.48f -> 0.55f)
                        label->setPosition(Vec2(cx + scaledCellWidth * 0.48f, cy - scaledCellHeight * 0.55f));
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
        float cx = startCenterX + (sel * stepX);
        float cy = startCenterY;
        _selector->setPosition(Vec2(cx, cy));
        
        // Scale selector to fit slot size
        Size selSize = _selector->getContentSize();
        if (selSize.width > 0) {
            // Make selector slightly larger than cell
            float targetSelW = scaledCellWidth * 1.1f;
            _selector->setScale(targetSelW / selSize.width);
        }
    }
}

bool HudLayer::isPointInToolbarWorld(const Vec2& p) const
{
    if (!_toolbar) return false;
    float scale = _toolbar->getScale();
    Size originalSize = _toolbar->getContentSize();
    Vec2 toolbarPos = _toolbar->getPosition();
    float scaledW = originalSize.width * scale;
    float scaledH = originalSize.height * scale;
    float left = toolbarPos.x - scaledW * 0.5f;
    float bottom = toolbarPos.y;
    Rect r(left, bottom, scaledW, scaledH);
    return r.containsPoint(p);
}

void HudLayer::onMouseDown(Event* event)
{
    if (!_inventory || !_toolbar) return;

    EventMouse* e = (EventMouse*)event;
    if (e->getMouseButton() != EventMouse::MouseButton::BUTTON_LEFT) return;

    Vec2 clickPos = e->getLocation();

    float scale = _toolbar->getScale();
    Size originalSize = _toolbar->getContentSize();
    Vec2 toolbarPos = _toolbar->getPosition();
    float scaledW = originalSize.width * scale;
    float scaledH = originalSize.height * scale;
    float left = toolbarPos.x - scaledW * 0.5f;
    float bottom = toolbarPos.y;
    Rect hitRectWorld(left, bottom, scaledW, scaledH);

    if (hitRectWorld.containsPoint(clickPos))
    {
        _consumingClick = true;
        event->stopPropagation();

        float localXPixel = (clickPos.x - left) / scale;
        float localYPixel = (clickPos.y - bottom) / scale;

        float yMin = RAW_BOTTOM_MARGIN;
        float yMax = RAW_BOTTOM_MARGIN + RAW_CELL_HEIGHT;
        float vTol = 30.0f;

        if (localYPixel >= yMin - vTol && localYPixel <= yMax + vTol)
        {
             float startX = RAW_LEFT_MARGIN;
             float stepX = RAW_CELL_WIDTH + RAW_GAP;
             
             float relX = localXPixel - startX;
             
             // Allow clicking slightly before the first slot
             if (relX >= -RAW_GAP && relX < (Inventory::TOOLBAR_SIZE * stepX))
             {
                 // Normalize
                 if (relX < 0) relX = 0;
                 
                 int index = static_cast<int>(relX / stepX);
                 
                 // Check if inside cell vs gap?
                 // Let's be generous: if it lands in the gap, give it to the nearest cell?
                 // Or just simpler: divide by stepX.
                 // index 0 covers [0, stepX)
                 // This includes the cell AND the gap to its right.
                 // This is good for UX.
                 
                 if (index >= 0 && index < Inventory::TOOLBAR_SIZE)
                 {
                     _inventory->setSelectedSlot(index);
                     updateInventoryUI();
                 }
             }
        }
        return;
    }
}

void HudLayer::onMouseUp(Event* event)
{
    _consumingClick = false;
}

bool HudLayer::onTouchBegan(Touch* touch, Event* event)
{
    if (!_inventory || !_toolbar) return false;
    Vec2 p = touch->getLocation();
    float scale = _toolbar->getScale();
    Size originalSize = _toolbar->getContentSize();
    Vec2 toolbarPos = _toolbar->getPosition();
    float scaledW = originalSize.width * scale;
    float scaledH = originalSize.height * scale;
    float left = toolbarPos.x - scaledW * 0.5f;
    float bottom = toolbarPos.y;
    Rect hitRectWorld(left, bottom, scaledW, scaledH);
    if (!hitRectWorld.containsPoint(p)) return false;
    _consumingClick = true;
    float localXPixel = (p.x - left) / scale;
    float localYPixel = (p.y - bottom) / scale;
    float yMin = RAW_BOTTOM_MARGIN;
    float yMax = RAW_BOTTOM_MARGIN + RAW_CELL_HEIGHT;
    float vTol = 30.0f;
    if (localYPixel >= yMin - vTol && localYPixel <= yMax + vTol)
    {
        float startX = RAW_LEFT_MARGIN;
        float stepX = RAW_CELL_WIDTH + RAW_GAP;
        float relX = localXPixel - startX;
        if (relX >= -RAW_GAP && relX < (Inventory::TOOLBAR_SIZE * stepX))
        {
            if (relX < 0) relX = 0;
            int index = static_cast<int>(relX / stepX);
            if (index >= 0 && index < Inventory::TOOLBAR_SIZE)
            {
                _inventory->setSelectedSlot(index);
                updateInventoryUI();
            }
        }
    }
    return true;
}

void HudLayer::onTouchEnded(Touch* touch, Event* event)
{
    _consumingClick = false;
}

void HudLayer::onTouchCancelled(Touch* touch, Event* event)
{
    _consumingClick = false;
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
