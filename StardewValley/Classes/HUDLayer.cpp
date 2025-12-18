#include "HudLayer.h"
#include "GameClock.h"
#include "Wallet.h"
#include "Inventory.h"
#include "GameScene.h"

USING_NS_CC;

HudLayer::HudLayer()
    : _mouseListener(nullptr)
    , _touchListener(nullptr)
    , _debugBounds(nullptr)
    , _clock(nullptr)
    , _wallet(nullptr)
    , _inventory(nullptr)
    , _toolbar(nullptr)
    , _backpack(nullptr)
    , _selector(nullptr)
    , _cachedToolbarLeft(0.0f)
    , _cachedToolbarBottom(0.0f)
    , _cachedScale(1.0f)
    , _timeLabel(nullptr)
    , _dateLabel(nullptr)
    , _weekLabel(nullptr)
    , _moneyLabel(nullptr)
{
}

HudLayer::~HudLayer()
{
    if (_selector)
    {
        _selector->release();
        _selector = nullptr;
    }
}

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
    _mouseListener->onMouseMove = CC_CALLBACK_1(HudLayer::onMouseMove, this); // Dragging
    _eventDispatcher->addEventListenerWithFixedPriority(_mouseListener, -128);

    _touchListener = EventListenerTouchOneByOne::create();
    _touchListener->setSwallowTouches(true);
    _touchListener->onTouchBegan = CC_CALLBACK_2(HudLayer::onTouchBegan, this);
    _touchListener->onTouchMoved = CC_CALLBACK_2(HudLayer::onTouchMoved, this);
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

void HudLayer::onExit()
{
    if (_mouseListener)
    {
        _eventDispatcher->removeEventListener(_mouseListener);
        _mouseListener = nullptr;
    }
    if (_touchListener)
    {
        _eventDispatcher->removeEventListener(_touchListener);
        _touchListener = nullptr;
    }
    Layer::onExit();
}

void HudLayer::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
    if (keyCode == EventKeyboard::KeyCode::KEY_E)
    {
        if (_backpack)
        {
            _backpack->setVisible(!_backpack->isVisible());
            updateInventoryUI(); // Force UI refresh immediately
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
        updateSelectorPosition();
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
        updateSelectorPosition();
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
        if (GameScene::sDebugMode)
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
        
        Size selSize = _selector->getContentSize();
        if (selSize.width > 0) {
            float targetSelW = scaledCellWidth * 1.1f;
            _selector->setScale(targetSelW / selSize.width);
        }
    }

    // ---------------------------------------------------------
    // RENDER BACKPACK ITEMS (if visible)
    // ---------------------------------------------------------
    if (_backpack && _backpack->isVisible())
    {       
        Size bpSize = _backpack->getContentSize();
        float bpScale = _backpack->getScale();
        Vec2 bpPos = _backpack->getPosition();
        // bpPos is Center (0.5, 0.5) usually? 
        // In init: _backpack->setPosition(Vec2(visibleSize.width * 0.5f + origin.x, visibleSize.height * 0.5f + origin.y));
        // Yes, center.
        
        float bpLeft = bpPos.x - (bpSize.width * bpScale * 0.5f);
        float bpBottom = bpPos.y - (bpSize.height * bpScale * 0.5f);
        
        // Debug Drawing for Backpack Hit Areas
        if (_debugBounds && GameScene::sDebugMode)
        {
            // Draw Close Button
            float cbLeft = bpLeft + (CLOSE_BTN_LEFT * bpScale);
            float cbBottom = bpBottom + (CLOSE_BTN_BOTTOM * bpScale);
            float cbW = CLOSE_BTN_W * bpScale;
            float cbH = CLOSE_BTN_H * bpScale;
            _debugBounds->drawRect(Vec2(cbLeft, cbBottom), Vec2(cbLeft + cbW, cbBottom + cbH), Color4F::RED);
            
            // Draw Trash Can
            float trLeft = bpLeft + (TRASH_LEFT * bpScale);
            float trBottom = bpBottom + (TRASH_BOTTOM * bpScale);
            float trW = TRASH_W * bpScale;
            float trH = TRASH_H * bpScale;
            _debugBounds->drawRect(Vec2(trLeft, trBottom), Vec2(trLeft + trW, trBottom + trH), Color4F::MAGENTA);
            
            // Draw Slots
            for (int i = 0; i < Inventory::BACKPACK_SIZE; i++)
            {
                int r = i / 12;
                int c = i % 12;
                
                float lx = BACKPACK_ROW1_LEFT + (c * (BACKPACK_CELL_W + BACKPACK_H_GAP));
                float ly = 0;
                if (r == 0) ly = BACKPACK_ROW1_BOTTOM;
                else if (r == 1) ly = BACKPACK_ROW1_BOTTOM - BACKPACK_V_GAP_1_2 - BACKPACK_CELL_H;
                else if (r == 2) ly = BACKPACK_ROW1_BOTTOM - BACKPACK_V_GAP_1_2 - BACKPACK_CELL_H - BACKPACK_V_GAP_2_3 - BACKPACK_CELL_H;
                
                float sx = bpLeft + (lx * bpScale);
                float sy = bpBottom + (ly * bpScale);
                float sw = BACKPACK_CELL_W * bpScale;
                float sh = BACKPACK_CELL_H * bpScale;
                
                _debugBounds->drawRect(Vec2(sx, sy), Vec2(sx + sw, sy + sh), Color4F(0, 0, 1, 0.5f)); // Blue for slots
            }
        }

        for (int i = 0; i < Inventory::BACKPACK_SIZE; i++)
        {
            if (_inventory->hasItem(i))
            {
                // If this item is being dragged, do NOT render it in the slot (it follows mouse)
                if (_isDragging && _dragSourceIndex == i) continue;

                const Item& item = _inventory->getItem(i);
                auto sprite = Sprite::create(item.iconPath);
                if (!sprite) continue;
                
                // Calculate Row/Col
                int row = i / 12; // 0, 1, 2
                int col = i % 12; // 0..11
                
                // Calculate Local Position in Image Space
                float localX = BACKPACK_ROW1_LEFT + (col * (BACKPACK_CELL_W + BACKPACK_H_GAP));
                // Add half width to get center
                float centerX = localX + BACKPACK_CELL_W * 0.5f;
                
                float localBottomY = 0;
                if (row == 0) localBottomY = BACKPACK_ROW1_BOTTOM;
                else if (row == 1) localBottomY = BACKPACK_ROW1_BOTTOM - BACKPACK_V_GAP_1_2 - BACKPACK_CELL_H; // 464
                else if (row == 2) localBottomY = BACKPACK_ROW1_BOTTOM - BACKPACK_V_GAP_1_2 - BACKPACK_CELL_H - BACKPACK_V_GAP_2_3 - BACKPACK_CELL_H; // 395
                
                float centerY = localBottomY + BACKPACK_CELL_H * 0.5f;
                
                // Convert to Screen Coordinates
                float screenX = bpLeft + (centerX * bpScale);
                float screenY = bpBottom + (centerY * bpScale);
                
                // Scale Item
                // Fit into 59x60
                float maxW = BACKPACK_CELL_W * bpScale * 0.8f;
                float maxH = BACKPACK_CELL_H * bpScale * 0.8f;
                 
                float sX = maxW / sprite->getContentSize().width;
                float sY = maxH / sprite->getContentSize().height;
                float iScale = std::min(sX, sY);
                
                sprite->setScale(iScale);
                sprite->setPosition(Vec2(screenX, screenY));
                addChild(sprite, 20); // Z=20 for backpack items (backpack is Z=2)
                _itemSprites.push_back(sprite); // Add to list for cleanup
                
                // Quantity
                if (item.maxStack > 1 && item.quantity > 1)
                {
                    auto label = Label::createWithSystemFont(std::to_string(item.quantity), "Arial", 16);
                    if (label)
                    {
                        label->setColor(Color3B::BLACK);
                        label->enableBold();
                        label->setAnchorPoint(Vec2(1.0f, 0.0f));
                        label->setPosition(Vec2(screenX + BACKPACK_CELL_W * bpScale * 0.45f, screenY - BACKPACK_CELL_H * bpScale * 0.45f));
                        addChild(label, 21);
                        _quantityLabels.push_back(label);
                    }
                }
            }
        }
    }
}

void HudLayer::updateSelectorPosition()
{
    if (!_inventory || !_toolbar || !_selector) return;

    float scale = _toolbar->getScale();
    Size originalSize = _toolbar->getContentSize();
    Vec2 toolbarPos = _toolbar->getPosition();

    float toolbarScreenLeft = toolbarPos.x - (originalSize.width * scale * 0.5f);
    float toolbarScreenBottom = toolbarPos.y;

    float scaledLeftMargin = RAW_LEFT_MARGIN * scale;
    float scaledBottomMargin = RAW_BOTTOM_MARGIN * scale;
    float scaledCellWidth = RAW_CELL_WIDTH * scale;
    float scaledGap = RAW_GAP * scale;

    float startCenterX = toolbarScreenLeft + scaledLeftMargin + (scaledCellWidth * 0.5f);
    float startCenterY = toolbarScreenBottom + scaledBottomMargin + (RAW_CELL_HEIGHT * scale * 0.5f);
    float stepX = scaledCellWidth + scaledGap;

    int sel = _inventory->getSelectedSlot();
    float cx = startCenterX + (sel * stepX);
    float cy = startCenterY;
    _selector->setPosition(Vec2(cx, cy));

    Size selSize = _selector->getContentSize();
    if (selSize.width > 0) {
        float targetSelW = scaledCellWidth * 1.1f;
        _selector->setScale(targetSelW / selSize.width);
    }
}

int HudLayer::getSlotIndexFromPoint(const Vec2& p)
{
    if (!_backpack || !_backpack->isVisible()) return -1;
    
    Vec2 localPos = _backpack->convertToNodeSpace(p);
    Size bpSize = _backpack->getContentSize();

    Rect bounds(0, 0, bpSize.width, bpSize.height);
    if (!bounds.containsPoint(localPos)) return -1;

    float localX = localPos.x;
    float localY = localPos.y;

    if (localX < BACKPACK_ROW1_LEFT) return -1;
    
    float relX = localX - BACKPACK_ROW1_LEFT;
    float colStep = BACKPACK_CELL_W + BACKPACK_H_GAP;
    int col = (int)(relX / colStep);
    
    // Check if within the cell width (not in the gap)
    float withinCell = relX - (col * colStep);
    if (withinCell > BACKPACK_CELL_W) col = -1; // Clicked in gap
    
    if (col < 0 || col >= 12) return -1;
        
    int row = -1;
    if (localY >= BACKPACK_ROW1_BOTTOM && localY <= BACKPACK_ROW1_BOTTOM + BACKPACK_CELL_H)
    {
        row = 0;
    }
    else
    {
        float r2Bot = BACKPACK_ROW1_BOTTOM - BACKPACK_V_GAP_1_2 - BACKPACK_CELL_H;
        if (localY >= r2Bot && localY <= r2Bot + BACKPACK_CELL_H)
        {
            row = 1;
        }
        else
        {
            float r3Bot = r2Bot - BACKPACK_V_GAP_2_3 - BACKPACK_CELL_H;
            if (localY >= r3Bot && localY <= r3Bot + BACKPACK_CELL_H)
            {
                row = 2;
            }
        }
    }
    
    if (row != -1)
    {
        return (row * 12) + col;
    }
    
    return -1;
}

bool HudLayer::isPointInCloseButton(const Vec2& p)
{
    if (!_backpack || !_backpack->isVisible()) return false;
    
    Vec2 localPos = _backpack->convertToNodeSpace(p);
    Rect btnRect(CLOSE_BTN_LEFT, CLOSE_BTN_BOTTOM, CLOSE_BTN_W, CLOSE_BTN_H);
    return btnRect.containsPoint(localPos);
}

bool HudLayer::isPointInTrashCan(const Vec2& p)
{
    if (!_backpack || !_backpack->isVisible()) return false;
    
    Vec2 localPos = _backpack->convertToNodeSpace(p);
    Rect trashRect(TRASH_LEFT, TRASH_BOTTOM, TRASH_W, TRASH_H);
    return trashRect.containsPoint(localPos);
}

void HudLayer::onMouseMove(Event* event)
{
    if (_isDragging) {
        return;
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

    // 优先检测背包 (Backpack)
    if (_backpack && _backpack->isVisible())
    {
        // 1. 检测关闭按钮
        if (isPointInCloseButton(clickPos))
        {
            _backpack->setVisible(false);
            _consumingClick = true;
            event->stopPropagation();
            updateInventoryUI();
            return;
        }

        // 2. 检测背包格子 -> 处理拖拽
        int slotIndex = getSlotIndexFromPoint(clickPos);
        if (slotIndex != -1)
        {
            if (_inventory->hasItem(slotIndex))
            {
                _isDragging = true;
                _dragSourceIndex = slotIndex;
                _consumingClick = true;
                event->stopPropagation();

                // 生成拖拽图标
                const Item& item = _inventory->getItem(slotIndex);
                _draggedItemSprite = Sprite::create(item.iconPath);
                if (_draggedItemSprite)
                {
                    _draggedItemSprite->setOpacity(180);
                    _draggedItemSprite->setScale(1.0f); // 保持原始比例或微调
                    _draggedItemSprite->setPosition(clickPos);
                    addChild(_draggedItemSprite, 1000);

                    if (item.quantity > 1) {
                        _draggedItemQty = Label::createWithSystemFont(std::to_string(item.quantity), "Arial", 16);
                        _draggedItemQty->setColor(Color3B::WHITE);
                        _draggedItemQty->enableOutline(Color4B::BLACK, 2);
                        _draggedItemQty->setPosition(Vec2(clickPos.x + 20, clickPos.y - 20));
                        addChild(_draggedItemQty, 1001);
                    }
                }
                updateInventoryUI();
            }
            // 即使是空格子，也算点击在了背包上，阻止事件继续向下传给工具栏
            return;
        }

        // 检测是否点在了背包背景图上
        Vec2 localP = _backpack->convertToNodeSpace(clickPos);
        Size bpSize = _backpack->getContentSize();
        if (Rect(0, 0, bpSize.width, bpSize.height).containsPoint(localP)) {
            _consumingClick = true;
            event->stopPropagation();
            return; // 吞噬事件，不做任何事，但也不让工具栏响应
        }

        return;
    }

    // 检测工具栏 (Toolbar)

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
        return;
    }

}

void HudLayer::onMouseUp(Event* event)
{
    _consumingClick = false;
    
    if (_isDragging)
    {
        EventMouse* e = (EventMouse*)event;
        Vec2 pos = e->getLocation();
        
        bool actionTaken = false;
        
        // Check Trash Can
        if (isPointInTrashCan(pos))
        {
            const auto& item = _inventory->getItem(_dragSourceIndex);
            if (item.type != ItemType::Tool)
            {
                _inventory->removeItem(_dragSourceIndex, 9999); // Remove all
                actionTaken = true;
            }
        }
        else
        {
            // Check Drop Target
            int targetSlot = getSlotIndexFromPoint(pos);
            if (targetSlot != -1)
            {
                // Swap
                _inventory->swapItems(_dragSourceIndex, targetSlot);
                actionTaken = true;
            }
        }
        
        // Cleanup
        if (_draggedItemSprite) {
            _draggedItemSprite->removeFromParent();
            _draggedItemSprite = nullptr;
        }
        if (_draggedItemQty) {
            _draggedItemQty->removeFromParent();
            _draggedItemQty = nullptr;
        }
        
        _isDragging = false;
        _dragSourceIndex = -1;
        
        updateInventoryUI();
    }
}

bool HudLayer::onTouchBegan(Touch* touch, Event* event)
{
    if (!_inventory || !_toolbar) return false;

    // 获取点击位置
    Vec2 p = touch->getLocation();

    // 优先检测背包 (Backpack) - 逻辑与 onMouseDown 保持一致
    if (_backpack && _backpack->isVisible())
    {
        // 检测关闭按钮
        if (isPointInCloseButton(p))
        {
            _backpack->setVisible(false);
            _consumingClick = true; // 标记正在处理点击
            updateInventoryUI();
            return true; // return true 表示吞噬事件，不再向下传递
        }

        // 检测背包格子 -> 开始拖拽
        int slotIndex = getSlotIndexFromPoint(p);
        if (slotIndex != -1)
        {
            if (_inventory->hasItem(slotIndex))
            {
                _isDragging = true;
                _dragSourceIndex = slotIndex;
                _consumingClick = true;

                // 创建拖拽图标
                const Item& item = _inventory->getItem(slotIndex);
                _draggedItemSprite = Sprite::create(item.iconPath);
                if (_draggedItemSprite)
                {
                    _draggedItemSprite->setOpacity(180);
                    _draggedItemSprite->setScale(1.0f);
                    _draggedItemSprite->setPosition(p);
                    addChild(_draggedItemSprite, 1000);

                    if (item.quantity > 1) {
                        _draggedItemQty = Label::createWithSystemFont(std::to_string(item.quantity), "Arial", 16);
                        _draggedItemQty->setColor(Color3B::WHITE);
                        _draggedItemQty->enableOutline(Color4B::BLACK, 2);
                        _draggedItemQty->setPosition(Vec2(p.x + 20, p.y - 20));
                        addChild(_draggedItemQty, 1001);
                    }
                }
                updateInventoryUI();
            }
            return true; // 吞噬事件
        }

        // 如果点在背包背景内，吞噬事件，防止穿透到工具栏
        Vec2 localP = _backpack->convertToNodeSpace(p);
        Size bpSize = _backpack->getContentSize();
        if (Rect(0, 0, bpSize.width, bpSize.height).containsPoint(localP)) {
            _consumingClick = true;
            return true;
        }
    }

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

void HudLayer::onTouchMoved(Touch* touch, Event* event)
{
    // 只有在拖拽状态下才更新位置
    if (_isDragging && _draggedItemSprite)
    {
        // touch->getLocation() 返回的是 GL 坐标
        Vec2 loc = touch->getLocation();

        _draggedItemSprite->setPosition(loc);

        // 如果有数字标签，也跟着一起动
        if (_draggedItemQty)
        {
            _draggedItemQty->setPosition(Vec2(loc.x + 20, loc.y - 20));
        }
    }
}

void HudLayer::onTouchEnded(Touch* touch, Event* event)
{
    _consumingClick = false;

    // 如果正在拖拽，处理物品放下逻辑
    if (_isDragging)
    {
        // 获取松手时的坐标 (Touch 坐标是准确的)
        Vec2 pos = touch->getLocation();

        bool actionTaken = false;

        // 检测是否扔进垃圾桶
        if (isPointInTrashCan(pos))
        {
            const auto& item = _inventory->getItem(_dragSourceIndex);
            if (item.type != ItemType::Tool)
            {
                _inventory->removeItem(_dragSourceIndex, 9999); // 删除全部
                actionTaken = true;
            }
        }
        else
        {
            // 检测是否放入背包格子 (交换/放置)
            int targetSlot = getSlotIndexFromPoint(pos);
            if (targetSlot != -1)
            {
                _inventory->swapItems(_dragSourceIndex, targetSlot);
                actionTaken = true;
            }
        }

        // 清理拖拽的图标
        if (_draggedItemSprite) {
            _draggedItemSprite->removeFromParent();
            _draggedItemSprite = nullptr;
        }
        if (_draggedItemQty) {
            _draggedItemQty->removeFromParent();
            _draggedItemQty = nullptr;
        }

        // 重置状态
        _isDragging = false;
        _dragSourceIndex = -1;

        // 刷新界面
        updateInventoryUI();
    }
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
