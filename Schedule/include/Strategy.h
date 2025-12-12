#ifndef STRATEGY_H_INTERFACE
#define STRATEGY_H_INTERFACE

#include "Worker.h"
#include "Datacmnctor.h"
#include <vector>
/**
 * 为策略模式设计的策略接口
 * 具体的策略算法类继承该类，以便
 * 使用不同的策略。
 * 
 * 纯虚函数：
 *     use_strategy(//args);
 *     接受需要被该策略调度的一组工人，
 *     和数据交互接口。
 */

class Strategy 
{
public:
    virtual ~Strategy() = default;

private:

public:
    virtual void use_strategy(
        std::vector<Worker*>,
        Datacmnctor*
    ) = 0;
};

#endif //STRATEGY_H_INTERFACE
