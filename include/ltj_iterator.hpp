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

    private:
        const rdf::triple_pattern *m_ptr_triple_pattern;
        index_scheme_type *m_ptr_index; //TODO: should be const
        value_type m_cur_s;
        value_type m_cur_p;
        value_type m_cur_o;
        bool m_is_empty = false;
        cltj::CTrie *m_trie;

        var_type m_var_owner;
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
        std::vector<std::string> m_order;

        void copy(const ltj_iterator &o) {
            m_ptr_triple_pattern = o.m_ptr_triple_pattern;
            m_ptr_index = o.m_ptr_index;
            m_cur_s = o.m_cur_s;
            m_cur_p = o.m_cur_p;
            m_cur_o = o.m_cur_o;
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
        }

        /*
            Moves the iterator to the next key
            TODO: Refactor.
        */
        void next(){
            if(m_at_root){
                throw "At root, doesn't have next";
            }
            if(m_at_end){
                throw "Iterator is atEnd";
            }

            uint32_t parent_child_count = m_trie->childrenCount(m_parent_it);
            if(parent_child_count == m_pos_in_parent){
                m_at_end = true;
            }
            else{
                m_pos_in_parent++;
                m_it = m_trie->child(m_parent_it, m_pos_in_parent);
            }
        }
        /*
            Returns the key of the current position of the iterator
            TODO: Refactor.
        */
        uint32_t key(){
            if(m_at_end){
                throw "Iterator is atEnd";
            }
            else if(m_at_root){
                throw "Root doesnt have key";
            }
            else{
                if(m_key_flag){
                    m_key_flag = false;
                    return m_key_val;
                }
                else return m_trie->key_at(m_it);
            }
        }
    public:
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
        const value_type &cur_s = m_cur_s;
        const value_type &cur_p = m_cur_p;
        const value_type &cur_o = m_cur_o;

        ltj_iterator() = default;
        ltj_iterator(const rdf::triple_pattern *triple, var_type var, std::string var_order, index_scheme_type *index) {
            m_ptr_triple_pattern = triple;
            m_ptr_index = index;
            m_cur_s = -1UL;
            m_cur_p = -1UL;
            m_cur_o = -1UL;
            m_var_owner = var;
            m_var_order = var_order;

            m_it = 2;
            m_at_root = true;
            m_at_end = false;
            m_depth = -1;
            m_key_flag = false;
            std::string order = get_order();
            m_trie = m_ptr_index->get_trie(order);
            m_order.reserve(3);
            //String to Vector
            std::stringstream ss(order);
            std::istream_iterator<std::string> begin(ss);
            std::istream_iterator<std::string> end;
            std::vector<std::string> vstrings(begin, end);
            for(auto& vstring : vstrings){
                m_order.emplace_back(vstring);
            }
            //TODO: Código de open(). ¿Es esto lo mismo que down?
            m_at_root = false;

            bool has_children = m_trie->childrenCount(m_it) != 0;
            if(has_children){
                m_parent_it = m_it;
                m_it = m_trie->child(m_it, 1);
                m_pos_in_parent = 1;
                m_depth++;
                //cout<<"printing key in iterator "<< order << " (constructor): "<<m_trie->key_at(m_it)<<endl;

                //Processing all the constants.
                size_type c = -1;
                for(const auto& o : m_order){
                    if(m_is_empty)
                        break;
                    if (o == "0"){
                        if(!m_ptr_triple_pattern->term_s.is_variable){
                            if(c!= -1){//Another constant was processed previously.
                                down();
                            }
                            c = leap(m_ptr_triple_pattern->term_s.value);
                            m_cur_s = c;
                            if(c != m_ptr_triple_pattern->term_s.value){
                                m_is_empty = true;
                            }
                        }
                    } else if(o == "1"){
                        if(!m_ptr_triple_pattern->term_p.is_variable){
                            if(c!= -1){//Another constant was processed previously.
                                down();
                            }
                            c = leap(m_ptr_triple_pattern->term_p.value);
                            m_cur_p = c;
                            if(c != m_ptr_triple_pattern->term_p.value){
                                m_is_empty = true;
                            }
                        }
                    } else {
                        if(!m_ptr_triple_pattern->term_o.is_variable){
                            if(c!= -1){//Another constant was processed previously.
                                down();
                            }
                            c = leap(m_ptr_triple_pattern->term_o.value);
                            m_cur_o = c;
                            if(c != m_ptr_triple_pattern->term_o.value){
                                m_is_empty = true;
                            }
                        }
                    }
                }
            }else{
                m_is_empty = true;
            }
            //TODO: Importante, marcar m_is_empty = true si corresponde!
        }
        const std::string get_order(){
            //a.
            orders_type variables;
            std::stringstream partial_order;
            if(!m_ptr_triple_pattern->term_s.is_variable){
                partial_order<<"0 ";
            }else if((var_type) m_ptr_triple_pattern->term_s.value != m_var_owner){
                variables.emplace_back("0");
            }
            if(!m_ptr_triple_pattern->term_p.is_variable){
                partial_order<<"1 ";
            }else if((var_type) m_ptr_triple_pattern->term_p.value != m_var_owner){
                variables.emplace_back("1");
            }
            if(!m_ptr_triple_pattern->term_o.is_variable){
                partial_order<<"2 ";
            }else if((var_type) m_ptr_triple_pattern->term_o.value != m_var_owner){
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
                m_cur_s = o.m_cur_s;
                m_cur_p = o.m_cur_p;
                m_cur_o = o.m_cur_o;
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
            }
            return *this;
        }

        void swap(ltj_iterator &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_ptr_triple_pattern, o.m_ptr_triple_pattern);
            std::swap(m_ptr_index, o.m_ptr_index);
            std::swap(m_cur_s, o.m_cur_s);
            std::swap(m_cur_p, o.m_cur_p);
            std::swap(m_cur_o, o.m_cur_o);
            std::swap(m_is_empty, o.m_is_empty);
            std::swap(m_trie, o.m_trie);
            std::swap(m_at_end, o.m_at_end);
            std::swap(m_at_root, o.m_at_root);
            std::swap(m_key_flag, o.m_key_flag);
            std::swap(m_depth, o.m_depth);
            std::swap(m_it, o.m_it);
            std::swap(m_parent_it, o.m_parent_it);
            std::swap(m_pos_in_parent, o.m_pos_in_parent);
            std::swap(m_key_val, o.m_key_val);
            std::swap(m_order, o.m_order);
        }

        const size_type get_child_count() const{
            return m_trie->childrenCount(m_it);
        }
        void down(){//var_type var, size_type c) { //Go down in the trie
            //TODO: Just go down one level in the trie :)
            if(m_at_root){
                m_at_root = false;
            }
            if(m_at_end){
                throw "Iterator is atEnd";
            }
            else{
                bool has_children = m_trie->childrenCount(m_it) != 0;
                if(has_children){
                    m_key_flag = false;//FABRIZIO: nuevo, para forzar a que key() recalcule la clave luego de este down.
                    m_parent_it = m_it;
                    m_it = m_trie->child(m_it, 1);
                    m_pos_in_parent = 1;
                    m_depth++;
                    //cout<<"printing key in down "<<m_trie->key_at(m_it)<<endl;
                }
                else throw "Node has no children";
            }
        };
        //Reverses the intevals changed by a previous 'down' for subjects.
        void up_iter_sub(){
            if(m_cur_p != -1UL && m_cur_o != -1UL){
                return;
            }else if(m_cur_p != -1UL){
                nullptr;
            }else if(m_cur_o != -1UL){
                nullptr;
            }else{
                nullptr;
            }
        }
        //Reverses the intevals changed by a previous 'down' for predicates.
        void up_iter_pred(){
            if(m_cur_s != -1UL && m_cur_o != -1UL){
                return;
            }else if(m_cur_s != -1UL){
                nullptr;
            }else if(m_cur_o != -1UL){
                nullptr;
            }else{
                nullptr;
            }
        }
        //Reverses the intevals changed by a previous 'down' for objects.
        void up_iter_obj(){
            if(m_cur_s != -1UL && m_cur_p != -1UL){
                return;
            }else if(m_cur_s != -1UL){
                nullptr;
            }else if(m_cur_p != -1UL){
                nullptr;
            }else{
                nullptr;
            }
        }
        //Reverses the intervals and variable weights. Also resets the current value.
        void up(var_type var) { //Go up in the trie
            //TODO: Just go up once.
            /*if (is_variable_subject(var)) {
                up_iter_sub();
                m_cur_s = -1UL;
#if VERBOSE
                std::cout << "Up in S" << std::endl;
#endif
            } else if (is_variable_predicate(var)) {
                up_iter_pred();
                m_cur_p = -1UL;
#if VERBOSE
                std::cout << "Up in P" << std::endl;
#endif
            } else if (is_variable_object(var)) {
                up_iter_obj();
                m_cur_o = -1UL;
#if VERBOSE
                std::cout << "Up in O" << std::endl;
#endif
            }
            */
        };

        value_type leap(size_type c) { //Return the minimum in the range
            //If c=-1 we need to get the minimum value for the current level.
            if(c== -1){
                c = key();//LPM FALTA HACER UN DOWN EN LTJ_ITERATOR CONSTRUCTOR.
            }
            //TODO: just do next, isnt it?
            //My version of LEAP...
            /*bool exit = false;
            value_type k = key();
            while(!exit){
                if(k == c){
                    return c;
                } else if(k > c){
                    exit = true;
                }
                next();
                k = key();
            }
            return -1;
            */
            //Theirs...
            //Tomado de compact_trie_iv_iterator.seek
            //cout<<"Se llama a leap (prev. seek) de "<<c<<endl;
            if(m_at_root){
                throw "At root, cant seek";
            }
            if(m_at_end){
                throw "At end, cant seek";
            }

            // Nos indica cuantos hijos tiene el padre de el it actual ->O(1)
            uint32_t parent_child_count = m_trie->childrenCount(m_parent_it);
            // Nos indica cuantos 0s hay hasta it - 2, es decir la posición en el string de el char correspondiente a la posición del it -> O(1)
            uint32_t i = m_trie->b_rank0(m_it)-2;
            // Nos indica la posición en el string de el char correspondiente a la posición del ultimo hijo del padre del it. -> O(1)
            uint32_t f = m_trie->b_rank0(m_trie->child(m_parent_it, parent_child_count))-2;

            bool found = false;
            /*cout<<"i y f "<<i<<" "<<f<<endl;
            cout<<"parent_child_count "<<parent_child_count<<endl;
            cout<<"it "<<m_it<<endl;

            cout<<"calling binary_search "<<endl;*/
            auto new_info = m_trie->binary_search_seek(c, i, f);
            auto val = new_info.first;
            auto pos = new_info.second;

            if(pos == f+1){
                m_at_end = true;
                return -1;
            }
            else{
                m_it = m_trie->b_sel0(pos+2)+1;
                m_pos_in_parent = m_trie->childRank(m_it);
                m_key_flag = true;
                m_key_val = val;
                return m_key_val;
            }
        };

        bool in_last_level(){
            return (m_cur_o !=-1UL && m_cur_p != -1UL) || (m_cur_s !=-1UL && m_cur_p != -1UL)
                    || (m_cur_o !=-1UL && m_cur_s != -1UL);
        }

        //Solo funciona en último nivel, en otro caso habría que reajustar
        std::vector<uint64_t> seek_all(var_type var){
            if (is_variable_subject(var)){
                return std::vector<uint64_t>();
            }else if (is_variable_predicate(var)){
                return std::vector<uint64_t>();
            }else if (is_variable_object(var)){
                return std::vector<uint64_t>();
            }
            return std::vector<uint64_t>();
        }
    };

}

#endif //LTJ_ITERATOR_HPP
