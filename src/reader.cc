#include "ctq_reader.h"

#include <fstream>
#include <iostream>
#include <bitset>
#include <string>

#include "xcdat.hpp"
#include <lz4.h>

extern "C" {

#include <string.h>

struct ctq_ctx_internal {
    ctq_ctx_internal(const std::string &filename) : reader(filename) {}

    CTQ::Reader reader;
};


ctq_ctx *ctq_create_reader(const char *filename) {
    ctq_ctx *ctx = new ctq_ctx_internal(std::string(filename));

    // TODO: catch errors

    return ctx;
}

void ctq_destroy_reader(ctq_ctx *ctx) {
    std::cout << "destroy reader" << std::endl;
    delete ctx;
}

ctq_find_ret *ctq_find(const ctq_ctx *ctx, const char *keyword, bool exact_match, size_t offset, size_t count, int path_idx) {
    auto ret = ctx->reader.find(std::string(keyword), exact_match, offset, count, path_idx);

    if (ret.size() == 0) 
        return NULL;

    ctq_find_ret *arr = new ctq_find_ret[ret.size() + 1];
    //arr->ret = ;
    //arr->size = ret.size();
    //find_ret[ret.size()].key = NULL;
    arr[ret.size()].ids = NULL;
    //find_ret[ret.size()].id_cnt = 0;

    int i = 0;
    for (const auto e : ret) {
        arr[i].key    = strdup(e.first.c_str());
        arr[i].id_cnt = e.second.size();
        arr[i].ids    = new uint64_t[e.second.size()];

        memcpy((char*)arr[i].ids, (char*)e.second.data(), e.second.size() * sizeof (uint64_t));

        ++i;
    }

    return arr;
}

char *ctq_get(ctq_ctx *ctx, uint64_t id) {
    std::string ret = ctx->reader.get(id);

    if (ret.size() == 0)
        return NULL;

    return strdup(ret.c_str());
}

void ctq_find_ret_free(ctq_find_ret *arr) {
    for (int i = 0; arr[i].ids != NULL; ++i) {
        free((void*)arr[i].key);
        delete[] arr[i].ids;
    }

    delete[] arr;
}

const char *ctq_writer_version(const ctq_ctx *ctx) {
    return strdup(ctx->reader.get_writer_version().c_str());
}

const char *ctq_reader_version(const ctq_ctx *ctx) {
    return strdup(ctx->reader.get_reader_version().c_str());
}

}

long read_cluster(std::istream &is, std::ostream &os) {
    int compressed_size;
    uint16_t cluster_size;
    char *inbuf;
    char *outbuf;

    is.read((char*)&cluster_size, sizeof cluster_size);
    is.read((char*)&compressed_size, sizeof compressed_size);
    std::cout << "cmp size " << compressed_size << std::endl;


    inbuf  = new char[compressed_size];
    outbuf = new char[cluster_size];

    is.read(inbuf, compressed_size);

    int rv = LZ4_decompress_safe(inbuf, outbuf, compressed_size, cluster_size);

    if (rv > 0) {
        os.write(outbuf, rv);
    }

    delete[] inbuf;
    delete[] outbuf;

    return rv;
}

namespace CTQ {

Reader::Reader(const std::string &filename) : input(filename) {
    uint16_t xalpha_sz  = 0;
    uint32_t id_cnt     = 0;
    uint32_t id_map_cnt = 0;

    // version
    {
        input.read((char*)&m_writer_version_major, sizeof m_writer_version_major);
        input.read((char*)&m_writer_version_minor, sizeof m_writer_version_minor);
        input.read((char*)&m_writer_version_patch, sizeof m_writer_version_patch);

        // TODO: check supported
    }

    // read xml_alphabet
    {
        input.read((char*)&xalpha_sz, sizeof xalpha_sz);

        std::string s = "";
        for (int i = 0; i < xalpha_sz; ++i) {
            char c = input.get();

            if (c == 0) {
                xml_alphabet.push_back(s);
                s.clear();
                continue;
            }

            s += c;
        }
    }

    // read ch_trie
    ch_trie = xcdat::load<trie_type>(input);

    // read ids and pos
    {
        input.read((char*)&id_cnt, sizeof id_cnt);

        ids.resize(id_cnt);
        pos.resize(id_cnt);
        cluster_offset_idx.resize(id_cnt);

        input.read((char*)ids.data(), id_cnt * sizeof ids[0]);
        input.read((char*)pos.data(), id_cnt * sizeof pos[0]);
        input.read((char*)cluster_offset_idx.data(), id_cnt * sizeof cluster_offset_idx[0]);
    }

    // id_mapping
    {
        for (uint32_t i = 0; i < ch_trie.num_keys(); ++i) {
            uint32_t cnt;
            std::vector<uint32_t> indexes;

            input.read((char*)&cnt, sizeof cnt);
            indexes.resize(cnt);

            input.read((char*)indexes.data(), cnt * sizeof indexes[0]);

            id_mapping.push_back(indexes);
        }
    }

    m_header_end = input.tellg();

    // read cluster offsets
    {
        uint32_t cnt;

        input.seekg(-(sizeof cnt), input.end);
        std::cout << "get from: " << input.tellg() << std::endl;
        input.read((char*)&cnt, sizeof cnt);
        cluster_offsets.resize(cnt);

        input.seekg(-(sizeof cnt + cnt * sizeof cluster_offsets[0]), input.end);
        std::cout << "get from: " << input.tellg() << std::endl;
        input.read((char*)cluster_offsets.data(), cnt * sizeof cluster_offsets[0]);
        input.seekg(m_header_end, input.beg);
    }

    
    if (0) {
        for (const auto e : xml_alphabet) {
            std::cout << e << ", ";
        }

        std::cout << std::endl;

        {
            std::cout << "Enumerate() = {" << std::endl;
            auto itr = ch_trie.make_enumerative_iterator();
            while (itr.next()) {
                std::cout << "   (" << itr.decoded_view() << ", " << itr.id() << ")," << std::endl;
            }
            std::cout << "}" << std::endl;
        }
    }

}

Reader::~Reader() {
    if (input.is_open()) {
        input.close();
        std::cout << "close input" << std::endl;
    }
}

std::map<std::string, std::vector<uint64_t>> Reader::find(const std::string &keyword, bool exact_match, size_t offset, size_t count, int path_idx) const {
    std::map<std::string, std::vector<uint64_t>> ret;
    
    auto it = ch_trie.make_predictive_iterator(keyword);
    size_t i = 0;

    while (it.next()) {
        std::vector<uint64_t> key_ids; 
        std::string key = it.decoded();
        uint64_t id = it.id();
    
        if (count && ret.size() >= count) break;
        if (i++ < offset) continue; 

        if (exact_match && key != keyword) 
            break;

        assert(id < id_mapping.size());

        for (const auto e : id_mapping[id]) {
            uint32_t id = ids[e >> 8];
            uint8_t  pidx = 7 & e;
            
            if (!path_idx || pidx == path_idx)
                key_ids.push_back(id);
            std::cout << "key(" << (int)pidx << ", " << path_idx << "): " << key << std::endl;
        }

        assert(key_ids.size() != 0 || path_idx);
        
        if (key_ids.size())
            ret[key] = key_ids;
    }

    return ret;
}

std::string Reader::get(uint64_t id) {
    auto it = std::lower_bound(ids.begin(), ids.end(), id);

    if (it == ids.end()) {
        return "";
    }

    uint32_t index = std::distance(ids.begin(), it);
    uint32_t cluster_offset = cluster_offsets[cluster_offset_idx[index]];
    uint16_t data_pos = pos[index]; 

    std::cout << "id: " << id << 
                "\nindex: " << index << 
                "\ncluster offset: " << cluster_offset <<
                "\ndata pos: " << data_pos <<  std::endl;

    input.seekg(cluster_offset, input.beg);

    std::vector<std::string> tag_stack;
    std::string output;
    uint8_t prev_type = 0;
    std::vector<bool> entry_bp;
    int entry_pop = 0;
    int entry_pop_cnt = 0;

    std::stringstream ss;
    uint32_t cluster_size = read_cluster(input, ss);

    std::cout << "cluster size: " << cluster_size << std::endl;

    assert(data_pos < cluster_size);

    ss.clear();
    ss.seekg(data_pos, ss.beg);

    auto close_tags = [&entry_bp, &output, &tag_stack] (int &i) {
        while (i < entry_bp.size() && entry_bp[i++] == 0) {
            assert(tag_stack.size());

            std::string tag = tag_stack.back();
            tag_stack.pop_back();

            output += "</" + tag + ">";
        }
    };

    std::cout << "ss state: " << ss.good() << std::endl;

    // read bp
    {
        int open_cnt = -1;

        // bp is known to be well-formed    
        do {
            char b;
            ss.read(&b, sizeof b);

            for (int j = 7; j >= 0; --j) {
                bool bit = 1 & (b >> j);

                // skip padding zeros
                if (entry_bp.size() != 0 || bit) {
                    if (open_cnt < 0) {
                        open_cnt = 0;
                    }

                    if (bit) {
                        ++open_cnt;
                        ++entry_pop;
                    } else {
                        if (entry_bp.back() == 1) {
                            ++entry_pop;
                        }

                        --open_cnt;
                    }

                    entry_bp.push_back(bit);
                }
            }
        } while (open_cnt && !ss.eof());
    }

    std::cout << "entry pop: " << entry_pop << " | ss state: " << ss.good() << std::endl;

    int i;
    for (i = 0; i < entry_bp.size() && !ss.eof();) {
        uint8_t   type;
        uint32_t  data;
        uint32_t  tmp;

        ss.read((char*)&tmp, sizeof tmp);

        type = (uint8_t)(3 & tmp);
        data = ((~0U << 2) & tmp) >> 2;

        if (i && prev_type != 1 && type != 2) {
            output += ">";
        }

        // data
        if (type == 1) {
            assert(data < ch_trie.num_keys());
            std::string key = ch_trie.decode(data);
            
            output += key;       
        } else {
            assert(data < xml_alphabet.size());

            std::string key = xml_alphabet[data];

            if (type == 0) {
                close_tags(i);

                output += "<" + key;
                tag_stack.push_back(key);
            } else {
                uint32_t value;

                ss.read((char*)&value, sizeof value);

                output += " " + key + "=\"" + xml_alphabet[value] + '"';
            }
        }

        prev_type = type;

        if (type != 2) ++entry_pop_cnt;
        if (entry_pop_cnt >= entry_pop) break;
    }

    close_tags(i);

    return output;
}

std::string Reader::get_writer_version() const {
    size_t size = std::snprintf(nullptr, 0, "%d.%d.%d", m_writer_version_major, m_writer_version_minor, m_writer_version_patch);
    std::string version(size, 0);
    std::snprintf(version.data(), size, "%d.%d.%d", m_writer_version_major, m_writer_version_minor, m_writer_version_patch);

    return version;
}

std::string Reader::get_reader_version() const {
    size_t size = std::snprintf(nullptr, 0, "%d.%d.%d", CTQ_READER_VERSION_MAJOR, CTQ_READER_VERSION_MINOR, CTQ_READER_VERSION_PATCH);
    std::string version(size, 0);
    std::snprintf(version.data(), size, "%d.%d.%d", CTQ_READER_VERSION_MAJOR, CTQ_READER_VERSION_MINOR, CTQ_READER_VERSION_PATCH);

    return version;
}

}
