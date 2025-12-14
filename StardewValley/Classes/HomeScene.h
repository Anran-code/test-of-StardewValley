#ifndef __HOME_SCENE_H__
#define __HOME_SCENE_H__

#include "cocos2d.h"

class HomeScene : public cocos2d::Scene
{
public:
    // 创建场景的静态方法
    static cocos2d::Scene* createScene();

    // 初始化方法
    virtual bool init();

    // 回调函数：当点击“开始游戏”时触发
    void onStartGameClicked(cocos2d::Ref* sender);

    // 回调函数：退出游戏
    void onExitClicked(cocos2d::Ref* sender);

    // 实现 CREATE_FUNC 宏，它会自动生成 create() 方法
    CREATE_FUNC(HomeScene);
};

#endif // __HOME_SCENE_H__