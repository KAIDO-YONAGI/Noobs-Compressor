#ifndef NAMESPACE_SFC_H
#define NAMESPACE_SFC_H

#include <vector>

namespace sfc{
    using byte = unsigned char;
    using block_t = std::vector<unsigned char>;
    using blocks_t = std::vector< block_t* >;
}


#endif //NAMESPACE_SFC_H