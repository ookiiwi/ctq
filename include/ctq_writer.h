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

class writer_exception : public std::exception {
public:
    explicit writer_exception(const char* msg) : msg_{msg} {}
    ~writer_exception() throw() override = default;

    const char* what() const throw() override {
        return msg_;
    }

  private:
    const char* msg_;
};

#define CTQ_WRITER_TO_STR_(n) #n
#define CTQ_WRITER_TO_STR(n) CTQ_WRITER_TO_STR_(n)
#define CTQ_WRITER_THROW(msg) throw CTQ::writer_exception(__FILE__ " : " CTQ_WRITER_TO_STR(__LINE__) " : " msg)

} // namespace CTQ

#endif