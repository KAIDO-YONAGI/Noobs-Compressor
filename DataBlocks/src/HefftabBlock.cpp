#include "../include/HefftabBlock.h"

HefftabBlock::HefftabBlock(Heffmap* hftab):
hftab(hftab)
{ }

HefftabBlock::~HefftabBlock()
{ }

void HefftabBlock::ready_put_value()
{
    if(hftab.hefftab == NULL)
        return;
    delete hftab.hefftab;
    hftab.hefftab = NULL;
}

Type& HefftabBlock::get_value()
{
    return hftab;
}

HeffmapType::HeffmapType(Heffmap *hftab):
hefftab(hftab)
{ }

HeffmapType::~HeffmapType() 
{ }
