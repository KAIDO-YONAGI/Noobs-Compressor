#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "include/Datacmnctor.h"
#include "include/Strategy.h"
#include "include/Worker.h"
#include <vector>
#include "include/Strategy.h"
class Scheduler
{
public:
    Scheduler(/* args */);
    ~Scheduler();

private:
    std::vector<Strategy*> strategys;
    std::vector<std::vector<Worker*>> worker_groups;
    std::vector<Datacmnctor*> datacmnctors;

public:
    
};

Scheduler::Scheduler(/* args */)
{
 
}

Scheduler::~Scheduler()
{

}


#endif //SCHEDULER_H
