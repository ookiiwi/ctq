#ifndef CTQ_READER_H
#define CTQ_READER_H

#include <string>
#include <vector>
#include <map>

#include "xcdat.hpp"

using trie_type = xcdat::trie_8_type;

namespace CTQ {

class Reader {
public:
    Reader(const std::string &filename);
    ~Reader();

    std::map<std::string, std::vector<uint64_t>> find(const std::string &keyword, bool exact_match = false, size_t offset = 0, size_t count = 0, int path_idx = 0) const;
    std::string get(uint64_t id);

private:
    std::ifstream                      input;
    trie_type                          ch_trie;
    std::vector<std::string>           xml_alphabet;
    std::vector<uint64_t>              ids;
    std::vector<uint16_t>              pos;
    std::vector<uint32_t>              cluster_offset_idx;
    std::vector<std::vector<uint32_t>> id_mapping;
    std::vector<uint32_t>              cluster_offsets;
    long                               m_header_end;
};

}

#endif