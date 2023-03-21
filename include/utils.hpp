/*
 * utils.hpp
 * Copyright (C) 2020 Author removed for double-blind evaluation
 *
 *
 * This is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <set>

namespace index_scheme {
    namespace util {
        typedef uint64_t size_type;//TODO: shouldn't they all get their values from a single source?
        typedef uint8_t var_type;

        /*Functions*/
        template<class Iterator>
        uint64_t get_size_interval(const Iterator &iter) {
            /*if(iter.cur_s == -1UL && iter.cur_p == -1UL && iter.cur_o == -1UL){
                return iter.get_i_s().size(); //open
            } else if (iter.cur_s == -1UL && iter.cur_p != -1UL && iter.cur_o == -1UL) {
                return iter.get_i_s().size(); //i_s = i_o
            } else if (iter.cur_s == -1UL && iter.cur_p == -1UL && iter.cur_o != -1UL) {
                return iter.get_i_s().size(); //i_s = i_p
            } else if (iter.cur_s != -1UL && iter.cur_p == -1UL && iter.cur_o == -1UL) {
                return iter.get_i_o().size(); //i_o = i_p
            } else if (iter.cur_s != -1UL && iter.cur_p != -1UL && iter.cur_o == -1UL) {
                return iter.get_i_o().size();
            } else if (iter.cur_s != -1UL && iter.cur_p == -1UL && iter.cur_o != -1UL) {
                return iter.get_i_o().size();
            } else if (iter.cur_s == -1UL && iter.cur_p != -1UL && iter.cur_o != -1UL) {
                return iter.get_i_s().size();
            }*/
            return 0;
        }
    }
}
#endif
