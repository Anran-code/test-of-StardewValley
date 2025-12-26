#include "GameScene.h"
#include "CropSystem.h"
#include "ForageSystem.h"

USING_NS_CC;

std::unordered_map<int, BackgroundLayer::ObstacleSaveData> BackgroundLayer::sSavedObstacles;
bool BackgroundLayer::sObstaclesInitialized = false;
int BackgroundLayer::sLastObstacleSeason = -1;

void BackgroundLayer::spawnObstacles(int count)
{
    if (!_map) return;
    Size mapSize = _map->getMapSize();
    Size tileSize = _map->getTileSize();
    float mapHeight = mapSize.height * tileSize.height;

    for (int i = 0; i < count; ++i)
    {
        int x = RandomHelper::random_int(0, (int)mapSize.width - 1);
        int y = RandomHelper::random_int(0, (int)mapSize.height - 1);

        int cx = mapSize.width / 2;
        int cy = mapSize.height / 2;
        if (abs(x - cx) < 3 && abs(y - cy) < 3) continue;

        float cxPos = (x + 0.5f) * tileSize.width;
        float cyPos = mapHeight - (y + 0.5f) * tileSize.height;
        Vec2 pos(cxPos, cyPos);
        Rect tileRect(x * tileSize.width, mapHeight - (y + 1) * tileSize.height, tileSize.width, tileSize.height);

        if (_hasHomeRect && (_homeRect.intersectsRect(tileRect) || _homeDoorRect.intersectsRect(tileRect) || _homeDoorTunnelRect.intersectsRect(tileRect))) continue;
        if (_hasHenhouseRect && (_henhouseRect.intersectsRect(tileRect) || _henhouseDoorRect.intersectsRect(tileRect))) continue;

        if (_hasHomeRect)
        {
            float bufferSize = tileSize.width * 3.0f;
            Rect doorBuffer = _homeDoorRect;
            doorBuffer.origin.x -= bufferSize;
            doorBuffer.origin.y -= bufferSize;
            doorBuffer.size.width += bufferSize * 2.0f;
            doorBuffer.size.height += bufferSize * 2.0f;

            if (doorBuffer.intersectsRect(tileRect)) continue;
        }

        if (_hasRightExit && _rightExitRect.intersectsRect(tileRect)) continue;
        if (_hasPoolRect && !_poolRects.empty())
        {
            bool insidePool = false;
            for (const auto& r : _poolRects)
            {
                if (r.intersectsRect(tileRect))
                {
                    insidePool = true;
                    break;
                }
            }
            if (insidePool) continue;
        }

        if (_hasBoundary)
        {
            if (tileRect.getMinX() < _boundaryLeftRect.getMaxX()) continue;
            if (tileRect.getMaxX() > _boundaryRightRect.getMinX()) continue;
            if (tileRect.getMinY() < _boundaryBottomRect.getMaxY()) continue;
            if (tileRect.getMaxY() > _boundaryTopRect.getMinY()) continue;
        }

        if (hasObstacle(Vec2(x, y))) continue;

        if (!CropSystem::getInstance()->canTill(Vec2(x, y))) continue;

        int type = RandomHelper::random_int(0, 2);

        int key = y * (int)mapSize.width + x;

        std::string file;
        if (type == 0) file = "block/Wood.png";
        else if (type == 1) file = "block/Stone.png";
        else if (type == 2) file = "block/Fiber.png";

        auto sprite = Sprite::create(file);
        if (sprite)
        {
            float cxPos2 = (x + 0.5f) * tileSize.width;
            float cyPos2 = mapHeight - (y + 0.5f) * tileSize.height;
            sprite->setPosition(Vec2(cxPos2, cyPos2));

            if (sprite->getContentSize().width > tileSize.width)
            {
                sprite->setScale(tileSize.width / sprite->getContentSize().width);
            }

            int zOrder = static_cast<int>(mapHeight - cyPos2);
            addChild(sprite, zOrder);

            Obstacle obs;
            obs.type = type;
            obs.sprite = sprite;
            obs.active = true;
            _obstacles[key] = obs;

            ObstacleSaveData data;
            data.type = type;
            data.active = true;
            sSavedObstacles[key] = data;
        }
    }
}

void BackgroundLayer::initObstacles()
{
    if (!_map) return;
    Size mapSize = _map->getMapSize();
    Size tileSize = _map->getTileSize();
    float mapHeight = mapSize.height * tileSize.height;

    int currentSeason = -1;
    if (GameScene::sClock)
    {
        currentSeason = (int)GameScene::sClock->getSeason();
    }

    if (sObstaclesInitialized)
    {
        for (const auto& pair : sSavedObstacles)
        {
            int key = pair.first;
            const auto& data = pair.second;

            if (!data.active) continue;

            int x = key % (int)mapSize.width;
            int y = key / (int)mapSize.width;

            std::string file;
            if (data.type == 0) file = "block/Wood.png";
            else if (data.type == 1) file = "block/Stone.png";
            else if (data.type == 2) file = "block/Fiber.png";

            auto sprite = Sprite::create(file);
            if (sprite)
            {
                float cxPos = (x + 0.5f) * tileSize.width;
                float cyPos = mapHeight - (y + 0.5f) * tileSize.height;
                sprite->setPosition(Vec2(cxPos, cyPos));

                if (sprite->getContentSize().width > tileSize.width)
                {
                    sprite->setScale(tileSize.width / sprite->getContentSize().width);
                }

                int zOrder = static_cast<int>(mapHeight - cyPos);
                addChild(sprite, zOrder);

                Obstacle obs;
                obs.type = data.type;
                obs.sprite = sprite;
                obs.active = true;
                _obstacles[key] = obs;
            }
        }

        if (sLastObstacleSeason != -1 && currentSeason != -1 && sLastObstacleSeason != currentSeason)
        {
            int newObstacleCount = (mapSize.width * mapSize.height) / 20;
            spawnObstacles(newObstacleCount);
            sLastObstacleSeason = currentSeason;
        }
    }
    else
    {
        int obstacleCount = (mapSize.width * mapSize.height) / 30;
        spawnObstacles(obstacleCount);
        sObstaclesInitialized = true;
        sLastObstacleSeason = currentSeason;
    }
}

bool BackgroundLayer::hasObstacle(const Vec2& tileIndex)
{
    if (!_map) return false;
    int key = (int)tileIndex.y * (int)_map->getMapSize().width + (int)tileIndex.x;
    return _obstacles.find(key) != _obstacles.end();
}

int BackgroundLayer::getObstacleType(const Vec2& tileIndex)
{
    if (!_map) return -1;
    int key = (int)tileIndex.y * (int)_map->getMapSize().width + (int)tileIndex.x;
    auto it = _obstacles.find(key);
    if (it != _obstacles.end()) return it->second.type;
    return -1;
}

bool BackgroundLayer::tryEatGrass(const Vec2& tileIndex)
{
    if (!_map) return false;
    int key = (int)tileIndex.y * (int)_map->getMapSize().width + (int)tileIndex.x;
    auto it = _obstacles.find(key);
    if (it != _obstacles.end())
    {
        if (it->second.type == 2)
        {
            removeObstacle(tileIndex);
            return true;
        }
    }
    return false;
}

bool BackgroundLayer::isColliding(const Rect& box)
{
    if (_hasBoundary)
    {
        if (box.intersectsRect(_boundaryLeftRect) || box.intersectsRect(_boundaryRightRect) ||
            box.intersectsRect(_boundaryTopRect) || box.intersectsRect(_boundaryBottomRect))
            return true;
    }

    if (_hasHomeRect && box.intersectsRect(_homeRect))
    {
        return true;
    }
    if (_hasHenhouseRect && box.intersectsRect(_henhouseRect)) return true;

    if (_hasPoolRect && !_poolRects.empty())
    {
        for (const auto& r : _poolRects)
        {
            if (box.intersectsRect(r)) return true;
        }
    }

    if (_hasHouseRect && !_houseRects.empty())
    {
        for (const auto& r : _houseRects)
        {
            if (box.intersectsRect(r)) return true;
        }
    }

    if (checkCollisionWithObstacles(box)) return true;

    return false;
}

bool BackgroundLayer::checkCollisionWithObstacles(const Rect& box)
{
    if (!_map) return false;

    Size tileSize = _map->getTileSize();
    Size mapSize = _map->getMapSize();
    float mapHeight = mapSize.height * tileSize.height;

    float minX = box.getMinX();
    float maxX = box.getMaxX();
    float minY = box.getMinY();
    float maxY = box.getMaxY();

    int tMinX = static_cast<int>(minX / tileSize.width);
    int tMaxX = static_cast<int>(maxX / tileSize.width);

    int tMinY = static_cast<int>((mapHeight - maxY) / tileSize.height);
    int tMaxY = static_cast<int>((mapHeight - minY) / tileSize.height);

    tMinX = std::max(0, std::min(tMinX, (int)mapSize.width - 1));
    tMaxX = std::max(0, std::min(tMaxX, (int)mapSize.width - 1));
    tMinY = std::max(0, std::min(tMinY, (int)mapSize.height - 1));
    tMaxY = std::max(0, std::min(tMaxY, (int)mapSize.height - 1));

    for (int x = tMinX; x <= tMaxX; ++x)
    {
        for (int y = tMinY; y <= tMaxY; ++y)
        {
            bool isObstacle = hasObstacle(Vec2(x, y));
            bool isForage = ForageSystem::getInstance()->hasItem(Vec2(x, y));

            if (isObstacle || isForage)
            {
                float cx = (x + 0.5f) * tileSize.width;
                float cy = mapHeight - (y + 0.5f) * tileSize.height;

                float shrinkFactor = 0.85f;
                float w = tileSize.width * shrinkFactor;
                float h = tileSize.height * shrinkFactor;

                Rect obsRect(cx - w * 0.5f, cy - h * 0.5f, w, h);

                if (box.intersectsRect(obsRect))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

void BackgroundLayer::removeObstacle(const Vec2& tileIndex)
{
    if (!_map) return;
    int key = (int)tileIndex.y * (int)_map->getMapSize().width + (int)tileIndex.x;
    auto it = _obstacles.find(key);
    if (it != _obstacles.end())
    {
        if (it->second.sprite) it->second.sprite->removeFromParent();
        _obstacles.erase(it);

        sSavedObstacles.erase(key);
    }
}

bool BackgroundLayer::isValidSpawnPosition(int x, int y)
{
    if (!_map) return false;
    Size mapSize = _map->getMapSize();
    Size tileSize = _map->getTileSize();
    float mapHeight = mapSize.height * tileSize.height;

    int cx = mapSize.width / 2;
    int cy = mapSize.height / 2;
    if (abs(x - cx) < 3 && abs(y - cy) < 3) return false;

    Rect tileRect(x * tileSize.width, mapHeight - (y + 1) * tileSize.height, tileSize.width, tileSize.height);

    if (_hasHomeRect && (_homeRect.intersectsRect(tileRect) || _homeDoorRect.intersectsRect(tileRect) || _homeDoorTunnelRect.intersectsRect(tileRect))) return false;
    if (_hasHenhouseRect && (_henhouseRect.intersectsRect(tileRect) || _henhouseDoorRect.intersectsRect(tileRect))) return false;

    if (_hasHomeRect)
    {
        float bufferSize = tileSize.width * 3.0f;
        Rect doorBuffer = _homeDoorRect;
        doorBuffer.origin.x -= bufferSize;
        doorBuffer.origin.y -= bufferSize;
        doorBuffer.size.width += bufferSize * 2.0f;
        doorBuffer.size.height += bufferSize * 2.0f;

        if (doorBuffer.intersectsRect(tileRect)) return false;
    }

    if (_hasRightExit && _rightExitRect.intersectsRect(tileRect)) return false;
    if (_hasPoolRect && !_poolRects.empty())
    {
        for (const auto& r : _poolRects)
        {
            if (r.intersectsRect(tileRect))
            {
                return false;
            }
        }
    }

    if (_hasBoundary)
    {
        if (tileRect.getMinX() < _boundaryLeftRect.getMaxX()) return false;
        if (tileRect.getMaxX() > _boundaryRightRect.getMinX()) return false;
        if (tileRect.getMinY() < _boundaryBottomRect.getMaxY()) return false;
        if (tileRect.getMaxY() > _boundaryTopRect.getMinY()) return false;
    }

    if (hasObstacle(Vec2(x, y))) return false;

    if (CropSystem::getInstance()->isOccupied(Vec2(x, y))) return false;

    return true;
}

