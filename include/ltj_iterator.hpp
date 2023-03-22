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

#include<triple_pattern.hpp>
#include<vector>
#define VERBOSE 0

namespace ltj {

    template<class index_scheme_t, class var_t, class cons_t>
    class ltj_iterator {

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


        void copy(const ltj_iterator &o) {
            m_ptr_triple_pattern = o.m_ptr_triple_pattern;
            m_ptr_index = o.m_ptr_index;
            m_cur_s = o.m_cur_s;
            m_cur_p = o.m_cur_p;
            m_cur_o = o.m_cur_o;
            m_is_empty = o.m_is_empty;
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

        ltj_iterator(const rdf::triple_pattern *triple, index_scheme_type *index) {
            m_ptr_triple_pattern = triple;
            m_ptr_index = index;
            m_cur_s = -1UL;
            m_cur_p = -1UL;
            m_cur_o = -1UL;
            
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
        }

        void down(var_type var, size_type c) { //Go down in the trie
            if (is_variable_subject(var)) {
                if (m_cur_o != -1UL && m_cur_p != -1UL){
#if VERBOSE
                    std::cout << "Nothing to do" << std::endl;
#endif
                    return;
                }
                if (m_cur_o != -1UL) {
                    //OS->P
#if VERBOSE
                    std::cout << "down_O_S" << std::endl;
#endif
                    nullptr;
                } else if (m_cur_p != -1UL) {
                    //PS->O
#if VERBOSE
                    std::cout << "down_P_S" << std::endl;
#endif
                    nullptr;
                } else {
                    //S->{OP,PO} same range in SOP and SPO
#if VERBOSE
                    std::cout << "down_S" << std::endl;
#endif
                    nullptr;
                }
                //m_states.emplace(state_type::s);
                m_cur_s = c;
            } else if (is_variable_predicate(var)) {
                if (m_cur_s != -1UL && m_cur_o != -1UL){
#if VERBOSE
                    std::cout << "Nothing to do" << std::endl;
#endif
                    return;
                }
                if (m_cur_o != -1UL) {
                    //OP->S
#if VERBOSE
                    std::cout << "down_O_P" << std::endl;
#endif
                    nullptr;
                } else if (m_cur_s != -1UL) {
                    //SP->O
#if VERBOSE
                    std::cout << "down_S_P" << std::endl;
#endif
                    nullptr;
                } else {
                    //P->{OS,SO} same range in POS and PSO
#if VERBOSE
                    std::cout << "down_P" << std::endl;
#endif
                    nullptr;
                }
                //m_states.emplace(state_type::p);
                m_cur_p = c;
            } else if (is_variable_object(var)) {
                if (m_cur_s != -1UL && m_cur_p != -1UL){
#if VERBOSE
                    std::cout << "Nothing to do" << std::endl;
#endif
                    return;
                }
                if (m_cur_p != -1UL) {
                    //PO->S
#if VERBOSE
                    std::cout << "down_P_O" << std::endl;
#endif
                    nullptr;
                } else if (m_cur_s != -1UL) {
                    //SO->P
#if VERBOSE
                    std::cout << "down_S_O" << std::endl;
#endif
                    nullptr;
                } else {
                    //O->{PS,SP} same range in OPS and OSP
#if VERBOSE
                    std::cout << "down_O" << std::endl;
#endif
                    nullptr;
                }
                //m_states.emplace(state_type::o);
                m_cur_o = c;
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
            if (is_variable_subject(var)) {
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

        };

        value_type leap(var_type var) { //Return the minimum in the range
            //0. Which term of our triple pattern is var
            if (is_variable_subject(var)) {
                //1. We have to go down through s
                if (m_cur_p != -1UL && m_cur_o != -1UL) {
                    //PO->S
#if VERBOSE
                    std::cout << "min_S_in_PO" << std::endl;
#endif
                    return 0;
                } else if (m_cur_o != -1UL) {
                    //O->S
#if VERBOSE
                    std::cout << "min_S_in_O" << std::endl;
#endif
                    return 0;
                } else if (m_cur_p != -1UL) {
                    //P->S
#if VERBOSE
                    std::cout << "min_S_in_P" << std::endl;
#endif
                    return 0;
                } else {
                    //S
#if VERBOSE
                    std::cout << "min_S" << std::endl;
#endif
                    return 0;
                }
            } else if (is_variable_predicate(var)) {
                //1. We have to go down in the trie of p
                if (m_cur_s != -1UL && m_cur_o != -1UL) {
                    //SO->P
#if VERBOSE
                    std::cout << "min_P_in_SO" << std::endl;
#endif
                    return 0;
                } else if (m_cur_s != -1UL) {
                    //S->P
#if VERBOSE
                    std::cout << "min_P_in_S" << std::endl;
#endif
                    return 0;
                } else if (m_cur_o != -1UL) {
                    //O->P
#if VERBOSE
                    std::cout << "min_P_in_O" << std::endl;
#endif
                    return 0;
                } else {
                    //P
#if VERBOSE
                    std::cout << "min_P" << std::endl;
#endif
                    return 0;
                }
            } else if (is_variable_object(var)) {
                //1. We have to go down in the trie of o
                if (m_cur_s != -1UL && m_cur_p != -1UL) {
                    //SP->O
#if VERBOSE
                    std::cout << "min_O_in_SP" << std::endl;
#endif
                    return 0;
                } else if (m_cur_s != -1UL) {
                    //S->O
#if VERBOSE
                    std::cout << "min_O_in_S" << std::endl;
#endif
                    return 0;
                } else if (m_cur_p != -1UL) {
                    //P->O
#if VERBOSE
                    std::cout << "min_O_in_P" << std::endl;
#endif
                    return 0;
                } else {
                    //O
#if VERBOSE
                    std::cout << "min_O" << std::endl;
#endif
                    return 0;
                }
            }
            return 0;
        };

        value_type leap(var_type var, size_type c) { //Return the next value greater or equal than c in the range
            //0. Which term of our triple pattern is var
            if (is_variable_subject(var)) {
                //1. We have to go down through s
                if (m_cur_p != -1UL && m_cur_o != -1UL) {
                    //PO->S
#if VERBOSE
                    std::cout << "next_S_in_PO" << std::endl;
#endif
                    return 0;
                } else if (m_cur_o != -1UL) {
                    //O->S
#if VERBOSE
                    std::cout << "next_S_in_O" << std::endl;
#endif
                    return 0;
                } else if (m_cur_p != -1UL) {
                    //P->S
#if VERBOSE
                    std::cout << "next_S_in_P" << std::endl;
#endif
                    return 0;
                } else {
                    //S
#if VERBOSE
                    std::cout << "next_S" << std::endl;
#endif
                    return 0;
                }
            } else if (is_variable_predicate(var)) {
                //1. We have to go down in the trie of p
                if (m_cur_s != -1UL && m_cur_o != -1UL) {
                    //SO->P
#if VERBOSE
                    std::cout << "next_P_in_SO" << std::endl;
#endif
                    return 0;
                } else if (m_cur_s != -1UL) {
                    //S->P
#if VERBOSE
                    std::cout << "next_P_in_S" << std::endl;
#endif
                    return 0;
                } else if (m_cur_o != -1UL) {
                    //O->P
#if VERBOSE
                    std::cout << "next_P_in_O" << std::endl;
#endif
                    return 0;
                } else {
                    //P
#if VERBOSE
                    std::cout << "next_P" << std::endl;
#endif
                    return 0;
                }
            } else if (is_variable_object(var)) {
                //1. We have to go down in the trie of o
                if (m_cur_s != -1UL && m_cur_p != -1UL) {
                    //SP->O
#if VERBOSE
                    std::cout << "next_O_in_SP" << std::endl;
#endif
                    return 0;
                } else if (m_cur_s != -1UL) {
                    //S->O
#if VERBOSE
                    std::cout << "next_O_in_S" << std::endl;
#endif
                    return 0;
                } else if (m_cur_p != -1UL) {
                    //P->O
#if VERBOSE
                    std::cout << "next_O_in_P" << std::endl;
#endif
                    return 0;
                } else {
                    //O
#if VERBOSE
                    std::cout << "next_O" << std::endl;
#endif
                    return 0;
                }
            }
            return 0;
        }

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
