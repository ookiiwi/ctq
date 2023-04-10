#ifndef CTQ_UTIL_HH
#define CTQ_UTIL_HH

#include <string>
#include <cctype>
#include <algorithm>
#include <utility>
#include <cassert>
#include <fstream>
#include <iostream>

inline std::string ltrim(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); }));
    return s;
}

inline std::string rtrim(std::string s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), s.end());
    return s;
}

inline std::string trim(std::string s) {
    return rtrim(ltrim(s));
}

inline bool is_bp_balenced(const std::vector<bool> &bp) {
    size_t open_cnt = 0;

    for (const auto e : bp) {
        e ? ++open_cnt : --open_cnt;
    }

    return open_cnt == 0;
}

template<typename T>
class Contiguous2dArray {
public:
    Contiguous2dArray() = default;

    Contiguous2dArray(const std::vector<std::vector<T>> &vec) {
        for (const auto e : vec) {
            unsigned start = m_arr.size();

            for (unsigned i = 0; i < e.size(); ++i) {
                m_arr.push_back(e[i]);
            }

            assert(m_arr.size());

            m_range_mapper.push_back(start);
        }
    }

    Contiguous2dArray(std::istream &is) {
        uint32_t cnt;
        uint32_t arr_size;

        is.read((char*)&cnt, sizeof cnt);
        is.read((char*)&arr_size, sizeof arr_size);

        m_range_mapper.resize(cnt);
        m_arr.resize(arr_size);

        is.read((char*)m_range_mapper.data(), cnt * sizeof m_range_mapper[0]);
        is.read((char*)m_arr.data(), arr_size * sizeof m_arr[0]);
    }

    inline unsigned size() const { return m_range_mapper.size(); }

    inline void save(std::ostream &os) const {
        uint32_t cnt = size();
        uint32_t arr_size = m_arr.size();

        os.write((char*)&cnt, sizeof cnt);
        os.write((char*)&arr_size, sizeof arr_size);
        os.write((char*)m_range_mapper.data(), cnt * sizeof m_range_mapper[0]);
        os.write((char*)m_arr.data(), arr_size * sizeof m_arr[0]);
    }


    inline const std::vector<T> operator[](unsigned index) const {
        unsigned start = m_range_mapper[index];
        unsigned end   = index+1 < m_range_mapper.size() ? m_range_mapper[index+1] : m_arr.size();
        auto beg = m_arr.begin();

        return std::vector<T>(beg + start, beg + end);
    }

private:
    std::vector<T> m_arr;
    std::vector<unsigned> m_range_mapper;
};

#endif