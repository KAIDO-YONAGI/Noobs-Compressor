#ifndef NAMESPACE_SFC_H
#define NAMESPACE_SFC_H

#include <vector>

namespace sfc{
    using byte = unsigned char;
    using block_t = std::vector<unsigned char>;
    using blocks_t = std::vector< std::vector<unsigned char> >;
}


#endif //NAMESPACE_SFC_H