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
        _fishChangeIntervalMin = 0.4f;
        _fishChangeIntervalMax = 1.0f;
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

    void setupDifficulty(float speed, float intervalMin, float intervalMax)
    {
        _fishSpeed = speed;
        _fishChangeIntervalMin = intervalMin;
        _fishChangeIntervalMax = intervalMax;
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
    float _fishChangeIntervalMin = 0.4f;
    float _fishChangeIntervalMax = 1.0f;
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
            _fishChangeTimer = cocos2d::RandomHelper::random_real(_fishChangeIntervalMin, _fishChangeIntervalMax);
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

        // Layout constants
        float trackWidth = _width * 0.75f;
        float meterWidth = _width * 0.20f;
        float gap = _width * 0.05f;

        // 1. Draw Water Track
        cocos2d::Vec2 trackBL(0.0f, 0.0f);
        cocos2d::Vec2 trackTR(trackWidth, _height);
        
        // Deep blue background for water
        _draw->drawSolidRect(trackBL, trackTR, cocos2d::Color4F(0.0f, 0.1f, 0.4f, 0.9f));
        // Gold/Brown border
        _draw->drawRect(trackBL, trackTR, cocos2d::Color4F(0.8f, 0.6f, 0.2f, 1.0f));

        // 2. Draw Green Bar (The "Bobber" control)
        float half = _barHeight * 0.5f;
        float barMin = _barPos - half;
        float barMax = _barPos + half;
        if (barMin < 0.0f) barMin = 0.0f;
        if (barMax > _height) barMax = _height;
        
        float barMargin = 4.0f;
        cocos2d::Vec2 barBL(barMargin, barMin);
        cocos2d::Vec2 barTR(trackWidth - barMargin, barMax);
        
        _draw->drawSolidRect(barBL, barTR, cocos2d::Color4F(0.4f, 0.8f, 0.2f, 0.6f)); // Semi-transparent green body
        _draw->drawRect(barBL, barTR, cocos2d::Color4F(0.2f, 0.6f, 0.1f, 1.0f)); // Dark green border

        // 3. Draw Fish
        float fishSize = 16.0f;
        float fy1 = _fishPos - fishSize * 0.5f;
        float fy2 = _fishPos + fishSize * 0.5f;
        float fishCenterX = trackWidth * 0.5f;
        
        cocos2d::Vec2 fishBL(fishCenterX - fishSize * 0.5f, fy1);
        cocos2d::Vec2 fishTR(fishCenterX + fishSize * 0.5f, fy2);
        
        _draw->drawSolidRect(fishBL, fishTR, cocos2d::Color4F(1.0f, 0.8f, 0.2f, 1.0f)); // Orange/Yellow fish
        _draw->drawRect(fishBL, fishTR, cocos2d::Color4F(0.8f, 0.4f, 0.0f, 1.0f));

        // 4. Draw Progress Meter (Vertical on right)
        if (_meterDraw)
        {
            cocos2d::Vec2 meterBL(trackWidth + gap, 0.0f);
            cocos2d::Vec2 meterTR(trackWidth + gap + meterWidth, _height);

            // Dark background for meter
            _meterDraw->drawSolidRect(meterBL, meterTR, cocos2d::Color4F(0.1f, 0.1f, 0.1f, 0.8f));
            _meterDraw->drawRect(meterBL, meterTR, cocos2d::Color4F(0.0f, 0.0f, 0.0f, 1.0f));

            if (_progress > 0.0f)
            {
                float fillHeight = _height * _progress;
                cocos2d::Vec2 fillTR(trackWidth + gap + meterWidth, fillHeight);
                
                // Green to Gold gradient logic could be added, but solid green is fine
                _meterDraw->drawSolidRect(meterBL, fillTR, cocos2d::Color4F(0.2f, 0.9f, 0.2f, 1.0f));
            }
        }
    }
};

#endif
