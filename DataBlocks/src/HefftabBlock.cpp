#include "../include/HefftabBlock.h"

HefftabBlock::HefftabBlock(Heffmap* hftab):
hftab(hftab)
{ }

HefftabBlock::~HefftabBlock()
{ }

void HefftabBlock::ready_put_value()
{
    delete hftab.hefftab;
    hftab.hefftab = NULL;
}

Type& HefftabBlock::get_value()
{
    return hftab;
}

HashmapType::HashmapType(Heffmap *hftab):
hefftab(hftab)
{ }

HashmapType::~HashmapType() 
{ }
