#ifndef HEFFTABBLOCK_H
#define HEFFTABBLOCK_H

#include "../../CompressionModules/hefftype/Heffman_type.h"
#include "../../Schedule/include/Datacmnctor.h"

/**
 * 赫夫曼编码表数据类
 * 实现了数据传输接口，使用方法：
 * 使用接口取出Type，向下转型为HashmapType，
 * 
 * 注意事项：
 *     如果向接口写入表，需要确保被写入的对象位于堆内存
 */
class HefftabBlock: public Datacmnctor
{
public:
    HefftabBlock(Heffmap*);
    ~HefftabBlock();

    void ready_put_value() override;
    Type& get_value() override;

private:
    HeffmapType hftab;
};

/**
 * 包装的编码表类型
 */
struct HeffmapType: public Type
{
    HeffmapType(Heffmap*);
    ~HeffmapType();

    //TODO: 有段错误风险？考虑修改hefftab在这里的存储
    //该写法需要确保接受的hefftab源对象位于堆内存
    const Heffmap *hefftab;
};

#endif //HEFFTABBLOCK_H