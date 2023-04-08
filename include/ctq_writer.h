#ifndef CTQ_WRITER_H
#define CTQ_WRITER_H

#include <string>
#include <vector>

namespace CTQ {

/**
 * @brief 
 * 
 * @param src 
 * @param dst 
 * @param paths Array of unique sorted path. UNIQUE AND SORTED !!!!
 * @param cluster_size Value in the range [0, 65535]
 * @return int 
 */
int write(const std::string &src, const std::string &dst, const std::vector<std::string> &paths = {}, uint16_t cluster_size = 64000);

}

#endif