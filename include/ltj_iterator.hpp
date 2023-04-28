/*
 * ltj_iterator.hpp
 * Copyright (C) 2023 Author removed for double-blind evaluation
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

#ifndef LTJ_ITERATOR_HPP
#define LTJ_ITERATOR_HPP

#include <triple_pattern.hpp>
#include <cltj_config.hpp>
#include <vector>
#include <utils.hpp>
#include <string>
#define VERBOSE 0

namespace ltj {

    template<class index_scheme_t, class var_t, class cons_t>
    class ltj_iterator {//TODO: if CLTJ is eventually integrated with the ring to form a Compact Index Schemes project then this class has to be renamed to CLTJ_iterator, for instance.

    public:
        typedef cons_t value_type;
        typedef var_t var_type;
        typedef index_scheme_t index_scheme_type;
        typedef uint64_t size_type;
        //enum state_type {s, p, o};
        //std::vector<value_type> leap_result_type;

        static size_type _id;// declaration (uses 'static')

    private:
        size_type m_id = 0;
        const rdf::triple_pattern *m_ptr_triple_pattern;
        index_scheme_type *m_ptr_index; //TODO: should be const
        size_type m_triple_index;
        //value_type m_cur_s;
        //value_type m_cur_p;
        //value_type m_cur_o;
        bool m_is_empty = false;
        cltj::CTrie *m_trie;
        size_type m_number_of_constants;

        var_type m_owner_var;
        std::string m_var_order;
        typedef std::vector<std::string> orders_type;
        //TODO: the following are legacy variables that needs to be refactor.
        bool m_at_end;
        bool m_at_root;
        bool m_key_flag;
        value_type m_depth;
        value_type m_it;
        value_type m_parent_it;
        value_type m_pos_in_parent;
        value_type m_key_val;
        std::vector<uint64_t> m_order;
        std::vector<rdf::term_pattern> m_vars_order;

        void copy(const ltj_iterator &o) {
            m_ptr_triple_pattern = o.m_ptr_triple_pattern;
            m_ptr_index = o.m_ptr_index;
            //m_cur_s = o.m_cur_s;
            //m_cur_p = o.m_cur_p;
            //m_cur_o = o.m_cur_o;
            m_number_of_constants = o.m_number_of_constants;
            m_is_empty = o.m_is_empty;
            m_trie = o.m_trie;
            m_at_end = o.m_at_end;
            m_at_root = o.m_at_root;
            m_key_flag = o.m_key_flag;
            m_depth = o.m_depth;
            m_it = o.m_it;
            m_parent_it = o.m_parent_it;
            m_pos_in_parent = o.m_pos_in_parent;
            m_key_val = o.m_key_val;
            m_order = o.m_order;
            m_id = o.m_id;
            m_var_order = o.m_var_order;
        }
    public:
        /*
            Returns the key of the current position of the iterator
        */
        uint32_t key(){
            if(!m_at_end && !m_at_root){
                /*if(m_key_flag){
                    m_key_flag = false;
                    return m_key_val;
                }
                else*/
                    return m_trie->key_at(m_it);
            }
            return 0;
        }
        inline bool is_variable_subject(var_type var) {
            return m_ptr_triple_pattern->term_s.is_variable && var == m_ptr_triple_pattern->term_s.value;
        }

        inline bool is_variable_predicate(var_type var) {
            return m_ptr_triple_pattern->term_p.is_variable && var == m_ptr_triple_pattern->term_p.value;
        }

        inline bool is_variable_object(var_type var) {
            return m_ptr_triple_pattern->term_o.is_variable && var == m_ptr_triple_pattern->term_o.value;
        }
        const bool &is_empty = m_is_empty;
        const std::vector<uint64_t>& order = m_order;
        const std::string& var_order = m_var_order;//TODO: rename, it is a really bad name!
        const var_type& owner_var = m_owner_var;
        const size_type& triple_index = m_triple_index;
        const size_type& depth = m_depth;
        const size_type& id = m_id;

        ltj_iterator() = default;
        ltj_iterator(const rdf::triple_pattern *triple, var_type var, std::string var_order, index_scheme_type *index, size_type triple_index) {
            m_id = ltj_iterator::_id++;
            m_ptr_triple_pattern = triple;
            m_ptr_index = index;
            m_triple_index = triple_index;
            //m_cur_s = -1UL;
            //m_cur_p = -1UL;
            //m_cur_o = -1UL;
            m_owner_var = var;
            m_var_order = var_order;
            m_number_of_constants = 0;

            m_it = 2;
            m_at_root = true;
            m_at_end = false;
            m_depth = -1;
            m_key_flag = false;
            m_trie = m_ptr_index->get_trie(m_var_order);
            m_order.reserve(3);
            m_vars_order.reserve(3);//legacy member
            //String to Vector
            std::stringstream ss(m_var_order);
            std::istream_iterator<std::string> begin(ss);
            std::istream_iterator<std::string> end;
            std::vector<std::string> vstrings(begin, end);
            for(auto& vstring : vstrings){
                auto aux = std::stoull(vstring);
                m_order.emplace_back(aux);
                if(aux == 0){
                    m_vars_order.emplace_back(m_ptr_triple_pattern->term_s);
                }else if(aux == 1){
                    m_vars_order.emplace_back(m_ptr_triple_pattern->term_p);
                }else{
                    m_vars_order.emplace_back(m_ptr_triple_pattern->term_o);
                }
            }
            //m_depth++;//taken from triejoin_open()
            //Starting down (open)
            m_at_root = false;
            bool has_children = m_trie->childrenCount(m_it) != 0;
            if(has_children){
                m_parent_it = m_it;
                m_it = m_trie->child(m_it, 1);
                m_pos_in_parent = 1;
                m_depth++;
                //cout<<"printing key in iterator "<< order << " (constructor): "<<m_trie->key_at(m_it)<<endl;

                //Processing all the constants.
                process_constants();
            }else{
                m_is_empty = true;
            }
        }
        void process_constants(){
            size_type c = -1;
            for(const auto& o : m_order){
                if(m_is_empty)
                    break;
                if (o == 0){
                    if(!m_ptr_triple_pattern->s_is_variable()){
                        m_number_of_constants++;
                        c = leap(m_ptr_triple_pattern->term_s.value);
                        if(c != m_ptr_triple_pattern->term_s.value){
                            m_is_empty = true;
                            break;
                        }
                        //Down for a constant
                        bool has_children = m_trie->childrenCount(m_it) != 0;
                        if(has_children){
                            m_parent_it = m_it;
                            m_it = m_trie->child(m_it, 1);
                            //m_prev_pos_in_parent = m_pos_in_parent;
                            m_pos_in_parent = 1;
                            m_depth++;
                            //cout<<"printing key in down "<<m_trie->key_at(m_it)<<endl;
                        }else{
                            m_is_empty = true;
                        }
                    }
                } else if(o == 1){
                    if(!m_ptr_triple_pattern->p_is_variable()){
                        m_number_of_constants++;
                        c = leap(m_ptr_triple_pattern->term_p.value);
                        if(c != m_ptr_triple_pattern->term_p.value){
                            m_is_empty = true;
                            break;
                        }
                        //Down for a constant
                        bool has_children = m_trie->childrenCount(m_it) != 0;
                        if(has_children){
                            m_parent_it = m_it;
                            m_it = m_trie->child(m_it, 1);
                            //m_prev_pos_in_parent = m_pos_in_parent;
                            m_pos_in_parent = 1;
                            m_depth++;
                            //cout<<"printing key in down "<<m_trie->key_at(m_it)<<endl;
                        }else{
                            m_is_empty = true;
                        }
                    }
                } else {
                    if(!m_ptr_triple_pattern->o_is_variable()){
                        m_number_of_constants++;
                        c = leap(m_ptr_triple_pattern->term_o.value);
                        if(c != m_ptr_triple_pattern->term_o.value){
                            m_is_empty = true;
                            break;
                        }
                        //Down for a constant
                        bool has_children = m_trie->childrenCount(m_it) != 0;
                        if(has_children){
                            m_parent_it = m_it;
                            m_it = m_trie->child(m_it, 1);
                            //m_prev_pos_in_parent = m_pos_in_parent;
                            m_pos_in_parent = 1;
                            m_depth++;
                            //cout<<"printing key in down "<<m_trie->key_at(m_it)<<endl;
                        }else{
                            m_is_empty = true;
                        }
                        m_number_of_constants++;
                    }
                }
            }
        }
        const std::string get_order(){
            //a.
            orders_type variables;
            std::stringstream partial_order;
            if(!m_ptr_triple_pattern->term_s.is_variable){
                partial_order<<"0 ";
            }else if((var_type) m_ptr_triple_pattern->term_s.value != m_owner_var){
                variables.emplace_back("0");
            }
            if(!m_ptr_triple_pattern->term_p.is_variable){
                partial_order<<"1 ";
            }else if((var_type) m_ptr_triple_pattern->term_p.value != m_owner_var){
                variables.emplace_back("1");
            }
            if(!m_ptr_triple_pattern->term_o.is_variable){
                partial_order<<"2 ";
            }else if((var_type) m_ptr_triple_pattern->term_o.value != m_owner_var){
                variables.emplace_back("2");
            }
            //Adding the variable order, the "owner" of this iterator.
            partial_order << m_var_order+ " ";
            //b.
            std::stringstream aux;
            aux << partial_order.str();
            for(const auto variable : variables){
                aux << variable << " ";
            }
            std::string str = aux.str();
            index_scheme::util::rtrim(str);
            return str;
        }

        const rdf::triple_pattern* get_triple_pattern() const{
            return m_ptr_triple_pattern;
        }
        //! Copy constructor
        ltj_iterator(const ltj_iterator &o) {
            copy(o);
        }

        //! Move constructor
        ltj_iterator(ltj_iterator &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        ltj_iterator &operator=(const ltj_iterator &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        ltj_iterator &operator=(ltj_iterator &&o) {
            if (this != &o) {
                m_ptr_triple_pattern = std::move(o.m_ptr_triple_pattern);
                m_ptr_index = std::move(o.m_ptr_index);
                //m_cur_s = o.m_cur_s;
                //m_cur_p = o.m_cur_p;
                //m_cur_o = o.m_cur_o;
                m_number_of_constants = o.m_number_of_constants;
                m_is_empty = o.m_is_empty;
                m_trie = o.m_trie;
                m_at_end = o.m_at_end;
                m_at_root = o.m_at_root;
                m_key_flag = o.m_key_flag;
                m_depth = o.m_depth;
                m_it = o.m_it;
                m_parent_it = o.m_parent_it;
                m_pos_in_parent = o.m_pos_in_parent;
                //m_prev_pos_in_parent = o.m_prev_pos_in_parent;
                m_key_val = o.m_key_val;
                m_order = o.m_order;
                m_id = o.m_id;
                m_var_order = o.m_var_order;
            }
            return *this;
        }

        void swap(ltj_iterator &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_ptr_triple_pattern, o.m_ptr_triple_pattern);
            std::swap(m_ptr_index, o.m_ptr_index);
            //std::swap(m_cur_s, o.m_cur_s);
            //std::swap(m_cur_p, o.m_cur_p);
            //std::swap(m_cur_o, o.m_cur_o);
            std::swap(m_number_of_constants, o.m_number_of_constants);
            std::swap(m_is_empty, o.m_is_empty);
            std::swap(m_trie, o.m_trie);
            std::swap(m_at_end, o.m_at_end);
            std::swap(m_at_root, o.m_at_root);
            std::swap(m_key_flag, o.m_key_flag);
            std::swap(m_depth, o.m_depth);
            std::swap(m_it, o.m_it);
            std::swap(m_parent_it, o.m_parent_it);
            std::swap(m_pos_in_parent, o.m_pos_in_parent);
            //std::swap(m_prev_pos_in_parent, o.m_prev_pos_in_parent);
            std::swap(m_key_val, o.m_key_val);
            std::swap(m_order, o.m_order);
            std::swap(m_id, o.m_id);
            std::swap(m_var_order, o.m_var_order);
        }
        //As size_type to enable reporting an error as -1UL
        const size_type get_var_at_depth(const size_type depth) const{
            size_type var = -1UL;
            if(depth <0 || depth > 2)
                return var;
            if(m_order[depth] == 0){
                if(m_ptr_triple_pattern->s_is_variable()){
                    var = m_ptr_triple_pattern->term_s.value;
                }
            } else if(m_order[depth] == 1){
                if(m_ptr_triple_pattern->p_is_variable()){
                    var = m_ptr_triple_pattern->term_p.value;
                }
            } else if(m_order[depth] == 2){
                if(m_ptr_triple_pattern->o_is_variable()){
                    var = m_ptr_triple_pattern->term_o.value;
                }
            }
            return var;
        }
        const size_type get_child_count() const{
            return m_trie->childrenCount(m_it);
        }
        const size_type get_weight() const{
            if(m_number_of_constants == 0){
                return -1UL; //Border Case, when there are no constants all the weight have to be the maximum value or the number of triples in the BGP (which is what the Ring does).
            }else{
                return m_trie->childrenCount(m_parent_it);
            }
        }
        void down(var_type var){// Go down in the trie
            if(m_at_root){
                m_at_root = false;
            }
            //When there's only a triple, seek_all will always set m_at_end=true.
            //Down needs to set it at false so the next variable can be eliminated properly.
            //This was added to solve a problem of lonely variables when the BGP is composed of a single triple,
            //meaning that all its vars are lonely, although only one of them is in the last level.
            m_at_end = false;

            //if(!m_at_end)
            if (is_variable_subject(var) ||
                is_variable_predicate(var) ||
                is_variable_object(var)) {
                if (m_depth == 2){
                    //Do nothing
                    return;
                }else{
                    bool has_children = m_trie->childrenCount(m_it) != 0;
                    if(has_children){
                        m_parent_it = m_it;
                        m_it = m_trie->child(m_it, 1);
                        //m_prev_pos_in_parent = m_pos_in_parent;
                        m_pos_in_parent = 1;
                        m_depth++;
                        //cout<<"printing key in down "<<m_trie->key_at(m_it)<<endl;
                    }
                }
            }
        };
        //Reverses the intervals and variable weights. Also resets the current value.
        void up(var_type var){ //Go up in the trie
            if(!m_at_root){
                m_depth--;
                m_it = m_parent_it;
                m_at_end = false;
                if(m_it==2){
                    m_at_root = true;
                    //cout<<"subi hasta la root"<<endl;
                }
                else{
                    //m_prev_pos_in_parent = m_pos_in_parent;
                    m_pos_in_parent = m_trie->childRank(m_it);
                    m_parent_it = m_trie->parent(m_it);
                }
            }
        };

        value_type leap(size_type c) { //Return the minimum in the range
            //If c=-1 we need to get the minimum value for the current level.
            if(c== -1){
                c = key();
            }
            //Tomado de compact_trie_iv_iterator.seek
            //cout<<"Se llama a leap (prev. seek) de "<<c<<endl;
            if(c == 0 || m_at_root || m_at_end){
                return 0;
            }

            // Nos indica cuantos hijos tiene el padre de el it actual ->O(1)
            uint32_t parent_child_count = m_trie->childrenCount(m_parent_it);
            // Nos indica cuantos 0s hay hasta it - 2, es decir la posición en el string de el char correspondiente a la posición del it -> O(1)
            uint32_t i = m_trie->b_rank0(m_it)-2;
            // Nos indica la posición en el string de el char correspondiente a la posición del ultimo hijo del padre del it. -> O(1)
            uint32_t f = m_trie->b_rank0(m_trie->child(m_parent_it, parent_child_count))-2;

            bool found = false;
            /*
            cout<<"i y f "<<i<<" "<<f<<endl;
            cout<<"parent_child_count "<<parent_child_count<<endl;
            cout<<"it "<<m_it<<endl;

            cout<<"calling binary_search "<<endl;
            */
            auto new_info = m_trie->binary_search_seek(c, i, f);
            auto val = new_info.first;
            auto pos = new_info.second;

            if(pos == f+1){
                m_at_end = true;
                //When we are at the end of the list, then we need to restart the pointers.
                m_pos_in_parent = 1;
                m_it = m_trie->child(m_parent_it, 1);
                return 0;
            }
            else{
                m_it = m_trie->b_sel0(pos+2)+1;
                //m_prev_pos_in_parent = m_pos_in_parent;
                m_pos_in_parent = m_trie->childRank(m_it);
                m_key_flag = true;
                m_key_val = val;
                return m_key_val;
            }
        };

        bool in_last_level(){
            return m_depth == 2;
        }
        /*
            Returns true if the iterator is past the last child of a node
        */
        /*bool is_at_end(){
            return at_end;
        }*/
        bool at_level_of_var(var_type x_j){
            /*if(m_vars_order.size() <= 0){
                //First variable corner case.
                return true;
            }*/
            if(m_vars_order[m_depth].is_variable && m_vars_order[m_depth].value == x_j){
                return true;
            }else{
                return false;
            }
        }
        /**
         * Seek_all must perform as many down() as necessary to be placed at 'var' level.
         */
        std::vector<uint64_t> seek_all(var_type x_j){

            std::vector<uint64_t> results;
            bool finished = false;
            if(!at_level_of_var(x_j)){
            //while(!at_level_of_var(x_j)){
                down(x_j);
            }
            if(!m_at_root){
                while(!m_at_end && !finished){
                    if(results.size() >= index_scheme::util::configuration.get_number_of_results()){
                        finished = true;
                    }else{
                        uint32_t m_parent_child_count = m_trie->childrenCount(m_parent_it);
                        if(m_parent_child_count == m_pos_in_parent){
                            m_at_end = true;
                            results.emplace_back(m_trie->key_at(m_it));//TODO: key() returns 0 when m_at_end == true. Standarize.
                        }
                        else{
                            results.emplace_back(key());
                            m_pos_in_parent++;
                            m_it = m_trie->child(m_parent_it, m_pos_in_parent);
                        }
                    }
                }
            }
            if(results.size()==0){
                restart_level_iterator(x_j);
            }
            return results;
        }
        const value_type get_depth() const{
            return m_depth;
        }
        /*
        When we require to restart a given iterator, by going to the first position in parent node.
        Useful when the list of values at a certain level has to be revisited from the beginning.
        */
        void restart_level_iterator(const var_type x_j, size_type c = -1){
            up(x_j);
            down(x_j);
        }
    };

    template<class index_scheme_t, class var_t, class cons_t>
    uint64_t ltj::ltj_iterator<index_scheme_t, var_t, cons_t>::_id = 0; // definition (does not use 'static')
}

#endif //LTJ_ITERATOR_HPP
