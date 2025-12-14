#include "HomeScene.h"
// #include "GameScene.h" // 假设你有一个游戏场景，需要引用头文件

USING_NS_CC;

Scene* HomeScene::createScene()
{
    // 直接创建并返回 HomeScene 对象
    return HomeScene::create();
}

// on "init" you need to initialize your instance
bool HomeScene::init()
{
    //////////////////////////////
    // 1. 调用父类的 init
    if (!Scene::init())
    {
        return false;
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 2. 添加一个背景 (这里使用简单的蓝色背景，你可以换成 Sprite)
    auto backgroundLayer = Sprite::create("home_bg.png");
    // 设置位置为屏幕中心
    backgroundLayer->setPosition(Vec2(visibleSize.width / 2 + origin.x, visibleSize.height / 2 + origin.y));

    this->addChild(backgroundLayer, 0);


    return true;
}


// 点击“退出”的回调
void HomeScene::onExitClicked(Ref* sender)
{
    // 结束导演类，退出程序
    Director::getInstance()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    exit(0);
#endif
}