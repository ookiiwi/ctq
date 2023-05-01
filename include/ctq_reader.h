#ifndef CTQ_READER_H
#define CTQ_READER_H

#define CTQ_WRITER_MAX_SUPPORTED_VERSION "0.0.1"
#define CTQ_WRITER_MIN_SUPPORTED_VERSION "0.0.1"

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
extern "C" {
#else
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#endif

typedef struct ctq_ctx_internal ctq_ctx;
typedef struct {
    const char *key;
    uint64_t   *ids;
    size_t      id_cnt;
} ctq_find_ret;

ctq_ctx      *ctq_create_reader(const char *filename);
void          ctq_destroy_reader(ctq_ctx *ctx);
ctq_find_ret *ctq_find(const ctq_ctx *ctx, const char *keyword, size_t offset, size_t count, int path_idx, const char *filter, int filter_path_idx);
char         *ctq_get (ctq_ctx *ctx, uint64_t id);
const char   *ctq_writer_version(const ctq_ctx *ctx);
const char   *ctq_reader_version(const ctq_ctx *ctx);

void ctq_find_ret_free(ctq_find_ret *arr);

#ifdef __cplusplus
}

#include <string>
#include <vector>
#include <map>
#include <set>
#include <exception>

#include "ctq_util.hh"
#include "xcdat.hpp"

using trie_type = xcdat::trie_8_type;

namespace CTQ {

class Reader {
public:
    Reader(const std::string &filename, bool enable_filters = false);
    ~Reader();

    std::map<std::string, std::vector<uint64_t>> find(const std::string &keyword, size_t offset = 0, size_t count = 0, int path_idx = 0, const std::string &filter = "", int filter_path_idx = 0) const;
    std::string get(uint64_t id);
    std::string get_writer_version() const;
    std::string get_reader_version() const;

public:
    const bool filter_support;

private:
    std::ifstream                          input;
    trie_type                              ch_trie;
    std::vector<std::string>               xml_alphabet;
    std::vector<uint64_t>                  ids;
    std::vector<uint16_t>                  pos;
    std::vector<uint32_t>                  cluster_offset_idx;
    Contiguous2dArray<uint32_t>            id_mapping;
    Contiguous2dArray<uint32_t>            paths_mapping;
    std::vector<uint32_t>                  cluster_offsets;
    long                                   m_header_end;
    uint32_t                               m_writer_version_major;
    uint32_t                               m_writer_version_minor;
    uint32_t                               m_writer_version_patch;
};

class reader_exception : public std::exception {
public:
    explicit reader_exception(const char* msg) : msg_{msg} {}
    ~reader_exception() throw() override = default;

    const char* what() const throw() override {
        return msg_;
    }

  private:
    const char* msg_;
};

#define CTQ_READER_TO_STR_(n) #n
#define CTQ_READER_TO_STR(n) CTQ_READER_TO_STR_(n)
#define CTQ_READER_THROW(msg) throw CTQ::reader_exception(__FILE__ ":" CTQ_READER_TO_STR(__LINE__) " : " msg)

} // namespace CTQ

#endif

#endif