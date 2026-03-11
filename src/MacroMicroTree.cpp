#include "MacroMicroTree.hpp"

#include <algorithm>
#include <unordered_map>
#include <utils/space_efficient_vector.hpp>
#include <typedefs.hpp>
using namespace std;

MacroMicroTree::MacroMicroTree(c_size_t n, space_efficient_vector<space_efficient_vector<c_size_t>>& g): n(n), g(g){
    par.resize(n, -1);
    depths.resize(n);
    height.resize(n);
    bct.resize(n);
    jumpNodes.resize(n, -1);
    ladderIdx.resize(n, -1);
    levelMask.resize(n);
    ladderPos.resize(n, -1);
    dfsOrd.resize(n, -1);
    dfsPos.resize(n, -1);
    shapes.resize(n, -1);
    bool pw2 = (n & (n - 1)) == 0;
    c_size_t lg = 31 - __builtin_clz(n);
    micro_threshold = (lg + pw2 + 3) / 4;
    for (c_size_t i = 0; i < n; i++){
        if (par[i] == -1){
            init_dfs(i, -1);
            if (jumpNodes[i] == -1){
                jumpNodes[i] = i;
            }
            constructLadders(i);
        }
    }
    jumpPointers.resize(n);
    unordered_map<c_size_t, c_size_t>  shapeSeen;
    c_size_t shapeIdx = 0;
    for (c_size_t i = 0; i < n; i++){
        if (jumpNodes[i] == i){
            if (depths[i]){
                c_size_t length = 32 - __builtin_clz(depths[i]);
                jumpPointers[i].resize(length);
                jumpPointers[i][0] = par[i];
                c_size_t currLadder = ladderIdx[par[i]];
                c_size_t posInLadder = ladderPos[par[i]];
                for (c_size_t j = 1; j < length; j++){
                    c_size_t need = c_size_t(1) << (j - 1);
                    if (posInLadder + need >= ladders[currLadder].size()){
                        need -= ladders[currLadder].size() - posInLadder;
                        c_size_t nxtNode = par[ladders[currLadder].back()];
                        currLadder = ladderIdx[nxtNode];
                        posInLadder = ladderPos[nxtNode];
                    }
                    posInLadder += need;
                    jumpPointers[i][j] = ladders[currLadder][posInLadder];
                }
            }
        }
        for (c_size_t x = 0; x < g[i].size(); x++){
            c_size_t neighbor = g[i][x];
            if (neighbor != par[i] && jumpNodes[neighbor] == -1){
                c_size_t mask = 0;
                c_size_t maskIdx = 0, dfsIndex = 0;
                space_efficient_vector<c_size_t> currPath;
                getShape(neighbor, i, mask, maskIdx, dfsIndex, currPath);
                mask |= c_size_t(1) << maskIdx;
                dfsPath.push_back(currPath);
                c_size_t shapepos = -1;
                if (shapeSeen.find(mask) == shapeSeen.end()){
                    shapepos = shapeIdx++;
                    space_efficient_vector<space_efficient_vector<c_size_t>> table(currPath.size(), space_efficient_vector<c_size_t> (currPath.size()));
                    maskIdx = 0;
                    c_size_t currNode = 0;
                    constructTable(currNode, -1, mask, maskIdx, 0, table);
                    shapeSeen[mask] = shapepos;
                    shapeTables.push_back(table);
                }
                else{
                    shapepos = shapeSeen[mask];
                }
                for (c_size_t j = 0; j < currPath.size(); j++){
                    shapes[currPath[j]] = shapepos;
                }
            }
        }
    }
    
}
c_size_t MacroMicroTree::init_dfs(c_size_t node, c_size_t parent){
    par[node] = parent;
    c_size_t tot_sz = 1;
    for (c_size_t x = 0; x < g[node].size(); x++){
        c_size_t neighbor = g[node][x];
        if (neighbor != parent){
            depths[neighbor] = depths[node] + 1;
            tot_sz += init_dfs(neighbor, node);
            if (jumpNodes[neighbor] != -1) jumpNodes[node] = jumpNodes[neighbor];
            height[node] = max(height[node], 1 + height[neighbor]);
        }
    }
    if (jumpNodes[node] == -1 && tot_sz >= micro_threshold) jumpNodes[node] = node;
    return tot_sz;
}
void MacroMicroTree::constructLadders(c_size_t node){
    space_efficient_vector<c_size_t> ladder;
    c_size_t idx = ladders.size();
    c_size_t currNode = node;
    for (c_size_t j = 0; j <= height[node]; j++){
        ladder.push_back(currNode);
        ladderIdx[currNode] = idx;
        for (c_size_t x = 0; x < g[currNode].size(); x++){
            c_size_t neighbor = g[currNode][x];
            if (height[neighbor] == height[currNode] - 1){
                currNode = neighbor;
                break;
            }
        }
    }
    ladder.reverse();
    c_size_t upNode = par[node];
    for (c_size_t j = 0; j <= height[node] && upNode != -1; j++){
        ladder.push_back(upNode);
        upNode = par[upNode];
    }
    ladders.push_back(ladder);
    for (c_size_t j = height[node]; j >= 0; j--){
        ladderPos[ladder[j]] = j;
    }
    for (c_size_t j = height[node]; j >= 0; j--){
        for (c_size_t x = 0; x < g[ladder[j]].size(); x++){
            c_size_t neighbor = g[ladder[j]][x];
            if (ladderIdx[neighbor] == -1){
                constructLadders(neighbor);
            }
        }
    }
}
void MacroMicroTree::getShape(c_size_t node, c_size_t nearJump, c_size_t& mask, c_size_t& index, c_size_t& dfsIdx, space_efficient_vector<c_size_t>& ord){
    ord.push_back(node);
    dfsOrd[node] = dfsIdx++;
    dfsPos[node] = dfsPath.size();
    jumpNodes[node] = nearJump;
    for (c_size_t x = 0; x < g[node].size(); x++){
        c_size_t neighbor = g[node][x];
        if (neighbor != par[node]){
            index++;
            getShape(neighbor, nearJump, mask, index, dfsIdx, ord);
            mask |= c_size_t(1) << index;
            index++;
        }
    }
}
void MacroMicroTree::constructTable(c_size_t& node, c_size_t par, c_size_t mask, c_size_t& index, c_size_t depth, space_efficient_vector<space_efficient_vector<c_size_t>>& table){
    if (par != -1){
        table[node][depth] = node;
        for (c_size_t i = depth - 1; i >= 0; i--){
            table[node][i] = table[par][i];
        }
    }
    c_size_t curr = node;
    while ((mask & (c_size_t(1) << index)) == 0){
        node++;
        index++;
        constructTable(node, curr, mask, index, depth + 1, table);
        index++;
    }
}
c_size_t MacroMicroTree::level_ancestor(c_size_t node, c_size_t depth){
    if (depth < 0 || depth > depths[node]) return -1;
    if (depths[node] == depth) return node;
    if (depths[jumpNodes[node]] < depths[node] && depth > depths[jumpNodes[node]]){
        return dfsPath[dfsPos[node]][shapeTables[shapes[node]][dfsOrd[node]][depth - depths[jumpNodes[node]] - 1]];
    }
    if (jumpNodes[node] == node){
        c_size_t diff = depths[node] - depth;
        c_size_t lg = 31 - __builtin_clz(diff);
        diff -= (1 << lg);
        c_size_t jumped = jumpPointers[node][lg];
        return ladders[ladderIdx[jumped]][ladderPos[jumped] + diff];
    }
    else{
        return level_ancestor(jumpNodes[node], depth);
    }
}