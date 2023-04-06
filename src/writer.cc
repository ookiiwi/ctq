#include "ctq_writer.hh"

#include <string>
#include <vector>
#include <set>
#include <map>
#include <utility>
#include <functional>
#include <iostream>
#include <fstream>
#include <cctype>
#include <memory>

#include <libxml/parser.h>
#include "xcdat.hpp"

#include <lz4hc.h>

using xmlAtt = std::map<std::string, std::string>;
using trie_type = xcdat::trie_8_type;

static std::vector<std::string> xml_alphabet;
static trie_type ch_trie;

struct data_type {
    data_type(uint8_t t) : t(t) {}
    uint8_t t : 2;
};

struct parserState {
    bool        in_body;
    bool        in_entry;
    std::string ch;
};

struct parseState : public parserState {
    std::vector<uint64_t> ids;
    std::set<std::string> xml_alpha{};
    std::set<std::string> ch_alpha{};
    uint32_t              id_mapping_bytes;
};

struct transformState : public parserState {
    transformState(const std::vector<uint64_t> &ids, const std::vector<std::string> &paths, std::ostream &os, size_t cluster_size) 
        :   ids(ids), 
            paths(paths),
            pos(std::vector<uint16_t>(ids.size())), 
            cluster_offset_idx(std::vector<uint32_t>(ids.size())), 
            os(os), 
            cluster_size(cluster_size), 
            id_mapping(std::vector<std::vector<uint32_t>>(ch_trie.num_keys())) {}

    const std::vector<uint64_t>        &ids; // sorted
    std::vector<uint16_t>              pos;
    std::vector<uint32_t>              cluster_offset_idx;
    std::ostringstream                 data;
    std::ostringstream                 tmp_data;
    std::ostream                       &os;
    size_t                             cluster_size;
    std::vector<uint32_t>              entry_id_idx_stack;
    std::vector<std::vector<uint32_t>> id_mapping;
    std::vector<uint32_t>              cluster_offsets;
    std::vector<bool>                  entry_bp;
    const std::vector<std::string>     &paths; // sorted
    std::string                        path;
};

std::string ltrim(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); }));
    return s;
}

std::string rtrim(std::string s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), s.end());
    return s;
}

std::string trim(std::string s) {
    return rtrim(ltrim(s));
}

void parse_characters(void *user_data, const xmlChar *ch, int len) {
    parseState *state = reinterpret_cast<parseState*>(user_data);

    if (!state->in_body) return;

    std::string str = trim(std::string((char*)ch, len));

    if (str.size() == 0) return;

    state->ch += str;
}

void parse_startElement(void *user_data, const xmlChar *name, const xmlChar **attrs) {
    parseState *state = reinterpret_cast<parseState*>(user_data);
    std::string str_name((char*)name); 

    if (str_name == "body") {
        state->in_body = true;
        return;
    }

    if (!(state->in_body)) return;

    if (str_name == "entry") {
        state->in_entry = true;
    }

    state->xml_alpha.insert(str_name);

    for (size_t i = 0; attrs != NULL && attrs[i] != NULL; i+=2) { 
        std::string att_name((char*)attrs[i]);
        std::string att_value((char*)attrs[i+1]);

        if (att_name == "xml:id") {
            att_value.erase(att_value.begin(), std::find_if(att_value.begin(), att_value.end(), [](char c) { return std::isdigit(c); }));
            state->ids.push_back(std::atol(att_value.c_str()));

            continue;
        }

        state->xml_alpha.insert(att_name);
        state->xml_alpha.insert(att_value);
    }
}

void parse_endElement(void *user_data, const xmlChar *name) {
    parseState *state = reinterpret_cast<parseState*>(user_data);
    std::string str_name((char*)name); 

    if (str_name == "body") {
        state->in_body = false;
        std::sort(state->ids.begin(), state->ids.end());
    } else if (str_name != "entry") {
        if (state->in_entry && state->ch.size()) {
            state->ch_alpha.insert(state->ch);
            state->id_mapping_bytes += sizeof(uint32_t);

            state->ch.clear();
        }

        return;
    } 

    state->in_entry = false;
}

void transform_characters(void *user_data, const xmlChar *ch, int len) {
    transformState *state = reinterpret_cast<transformState*>(user_data);

    if (!(state->in_body)) return;

    std::string str = trim(std::string((char*)ch, len));

    if (str.size() == 0) return;

    state->ch += str;
}

void transform_startElement(void *user_data, const xmlChar *name, const xmlChar **attrs) {
    transformState *state = reinterpret_cast<transformState*>(user_data);
    std::string str_name((char*)name); 

    auto xalpha_idx = [](const std::string& s) -> long {
        auto it = std::lower_bound(xml_alphabet.begin(), xml_alphabet.end(), s);
        return it != xml_alphabet.end() ? std::distance(xml_alphabet.begin(), it) : -1;
    };

    if (str_name == "body") {
        state->in_body = true;
        return;
    }

    if (!(state->in_body)) return;

    if (str_name == "entry") {
        state->in_entry = true;
    } 

    state->entry_bp.push_back(1);
    state->path += "/" + str_name;

    uint32_t name_idx = xalpha_idx(str_name);
    assert(name_idx >= 0);

    uint32_t tmp = (uint32_t)(name_idx << 2);
    state->tmp_data.write((char*)&tmp, sizeof tmp);

    for (size_t i = 0; attrs != NULL && attrs[i] != NULL; i+=2) {
        std::string att_name((char*)attrs[i]);
        std::string att_value((char*)attrs[i+1]);

        if (att_name == "xml:id") {
            std::string id = att_value;
            id.erase(id.begin(), std::find_if(id.begin(), id.end(), [](char c) { return std::isdigit(c); }));

            auto it = std::lower_bound(state->ids.begin(), state->ids.end(), std::atol(id.c_str()));
            assert(it != state->ids.end());

            size_t index = std::distance(state->ids.begin(), it);
            state->entry_id_idx_stack.push_back(index);
            
            continue;
        }

        uint32_t att_name_idx = xalpha_idx(att_name);
        uint32_t att_val_idx = xalpha_idx(att_value);

        assert(att_name_idx >= 0);
        assert(att_val_idx >= 0);

        uint32_t tmp = (uint32_t)((att_name_idx << 2) | 2U);
        state->tmp_data.write((char*)&tmp, sizeof tmp);
        state->tmp_data.write((char*)&att_val_idx, sizeof att_val_idx);
    }
}

void transform_endElement(void *user_data, const xmlChar *name) {
    transformState *state = reinterpret_cast<transformState*>(user_data);
    std::string str_name((char*)name); 

    if (state->in_entry) {
        state->entry_bp.push_back(0);
    }

    auto get_path_idx = [&state](const std::string &path) -> uint8_t {
        auto it = std::lower_bound(state->paths.begin(), state->paths.end(), path);

        if (it == state->paths.end() || *it != path) return 0; // hidden key

        return std::distance(state->paths.begin(), it) + 1;
    };

    if (str_name == "body") {
        state->in_body = false;
    } else if (str_name != "entry") {

        if (state->in_entry && state->ch.size()) {
            auto it = ch_trie.make_predictive_iterator(state->ch);
            it.next();

            uint32_t tmp = (uint32_t)((it.id() << 2) | 1U);
            state->tmp_data.write((char*)&tmp, sizeof tmp);

            uint8_t path_idx = get_path_idx(state->path);
            uint32_t idx = (state->entry_id_idx_stack.back() << 8) | path_idx;

            state->id_mapping[it.id()].push_back(idx);
            state->ch.clear();

            assert(state->id_mapping[it.id()].size() <= 0xFFFF);
        }

        // remove last node
        size_t pos  = state->path.find_last_of("/");
        state->path = state->path.substr(0, pos);

        return;
    }

    state->path.clear();

    auto get_cluster_size = [&state] () {
        long data_pos       = state->data.tellp();
        long tmp_data_pos   = state->tmp_data.tellp();

        return data_pos + tmp_data_pos;
    };

    long cluster_size = get_cluster_size();

    state->in_entry = false;

    auto append_stream = [] (std::ostringstream &oss, std::ostream &os) {
        std::string str = oss.str();
        os.write(str.data(), str.size());
    };

    auto reset_stream = [](std::ostream &os) {
        os.clear();
        os.seekp(0);
    };

    auto add_tmp = [&] () {
        if (state->tmp_data.tellp() == 0) return;

        // set entry position in cluster
        state->pos[state->entry_id_idx_stack.back()] = (uint16_t)(state->data.tellp());

        // bp
        {
            // pad entry bp to fit in byte array
            int pad = 8 - (state->entry_bp.size() % 8);

            if (pad != 0) {
                state->entry_bp.insert(state->entry_bp.begin(), pad, 0);
            }

            std::vector<char> bp;

            for (int i = 0; i < state->entry_bp.size();) {
                char b = 0;

                for (int j = 7; j >= 0 && i < state->entry_bp.size(); --j, ++i) {
                    b |= (state->entry_bp[i] << j);
                }

                bp.push_back(b);
            }

            state->entry_bp.clear();
            state->data.write((char*)bp.data(), bp.size());
        }

        append_stream(state->tmp_data, state->data);
        reset_stream(state->tmp_data);
    };

    auto write_cluster = [&state, &add_tmp, &append_stream, &reset_stream](uint16_t cluster_size) {
        long last_entry_id_idx = -1;

        if (cluster_size <= state->cluster_size) {
            add_tmp();
        } else {
            last_entry_id_idx = state->entry_id_idx_stack.back();
            state->entry_id_idx_stack.pop_back();
        }

        long cluster_offset = state->os.tellp();
        state->cluster_offsets.push_back(cluster_offset);

        for (const auto e : state->entry_id_idx_stack) {
            state->cluster_offset_idx[e] = state->cluster_offsets.size() - 1;
        }

        state->entry_id_idx_stack.clear();

        if (last_entry_id_idx >= 0) {
            state->entry_id_idx_stack.push_back(last_entry_id_idx);
        }

        state->os.write((char*)&cluster_size, sizeof cluster_size);

        // TODO: compresss
        {
            int bound = LZ4_compressBound(cluster_size);
            char *inbuf  = new char[cluster_size];
            char *outbuf = new char[bound];

            std::stringstream is;
            
            append_stream(state->data, is);
            is.read(inbuf, cluster_size);

            int rv = LZ4_compress_HC(inbuf, outbuf, cluster_size, bound, LZ4HC_CLEVEL_MAX);

            if (rv > 0) {
                state->os.write((char*)&rv, sizeof rv);
                state->os.write(outbuf, rv);
            }

            reset_stream(state->data);

            delete[] inbuf;
            delete[] outbuf;

            if (rv <= 0) {
                std::cerr << "Cannot compress cluster" << std::endl;
            }
        }
    };

    if (cluster_size >= state->cluster_size) {
        write_cluster(cluster_size);
    }

    cluster_size = get_cluster_size();
    if (str_name == "body" && cluster_size) {
        write_cluster(cluster_size);
    } else if (cluster_size) {
        add_tmp();
    }
}

std::unique_ptr<parseState> parse_input(const std::string &src) {
    std::unique_ptr<parseState> state{ new parseState() };
    xmlSAXHandler handler = { .startElement = parse_startElement, .endElement = parse_endElement, .characters = parse_characters };

    if (xmlSAXUserParseFile(&handler, state.get(), src.c_str()) < 0) {
        return nullptr;
    }

    try {
        xml_alphabet = std::vector<std::string>(state->xml_alpha.begin(), state->xml_alpha.end());
        std::sort(xml_alphabet.begin(), xml_alphabet.end());

        std::vector<std::string> tmp(state->ch_alpha.begin(), state->ch_alpha.end());
        std::sort(tmp.begin(), tmp.end());
        ch_trie = trie_type(tmp);
    } catch (const xcdat::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return nullptr;
    }


    return state;
}

int transform_input(const std::string &src, std::ostream &os, const parseState &parse_state, const std::vector<std::string> &paths, uint32_t cluster_size) {
    const long start_pos = os.tellp();
    size_t header_bytes = 0;
    uint32_t cluster_offsets_pos = 0;


    transformState state = transformState(parse_state.ids, paths, os, cluster_size);
    xmlSAXHandler handler = { .startElement = transform_startElement, .endElement = transform_endElement, .characters = transform_characters };

    // get room for header
    {
        uint32_t cnt = state.ids.size();

        std::cout << "cnt:             " << sizeof cnt << '\n'
                  << "ids:             " << cnt * sizeof state.ids[0] << '\n'
                  << "pos:             " << cnt * sizeof state.pos[0] << '\n'
                  << "cluster off idx: " << cnt * sizeof state.cluster_offset_idx[0] << '\n'
                  << "id mapping cnt:  " << sizeof (uint32_t) << '\n'
                  << "id mapping:      " << parse_state.id_mapping_bytes + ch_trie.num_keys() * sizeof (uint16_t) << std::endl;

        header_bytes += sizeof cnt;
        header_bytes += cnt * sizeof state.ids[0]; // ids
        header_bytes += cnt * sizeof state.pos[0]; // pos
        header_bytes += cnt * sizeof state.cluster_offset_idx[0]; // cluster offset idx
        header_bytes += sizeof (uint32_t);         // id_mapping cnt
        header_bytes += parse_state.id_mapping_bytes + ch_trie.num_keys() * sizeof (uint16_t); // id_mapping

        std::vector<char> buf(header_bytes, 0);
        os.write(buf.data(), buf.size());
    }

    if (xmlSAXUserParseFile(&handler, &state, src.c_str()) < 0) {
        return -1;
    }
        
    long cur_pos = os.tellp();
    assert(cur_pos != start_pos);

    os.seekp(start_pos, os.beg);

    // write header
    {
        uint32_t cnt = state.ids.size();

        assert(cnt == state.pos.size());

        os.write((char*)&cnt, sizeof cnt);
        os.write((char*)state.ids.data(), cnt * sizeof state.ids[0]);
        os.write((char*)state.pos.data(), cnt * sizeof state.pos[0]);
        os.write((char*)state.cluster_offset_idx.data(), cnt * sizeof state.cluster_offset_idx[0]);

        // id mapping
        {
            uint32_t map_cnt = state.id_mapping.size();
            os.write((char*)&map_cnt, sizeof map_cnt);

            for (const auto e : state.id_mapping) {
                uint16_t cnt = (uint16_t)e.size();

                os.write((char*)&cnt, sizeof cnt);
                os.write((char*)e.data(), cnt * sizeof e[0]);
            }
        }
    }

    std::cout << "header bytes: " << os.tellp() - start_pos << " : " << header_bytes << std::endl;
    assert(os.tellp() - start_pos == header_bytes);

    os.seekp(cur_pos, os.beg);

    // write cluster offsets
    {   
        uint32_t cluster_cnt = state.cluster_offsets.size();
        uint32_t bytes = cluster_cnt * sizeof state.cluster_offsets[0];

        os.write((char*)state.cluster_offsets.data(), bytes);
        os.write((char*)&cluster_cnt, sizeof cluster_cnt);
    }

    return 0;
}

int save_alphabets(std::ostream &os) {
    long start_pos = os.tellp(); 
    uint16_t xalpha_sz = 0;
    uint64_t ch_trie_bytes = 0;

    os.write((char*)&xalpha_sz, sizeof xalpha_sz);

    for (const auto e : xml_alphabet) {
        os.write(e.c_str(), e.size() + 1);
        xalpha_sz += e.size() + 1;
    }
    
    os.seekp(start_pos, os.beg);
    os.write((char*)&xalpha_sz, sizeof xalpha_sz);

    os.seekp(xalpha_sz, os.cur);

    ch_trie_bytes = xcdat::memory_in_bytes(ch_trie);
    xcdat::save(ch_trie, os);

    return 0;
}

int ctq_write(const std::string &src, const std::string &dst, const std::vector<std::string> &paths, uint16_t cluster_size) {
    std::ofstream output;

    if (!output) {
        std::cout << "bad" << std::endl;
        return -1;
    }

    std::unique_ptr<parseState> parse_state = parse_input(src);

    if (parse_state == nullptr) {
        return -1;
    } 

    output = std::ofstream(dst, std::ios::binary);

    save_alphabets(output);
    transform_input(src, output, *parse_state, paths, cluster_size);

    output.close();
    
    return 0;
}
