/*#pragma once
#include <unordered_map>
#include <set>
#include "World.hpp"
namespace lumir{
    struct Solver{
        std::unordered_map<int, int> island_to_continent;

        int find(int x, std::unordered_map<int, int> &island_to_continent)
        {
            while (island_to_continent[x] != x)
                x = island_to_continent[x];

            return x;
        }
        void fill(auto ids){
            for (int i : ids){
                island_to_continent[i] = i;
            }
        }
        void pair(World::IDsDist ids_dis){

            if (ids_dis.dist >= 10.f)
                return;

            int a = ids_dis.IDs.x;
            int b = ids_dis.IDs.y;

            a = find(a, island_to_continent);
            b = find(b, island_to_continent);

            if (a != b)
                island_to_continent[b] = a;
        }
        auto get_continent_map(){
            std::unordered_map<int,std::set<int>> continents;
            for (auto [key, value] : island_to_continent){
                continents[value].insert(key);
            }
            return continents;
        }
    };
}
*/
#pragma once
#include <unordered_map>
#include <set>
#include "World.hpp"
#include "Continent.hpp"

namespace lumir {

struct Solver {
    std::unordered_map<int, int> m_island_to_continent;

    // Initialize islands
    void fill(auto ids) {
        for (int i : ids) {
            m_island_to_continent[i] = i;
        }
    }

    int find(int i) {
      
        // If i itself is root or representative
        if (m_island_to_continent[i] == i) {
            return i;
        }
      
        // Else recursively find the representative 
        // of the parent
        return find(m_island_to_continent[i]);
    }

    // Merge two islands if close enough
    void pair(const World::IDsDist& ids_dis) {
        if (ids_dis.dist >= 10.f)
            return;

        int a_rep = find(ids_dis.IDs.x);
        int b_rep = find(ids_dis.IDs.y);

        m_island_to_continent[a_rep] = b_rep;
    }

    // Build final continent map
        auto get_continent_map(){
            std::unordered_map<int,Continent> continents;
            for (auto [key, value] : m_island_to_continent){
                int value_rep = find(value);
                continents[value_rep].m_island_ids.insert(key);
            }
            return continents;
        }
        auto get_island_to_continent()
        {
            std::unordered_map<int, int> island_to_continent;

            for (auto [island, continent] : m_island_to_continent)
            {
                island_to_continent[island] = find(continent);
            }

            return island_to_continent;
        }
    };
}