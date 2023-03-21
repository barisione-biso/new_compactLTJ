#ifndef CLTJ
#define CLTJ

#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include <cltj_index.hpp>
#include "cltj_utils.hpp"

using namespace std;
using namespace sdsl;
namespace index_scheme {

    class compactLTJ {
        private:
            unique_ptr<cltj::cltj_index> index;
        public:
            typedef uint64_t size_type;
            typedef uint64_t value_type;

            compactLTJ() {};
            compactLTJ(std::string file_name) {
                string file_extention = file_name.substr(file_name.size()-4, 4);
                if(file_extention != ".txt" && file_extention != ".dat") {
                    index = std::move(std::unique_ptr<cltj::cltj_index>{new cltj::cltj_index(file_name)});
                }
                else{
                    throw "Index must be built before queries can be answered, run \n\
                    > ./build_index "+file_name;
                }
            }
    };
}

#endif