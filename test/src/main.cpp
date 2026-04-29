#include <bits/stdc++.h>
#include "suffix_array.hpp"
#include "packed_triple.hpp"
#include "CountQueryComponent.hpp"
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <string>
using namespace std;
string generateRandomString(c_size_t n){
    //returns a random string of n lowercase English letters
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<c_size_t> dist(0, 25);
    string st;
    for (c_size_t i = 0; i < n; i++){
        st.push_back('a' + dist(gen));
    }
    return st;
}
string generateFibonacciString(c_size_t n, string f0, string f1){
    //returns a Fibonacci string of length at least n
    //F_0 = f0, F_1 = f1, F_i = F_(i - 1) + F_(i - 2)
    if (f0.size() >= n) return f0;
    if (f1.size() >= n) return f1;
    while (f1.size() < n){
        string f2 = f1 + f0;
        f0 = f1;
        f1 = f2;
    }
    return f1;
}
string generateThueMorseString(c_size_t n){
    //returns a Thue Morse String of length at least n
    //T_0 = "0", T_i = T_(i - 1) + FLIP(T_(i - 1))
    string t;
    for (c_size_t i = 0; i < n || __builtin_popcountll(i) != 1; i++){
        t.push_back('0' + (__builtin_popcountll(i) & 1));
    }
    return t;
}
string generateRandomMorphString(c_size_t n, c_size_t initialLength = 5){
    //returns a random string of length at least n lowercase English letters through randomly generated morphisms of length at most 5
    space_efficient_vector<string> morphMap(26);
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<c_size_t> dist(2, 5);
    for (c_size_t i = 0; i < 26; i++){
        morphMap[i] = generateRandomString(dist(gen));
    }
    string curr = generateRandomString(initialLength);
    while (curr.size() < n){
        string nxt;
        for (char c: curr) nxt += morphMap[c - 'a'];
        curr = nxt;
    }
    return curr;
}
string generateRandomLZ77String(c_size_t n){
    //returns a random string of length at least n formed from a random LZ77-like factorization
    string s;
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<c_size_t> dist(0, 1), charDist(0, 25);
    while (s.size() < n){
        bool literal = s.empty() || (dist(gen) == 1);
        if (literal){
            c_size_t c = charDist(gen);
            s.push_back('a' + c);
        }
        else{
            uniform_int_distribution<c_size_t> indexDist(0, s.size() - 1);
            c_size_t idx = indexDist(gen);
            c_size_t remLength = s.size() - idx;
            uniform_int_distribution<c_size_t> lengthDist(1, remLength);
            c_size_t length = lengthDist(gen);
            s += s.substr(idx, length);
        }
    }
    return s;
}
void generateAndWriteRandomQueries(c_size_t n, c_size_t q, space_efficient_vector<packed_triple<c_size_t, c_size_t, c_size_t>>& queries, string query_path){
    //Generates q random queries for a length n string
    //writes all queries to the given query path
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<c_size_t> idxDist(0, n - 1);
    queries.resize(q);
    for (c_size_t i = 0; i < q / 3; i++){
        //report occurrences queries
        c_size_t idx = idxDist(gen);
        uniform_int_distribution<c_size_t> lengthDist(1, n - idx);
        c_size_t length = lengthDist(gen);
        queries[i] = packed_triple<c_size_t, c_size_t, c_size_t> (1, idx, length);
    }
    for (c_size_t i = q / 3; i < 2 * (q / 3); i++){
        //leftmost occurrence queries
        c_size_t idx = idxDist(gen);
        uniform_int_distribution<c_size_t> lengthDist(1, n - idx);
        c_size_t length = lengthDist(gen);
        queries[i] = packed_triple<c_size_t, c_size_t, c_size_t>(2, idx, length);
    }
    for (c_size_t i = 2 * (q / 3); i < q; i++){
        //count queries
        c_size_t idx = idxDist(gen);
        uniform_int_distribution<c_size_t> lengthDist(1, n - idx);
        c_size_t length = lengthDist(gen);
        queries[i] = packed_triple<c_size_t, c_size_t, c_size_t>(3, idx, length);
    }
    ofstream out(query_path, ios::trunc);
    for (c_size_t i = 0; i < q; i++){
        out << queries[i].first << " " << queries[i].second << " " << queries[i].third << '\n';
    }
}
void test_countComp(c_size_t n, SuffixArray& sa, RecompressionRLSLP* rlslp, RecompressionRLSLP* rev_rlslp, CountQueryComponent& countComp, c_size_t num_queries = 5000){
    cout << "Testing Count Component Period & Leftmost IPM Query functions " << '\n';
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<c_size_t> idxDist(0, n - 1);
    for (c_size_t i = 0; i < num_queries; i++){
        c_size_t l = idxDist(gen);
        uniform_int_distribution<c_size_t> rDist(l, n - 1);
        c_size_t r = rDist(gen);
        c_size_t countPer = countComp.periodQuery(l, r, rlslp, rev_rlslp);
        c_size_t saPer = sa.periodQuery(l, r);
        if (countPer != saPer){
            cerr << "Period Mismatch for " << l << " " << r << '\n';
            cerr << "Expected " << saPer << " Got " << countPer << '\n';
            exit(1);
        }
        c_size_t xLength = r - l + 1;
        c_size_t maxYLength = min(n, 2 * xLength - 1);
        uniform_int_distribution<c_size_t> yLengthDist(1, maxYLength);
        c_size_t yLength = yLengthDist(gen);
        uniform_int_distribution<c_size_t> l1Dist(0, n - yLength);
        c_size_t l1 = l1Dist(gen);
        c_size_t r1 = l1 + yLength - 1;
        c_size_t countLeftMost = countComp.leftMostIPMQuery(l, r, l1, r1, rlslp, rev_rlslp);
        c_size_t saLeftMost = sa.leftMostIPMQuery(l, r, l1, r1);
        if (countLeftMost != saLeftMost){
            cerr << "Leftmost IPM query Mismatch for " << l << " " << r << " " << l1 << " " << r1 << '\n';
            cerr << "Expected " << saLeftMost << " Got " << countLeftMost << '\n';
            exit(1);
        }
    }
    cout << "Passed Count Component Period & Leftmost IPM Query tests" << '\n';
}
void test_string(string& s, c_size_t queryNum, string test_type, bool test_count = false, string query_path = "queries.txt", string input_path = "input"){
    c_size_t n = s.size();
    string output_path = "output.txt";
    ofstream out(input_path, ios::trunc);
    out << s << '\n';
    out.close();
    SuffixArray sa(s);
    space_efficient_vector<packed_triple<c_size_t, c_size_t, c_size_t>> queries;
    generateAndWriteRandomQueries(n, queryNum, queries, query_path);
    string cmd = "./src/run_rlslp.sh " + input_path + " " + query_path + " " + output_path;
    system(cmd.c_str());
    if (test_count){
        RecompressionRLSLP rlslp;
        rlslp.read_from_file(input_path + ".rlslp");
        rlslp.initStructures();
        rlslp.constructTrees();
        RecompressionRLSLP rev_rlslp;
        c_size_t nonterms = rlslp.nonterm.size();
        rev_rlslp.nonterm.resize(nonterms, RLSLPNonterm('0', -1, -1));
        rlslp.reverseRLSLP(rlslp.nonterm.size() - 1, &rev_rlslp);
        CountQueryComponent countComp(rlslp.nonterm);
        test_countComp(s.size(), sa, &rlslp, &rev_rlslp, countComp);
    }
    ifstream ifs(query_path, ios::binary), output_file(output_path, ios::binary);
    c_size_t type, index, length;
    cout << "Testing Main Functions" << '\n';
    while (ifs >> type >> index >> length){
        bool fail = false;
        if (type == 1){
            space_efficient_vector<c_size_t> occs;
            sa.reportOccurrences(index, length, occs);
            occs.sort();
            string line;
            getline(output_file >> ws, line);
            stringstream ss(line);
            space_efficient_vector<c_size_t> rlslp_occs;
            c_size_t x;
            while (ss >> x){
                rlslp_occs.push_back(x);
            }
            rlslp_occs.sort();
            if (rlslp_occs.size() != occs.size()){
                fail = true;
                cerr << "Report Occurrence size Mismatch" << '\n';
            }
            else{
                for (c_size_t i = 0; i < rlslp_occs.size(); i++){
                    if (rlslp_occs[i] != occs[i]){
                        fail = true;
                    }
                }
            }
        }
        else if (type == 2){
            c_size_t leftMostOcc = sa.getLeftMostOccurrence(index, length);
            c_size_t rlslpLeftMost;
            output_file >> rlslpLeftMost;
            if (rlslpLeftMost != leftMostOcc){
                cerr << "Expected " << leftMostOcc << " Got " << rlslpLeftMost << '\n';
                fail = true;
            }
        }
        else if (type == 3){
            c_size_t countOccs = sa.countOccurrences(index, length);
            c_size_t rlslpCount;
            output_file >> rlslpCount;
            if (rlslpCount != countOccs){
                cerr << "Expected " << countOccs << " Got " << rlslpCount << '\n';
                fail = true;
            }
        }
        if (fail){
            cerr << "Failed on " << test_type << ": " << "Mismatch for query " << type << " " << index << " " << length << '\n';
            exit(1);
        }
    }
    cout << "Passed Main Function tests" << '\n';
}
int main() {
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<c_size_t> smallDist(50, 500);
    string test_type = "Small Random";
    c_size_t test_num = 1;
    cout << "Beginning " << test_type << " test(s)" << '\n';
    for (c_size_t i = 0; i < 5; i++, test_num++){
        // 100 small random strings of length between 50 and 500
        c_size_t n = smallDist(gen);
        string s = generateRandomString(n);
        test_string(s, 2500, test_type, true);
        cout << "Test #" << test_num << ": Passed" << '\n';
    }
    test_type = "Fibonacci";
    cout << "Beginning " << test_type << " test(s)" << '\n';
    uniform_int_distribution<c_size_t> dist(1000, 500000);
    uniform_int_distribution<c_size_t> baseLength(1, 7);
    for (c_size_t i = 0; i < 5; i++, test_num++){
        c_size_t n = dist(gen);
        string s = generateFibonacciString(n, generateRandomString(baseLength(gen)), generateRandomString(baseLength(gen)));
        test_string(s, 1000000, test_type, true);
        cout << "Test #" << test_num << ": Passed" << '\n';
    }
    test_type = "Thue Morse";
    cout << "Beginning " << test_type << " test(s)" << '\n';
    for (c_size_t i = 0; i < 1; i++, test_num++){
        c_size_t n = 1000000;
        string s = generateThueMorseString(n);
        test_string(s, 1000000, test_type, true);
        cout << "Test #" << test_num << ": Passed" << '\n';
    }
    test_type = "Morphisms";
    cout << "Beginning " << test_type << " test(s)" << '\n';
    for (c_size_t i = 0; i < 5; i++, test_num++){
        c_size_t n = dist(gen);
        c_size_t base = baseLength(gen);
        string s = generateRandomMorphString(n, base);
        test_string(s, 1000000, test_type, true);
        cout << "Test #" << test_num << ": Passed" << '\n';
    }
    test_type = "LZ77";
    cout << "Beginning " << test_type << " test(s)" << '\n';
    for (c_size_t i = 0; i < 5; i++, test_num++){
        c_size_t n = dist(gen);
        string s = generateRandomLZ77String(n);
        test_string(s, 1000000, test_type, true);
        cout << "Test #" << test_num << ": Passed" << '\n';
    }
    return 0;
}
