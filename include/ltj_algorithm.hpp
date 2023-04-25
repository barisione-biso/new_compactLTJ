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
        typedef std::string order_type;
        typedef std::unordered_map<var_type, std::vector<ltj_iter_type*>> var_to_iterators_type;
        typedef std::vector<std::pair<var_type, value_type>> tuple_type;
        typedef std::chrono::high_resolution_clock::time_point time_point_type;
        typedef std::vector<order_type> orders_type;
        typedef std::unordered_map<var_type, orders_type> var_to_orders_type;
        typedef std::unordered_map<order_type,ltj_iter_type*> orders_to_iterators_type;
        typedef std::vector<orders_to_iterators_type> bgp_triple_iters;
        typedef std::pair<size_type, var_type> pair_type;
        typedef std::priority_queue<pair_type, std::vector<pair_type>, std::greater<pair_type>> min_heap_type;

        typedef struct {
            orders_to_iterators_type order_to_iterator;
            std::unordered_set<var_type> related;
            bool empty = true;
        } triple_info_type;
        typedef struct {
            var_type name;
            size_type weight;
            size_type n_triples;
            std::unordered_set<var_type> related;
            //std::vector<size_type> triple_ids;//Stores a list of k triples_{x_j} to do a linear search over them and not against all triples.
            std::unordered_map<size_type, triple_info_type> triples;
        } info_var_type;
    private:
        std::vector<info_var_type> m_var_info;
        bgp_triple_iters m_triple_iters;
        std::unordered_map<var_type, size_type> m_hash_table_position;
        const std::vector<rdf::triple_pattern>* m_ptr_triple_patterns;
        std::vector<var_type> m_gao; //TODO: should be a class
        index_scheme_type* m_ptr_index;
        std::stack<var_type> m_gao_stack;
        //m_gao_vars is a 1:1 umap representation the m_gao_stack, everything in m_gao_stack is true in this structure.
        std::unordered_map<var_type, bool> m_gao_vars;
        //gao_type m_gao_test;
        var_to_iterators_type m_var_to_iterators;
        bool m_is_empty = false;
        gao_size<info_var_type, var_to_iterators_type, index_scheme_type> m_gao_size;
        std::unordered_map<var_type, size_type> m_var_to_n_triples;
        void copy(const ltj_algorithm &o) {
            m_ptr_triple_patterns = o.m_ptr_triple_patterns;
            m_gao = o.m_gao;
            m_ptr_index = o.m_ptr_index;
            m_var_to_iterators = o.m_var_to_iterators;
            m_is_empty = o.m_is_empty;
            m_var_info = o.m_var_info;
            m_hash_table_position = o.m_hash_table_position;
        }
        info_var_type& get_var_info(var_type x_j) {
            return m_var_info[m_hash_table_position[x_j]];
        }
        triple_info_type& get_triple_info(var_type x_j, size_type triple_index){
            return get_var_info(x_j).triples[triple_index];
        }
        std::unordered_map<size_type, triple_info_type>& get_triples_info(var_type x_j){
            return get_var_info(x_j).triples;
        }
        orders_to_iterators_type& get_var_iterators_by_triple(var_type x_j, size_type triple_index){
            return get_triple_info(x_j, triple_index).order_to_iterator;
        }

        ltj_iter_type* get_var_iterator_by_triple_and_order(var_type x_j, size_type triple_index, order_type order){
            return get_triple_info(x_j, triple_index).order_to_iterator[order];
        }
        bool is_var_lonely(var_type x_j){
            if(m_var_to_n_triples.find(x_j) != m_var_to_n_triples.end()){
                return  m_var_to_n_triples[x_j] == 1 ? true : false;
            }
            return false;
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
        void var_to_vector(const var_type var, const size_type weight,
                            std::unordered_map<var_type, size_type> &hash_table,
                            std::vector<info_var_type> &vec){

            auto it = hash_table.find(var);
            if(it == hash_table.end()){
                info_var_type info;
                info.name = var;
                info.weight = weight;
                vec.emplace_back(info);
                hash_table.insert({var, vec.size()-1});
            }else{
                info_var_type& info = vec[it->second];
                if(info.weight > weight){
                    info.weight = weight;
                }
            }
        }
        var_to_orders_type get_orders(const rdf::triple_pattern& triple){
            var_to_orders_type var_to_orders;
            size_type number_of_variables = 0;
            std::stringstream order_aux;
            //This must be removed if we don't want to support it anymore.
            //std::vector<var_type> all_vars_to_calculate_lonely_weight;
            //First count variables and setup the constants.
            if(triple.s_is_variable()){
                number_of_variables++;
            }else{
                order_aux<<"0 ";
            }
            if(triple.p_is_variable()){
                number_of_variables++;
            }else{
                order_aux<<"1 ";
            }
            if(triple.o_is_variable()){
                number_of_variables++;
            }else{
                order_aux<<"2 ";
            }
            //Then evaluate case by case, 1 var, 2 vars and 3 vars.
            if(number_of_variables == 1){
                //1 iterator
                var_type var = '\0';
                if(triple.s_is_variable()){
                    order_aux<<"0 ";
                    var = triple.term_s.value;
                }
                if(triple.p_is_variable()){
                    order_aux<<"1 ";
                    var = triple.term_p.value;
                }
                if(triple.o_is_variable()){
                    order_aux<<"2 ";
                    var = triple.term_o.value;
                }

                std::string str = order_aux.str();
                index_scheme::util::rtrim(str);
                var_to_orders[var].emplace_back(str);
            }
            if(number_of_variables >= 2){
                std::vector<var_type> variables;
                std::vector<var_type> lonely_variables;
                std::stringstream order_lonely_vars;
                //2 non-lonely variables yield 2 iterators.
                //3 non-lonely variables yield 6 iterators.
                if(triple.s_is_variable()){
                    //if(!is_var_lonely(triple.term_s.value)){ //Commented to not bypass lonely var iterator.
                        variables.emplace_back(triple.term_s.value);
                    /*}else{ //Commented to not bypass lonely var iterator.
                        lonely_variables.emplace_back(triple.term_s.value);
                        order_lonely_vars << "0 ";
                    }*/
                }
                if(triple.p_is_variable()){
                    //if(!is_var_lonely(triple.term_p.value)){ //Commented to not bypass lonely var iterator.
                        variables.emplace_back(triple.term_p.value);
                    /*}else{ //Commented to not bypass lonely var iterator.
                        lonely_variables.emplace_back(triple.term_p.value);
                        order_lonely_vars << "1 ";
                    }*/
                }
                if(triple.o_is_variable()){
                    //if(!is_var_lonely(triple.term_o.value)){ //Commented to not bypass lonely var iterator.
                        variables.emplace_back(triple.term_o.value);
                    /*}else{ //Commented to not bypass lonely var iterator.
                        lonely_variables.emplace_back(triple.term_o.value);
                        order_lonely_vars << "2 ";
                    }*/
                }
                //get all permutations
                std::sort(variables.begin(), variables.end());
                do{
                    size_type owner_var = -1UL;
                    std::stringstream order_vars;
                    for(var_type& var: variables){
                        if(triple.s_is_variable() && (var_type) triple.term_s.value == var){
                            order_vars<<"0 ";
                            if(owner_var == -1UL){
                                owner_var = triple.term_s.value;
                            }
                        }
                        if(triple.p_is_variable() && (var_type) triple.term_p.value == var){
                            order_vars<<"1 ";
                            if(owner_var == -1UL){
                                owner_var = triple.term_p.value;
                            }
                        }
                        if(triple.o_is_variable() && (var_type) triple.term_o.value == var){
                            order_vars<<"2 ";
                            if(owner_var == -1UL){
                                owner_var = triple.term_o.value;
                            }
                        }
                    }

                    std::string str = order_aux.str() + order_vars.str() + order_lonely_vars.str();
                    index_scheme::util::rtrim(str);
                    if(owner_var != -1UL){
                        var_to_orders[(var_type) owner_var].emplace_back(str);
                    }
                    for(auto& lonely_variable : lonely_variables){
                        var_to_orders[(var_type) lonely_variable].emplace_back(str);
                    }

                }while(std::next_permutation(variables.begin(), variables.end()));
            }
            return var_to_orders;//Case 0 vars: returns an empty vector.
        }
        void categorize_var(var_type var, size_type i,
                            std::unordered_map<var_type, std::vector<order_type>>& regular_var_to_orders,
                            std::unordered_map<var_type, std::vector<order_type>>& lonely_var_to_orders,
                            size_type& number_of_lonely){
            triple_info_type& triple_info = get_triple_info(var, i);
            if(is_var_lonely(var)){
                number_of_lonely++;
                for(auto& iter : triple_info.order_to_iterator){
                    lonely_var_to_orders[var].emplace_back(iter.first);
                }
            }else{
                for(auto& iter : triple_info.order_to_iterator){
                    regular_var_to_orders[var].emplace_back(iter.first);
                }
            }
        }
        void refresh_lonely_var_iters(){
            /*Up to this point we have calculated the weight of all variables using their iterators, even for the lonely ones.
            Now we need to do the following:
            per each triple t:
                a. Separate vars into non-lonely and lonely vars and keep a reference to their iterators / orders.
                then,
                b. If there is 1 lonely:
                    Delete the lonely var iterators_{t} and add a reference to the iterators of the non-lonely vars_{t}, either 1 or 2.
                c. If there are 2 lonely:
                    Delte their iterators_{t} and add a reference to the iterator of the non-lonely var_{t}
                d. If there are 3 (or 0) lonely:
                    Do nothing.
            */
            if(m_ptr_triple_patterns->size() <= 1){
                //if the BGP has only a single triple then there's nothing to do in this function.
                return;
            }
            size_type i = 0;

            for(const auto& triple : *m_ptr_triple_patterns){
                std::unordered_map<var_type, std::vector<order_type>> regular_var_to_orders;
                std::unordered_map<var_type, std::vector<order_type>> lonely_var_to_orders;//TODO: it is not required to be an umap. maybe just a vector?
                size_type number_of_lonely = 0;
                //a.
                if(triple.s_is_variable()){
                    var_type var = (var_type) triple.term_s.value;
                    categorize_var(var, i, regular_var_to_orders, lonely_var_to_orders, number_of_lonely);
                }
                if(triple.p_is_variable()){
                    var_type var = (var_type) triple.term_p.value;
                    categorize_var(var, i, regular_var_to_orders, lonely_var_to_orders, number_of_lonely);
                }
                if(triple.o_is_variable()){
                    var_type var = (var_type) triple.term_o.value;
                    categorize_var(var, i, regular_var_to_orders, lonely_var_to_orders, number_of_lonely);
                }

                //d.
                if(number_of_lonely == 0 || number_of_lonely == 3){
                    return;
                //a & b.
                } else if(number_of_lonely == 1 || number_of_lonely == 2){
                    for(auto& lonely_it : lonely_var_to_orders){
                        var_type var = lonely_it.first;
                        //Delete current iterators and clear the map.
                        orders_to_iterators_type& iterator_per_order = get_var_iterators_by_triple(var, i);
                        for(auto& it : iterator_per_order){
                            //deleting ltj_iter_type* iter.
                            delete it.second;//TODO: This delete does not work. Fix it!
                        }
                        iterator_per_order.clear();
                        //Add a reference to the non-lonely var iterator(s).
                        for(auto& var_to_orders : regular_var_to_orders){
                            auto& regular_var = var_to_orders.first;
                            auto& orders = var_to_orders.second;
                            for(auto& order : orders){
                                iterator_per_order[order] = get_var_iterator_by_triple_and_order(regular_var, i, order);
                            }
                        }
                    }
                }
                i++;
            }
        }
        void create_iterators(var_type x_j, const rdf::triple_pattern& triple, size_type triple_index, const orders_type& orders){
            if(orders.size() == 0){
                m_is_empty = true;
                return;
            }else{
                for(auto& order : orders){
                    auto* iter = new ltj_iter_type(&triple, x_j, order, m_ptr_index, triple_index);
                    if(iter->is_empty){
                        m_is_empty = true;
                    }
                    auto weight = iter->get_weight();
                    //std::cout << "New iter for " << (size_type) x_j << " with order " << order << " and weight " << weight << std::endl;
                    var_to_vector(x_j, weight, m_hash_table_position, m_var_info);
                    //>> Ordenar
                    auto& triples = get_triples_info(x_j);
                    auto& triple = triples[triple_index];
                    auto& order_to_iterator = triple.order_to_iterator;
                    order_to_iterator[order] = iter;
                    get_triple_info(x_j, triple_index).empty = false;
                    //<< Ordenar
                }
            }
        }

        ltj_algorithm(const std::vector<rdf::triple_pattern>* triple_patterns, index_scheme_type* index){

            m_ptr_triple_patterns = triple_patterns;
            size_type number_of_triples = m_ptr_triple_patterns->size();
            m_ptr_index = index;
            size_type i = 0;
            //TODO: >> move to another function / class that manages the variable information.
            //Count num of triples per variable.
            for(const auto& triple : *m_ptr_triple_patterns){
                if(triple.s_is_variable()){
                    var_type var = (var_type) triple.term_s.value;
                    if(m_var_to_n_triples.find(var) == m_var_to_n_triples.end()){
                        m_var_to_n_triples[var] = 1;
                    }else{
                        m_var_to_n_triples[var] = m_var_to_n_triples[var] + 1;
                    }
                }
                if(triple.p_is_variable()){
                    var_type var = (var_type) triple.term_p.value;
                    if(m_var_to_n_triples.find(var) == m_var_to_n_triples.end()){
                        m_var_to_n_triples[var] = 1;
                    }else{
                        m_var_to_n_triples[var] = m_var_to_n_triples[var] + 1;
                    }
                }
                if(triple.o_is_variable()){
                    var_type var = (var_type) triple.term_o.value;
                    if(m_var_to_n_triples.find(var) == m_var_to_n_triples.end()){
                        m_var_to_n_triples[var] = 1;
                    }else{
                        m_var_to_n_triples[var] = m_var_to_n_triples[var] + 1;
                    }
                }
            }
            for(const auto& triple : *m_ptr_triple_patterns){
                var_type var_s, var_p, var_o;
                bool s = false, p = false, o = false;
                var_to_orders_type var_to_orders;
                var_to_orders = get_orders(triple);//TODO: try to use a const-ref.
                if(triple.s_is_variable()){
                    var_s = (var_type) triple.term_s.value;
                    create_iterators(var_s, triple, i, var_to_orders[var_s]);
                    s = true;
                    if(triple.p_is_variable()){
                        get_triple_info(var_s, i).related.insert((var_type) triple.term_p.value);
                    }
                    if(triple.o_is_variable()){
                        get_triple_info(var_s, i).related.insert((var_type) triple.term_o.value);
                    }
                }
                if(triple.p_is_variable()){
                    var_p = (var_type) triple.term_p.value;
                    create_iterators(var_p, triple, i, var_to_orders[var_p]);
                    p = true;
                    if(triple.s_is_variable()){
                        get_triple_info(var_p, i).related.insert((var_type) triple.term_s.value);
                    }
                    if(triple.o_is_variable()){
                        get_triple_info(var_p, i).related.insert((var_type) triple.term_o.value);
                    }
                }
                if(triple.o_is_variable()){
                    var_o = (var_type) triple.term_o.value;
                    create_iterators(var_o, triple, i, var_to_orders[var_o]);
                    o = true;
                    if(triple.s_is_variable()){
                        get_triple_info(var_o, i).related.insert((var_type) triple.term_s.value);
                    }
                    if(triple.p_is_variable()){
                        get_triple_info(var_o, i).related.insert((var_type) triple.term_p.value);
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
            //Improve the following code, unify.
            for(auto& var_info : m_var_info){
                var_info.n_triples = m_var_to_n_triples[var_info.name];
            }
            //Refresh lonely var iterators.
            refresh_lonely_var_iters();
            //Calculate the GAO.
            m_gao_size = gao_size<info_var_type, var_to_iterators_type, index_scheme_type>(m_ptr_triple_patterns, m_var_info, m_hash_table_position, &m_var_to_iterators, m_ptr_index, m_gao);
            m_gao_vars.reserve(m_gao_size.number_of_variables);
            //Initializing BGP Triples structure.
            m_triple_iters.reserve(m_ptr_triple_patterns->size());
            for(int k=0; k < m_ptr_triple_patterns->size(); k++){
                orders_to_iterators_type aux;
                m_triple_iters.emplace_back(aux);
            }
            //Finally assigning only a single iterator per triple (using m_m_var_to_iterators).
            /*if(index_scheme::util::configuration.is_adaptive()){
                manage_iterators(m_gao_size.get_starting_var());
            }else{*/
            if(!index_scheme::util::configuration.is_adaptive()){
                //Go through all vars in GAO and assign iterators into m_var_to_iterators.
                for(var_type& x_j : m_gao){
                    assign_iterators_to_bgp_triples(x_j);
                }
                //Cuando terminamos hay un solo iterador por cada triple. Debemos asignar dicho iterador para todas las variables x_k / 0 <= k <= 3 y que pertenecen al triple[i], i in |bgp|.
                //usando m_var_to_iterators[x_k] via add_var_to_iterator
                for(int k = 0; k < m_triple_iters.size(); k++){
                    if(m_triple_iters[k].empty()){
                        continue;
                    }
                    const auto& triple_definition = m_ptr_triple_patterns->at(k);
                    auto* iterator = m_triple_iters[k].begin()->second;
                    if(triple_definition.s_is_variable()){
                        add_var_to_iterator(triple_definition.term_s.value, iterator);
                    }
                    if(triple_definition.p_is_variable()){
                        add_var_to_iterator(triple_definition.term_p.value, iterator);;
                    }
                    if(triple_definition.o_is_variable()){
                        add_var_to_iterator(triple_definition.term_o.value, iterator);
                    }
                }
            }
            //std::cout << "done." << std::endl;
        }

        void clear_iterators(var_type var, size_type triple_index, var_type iterator_owner_var){
            //Por cada iterador de var.
            for(auto it = m_var_to_iterators[var].begin();it != m_var_to_iterators[var].end();){
                //Restringidos al triple dado por 'triple_index'
                if((*it)->triple_index == triple_index){
                    if(m_var_to_iterators[var].size() <= 0){
                        break;
                    }
                    if((*it)->owner_var == iterator_owner_var){
                        m_var_to_iterators[var].erase(it);
                    }else{
                        ++it;
                    }
                }else{
                    ++it;
                }
            }
        }
        //Used exclusively in the adaptive algorithm.
        void clear_iterators(var_type x_j){
            //1. Clearing all iterators in m_var_to_iterators for x_j owned by x_j.
            //m_var_to_iterators[x_j].clear();
            //2. Then, among the triples_{x_j}, check whether the iters of their related vars are owned by x_j. If so the iterator must be deleted.
            //Triples_{x_j}
            auto& triples_xj = get_triples_info(x_j);
            for(auto& triple_xj : triples_xj){
                size_type t_index = triple_xj.first;//Su indice.
                auto& triple_info = triple_xj.second;//triple_info, contiene related variables.
                //1. Clearing all iterators in m_var_to_iterators for x_j owned by x_j.
                clear_iterators(x_j, t_index, x_j);
                //2. Then, among the triples_{x_j}, check whether the iters of their related vars are owned by x_j. If so the iterator must be deleted.
                for(var_type rel_var : triple_info.related){
                    clear_iterators(rel_var, t_index, x_j);
                }

                //clearing BGP triple status as well.
                orders_to_iterators_type& triple_iter = m_triple_iters[t_index];
                for(auto it = triple_iter.begin();it != triple_iter.end();){
                    if(triple_iter.size() <= 0){//TODO: esto podria ser un punto de problema cuando hay 3 vars o no?
                        break;
                    }
                    if(it->second->owner_var == x_j){
                        triple_iter.erase(it);
                    }else{
                        ++it;
                    }
                }
                //The entry in `m_triple_iters` that we will check to know whether it has iterators owned by x_j .
                /*orders_to_iterators_type& iterators = m_triple_iters[t_index];//std::cout << "Checking triple #" << t_index << " with variable : " <<(int) x_j <<std::endl;
                for (auto it = iterators.begin(); it != iterators.end();){
                    if(it->second->owner_var == x_j){
                        //>>Besides deleting the iter from m_triples_iter[t_index], we need to delete it from m_var_to_iterators x_j' related_variables.
                        for(var_type rel_var :triple_info.related){
                            auto& iter_vector = m_var_to_iterators[rel_var];//check each related variables iterators.
                            for(auto rel_var_it = iter_vector.begin();rel_var_it != iter_vector.end();){
                                if((*rel_var_it)->owner_var == x_j){
                                    m_var_to_iterators[rel_var].erase(rel_var_it);
                                }else{
                                    ++rel_var_it;
                                }
                            }
                            //m_var_to_iterators[rel_var].emplace_back(it->second);???
                        }
                        //<<Besides deleting the iter from m_triples_iter[t_index], we need to delete it from m_var_to_iterators x_j' related_variables.
                        //iterators.erase(it);
                    } //else{
                    ++it;
                    //}
                }*/
            }

        }
        //Used by Precalculated GAO and adaptive variants.
        //Scenarios handled: 0 <= b <= 3, with b the number of constants.
        void assign_iterators_to_bgp_triples(var_type x_j){
            //Triples_{x_j}
            auto& triples_xj = get_triples_info(x_j);
            for(auto& triple_xj : triples_xj){
                size_type t_index = triple_xj.first;
                //The entry in `m_triple_iters` that we will check to know whether it has iterators or not.
                orders_to_iterators_type& triple_iter = m_triple_iters[t_index];//std::cout << "Checking triple #" << t_index << " with variable : " <<(int) x_j <<std::endl;
                if(triple_iter.empty()){//If the map is empty then there aren't any Iterators attached to it.
                    //If empty then assign all x_j iters.
                    triple_info_type& triple_info = triple_xj.second;
                    for(auto& order_to_iterator : triple_info.order_to_iterator){
                        auto& order = order_to_iterator.first;
                        //auto& iterators = order_to_iterator.second;
                        //Adding x_j iterator for triple 'i' with order 'order'.
                        size_type i = triple_xj.first;
                        //std::cout << "Assigning iterator with order "<< order << " to triple " << t_index<< " of variable " << (int) x_j << std::endl;
                        triple_iter[order] = get_var_iterator_by_triple_and_order(x_j, i, order);
                    }
                }else{
                    //If the map has only one element, that means there is a single iterator in the triple.
                    if(triple_iter.size() == 1){
                        //Do nothing.
                        continue;
                        //Two iterators in the triple. Meaning there was one variable processed before (and is the one in position order[0]).
                    } else if(triple_iter.size() == 2){
                        //for(auto& t : triple_iter){//triple_iter is orders_to_iterators_type, and t => pair of order:iterator
                        for (auto it = triple_iter.begin(); it != triple_iter.end();){
                            if(triple_iter.size() <= 1){
                                break;//This condition exist to not delete all the iterators. We need to leave 1.
                            }
                            auto&iter = it->second;
                            const auto& triple_definition = m_ptr_triple_patterns->at(t_index);
                            //iter.order is a vector of length 3.
                            if(iter->order[1] == 0){ //second level of trie is a subject
                                if(triple_definition.s_is_variable() && triple_definition.term_s.value != x_j){
                                    //This iterator must be deleted of our bgp_triples structure.
                                    //Removes specified elements from the container. The order of the remaining elements is preserved. (This makes it possible to erase individual elements while iterating through the container.)
                                    //https://en.cppreference.com/w/cpp/container/unordered_map/erase
                                    triple_iter.erase(it);
                                }else{
                                    ++it;
                                }
                            } else if(iter->order[1] == 1){ //second level of trie is a predicate
                                if(triple_definition.p_is_variable() && triple_definition.term_p.value != x_j){
                                    //This iterator must be deleted of our bgp_triples structure.
                                    triple_iter.erase(it);
                                }else{
                                    ++it;
                                }
                            } else {//second level of trie is an object
                                if(triple_definition.o_is_variable() && triple_definition.term_o.value != x_j){
                                    //This iterator must be deleted of our bgp_triples structure.
                                    triple_iter.erase(it);
                                }else{
                                    ++it;
                                }
                            }
                        }
                    }
                }
            }
        }
        inline void add_var_to_iterator(const var_type var, ltj_iter_type* ptr_iterator){
            auto it =  m_var_to_iterators.find(var);
            if(it != m_var_to_iterators.end()){
                it->second.push_back(ptr_iterator);
            }else{
                std::vector<ltj_iter_type*> vec = {ptr_iterator};
                m_var_to_iterators.insert({var, vec});
            }
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
                var_type new_var = '\0';
                const var_type& cur_var = m_gao_stack.top();
                const std::unordered_map<var_type, bool> & b_vars = m_gao_vars;
                //refresh rel var iterators?
                m_gao_size.update_weights(j, cur_var, b_vars, m_var_to_iterators);
                new_var = m_gao_size.get_next_var(j, m_gao_vars);
                std::cout << "<< manage_iterators " << x_j << std::endl;
                manage_iterators(new_var);
                std::cout << "<< manage_iterators " << x_j << std::endl;
                return new_var;
            }
            return m_gao[j];
        }
        void manage_iterators(var_type new_var){
            //1.assign iters to bgp_triples
            assign_iterators_to_bgp_triples(new_var);
            //2. Copy the iterators to m_var_to_iterators map which is used by LTJ.
            //Todos los triples de new_var: contiene informacion de X_j, sus triples
            //Con todas las permutaciones posibles de iteradores.

            std::cout << ">> copying the iterators " << x_j << std::endl;
            auto& triples_xj = get_triples_info(new_var);
            for(auto& triple_xj : triples_xj){
                size_type t_index = triple_xj.first;//Su indice.
                auto& triple_info = triple_xj.second;//triple_info, contiene related variables.
                //Status de iteradores asignados a triples, usando la estructura BGP triples.
                orders_to_iterators_type& iterators = m_triple_iters[t_index];//Los iteradores del triple.
                const auto& triple_definition = m_ptr_triple_patterns->at(t_index);
                for (auto it = iterators.begin(); it != iterators.end(); ++it){
                    /*
                    Si la variable dueña del iterador no ha sido procesada,
                    entonces no existe su iterador en m_var_to_iterators,
                    ya sea como la siguiente variable (new_var) o como una de sus relacionadas.
                    */
                    if(!m_gao_vars[it->second->owner_var]){
                        m_var_to_iterators[new_var].emplace_back(it->second);
                        for(var_type rel_var :triple_info.related){
                            if(!m_gao_vars[rel_var]){
                                if(it->second->order[it->second->get_depth() + 1] == 0 &&
                                    triple_definition.s_is_variable() &&
                                    triple_definition.term_s.value == rel_var){
                                    //Subject
                                    m_var_to_iterators[rel_var].emplace_back(it->second);
                                } else if(it->second->order[it->second->get_depth() + 1] == 1 &&
                                    triple_definition.p_is_variable() &&
                                    triple_definition.term_p.value == rel_var){
                                    //Predicate
                                    m_var_to_iterators[rel_var].emplace_back(it->second);
                                } else if(it->second->order[it->second->get_depth() + 1] == 2 &&
                                    triple_definition.o_is_variable() &&
                                    triple_definition.term_o.value == rel_var){
                                    //Object
                                    m_var_to_iterators[rel_var].emplace_back(it->second);
                                }
                            }
                        }
                    }
                }
            }
            std::cout << "<< copying the iterators " << x_j << std::endl;
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
            if(timeout_seconds > 0){
                time_point_type stop = std::chrono::high_resolution_clock::now();
                size_type sec = std::chrono::duration_cast<std::chrono::seconds>(stop-start).count();
                if(sec > timeout_seconds) return false;
            }

            //(Optional) Check limit
            if(limit_results > 0 && res.size() == limit_results) return false;

            if(j == m_gao_size.number_of_variables){
                //Report results
                
                std::cout << "tuple : ";
                for(auto& pair : tuple){
                    std::cout << int(pair.first) << " = " << pair.second << std::endl;
                }
                std::cout << " " << std::endl;
                
                res.emplace_back(tuple);
            }else{
                //assert(m_gao_stack.size() == m_gao_vars.size());
                var_type x_j = next(j);
                push_var_to_stack(x_j);
                std::vector<ltj_iter_type*>& itrs = m_var_to_iterators[x_j];
                bool ok;
                /*
                The second case is intented to exclude the down() from happening when there's only a single triple in the BGP. 
                    In this case all its variables are lonely (Some of them are not in the last level though). Therefore we dont need to do an extra down.
                */
               /*
                if(itrs.size() == 1 && !itrs[0]->in_last_level() && m_ptr_triple_patterns->size() > 1){
                    itrs[0]->down(x_j);
                }*/
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
                    std::cout << "Seek (init): (" << (uint64_t) x_j << ": " << c << ")" <<std::endl;

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
                        std::cout << "Seek (bucle): (" << (uint64_t) x_j << ": " << c << ")" <<std::endl;
                    }
                }
                if(index_scheme::util::configuration.is_adaptive()){
                    m_gao_size.set_previous_weight();
                    std::cout << ">> clear_iterators " << x_j << std::endl;
                    clear_iterators(x_j);
                    std::cout << "<< clear_iterators " << x_j << std::endl;
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
