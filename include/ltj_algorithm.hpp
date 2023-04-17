/*
 * ltj_algorithm.hpp
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



#ifndef LTJ_ALGORITHM_HPP
#define LTJ_ALGORITHM_HPP


#include <triple_pattern.hpp>
#include <cltj.hpp>
#include <gao.hpp>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <ltj_iterator.hpp>
namespace ltj {

    template<class index_scheme_t = index_scheme::compactLTJ, class var_t = uint8_t, class cons_t = uint64_t, class ltj_iterator_t = ltj_iterator<index_scheme_t, var_t,cons_t>>//, class gao = gao_t<>
    class ltj_algorithm {

    public:
        typedef uint64_t value_type;
        typedef uint64_t size_type;
        typedef var_t var_type;
        typedef index_scheme_t index_scheme_type;
        typedef cons_t const_type;
        typedef ltj_iterator_t ltj_iter_type;
        typedef std::unordered_map<var_type, std::vector<ltj_iter_type*>> var_to_iterators_type;
        typedef std::vector<std::pair<var_type, value_type>> tuple_type;
        typedef std::chrono::high_resolution_clock::time_point time_point_type;

        typedef std::pair<size_type, var_type> pair_type;
        typedef std::priority_queue<pair_type, std::vector<pair_type>, std::greater<pair_type>> min_heap_type;

        typedef struct {
            size_type triple_number;
            ltj_iter_type* iterator;
            std::unordered_set<var_type> related;
            bool empty = true;
        } triple_iter_related_type;
        typedef struct {
            var_type name;
            size_type weight;
            size_type n_triples;
            std::unordered_set<var_type> related;
            std::vector<size_type> triple_ids;//Stores a list of k triples_{x_j} to do a linear search over them and not against all triples.
            std::vector<triple_iter_related_type> triple_iter_related_details;
        } info_var_type;
    private:
        std::vector<info_var_type> m_var_info;
        std::unordered_map<var_type, size_type> m_hash_table_position;
        const std::vector<rdf::triple_pattern>* m_ptr_triple_patterns;
        std::vector<var_type> m_gao; //TODO: should be a class
        index_scheme_type* m_ptr_index;
        std::stack<var_type> m_gao_stack;
        //m_gao_vars is a 1:1 umap representation the m_gao_stack, everything in m_gao_stack is true in this structure.
        std::unordered_map<var_type, bool> m_gao_vars;
        //gao_type m_gao_test;
        std::vector<ltj_iter_type> m_iterators;
        var_to_iterators_type m_var_to_iterators;
        bool m_is_empty = false;
        gao_size<info_var_type, var_to_iterators_type, index_scheme_type> m_gao_size;
        void copy(const ltj_algorithm &o) {
            m_ptr_triple_patterns = o.m_ptr_triple_patterns;
            m_gao = o.m_gao;
            m_ptr_index = o.m_ptr_index;
            m_iterators = o.m_iterators;
            m_var_to_iterators = o.m_var_to_iterators;
            m_is_empty = o.m_is_empty;
            m_var_info = o.m_var_info;
            m_hash_table_position = o.m_hash_table_position;
        }
        info_var_type& get_var_info(var_type x_j) {
            return m_var_info[m_hash_table_position[x_j]];
        }
        bool is_var_lonely(var_type x_j){
            return get_var_info(x_j).n_triples == 1;
        }
    public:


        ltj_algorithm() = default;
        //TODO: move to another function / class that manages the variable information.
        void var_to_related(const var_type var, const var_type rel,
                            std::unordered_map<var_type, size_type> &hash_table,
                            std::vector<info_var_type> &vec){

            auto pos_var = hash_table[var];
            vec[pos_var].related.insert(rel);
            auto pos_rel = hash_table[rel];
            vec[pos_rel].related.insert(var);
        }
        //TODO: move to another function / class that manages the variable information.
        void var_to_vector(const var_type var, const size_type weight, uint64_t num_of_triples,
                            std::unordered_map<var_type, size_type> &hash_table,
                            std::vector<info_var_type> &vec){

            auto it = hash_table.find(var);
            if(it == hash_table.end()){
                info_var_type info;
                info.name = var;
                info.weight = weight;
                info.n_triples = 1;
                info.triple_iter_related_details.reserve(num_of_triples);
                for(uint64_t i = 0 ; i < num_of_triples; i++){
                    info.triple_iter_related_details.emplace_back();
                }
                vec.emplace_back(info);
                hash_table.insert({var, vec.size()-1});
            }else{
                info_var_type& info = vec[it->second];
                ++info.n_triples;
                if(info.weight > weight){
                    info.weight = weight;
                }
            }
        }

        ltj_algorithm(const std::vector<rdf::triple_pattern>* triple_patterns, index_scheme_type* index){

            m_ptr_triple_patterns = triple_patterns;
            uint64_t number_of_triples = m_ptr_triple_patterns->size();
            m_ptr_index = index;
            size_type i = 0;
            m_iterators.resize(m_ptr_triple_patterns->size());//TODO: get rid of it.
            //TODO: >> move to another function / class that manages the variable information.
            for(const auto& triple : *m_ptr_triple_patterns){
                var_type var_s, var_p, var_o;
                bool s = false, p = false, o = false;
                if(triple.s_is_variable()){
                    auto* iter = new ltj_iter_type(&triple, triple.term_s.value, "0", m_ptr_index);
                    //add_var_to_iterator(triple.term_s.value, &iter);//TODO: It seems is not necessary anymore.
                    if(iter->is_empty){
                        m_is_empty = true;
                        return;
                    }
                    s = true;
                    var_s = (var_type) triple.term_s.value;
                    auto weight = iter->get_weight();
                    var_to_vector(var_s, weight, number_of_triples, m_hash_table_position, m_var_info);

                    auto& triple_iter_related = get_var_info(var_s).triple_iter_related_details[i];
                    triple_iter_related.triple_number = i;
                    triple_iter_related.iterator = iter;
                    triple_iter_related.empty = false;
                    get_var_info(var_s).triple_ids.push_back(i);
                    if(triple.p_is_variable()){
                        triple_iter_related.related.insert((var_type) triple.term_p.value);
                    }
                    if(triple.o_is_variable()){
                        triple_iter_related.related.insert((var_type) triple.term_o.value);
                    }
                }
                if(triple.p_is_variable()){
                    auto* iter = new ltj_iter_type(&triple, triple.term_p.value, "1", m_ptr_index);
                    //add_var_to_iterator(triple.term_p.value, &iter);//TODO: It seems is not necessary anymore.

                    if(iter->is_empty){
                        m_is_empty = true;
                        return;
                    }
                    p = true;
                    var_p = (var_type) triple.term_p.value;
                    auto weight = iter->get_weight();
                    var_to_vector(var_p, weight, number_of_triples, m_hash_table_position, m_var_info);

                    auto& triple_iter_related = get_var_info(var_p).triple_iter_related_details[i];
                    triple_iter_related.triple_number = i;
                    triple_iter_related.iterator = iter;
                    triple_iter_related.empty = false;
                    get_var_info(var_p).triple_ids.push_back(i);
                    if(triple.s_is_variable()){
                        triple_iter_related.related.insert((var_type) triple.term_s.value);
                    }
                    if(triple.o_is_variable()){
                        triple_iter_related.related.insert((var_type) triple.term_o.value);
                    }
                }
                if(triple.o_is_variable()){
                    auto* iter = new ltj_iter_type(&triple, triple.term_o.value, "2", m_ptr_index);//TODO: MemLeak
                    //add_var_to_iterator(triple.term_o.value, &iter);//TODO: It seems is not necessary anymore.

                    if(iter->is_empty){
                        m_is_empty = true;
                        return;
                    }
                    o = true;
                    var_o = triple.term_o.value;
                    auto weight = iter->get_weight();
                    var_to_vector(var_o, weight, number_of_triples, m_hash_table_position, m_var_info);

                    auto& triple_iter_related = get_var_info(var_o).triple_iter_related_details[i];
                    triple_iter_related.triple_number = i;
                    triple_iter_related.iterator = iter;
                    triple_iter_related.empty = false;
                    get_var_info(var_o).triple_ids.push_back(i);
                    if(triple.s_is_variable()){
                        triple_iter_related.related.insert((var_type) triple.term_s.value);
                    }
                    if(triple.p_is_variable()){
                        triple_iter_related.related.insert((var_type) triple.term_p.value);
                    }
                }
                if(s && p){
                    var_to_related(var_s, var_p, m_hash_table_position, m_var_info);
                }
                if(s && o){
                    var_to_related(var_s, var_o, m_hash_table_position, m_var_info);
                }
                if(p && o){
                    var_to_related(var_p, var_o, m_hash_table_position, m_var_info);
                }
                ++i;
                //TODO: << move to another function / class that manages the variable information.
            }
            m_gao_size = gao_size<info_var_type, var_to_iterators_type, index_scheme_type>(m_ptr_triple_patterns, m_var_info, m_hash_table_position, &m_var_to_iterators, m_ptr_index, m_gao);
            m_gao_vars.reserve(m_gao_size.number_of_variables);
        }

        //! Copy constructor
        ltj_algorithm(const ltj_algorithm &o) {
            copy(o);
        }

        //! Move constructor
        ltj_algorithm(ltj_algorithm &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        ltj_algorithm &operator=(const ltj_algorithm &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        ltj_algorithm &operator=(ltj_algorithm &&o) {
            if (this != &o) {
                m_ptr_triple_patterns = std::move(o.m_ptr_triple_patterns);
                m_gao = std::move(o.m_gao);
                m_ptr_index = std::move(o.m_ptr_index);
                m_iterators = std::move(o.m_iterators);
                m_var_to_iterators = std::move(o.m_var_to_iterators);
                m_is_empty = o.m_is_empty;
                m_var_info = std::move(o.m_var_info);
                m_hash_table_position = std::move(o.m_hash_table_position);
            }
            return *this;
        }

        void swap(ltj_algorithm &o) {
            std::swap(m_ptr_triple_patterns, o.m_ptr_triple_patterns);
            std::swap(m_gao, o.m_gao);
            std::swap(m_ptr_index, o.m_ptr_index);
            std::swap(m_iterators, o.m_iterators);
            std::swap(m_var_to_iterators, o.m_var_to_iterators);
            std::swap(m_is_empty, o.m_is_empty);
            std::swap(m_var_info, o.m_var_info);
            std::swap(m_hash_table_position, o.m_hash_table_position);
        }


        /**
        *
        * @param res               Results
        * @param limit_results     Limit of results
        * @param timeout_seconds   Timeout in seconds
        */
        void join(std::vector<tuple_type> &res,
                  const size_type limit_results = 0, const size_type timeout_seconds = 0){
            if(m_is_empty) return;
            time_point_type start = std::chrono::high_resolution_clock::now();
            tuple_type t(m_gao_size.number_of_variables);
            search(0, t, res, start, limit_results, timeout_seconds);
        };

        std::string get_gao(std::unordered_map<uint8_t, std::string>& ht) const{
            std::string str = "";
            for(const auto& var : m_gao){
                str += "?" + ht[var] + " ";
            }
            return str;
        }
        var_type next(const size_type j) {
            if(index_scheme::util::configuration.is_adaptive()){
                var_type var = '\0';
                const var_type& cur_var = m_gao_stack.top();
                const std::unordered_map<var_type, bool> & b_vars = m_gao_vars;
                //refresh rel var iterators?
                m_gao_size.update_weights(j, cur_var, b_vars, m_var_to_iterators);
                var = m_gao_size.get_next_var(j, m_gao_vars);
                return var;
            }
            return m_gao[j];
        }

        void push_var_to_stack(const var_type& x_j){
            //assert (m_gao_stack.top() == x_j);
            m_gao_stack.push(x_j);
            m_gao_vars[x_j]=true;
        }

        void pop_var_of_stack(){
            auto v = m_gao_stack.top();
            //clean ref to iters.
            m_gao_stack.pop();
            m_gao_vars[v]=false;
        }
        /**
         *
         * @param j                 Index of the variable
         * @param tuple             Tuple of the current search
         * @param res               Results
         * @param start             Initial time to check timeout
         * @param limit_results     Limit of results
         * @param timeout_seconds   Timeout in seconds
         */
        bool search(const size_type j, tuple_type &tuple, std::vector<tuple_type> &res,
                    const time_point_type start,
                    const size_type limit_results = 0, const size_type timeout_seconds = 0){

            //(Optional) Check timeout
            /*if(timeout_seconds > 0){
                time_point_type stop = std::chrono::high_resolution_clock::now();
                size_type sec = std::chrono::duration_cast<std::chrono::seconds>(stop-start).count();
                if(sec > timeout_seconds) return false;
            }*/

            //(Optional) Check limit
            if(limit_results > 0 && res.size() == limit_results) return false;

            if(j == m_gao_size.number_of_variables){
                //Report results
                /*
                std::cout << "tuple : ";
                for(auto& pair : tuple){
                    std::cout << int(pair.first) << " = " << pair.second << std::endl;
                }
                std::cout << " " << std::endl;
                */
                res.emplace_back(tuple);
            }else{
                //assert(m_gao_stack.size() == m_gao_vars.size());
                var_type x_j = next(j);
                push_var_to_stack(x_j);
                std::vector<ltj_iter_type*>& itrs = m_var_to_iterators[x_j];
                bool ok;
                //The second case means when there's only a triple, all vars are lonely (Some of them are not in the last level though). Therefore we dont need to do an extra down.
                if(itrs.size() == 1 && !itrs[0]->in_last_level() && m_ptr_triple_patterns->size() > 1){
                    itrs[0]->down(x_j);
                }
                if(itrs.size() == 1 && itrs[0]->in_last_level()) {//Lonely variables in the last level.

                    auto results = itrs[0]->seek_all(x_j);
                    //std::cout << "Results: " << results.size() << std::endl;
                    for (const auto &c : results) {
                        //1. Adding result to tuple
                        tuple[j] = {x_j, c};
                        //std::cout << "current var: " << int(std::get<0>(tuple[j])) << " = " << std::get<1>(tuple[j]) << std::endl;
                        //2. Going down in the trie by setting x_j = c (\mu(t_i) in paper)
                        itrs[0]->down(x_j);//x_j, c
                        //2. Search with the next variable x_{j+1}
                        ok = search(j + 1, tuple, res, start, limit_results, timeout_seconds);
                        if(!ok) return false;
                        //4. Going up in the trie by removing x_j = c
                        itrs[0]->up(x_j);
                    }
                }else {

                    value_type c = seek(x_j, j);
                    //std::cout << "Seek (init): (" << (uint64_t) x_j << ": " << c << ")" <<std::endl;

                    while (c != 0) { //If empty c=0
                        //1. Adding result to tuple
                        tuple[j] = {x_j, c};
                        //std::cout << "current var: " << int(std::get<0>(tuple[j])) << " = " << std::get<1>(tuple[j]) << std::endl;
                        //2. Going down in the tries by setting x_j = c (\mu(t_i) in paper)
                        for (ltj_iter_type* iter : itrs) {
                            iter->down(x_j);
                        }
                        //3. Search with the next variable x_{j+1}
                        ok = search(j + 1, tuple, res, start, limit_results, timeout_seconds);
                        if(!ok) return false;
                        //4. Going up in the tries by removing x_j = c
                        for (ltj_iter_type *iter : itrs) {
                            iter->up(x_j);
                        }//el down y up siempre tienen que ir porque cuando reporto necesito hacer un up despues.
                        //5. Next constant for x_j
                        c = seek(x_j, j, c + 1);//<-- AQUI DEBO preocuparme de que los iters esten en el nivel de la varible.
                        //std::cout << "Seek (bucle): (" << (uint64_t) x_j << ": " << c << ")" <<std::endl;
                    }
                }
                if(index_scheme::util::configuration.is_adaptive()){
                    m_gao_size.set_previous_weight();
                }
                pop_var_of_stack();
                //std::cout << " pop. " << std::endl;
            }
            return true;
        };

        bool inline is_bound_subject_variable(ltj_iter_type* iter){
            return iter->get_triple_pattern()->term_s.is_variable &&
                m_gao_vars[iter->get_triple_pattern()->term_s.value];
        }

        bool inline is_bound_predicate_variable(ltj_iter_type* iter){
            return iter->get_triple_pattern()->term_p.is_variable &&
                m_gao_vars[iter->get_triple_pattern()->term_p.value];
        }

        bool inline is_bound_object_variable(ltj_iter_type* iter){
            return iter->get_triple_pattern()->term_o.is_variable &&
                m_gao_vars[iter->get_triple_pattern()->term_o.value];
        }
        /*
        Called within seek() before returning 0 when leap() finds no intersection. TODO: this should be a wrapper to another func in ltj_iterator.move to ltj_iterator.        */
        void check_restart_var_level_iterator(const var_type x_j,size_type c){
            for (auto& iter : m_var_to_iterators[x_j]){
                bool restart_iter = true;
                //TODO: improve this linear search.
                //If in the triple referred by iterator 'iter' there is a bound variable in the subject that is not 'x_j'.
                //We have to restart from from the first child of the parent node, which is accomplished with an up() followed by a down().
                if(is_bound_subject_variable(iter) &&
                iter->get_triple_pattern()->term_s.value != x_j &&
                iter->get_triple_pattern()->term_s.value != m_gao_size.get_starting_var() //If the other rel variable is the first one, do not go up.TODO: possible problem when 3 vars are in a triple? how to know the level? I guess if level is 0 do not go up. Get the level of the first variable.
                ){
                    restart_iter = false;
                }
                if(is_bound_predicate_variable(iter) &&
                iter->get_triple_pattern()->term_p.value != x_j &&
                iter->get_triple_pattern()->term_p.value != m_gao_size.get_starting_var() //If the other rel variable is the first one, do not go up.TODO: possible problem when 3 vars are in a triple? how to know the level? I guess if level is 0 do not go up. Get the level of the first variable.
                ){
                    restart_iter = false;
                }
                if(is_bound_object_variable(iter) &&
                iter->get_triple_pattern()->term_o.value != x_j &&
                iter->get_triple_pattern()->term_o.value != m_gao_size.get_starting_var() //If the other rel variable is the first one, do not go up.TODO: possible problem when 3 vars are in a triple? how to know the level? I guess if level is 0 do not go up. Get the level of the first variable.
                ){
                    restart_iter = false;
                }
                if(restart_iter){
                    iter->restart_level_iterator(x_j, c);
                }else{
                    iter->up(x_j);
                }
            }
        }

        /*
        std::vector<uint64_t> seek_all(ltj_iter_type* iter,var_type x_j){
            return iter->seek_all(x_j);
        }
        */
        /**
         *
         * @param x_j   Variable
         * @param j     Number of variables eliminated
         * @param c     Constant. If it is unknown the value is -1
         * @return      The next constant that matches the intersection between the triples of x_j.
         *              If the intersection is empty, it returns 0.
         */

        value_type seek(const var_type x_j, const size_type j, value_type c=-1){
            std::vector<ltj_iter_type*>& itrs = m_var_to_iterators[x_j];
            value_type c_i, c_prev = 0, i = 0, n_ok = 0;

            //if(c!= -1){
                for(auto& iter : itrs){
                    if(!iter->at_level_of_var(x_j)){
                        iter->down(x_j);
                    }
                }
            //}

            while (true){
                //Compute leap for each triple that contains x_j
                c_i = itrs[i]->leap(c);
                if(c_i == 0){
                    check_restart_var_level_iterator(x_j, c);
                    return 0; //Empty intersection
                }
                n_ok = (c_i == c_prev) ? n_ok + 1 : 1;
                if(n_ok == itrs.size()) return c_i;
                c = c_prev = c_i;
                i = (i+1 == itrs.size()) ? 0 : i+1;
            }
        }

    };
}

#endif //LTJ_ALGORITHM_HPP
