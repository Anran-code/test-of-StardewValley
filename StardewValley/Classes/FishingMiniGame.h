#ifndef __FISHING_MINI_GAME_H__
#define __FISHING_MINI_GAME_H__

#include "cocos2d.h"

class FishingMiniGame : public cocos2d::Node
{
public:
    static FishingMiniGame* create(const cocos2d::Size& size)
    {
        auto p = new (std::nothrow) FishingMiniGame();
        if (p && p->initWithSize(size))
        {
            p->autorelease();
            return p;
        }
        delete p;
        return nullptr;
    }

    bool initWithSize(const cocos2d::Size& size)
    {
        if (!Node::init())
        {
            return false;
        }
        _width = size.width;
        _height = size.height;
        setContentSize(size);
        _draw = cocos2d::DrawNode::create();
        addChild(_draw);
        _meterDraw = cocos2d::DrawNode::create();
        addChild(_meterDraw);
        _barHeight = _height * 0.25f;
        _barPos = _height * 0.5f;
        _barVel = 0.0f;
        _gravity = 200.0f;
        _boost = 300.0f;
        _holding = false;
        _fishPos = _height * 0.5f;
        _fishTarget = _fishPos;
        _fishSpeed = 120.0f;
        _fishChangeTimer = 0.5f;
        _progress = 0.0f;
        _finished = false;
        _success = false;
        _inputActive = true;
        _elapsed = 0.0f;
        _maxDuration = 15.0f;
        scheduleUpdate();
        redraw();
        return true;
    }

    virtual void update(float dt) override
    {
        if (_finished)
        {
            return;
        }
        updateBar(dt);
        updateFish(dt);
        updateProgress(dt);
        _elapsed += dt;
        if (!_finished && _elapsed >= _maxDuration)
        {
            _finished = true;
            _success = false;
        }
        redraw();
    }

    void setInputActive(bool active)
    {
        _inputActive = active;
        if (!active)
        {
            _holding = false;
        }
    }

    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode)
    {
        if (!_inputActive)
        {
            return;
        }
        if (keyCode == cocos2d::EventKeyboard::KeyCode::KEY_SPACE)
        {
            _holding = true;
        }
    }

    void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode)
    {
        if (keyCode == cocos2d::EventKeyboard::KeyCode::KEY_SPACE)
        {
            _holding = false;
        }
    }

    void restart()
    {
        _barPos = _height * 0.5f;
        _barVel = 0.0f;
        _holding = false;
        _fishPos = _height * 0.5f;
        _fishTarget = _fishPos;
        _fishChangeTimer = 0.5f;
        _progress = 0.0f;
        _finished = false;
        _success = false;
        _inputActive = true;
        _elapsed = 0.0f;
        redraw();
    }

    bool isFinished() const
    {
        return _finished;
    }

    bool isSuccess() const
    {
        return _success;
    }

private:
    cocos2d::DrawNode* _draw = nullptr;
    cocos2d::DrawNode* _meterDraw = nullptr;
    float _width = 0.0f;
    float _height = 0.0f;
    float _barHeight = 0.0f;
    float _barPos = 0.0f;
    float _barVel = 0.0f;
    bool _holding = false;
    float _gravity = 0.0f;
    float _boost = 0.0f;
    float _fishPos = 0.0f;
    float _fishTarget = 0.0f;
    float _fishSpeed = 0.0f;
    float _fishChangeTimer = 0.0f;
    float _progress = 0.0f;
    bool _finished = false;
    bool _success = false;
    bool _inputActive = false;
    float _elapsed = 0.0f;
    float _maxDuration = 0.0f;

    void updateBar(float dt)
    {
        if (!_inputActive)
        {
            return;
        }
        if (_holding)
        {
            _barVel += _boost * dt;
        }
        else
        {
            _barVel -= _gravity * dt;
        }
        _barPos += _barVel * dt;
        float half = _barHeight * 0.5f;
        float minY = half;
        float maxY = _height - half;
        if (_barPos < minY)
        {
            _barPos = minY;
            _barVel = 0.0f;
        }
        else if (_barPos > maxY)
        {
            _barPos = maxY;
            _barVel = 0.0f;
        }
    }

    void updateFish(float dt)
    {
        _fishChangeTimer -= dt;
        if (_fishChangeTimer <= 0.0f)
        {
            float margin = _barHeight * 0.5f;
            float minY = margin;
            float maxY = _height - margin;
            float r = cocos2d::RandomHelper::random_real(0.0f, 1.0f);
            _fishTarget = minY + (maxY - minY) * r;
            _fishChangeTimer = cocos2d::RandomHelper::random_real(0.4f, 1.0f);
        }
        float diff = _fishTarget - _fishPos;
        float maxStep = _fishSpeed * dt;
        if (diff > maxStep)
        {
            diff = maxStep;
        }
        else if (diff < -maxStep)
        {
            diff = -maxStep;
        }
        _fishPos += diff;
    }

    void updateProgress(float dt)
    {
        float half = _barHeight * 0.5f;
        float barMin = _barPos - half;
        float barMax = _barPos + half;
        bool inBar = (_fishPos >= barMin && _fishPos <= barMax);
        float inc = 0.7f;
        float dec = 0.6f;
        if (inBar)
        {
            _progress += inc * dt;
        }
        else
        {
            _progress -= dec * dt;
        }
        if (_progress < 0.0f)
        {
            _progress = 0.0f;
        }
        if (_progress >= 1.0f)
        {
            _progress = 1.0f;
            _finished = true;
            _success = true;
        }
    }

    void redraw()
    {
        if (!_draw)
        {
            return;
        }
        _draw->clear();
        if (_meterDraw)
        {
            _meterDraw->clear();
        }
        cocos2d::Vec2 a(0.0f, 0.0f);
        cocos2d::Vec2 b(_width, _height);
        _draw->drawSolidRect(a, b, cocos2d::Color4F(0.0f, 0.0f, 0.0f, 0.5f));
        _draw->drawRect(a, b, cocos2d::Color4F(1.0f, 1.0f, 1.0f, 1.0f));

        float half = _barHeight * 0.5f;
        float barMin = _barPos - half;
        float barMax = _barPos + half;
        if (barMin < 0.0f)
        {
            barMin = 0.0f;
        }
        if (barMax > _height)
        {
            barMax = _height;
        }
        float barLeft = _width * 0.15f;
        float barRight = _width * 0.45f;
        cocos2d::Vec2 bar1(barLeft, barMin);
        cocos2d::Vec2 bar2(barRight, barMax);
        _draw->drawSolidRect(bar1, bar2, cocos2d::Color4F(0.2f, 0.8f, 0.2f, 0.8f));
        _draw->drawRect(bar1, bar2, cocos2d::Color4F(0.0f, 1.0f, 0.0f, 1.0f));

        float fishHalf = 6.0f;
        float fy1 = _fishPos - fishHalf;
        float fy2 = _fishPos + fishHalf;
        float fishLeft = barLeft + (barRight - barLeft) * 0.25f;
        float fishRight = barLeft + (barRight - barLeft) * 0.75f;
        cocos2d::Vec2 fish1(fishLeft, fy1);
        cocos2d::Vec2 fish2(fishRight, fy2);
        _draw->drawSolidRect(fish1, fish2, cocos2d::Color4F(1.0f, 1.0f, 0.0f, 1.0f));
        _draw->drawRect(fish1, fish2, cocos2d::Color4F(1.0f, 1.0f, 0.0f, 1.0f));

        if (_meterDraw)
        {
            float meterHeight = 10.0f;
            float meterY2 = _height - 4.0f;
            float meterY1 = meterY2 - meterHeight;
            if (meterY1 < 0.0f)
            {
                meterY1 = 0.0f;
                meterY2 = meterHeight;
            }
            cocos2d::Vec2 m1(0.0f, meterY1);
            cocos2d::Vec2 m2(_width, meterY2);
            _meterDraw->drawRect(m1, m2, cocos2d::Color4F(1.0f, 1.0f, 1.0f, 1.0f));
            float fillWidth = _width * _progress;
            cocos2d::Vec2 f1(0.0f, meterY1);
            cocos2d::Vec2 f2(fillWidth, meterY2);
            _meterDraw->drawSolidRect(f1, f2, cocos2d::Color4F(0.0f, 0.6f, 1.0f, 0.9f));
        }
    }
};

#endif
