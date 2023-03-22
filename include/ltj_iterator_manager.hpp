/*
 * ltj_iterator_manager.hpp
 * Copyright (C) 2023 Fabrizio
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

#ifndef LTJ_ITERATOR_MANAGER_HPP
#define LTJ_ITERATOR_MANAGER_HPP

#include <triple_pattern.hpp>
#include <ltj_iterator.hpp>

#define VERBOSE 0

namespace ltj {

    template<class index_scheme_t,
    class var_t,
    class cons_t>
    class ltj_iterator_manager {

    public:
        typedef cons_t value_type;
        typedef var_t var_type;
        typedef index_scheme_t index_scheme_type;
        typedef uint64_t size_type;
        typedef ltj_iterator<index_scheme_type, var_type, value_type> ltj_iter_type;

    private:
        const rdf::triple_pattern *m_ptr_triple_pattern;
        index_scheme_type *m_ptr_index; //TODO: should be const
        value_type m_cur_s;
        value_type m_cur_o;
        value_type m_cur_p;
        bool m_is_empty = false;
        ltj_iter_type m_iter;//TODO: It has to be a vector?.
        std::string m_last_iter;//TODO: to be removed.
        //Stores the variable that 'owns' the iterator. This means the following:
        //Any variable which set 'm_last_iter' is flagged as owner, which is useful by adaptive algorithms to 'release' or 'unset' the iterator when needed.
        var_type m_var_owner;
        void copy(const ltj_iterator_manager &o) {
            m_ptr_triple_pattern = o.m_ptr_triple_pattern;
            m_ptr_index = o.m_ptr_index;
            m_cur_s = o.m_cur_s;
            m_cur_p = o.m_cur_p;
            m_cur_o = o.m_cur_o;
            m_is_empty = o.m_is_empty;
            m_iter = o.m_iter;
        }
    public:

        bool is_owner_variable_subject() {
            return m_ptr_triple_pattern->term_s.is_variable && m_var_owner == m_ptr_triple_pattern->term_s.value;
        }

        bool is_owner_variable_predicate(var_type var) {
            return m_ptr_triple_pattern->term_p.is_variable && m_var_owner == m_ptr_triple_pattern->term_p.value;
        }

        bool is_owner_variable_object(var_type var) {
            return m_ptr_triple_pattern->term_o.is_variable && m_var_owner == m_ptr_triple_pattern->term_o.value;
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

        ltj_iterator_manager() = default;

        ltj_iterator_manager(const rdf::triple_pattern *triple, index_scheme_type *index) : m_last_iter(""), m_var_owner('\0') {
            m_ptr_triple_pattern = triple;
            m_ptr_index = index;

            m_iter = ltj_iter_type(triple, m_ptr_index);
            if(m_iter.is_empty){
                m_is_empty = true;
                return;
            }
            //Currently all the members below are used exclusively to precalculate gao and to do so we use SPO index values.
            //>>
            m_cur_s = m_iter.cur_s;
            m_cur_p = m_iter.cur_p;
            m_cur_o = m_iter.cur_o;
            if(m_iter.is_empty){
                m_is_empty = true;
                return;
            }
        }
        const rdf::triple_pattern* get_triple_pattern() const{
            return m_ptr_triple_pattern;
        }
        //! Copy constructor
        ltj_iterator_manager(const ltj_iterator_manager &o) {
            copy(o);
        }

        //! Move constructor
        ltj_iterator_manager(ltj_iterator_manager &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        ltj_iterator_manager &operator=(const ltj_iterator_manager &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        ltj_iterator_manager &operator=(ltj_iterator_manager &&o) {
            if (this != &o) {
                m_cur_s = o.m_cur_s;
                m_cur_p = o.m_cur_p;
                m_cur_o = o.m_cur_o;
                m_is_empty = o.m_is_empty;
                m_ptr_triple_pattern = std::move(o.m_ptr_triple_pattern);
                m_ptr_index = std::move(o.m_ptr_index);
                m_iter = std::move(o.m_iter);
            }
            return *this;
        }
        std::string get_index_permutation() const{
            return m_last_iter;
        }
        void swap(ltj_iterator_manager &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_ptr_triple_pattern, o.m_ptr_triple_pattern);
            std::swap(m_ptr_index, o.m_ptr_index);
            std::swap(m_cur_s, o.m_cur_s);
            std::swap(m_cur_p, o.m_cur_p);
            std::swap(m_cur_o, o.m_cur_o);
            std::swap(m_is_empty, o.m_is_empty);
            std::swap(m_iter, o.m_iter);
        }
        /*Unset the variable iterator ownership. Only used by adaptive algorithms.*/
        //TODO: not needed.
        void unset_iter(var_type var_owner){
            if(var_owner == m_var_owner){
                m_last_iter ="";
            }
        }
        //TODO: not needed.
        void set_iter(var_type var){
            m_var_owner = var;
            if (is_variable_subject(var)) {
                if (m_cur_o != -1UL && m_cur_p != -1UL){
                    //OP->S
                    m_last_iter = "SPO";
                    return;
                } else if (m_cur_o != -1UL) {
                    //OS->P
                    m_last_iter = "SOP";
                } else if (m_cur_p != -1UL) {
                    //PS->O
                    m_last_iter = "SPO";
                }
            } else if (is_variable_predicate(var)) {
                if (m_cur_s != -1UL && m_cur_o != -1UL){
                    m_last_iter = "SPO";
                    return;
                } else if (m_cur_o != -1UL) {
                    //OP->S
                    m_last_iter = "SPO";
                } else if (m_cur_s != -1UL) {
                    //SP->O
                    m_last_iter = "SOP";
                }
            } else if (is_variable_object(var)) {
                if (m_cur_s != -1UL && m_cur_p != -1UL){
                    m_last_iter = "SPO";
                    return;
                }
                if (m_cur_p != -1UL) {
                    //PO->S
                    m_last_iter = "SOP";
                } else if (m_cur_s != -1UL) {
                    //SO->P
                    m_last_iter = "SPO";
                }
            }
        }
        void down(var_type var, size_type c) { //Go down in the trie
            //spo_iter.down(var,c);
            if (is_variable_subject(var)) {
                if (m_cur_o != -1UL && m_cur_p != -1UL){
                    return;
                } else if (m_cur_o != -1UL) {
                    //OS->P
                    m_iter.down(var,c);
                    //m_last_iter = "SOP";
                } else if (m_cur_p != -1UL) {
                    //PS->O
                    m_iter.down(var,c);
                    //m_last_iter = "SPO";
                } else {
                    //S->{OP,PO}
                    m_iter.down(var,c);
                }
                m_cur_s = c;
            } else if (is_variable_predicate(var)) {
                if (m_cur_s != -1UL && m_cur_o != -1UL){
                    return;
                } else if (m_cur_o != -1UL) {
                    //OP->S
                    m_iter.down(var,c);
                    //m_last_iter = "SPO";
                } else if (m_cur_s != -1UL) {
                    //SP->O
                    m_iter.down(var,c);
                    //m_last_iter = "SOP";
                } else {
                    //P->{OS,SO} same range in POS and PSO
                    m_iter.down(var,c);
                }
                m_cur_p = c;
            } else if (is_variable_object(var)) {
                if (m_cur_s != -1UL && m_cur_p != -1UL){
                    return;
                }
                if (m_cur_p != -1UL) {
                    //PO->S
                    m_iter.down(var,c);
                    //m_last_iter = "SOP";
                } else if (m_cur_s != -1UL) {
                    //SO->P
                    m_iter.down(var,c);
                    //m_last_iter = "SPO";
                } else {
                    //O->{PS,SP} same range in OPS and OSP
                    m_iter.down(var,c);
                }
                m_cur_o = c;
            }

        };
        //Reverses the intervals and variable weights. Also resets the current value.
        void up(var_type var) { //Go up in the trie
            //spo_iter.up(var);
            if (is_variable_subject(var)) {
                if (m_cur_o != -1UL && m_cur_p != -1UL){ //leaf of virtual trie.
                    m_iter.up(var);
                } else if (m_cur_o != -1UL || m_cur_p != -1UL) {//second level nodes.
                    m_iter.up(var);
                } else {//first level nodes. //TODO: I think this code is unused cause we can't go up one level if we are in the first one.
                    m_iter.up(var);
                }
                m_cur_s = -1UL;
            } else if (is_variable_predicate(var)) {
                if (m_cur_s != -1UL && m_cur_o != -1UL){
                    m_iter.up(var);
                } else if (m_cur_o != -1UL || m_cur_s != -1UL) {
                    m_iter.up(var);
                } else {
                    m_iter.up(var);
                }
                m_cur_p = -1UL;
            } else if (is_variable_object(var)) {
                if (m_cur_s != -1UL && m_cur_p != -1UL){
                    m_iter.up(var);
                }
                if (m_cur_p != -1UL || m_cur_s != -1UL) {
                    m_iter.up(var);
                } else {
                    m_iter.up(var);
                }
                m_cur_o = -1UL;
            }
        };

        bool in_last_level(){
            bool r = false;
            r = m_iter.in_last_level();
            return r;
        }

        //Solo funciona en último nivel, en otro caso habría que reajustar
        std::vector<uint64_t> seek_all(var_type var){
            return m_iter.seek_all(var);
        }
        value_type leap(var_type var) {
            return m_iter.leap(var);
        }
        value_type leap(var_type var, size_type c) {
            return m_iter.leap(var,c);
        }
    };
}

#endif