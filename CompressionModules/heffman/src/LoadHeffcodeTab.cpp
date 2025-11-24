#include "../include/LoadHeffcodeTab.h"

LoadHeffcodeTab::LoadHeffcodeTab(Heffman* heffcore):
heffman(heffcore)
{ }

LoadHeffcodeTab::~LoadHeffcodeTab()
{ }

void LoadHeffcodeTab::work(Datacmnctor* datacmnctor)
{
    auto type = datacmnctor->get_value();
    try
    {
        HeffmapType& heffmap = dynamic_cast<HeffmapType&>(type);
        heffman->receiveHashtab(heffmap.hefftab);
        heffman->gen_hefftree();
    }
    catch(std::bad_cast& e)
    {
        //FIXME: 处理向下转型异常
    }
}