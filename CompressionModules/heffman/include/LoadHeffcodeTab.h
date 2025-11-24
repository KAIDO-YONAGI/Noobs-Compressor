#ifndef LOADHEFFCODETAB_H
#define LOADHEFFCODETAB_H

#include <memory>
#include "heffman.h"
#include "../../Schedule/include/Datacmnctor.h"
#include "../../Schedule/include/Worker.h"
#include "../../../DataBlocks/include/HefftabBlock.h"
#include "../../hefftype/Heffman_type.h"

/**
 * 赫夫曼功能模块之一：接受编码表（从文件中获
 * 取），并构建解码树。
 */
class LoadHeffcodeTab: public Worker
{
public:
    LoadHeffcodeTab(Heffman*);
    ~LoadHeffcodeTab();

    void work(Datacmnctor*) override;

private:
    std::shared_ptr<Heffman> heffman;
};

#endif //LOADHEFFCODETAB_H