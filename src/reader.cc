#include "ctq_reader.h"

#include <fstream>
#include <iostream>
#include <bitset>
#include <string>
#include <stack>
#include <ctime>
#include <set>
#include <cstring>

#include "xcdat.hpp"
#include <lz4.h>

extern "C" {

#include <string.h>

struct ctq_ctx_internal {
    ctq_ctx_internal(const std::string &filename) : reader(filename) {}

    CTQ::Reader reader;
};


ctq_ctx *ctq_create_reader(const char *filename) {
    ctq_ctx *ctx = NULL;

    try {
        ctx = new ctq_ctx_internal(std::string(filename));
    } catch (const CTQ::reader_exception& ex) {
        if (ctx) delete ctx;

        std::cerr << ex.what() << std::endl;    
        return NULL;
    }

    return ctx;
}

void ctq_destroy_reader(ctq_ctx *ctx) {
    delete ctx;
}

ctq_find_ret *ctq_find(const ctq_ctx *ctx, const char *keyword, size_t offset, size_t count, int path_idx, const char *filter, int filter_path_idx) {
    try {
        auto ret = ctx->reader.find(std::string(keyword), offset, count, path_idx, std::string(filter), filter_path_idx);

        if (ret.size() == 0) 
            return NULL;

        ctq_find_ret *arr = new ctq_find_ret[ret.size() + 1];
        arr[ret.size()].ids = NULL;

        int i = 0;
        for (const auto e : ret) {
            arr[i].key    = strdup(e.first.c_str());
            arr[i].id_cnt = e.second.size();
            arr[i].ids    = new uint64_t[e.second.size()];

            memcpy((char*)arr[i].ids, (char*)e.second.data(), e.second.size() * sizeof (uint64_t));

            ++i;
        }

        return arr;
    }  catch (const CTQ::reader_exception& ex) {    
        std::cerr << ex.what() << std::endl;    
        return NULL;
    }
}

char *ctq_get(ctq_ctx *ctx, uint64_t id) {
    try {
        std::string ret = ctx->reader.get(id);

        if (ret.size() == 0)
            return NULL;

        return strdup(ret.c_str());
    }  catch (const CTQ::reader_exception& ex) {   
        std::cerr << ex.what() << std::endl;    
        return NULL;
    }
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

struct element {
    element(uint8_t type, uint32_t data) : type(type), data(data) {}

    uint8_t type;
    uint32_t data;
};

namespace CTQ {

Reader::Reader(const std::string &filename, bool enable_filters) : input(filename), filter_support(false) {
    uint16_t xalpha_sz  = 0;
    uint32_t id_cnt     = 0;
    uint32_t id_map_cnt = 0;
    uint32_t footer_start = 0;

    if (!input.good()) {
        CTQ_READER_THROW("Cannot open file");
    }

    // version
    {
        input.read((char*)&m_writer_version_major, sizeof m_writer_version_major);
        input.read((char*)&m_writer_version_minor, sizeof m_writer_version_minor);
        input.read((char*)&m_writer_version_patch, sizeof m_writer_version_patch);

        // TODO: check supported
        std::string version = get_writer_version();

        if (version < CTQ_WRITER_MIN_SUPPORTED_VERSION || version > CTQ_WRITER_MAX_SUPPORTED_VERSION) {
            CTQ_READER_THROW("Unsupported version");
        }
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

    input.read((char*)&footer_start, sizeof footer_start);
    m_header_end = input.tellg();

    input.seekg(footer_start, input.beg);

    // load id mapping
    id_mapping = Contiguous2dArray<uint32_t>(input);

    // load paths mapping
    paths_mapping = Contiguous2dArray<uint32_t>(input);

    // read cluster offsets
    {
        uint32_t cnt;

        input.read((char*)&cnt, sizeof cnt);
        cluster_offsets.resize(cnt);

        input.read((char*)cluster_offsets.data(), cnt * sizeof cluster_offsets[0]);
    }

    input.seekg(m_header_end, input.beg);
}

Reader::~Reader() {
    if (input.is_open()) {
        input.close();
    }
}

std::map<std::string, std::vector<uint64_t>> Reader::find(const std::string &keyword, size_t offset, size_t count, int path_idx, const std::string &filter, int filter_path_idx) const {
    auto is_exact_match = [](const std::string &s) {
        auto rbeg = s.rbegin();

        return (*rbeg != '%') || (*(++rbeg) == '\\');
    };

    auto clean_keyword = [] (const std::string &s, bool exact_match) {
        if (exact_match) {
            return s;
        }

        return std::string(s.begin(), s.end() - 1);
    };

    auto check_pidx = [&](int idx, uint64_t ch_id) {
        if (idx == 0) return true;
        auto indexes = paths_mapping[ch_id];

        return std::binary_search(indexes.begin(), indexes.end(), idx);
    };

    std::map<std::string, std::vector<uint64_t>> ret;
    bool exact_match = is_exact_match(keyword);
    std::string clean_key = clean_keyword(keyword, exact_match);
    
    auto it = ch_trie.make_predictive_iterator(clean_key);
    size_t i = 0;
    size_t id_cnt = 0;

    std::set<uint32_t> id_register{};

    while (it.next() && (!count || id_cnt < count)) {
        std::vector<uint64_t> key_ids; 
        std::string key = it.decoded();
        uint64_t ch_id = it.id();

        if (exact_match && key != clean_key) 
            break;

        if (ch_id >= id_mapping.size()) {
            CTQ_READER_THROW("Corrupted file");
        }

        assert(ch_id < id_mapping.size());

        for (const auto e : id_mapping[ch_id]) {
            uint64_t id = ids[e >> 8];
            uint8_t  pidx = 7 & e;
            
            if ((id_register.find(id) == id_register.end()) && (!path_idx || pidx == path_idx)) {
                bool add_id = true;

                if (filter.size()) {
                    add_id = false;

                    bool is_filter_exact_match = is_exact_match(filter);
                    std::string clean_filter = clean_keyword(filter, is_filter_exact_match);

                    for (const auto filter_id : paths_mapping[e >> 8]) {
                        std::string dec = ch_trie.decode(filter_id);

                        if (dec.size() < clean_filter.size()) continue;

                        if (is_filter_exact_match) {
                            add_id = (clean_filter == dec);
                        } else {
                            add_id = (strncmp(clean_filter.c_str(), dec.c_str(), clean_filter.size()) == 0);
                        }


                        if (add_id) {
                            if (filter_path_idx) {
                                auto mapping = id_mapping[filter_id];
                                bool foundIdForPath = std::binary_search(mapping.begin(), mapping.end(), (((e >> 8) << 8) | filter_path_idx));
                                
                                if (foundIdForPath) {
                                    break;
                                }
                                
                                add_id = false;
                            } else {
                                break;
                            }
                        }
                    }
                }

                if (add_id) {
                    id_register.insert(id);

                    if (i++ >= offset) {
                        ++id_cnt;
                        key_ids.push_back(id);
                    }
                }
            }            
        }

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

    input.seekg(cluster_offset, input.beg);

    std::stack<std::string> open_tags;
    std::string output;
    std::vector<bool> entry_bp;
    uint8_t last_node_pop;
    long last_bp_open = 0;

    std::stringstream ss;
    uint32_t cluster_size = read_cluster(input, ss);

    if (data_pos >= cluster_size) {
        CTQ_READER_THROW("Corrupted file");
    }

    ss.clear();
    ss.seekg(data_pos, ss.beg);
    ss.read((char*)&last_node_pop, sizeof last_node_pop);

    /// increment i
    auto close_tags = [&entry_bp, &output, &open_tags] (int &i) {
        while (i < entry_bp.size() && entry_bp[i++] == 0) {
            assert(!open_tags.empty());

            std::string tag = open_tags.top();
            open_tags.pop();

            output += "</" + tag + ">";
        }
    };

    auto read_element = [&ss](bool data_only = false) -> element {
        if (ss.eof()) return element(0, 0);

        uint32_t tmp;
        ss.read((char*)&tmp, sizeof tmp);

        return data_only ? element(0, tmp) : element(3 & tmp, tmp >> 2);
    };

    // read bp
    {
        int open_cnt = -1;

        // bp is known to be well-formed    
        for (int i = 0; open_cnt && !ss.eof(); ) {
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
                        last_bp_open = i;
                    } else {
                        --open_cnt;
                    }
                    ++i;

                    entry_bp.push_back(bit);
                }
            }
        }
    }

    int prev_type = 0;
    int last_node_pop_cnt = -1;
    element elt = read_element();

    if (ss.eof()) {
        CTQ_READER_THROW("Corrupted file");
    }

    for (int i = 0; i < entry_bp.size(); ++i) {
        if (entry_bp[i] == 0) {
            if (prev_type != 1) {
                output += '>';
            }

            assert(!open_tags.empty());

            output += "</" + open_tags.top() + '>';
            open_tags.pop();

            continue;
        }

        do {            
            if (elt.type == 0) {
                if (prev_type != 1 && output.size() && entry_bp[i-1]) {
                    output += '>';
                }

                if (elt.data >= xml_alphabet.size()) {
                    CTQ_READER_THROW("Corrupted file");
                }

                std::string key = xml_alphabet[elt.data];
                output += '<' + key;

                open_tags.push(key);
                last_node_pop_cnt = -1;
            } else if (elt.type == 1) {
                if (elt.data >= ch_trie.num_keys()) {
                    CTQ_READER_THROW("Corrupted file");
                }

                std::string key = ch_trie.decode(elt.data);
                output += '>' + key;
            } else {
                uint32_t dataName = elt.data;
                elt = read_element(true);
                uint32_t dataValue = elt.data;

                if (dataName >= xml_alphabet.size() || dataValue >= xml_alphabet.size()) {
                    CTQ_READER_THROW("Corrupted file");
                }

                std::string name  = xml_alphabet[dataName];
                std::string value = xml_alphabet[dataValue];

                output += ' ' + name + "=\"" + value + '"';
            }

            ++last_node_pop_cnt;
            prev_type = elt.type;

            elt = read_element();
        } while ( !ss.eof() && elt.type != 0 && (i != last_bp_open || last_node_pop_cnt < last_node_pop));
    }

    return output;
}

std::string Reader::get_writer_version() const {
    size_t size = std::snprintf(nullptr, 0, "%d.%d.%d", m_writer_version_major, m_writer_version_minor, m_writer_version_patch);
    std::string version(size, 0);
    std::snprintf(version.data(), size + 1, "%d.%d.%d", m_writer_version_major, m_writer_version_minor, m_writer_version_patch);

    return version;
}

std::string Reader::get_reader_version() const {
    size_t size = std::snprintf(nullptr, 0, "%d.%d.%d", CTQ_READER_VERSION_MAJOR, CTQ_READER_VERSION_MINOR, CTQ_READER_VERSION_PATCH);
    std::string version(size, 0);
    std::snprintf(version.data(), size + 1, "%d.%d.%d", CTQ_READER_VERSION_MAJOR, CTQ_READER_VERSION_MINOR, CTQ_READER_VERSION_PATCH);

    return version;
}

}
