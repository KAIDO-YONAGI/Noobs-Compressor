#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "include/DataConnector.h"
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
    std::vector<Strategy*> strategies;
    std::vector<std::vector<Worker*>> workerGroups;
    std::vector<DataConnector*> dataConnectors;

public:
    
};

Scheduler::Scheduler(/* args */)
{
 
}

Scheduler::~Scheduler()
{

}


#endif //SCHEDULER_H
