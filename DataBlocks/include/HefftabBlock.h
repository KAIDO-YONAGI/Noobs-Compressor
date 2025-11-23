#ifndef HEFFTABBLOCK_H
#define HEFFTABBLOCK_H

#include "../../CompressionModules/hefftype/Heffman_type.h"
#include "../../Schedule/include/Datacmnctor.h"

/**
 * 赫夫曼编码表数据类
 * 实现了数据传输接口，使用方法：
 * 使用接口取出Type，向下转型为HashmapType，
 * 然后调用其中的get_hefftab()方法
 */
class HefftabBlock: public Datacmnctor
{
public:
    HefftabBlock(Heffmap*);
    ~HefftabBlock();

    void ready_put_value() override;
    Type& get_value() override;

private:
    HashmapType hftab;
};

/**
 * 包装的编码表类型
 */
struct HashmapType: public Type
{
    HashmapType(Heffmap*);
    ~HashmapType();

    Heffmap *hefftab;
};

#endif //HEFFTABBLOCK_H