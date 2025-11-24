#include "../include/GenHeffcodeTab.h"

GenHeffcodeTab::GenHeffcodeTab(Heffman* heffcore):
heffman(heffcore)
{ }

GenHeffcodeTab::~GenHeffcodeTab()
{ }

void GenHeffcodeTab::work(Datacmnctor* datacmnctor)
{
    datacmnctor->ready_put_value();
    auto type = datacmnctor->get_value();
    try
    {
        HeffmapType& heffmap = dynamic_cast<HeffmapType&>(type);
        
        heffman->merge_ttabs();
        heffman->gen_hefftree();
        heffman->save_code_inTab();
        
        heffmap.hefftab = heffman->getHashtab();
    }
    catch(std::bad_cast& e)
    {
        //FIXME: 处理向下转型异常
    }  
}

