#include "argparse/argparse.hpp"
#include "ctq_writer.h"
#include "ctq_util.hh"

void print_paths(const std::vector<std::string> &paths, int max_path_len) {
    if (paths.size() == 0) return;

    std::string pheader = "Paths";
    std::string iheader = "Indexes";

    pheader.append(max_path_len - pheader.size(), ' ');

    std::cout << pheader << " | " << iheader << std::endl; 
    for (int i = 0; i < paths.size(); ++i) {
        std::string idx = std::to_string(i+1);
        idx.insert(0, iheader.size() - idx.size(), ' ');
        std::cout << paths[i] << " | " << idx << std::endl;
    }
}

int main(int argc, char **argv) {
    argparse::ArgumentParser program("ctq_cli");
    program.add_argument("-s", "--source").required();
    program.add_argument("-d", "--destination").default_value("");
    program.add_argument("-p", "--paths").default_value("");
    program.add_argument("-c", "--cluster_size").default_value(64000).scan<'i', int>();

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;

        return 1;
    }

    std::string arg_src   = program.get<std::string>("--source");
    std::string arg_dst   = program.get<std::string>("--destination");
    std::string arg_paths = program.get<std::string>("--paths");
    uint16_t cluster_size = program.get<int>("--cluster_size");

    std::vector<std::string> paths;
    int max_path_len = 0;

    if (arg_dst.size() == 0) {
        size_t pos = arg_src.find_last_of(".");
        arg_dst = arg_src.substr(0, pos) + ".ctq";
    }

    std::cout << "source:       " << arg_src << std::endl;
    std::cout << "destination:  " << arg_dst << std::endl;
    std::cout << "paths:        " << arg_paths << std::endl;
    std::cout << "cluster size: " << cluster_size << std::endl;

    auto set_max_path_len = [&max_path_len](const std::string &s) {
        if (s.size() > max_path_len) {
            max_path_len = s.size();
        }
    };

    if (arg_paths.size()) {
        size_t pos = 0;
        std::string token;

        while ((pos = arg_paths.find(",")) != std::string::npos) {
            token = trim(arg_paths.substr(0, pos));
            
            if (token.size()) {
                paths.push_back(token);
                set_max_path_len(token);

            }

            arg_paths.erase(0, pos + 1);
        }

        arg_paths = trim(arg_paths);
        if (arg_paths.size()) {
            paths.push_back(arg_paths);
            set_max_path_len(arg_paths);
        }   

        std::sort(paths.begin(), paths.end());
    }

    print_paths(paths, max_path_len);

    CTQ::write(arg_src, arg_dst, paths, cluster_size);

    return 0;
}