#include "recompression_definitions.hpp"
#include "2DRangeQuery.hpp"

#include <fstream>
#include <memory>
#include <string>
#include <chrono>
// #endregion
struct NonTerminalInfo{
    Fragment leftFragment, rightFragment;
    c_size_t nonterm, minWeight, countWeight;
    NonTerminalInfo(){
    }
};
struct NonTermInfoReverseComparator{
    RecompressionRLSLP* rlslp;
    c_size_t n;
    NonTermInfoReverseComparator(c_size_t n, RecompressionRLSLP* rlslp): rlslp(rlslp), n(n){}
    bool operator()(const NonTerminalInfo& x, const NonTerminalInfo& y) const { 
        c_size_t lstart = n - 1 - (x.leftFragment.index + x.leftFragment.length - 1);
        c_size_t rstart = n - 1 - (y.leftFragment.index + y.leftFragment.length - 1);
        c_size_t lce = rlslp->lce(lstart, rstart);
        if (lce >= x.leftFragment.length || lce >= y.leftFragment.length){
            return x.leftFragment.length < y.leftFragment.length;
        }
        char c1 = rlslp->getCharacter(lstart + lce), c2 = rlslp->getCharacter(rstart + lce);
        return c1 < c2;
    }
};
struct NonTermComparator{
    RecompressionRLSLP* rlslp;
    NonTermComparator(RecompressionRLSLP* rlslp): rlslp(rlslp){}
    bool operator()(const c_size_t& x, const c_size_t& y) const{
        if (x == y) return false;
        if (x == 0){
            return true;
        }
        else if (y == 0){
            return false;
        }
        c_size_t firstX = rlslp->firstNodes[x].l, firstY = rlslp->firstNodes[y].l;
        c_size_t lengthX = rlslp->nonterm[x].explen, lengthY = rlslp->nonterm[y].explen;
        c_size_t lce = rlslp->lce(firstX, firstY);
        if (lce >= lengthX || lce >= lengthY){
            return lengthX < lengthY;
        }
        char c1 = rlslp->getCharacter(firstX + lce), c2 = rlslp->getCharacter(firstY + lce);
        return c1 < c2;
    }
};
void reverseRLSLP(c_size_t node, RecompressionRLSLP* recomp_rlslp, RecompressionRLSLP* rev_rlslp){
    if (rev_rlslp->nonterm[node].first != -1) return;
    const RLSLPNonterm& nt = recomp_rlslp->nonterm[node];
    rev_rlslp->nonterm[node].explen = nt.explen;
    rev_rlslp->nonterm[node].type = nt.type;
    if (nt.type == '0'){
        rev_rlslp->nonterm[node].first = nt.first;
        rev_rlslp->nonterm[node].second = nt.second;
    }
    else if (nt.type == '1'){
        reverseRLSLP(nt.first, recomp_rlslp, rev_rlslp);
        reverseRLSLP(nt.second, recomp_rlslp, rev_rlslp);
        rev_rlslp->nonterm[node].first = nt.second;
        rev_rlslp->nonterm[node].second = nt.first;
    }
    else{
        reverseRLSLP(nt.first, recomp_rlslp, rev_rlslp);
        rev_rlslp->nonterm[node].first = nt.first;
        rev_rlslp->nonterm[node].second = nt.second;
    }
}
packed_pair<c_size_t, c_size_t> getDimensionRange(c_size_t l, c_size_t r, space_efficient_vector<Fragment>& arr, RecompressionRLSLP* rlslp){
    c_size_t lp = 0, rp = arr.size() - 1;
    c_size_t n = rlslp->nonterm.back().explen;
    packed_pair<c_size_t, c_size_t> result;
    while (lp <= rp){
        //find first >= this suffix
        c_size_t mid = (lp + rp) / 2;
        const Fragment& frag = arr[mid];
        if (frag.index == numeric_limits<c_size_t>::max()){
            rp = mid - 1;
            continue;
        }
        c_size_t lce = rlslp->lce(l, frag.index);
        if (lce >= r - l + 1){
            if (frag.length >= r - l + 1){
                rp = mid - 1;
            }
            else{
                lp = mid + 1;
            }
        }
        else if (lce >= frag.length){
            lp = mid + 1;
        }
        else{
            char c1 = rlslp->getCharacter(l + lce), c2 = rlslp->getCharacter(frag.index + lce);
            if (c1 > c2){
                lp = mid + 1;
            }
            else{
                rp = mid - 1;
            }
        }
    }
    result.first = lp;
    rp = arr.size() - 1;
    while (lp <= rp){
        //find last <= suffix
        c_size_t mid = (lp + rp) / 2;
        const Fragment& frag = arr[mid];
        if (frag.index == numeric_limits<c_size_t>::max()){
            rp = mid - 1;
            continue;
        }
        c_size_t lce = rlslp->lce(l, frag.index);
        if (lce >= r - l + 1 && r - l + 1 <= frag.length){
            lp = mid + 1;
        }
        else{
            rp = mid - 1;
        }
    }
    result.second = rp;
    return result;
}
space_efficient_vector<c_size_t> mapToRange(c_size_t start, c_size_t length, c_size_t anchor, RecompressionRLSLP* recomp_rlslp, RecompressionRLSLP* rev_rlslp, RangeQuery& queryStructure){
    space_efficient_vector<c_size_t> resultRange(4);
    packed_pair<c_size_t, c_size_t> xRange = getDimensionRange(recomp_rlslp->nonterm.back().explen - 1 - (start + anchor - 1), recomp_rlslp->nonterm.back().explen - 1 - start, queryStructure.X, rev_rlslp);
    resultRange[0] = xRange.first;
    resultRange[1] = xRange.second;
    packed_pair<c_size_t, c_size_t> yRange = getDimensionRange(start + anchor, start + length - 1, queryStructure.Y, recomp_rlslp);
    resultRange[2] = yRange.first;
    resultRange[3] = yRange.second;
    return resultRange;
}
space_efficient_vector<c_size_t> reportOccurrences(RecompressionRLSLP* recomp_rlslp, RecompressionRLSLP* rev_rlslp, c_size_t index, c_size_t length, RangeQuery& rectangleQuery){
    if (length == 1){
        c_size_t symbol = recomp_rlslp->getSymbol(index);
        space_efficient_vector<Node> nodes = recomp_rlslp->enumerateNodes(symbol);
        space_efficient_vector<c_size_t> left(nodes.size());
        for (c_size_t i = 0; i < nodes.size(); i++){
            left[i] = nodes[i].l;
        }
        return left;
    }
    space_efficient_vector<c_size_t> anchors = recomp_rlslp->getAnchors(index, length);
    space_efficient_vector<c_size_t> results;
    for (c_size_t i = 0; i < anchors.size(); i++){
        c_size_t anchorLength = anchors[i];
        space_efficient_vector<c_size_t> rectangle = mapToRange(index, length, anchorLength, recomp_rlslp, rev_rlslp, rectangleQuery);
        space_efficient_vector<c_size_t> vars = rectangleQuery.rangeReport(rectangle[0], rectangle[1], rectangle[2], rectangle[3]);
        for (c_size_t j = 0; j < vars.size(); j++){
            c_size_t hook = vars[j];
            c_size_t leftChild = recomp_rlslp->nonterm[hook].first;
            c_size_t leftLength = recomp_rlslp->nonterm[leftChild].explen;
            space_efficient_vector<Node> nontermOccs = recomp_rlslp->enumerateNodes(hook);
            char hookType = recomp_rlslp->nonterm[hook].type;
            c_size_t occTimes = hookType == '1' ? 1 : recomp_rlslp->nonterm[hook].second - (length - anchorLength + leftLength - 1) / leftLength;
            for (c_size_t k = 0; k < nontermOccs.size(); k++){
                if (hookType == '1'){
                    results.push_back(nontermOccs[k].l + leftLength - anchorLength);
                }
                else{
                    c_size_t leftPos = nontermOccs[k].l;
                    for (c_size_t l = 0; l < occTimes; l++){
                        results.push_back(leftPos + leftLength - anchorLength);
                        leftPos += leftLength;
                    }
                }
            }
        }
    }
    return results;
}
c_size_t getLeftMostOccurrence(RecompressionRLSLP* recomp_rlslp, RecompressionRLSLP* rev_rlslp, c_size_t index, c_size_t length, RangeQuery& minQuery){
    if (length == 1){
        c_size_t symbol = recomp_rlslp->getSymbol(index);
        return recomp_rlslp->firstNodes[symbol].l;
    }

    space_efficient_vector<c_size_t> anchors = recomp_rlslp->getAnchors(index, length);

    c_size_t result =  index;
    for (c_size_t i = 0; i < anchors.size(); i++){
        c_size_t anchorLength = anchors[i];
        space_efficient_vector<c_size_t> rectangle = mapToRange(index, length, anchorLength, recomp_rlslp, rev_rlslp, minQuery);
        c_size_t minAnchorOcc = minQuery.rangeMinimum(rectangle[0], rectangle[1], rectangle[2], rectangle[3]);
        if (minAnchorOcc != numeric_limits<c_size_t>::max()){
            result = min(result, minAnchorOcc - anchorLength);
        }
    }
    return result;
}
c_size_t countRegularOccurrences(RecompressionRLSLP* recomp_rlslp, RecompressionRLSLP* reverse_rlslp, c_size_t index, c_size_t length, RangeQuery& countQuery){
    if (length == 1){
        c_size_t symbol = recomp_rlslp->getSymbol(index);
        return recomp_rlslp->countNodes[symbol];
    }
    space_efficient_vector<c_size_t> anchors = recomp_rlslp->getAnchors(index, length);
    c_size_t result = 0;
    for (c_size_t i = 0; i < anchors.size(); i++){
        c_size_t anchorLength = anchors[i];
        space_efficient_vector<c_size_t> rectangle = mapToRange(index, length, anchorLength, recomp_rlslp, reverse_rlslp, countQuery);
        c_size_t anchorOccs = countQuery.rangeSum(rectangle[0], rectangle[1], rectangle[2], rectangle[3]);
        result += anchorOccs;
    }
    return result;
}
c_size_t leftMostIPMQuery(c_size_t xl, c_size_t xr, c_size_t yl, c_size_t yr, RecompressionRLSLP* recomp_rlslp, RecompressionRLSLP* reverse_rlslp){
    //left most position of x in y relative to y (1 indexed)
    space_efficient_vector<Node> hooks;
    c_size_t n = recomp_rlslp->nonterm.back().explen;
    c_size_t xLength = xr - xl + 1;
    recomp_rlslp->getPotentialHooks(recomp_rlslp->nonterm.size() - 1, 0, n - 1, yl, yr, xLength, hooks);
    space_efficient_vector<c_size_t> anchors = recomp_rlslp->getAnchors(xl, xLength);
    c_size_t leftPos = numeric_limits<c_size_t>::max();
    for (c_size_t i = 0; i < anchors.size(); i++){
        c_size_t anchorLength = anchors[i];
        for (c_size_t j = 0; j < hooks.size(); j++){
        Node hook = hooks[j];
        const RLSLPNonterm& nt = recomp_rlslp->nonterm[hook.var];
        if (nt.type == '0'){
            leftPos = min(leftPos, hook.l - (yl - 1));
        }
        else if (nt.type == '1'){
            c_size_t leftLength = recomp_rlslp->nonterm[nt.first].explen;
            c_size_t rightLength = recomp_rlslp->nonterm[nt.second].explen;
            c_size_t occStart = hook.l + leftLength - anchorLength;
            if (occStart >= yl){
                c_size_t leftLCE = min(min(leftLength, anchorLength), reverse_rlslp->lce(n - 1 - (xl + anchorLength - 1), n - 1 - (hook.l + leftLength - 1)));
                c_size_t rightLCE = min(min(rightLength, xLength - anchorLength), recomp_rlslp->lce(xl + anchorLength, hook.l + leftLength));
                if (leftLCE >= anchorLength && rightLCE >= xLength - anchorLength){
                    leftPos = min(leftPos, hook.l + leftLength - anchorLength - (yl - 1));
                }
            }
        }
        else{
            c_size_t childLength = recomp_rlslp->nonterm[nt.first].explen;
            c_size_t k = nt.second;
            c_size_t minI = 1;
            if (hook.l + childLength < yl + anchorLength) {
                c_size_t need = yl + anchorLength - hook.l;
                minI = (need + childLength - 1) / childLength;
            }
            if (minI < k) {
                c_size_t pos = hook.l + minI * childLength;
                c_size_t occStart = pos - anchorLength;
                if (occStart >= yl){
                    c_size_t leftLCE = min(min(anchorLength, childLength), reverse_rlslp->lce(n - 1 - (xl + anchorLength - 1), n -1 - (pos - 1)));
                    c_size_t rightLCE = min(min(childLength * (k - minI), xLength - anchorLength), recomp_rlslp->lce(xl + anchorLength, pos));
                    if (leftLCE >= anchorLength && rightLCE >= xLength - anchorLength){
                        leftPos = min(leftPos, pos - anchorLength - (yl - 1));
                    }
                }
            }
        }
        }
    }
    return leftPos;
}
c_size_t periodQuery(c_size_t l, c_size_t r, RecompressionRLSLP* recomp_rlslp, RecompressionRLSLP* reverse_rlslp){
    c_size_t length = r - l + 1;
    c_size_t half = (length + 1) / 2;
    c_size_t leftOcc = leftMostIPMQuery(l, l + half - 1, l + 1, r, recomp_rlslp, reverse_rlslp);
    if (leftOcc == numeric_limits<c_size_t>::max()) return -1; // no period found
    c_size_t lce = recomp_rlslp->lce(l, l + leftOcc);
    if (lce >= length - leftOcc) return leftOcc;
    return -1;
}
c_size_t countSpecialOccurrences(RecompressionRLSLP* recomp_rlslp, RecompressionRLSLP* reverse_rlslp, c_size_t index, c_size_t length, space_efficient_vector<c_size_t>& sortedNonterms, RangeQuery& countQuery){
    c_size_t period = periodQuery(index, index + length - 1, recomp_rlslp, reverse_rlslp);
    if (period == -1) return 0;
    space_efficient_vector<c_size_t> anchors = recomp_rlslp->getAnchors(index, length);
    c_size_t result = 0;
    for (c_size_t i = 0; i < anchors.size(); i++){
        c_size_t anchorLength = anchors[i];
        if (anchorLength >= 1 && anchorLength < length && anchorLength <= period && period * 2 < length - anchorLength){
            c_size_t lp = 1, rp = sortedNonterms.size() - 1;
            c_size_t firstSymbol = -1;
            c_size_t l = index + anchorLength;
            c_size_t r = l + period - 1;
            c_size_t segLength = r - l + 1;
            while (lp <= rp){
                c_size_t mid = (lp + rp) / 2;
                c_size_t termLength = recomp_rlslp->nonterm[sortedNonterms[mid]].explen;
                c_size_t lce = recomp_rlslp->lce(l, recomp_rlslp->firstNodes[sortedNonterms[mid]].l);
                if (lce >= termLength && lce >= segLength){
                    if (termLength == segLength){
                        firstSymbol = sortedNonterms[mid];
                        break;
                    }
                    else if (termLength > segLength){
                        rp = mid - 1;
                    }
                    else{
                        lp = mid + 1;
                    }
                }
                else if (lce >= segLength){
                    rp = mid - 1;
                }
                else if (lce >= termLength){
                    lp = mid + 1;
                }
                else{
                    char c1 = recomp_rlslp->getCharacter(l + lce), c2 = recomp_rlslp->getCharacter(recomp_rlslp->firstNodes[sortedNonterms[mid]].l + lce);
                    if (c1 > c2){
                        lp = mid + 1;
                    }
                    else{
                        rp = mid - 1;
                    }
                }
            }
            if (firstSymbol != -1){
                result += recomp_rlslp->queryCount(firstSymbol, (length - anchorLength + period - 1) / period);
            }
        }
    }
    return result;
}
c_size_t countOccurrences(RecompressionRLSLP* recomp_rlslp, RecompressionRLSLP* reverse_rlslp, c_size_t index, c_size_t length, space_efficient_vector<c_size_t>& sortedNonterms, RangeQuery& countQuery){
    return countRegularOccurrences(recomp_rlslp, reverse_rlslp, index, length, countQuery) + countSpecialOccurrences(recomp_rlslp, reverse_rlslp, index, length, sortedNonterms, countQuery);
}
int main(int argc, char *argv[]){
    if (argc <= 1){
        cerr << "Usage: " + string(argv[0])
            << "-i recompression_rlslp_file -qf query_file [-o output_file]" << endl;
        exit(1);
    }
    //Type 1 query reports all occurrences of the substring starting at index with the given length in T: 1 index length 
    //Type 2 query returns the leftmost occurrence of the substring starting at index with the given length in T: 2 index length
    //Type 3 query returns the count of the substring starting at index with the given length in T: 3 index length
    //By default, answers to queries appear in stdout unless an output file is provided
    string input_file, query_file, output_file;
    for (len_t i = 1; i < argc; i++){
        string curr_arg(argv[i]);
        if (curr_arg == "-i"){
            input_file = argv[++i];
        }
        else if (curr_arg == "-qf"){
            query_file = argv[++i];
        }
        else if (curr_arg == "-o"){
            output_file = argv[++i];
        }
    }
    if (input_file.empty() || query_file.empty()){
        cerr << "Usage: " + string(argv[0])
            << "-i recompression_rlslp_file -qf query_file [-o output_file]" << endl;
        exit(1);
    }
    auto recomp_rlslp = make_unique<RecompressionRLSLP>();
    if (!recomp_rlslp->read_from_file(input_file)) exit(1);
    recomp_rlslp->init(recomp_rlslp->nonterm.size());
    auto reverse_rlslp = make_unique<RecompressionRLSLP>();
    recomp_rlslp->initStructures();
    reverse_rlslp->nonterm.resize(recomp_rlslp->nonterm.size(), RLSLPNonterm('0', -1, -1));
    reverse_rlslp->init(recomp_rlslp->nonterm.size());
    reverseRLSLP(recomp_rlslp->nonterm.size() - 1, recomp_rlslp.get(), reverse_rlslp.get());
    reverse_rlslp->initStructures();
    recomp_rlslp->constructTrees();
    c_size_t nonterms = recomp_rlslp->nonterm.size();
    space_efficient_vector<NonTerminalInfo> arr;
    space_efficient_vector<c_size_t> sortedNonterms(nonterms);
    for (c_size_t i = 0; i < nonterms; i++){
        const RLSLPNonterm& nt = recomp_rlslp->nonterm[i];
        sortedNonterms[i] = i;
        if (nt.type != '0'){
            arr.push_back(NonTerminalInfo());
            const Node& firstNode = recomp_rlslp->firstNodes[i];
            c_size_t leftLength = recomp_rlslp->nonterm[nt.first].explen;
            Fragment leftFragment(firstNode.l, leftLength);
            arr.back().leftFragment = leftFragment;
            if (nt.type == '1'){
                Fragment rightFragment(firstNode.l + leftLength, recomp_rlslp->nonterm[nt.second].explen);
                arr.back().rightFragment = rightFragment;
            }
            else{
                c_size_t k = nt.second;
                Fragment rightFragment(firstNode.l, leftLength * (k - 1));
                arr.back().rightFragment = rightFragment;
            }
            arr.back().nonterm = i;
            arr.back().minWeight = firstNode.l + leftLength;
        }
    }
    NonTermComparator sortComp(recomp_rlslp.get());
    sortedNonterms.sort(sortComp);
    NonTermInfoReverseComparator ntcomp(recomp_rlslp->nonterm.back().explen, reverse_rlslp.get());
    arr.sort(ntcomp);
    c_size_t nonterm_sz = arr.size();
    space_efficient_vector<Fragment> X(nonterm_sz), Y(nonterm_sz);
    space_efficient_vector<c_size_t> nontermIDs(nonterm_sz);
    space_efficient_vector<c_size_t> minWeights(nonterm_sz);
    for (c_size_t i = 0; i < nonterm_sz; i++){
        X[i] = arr[i].leftFragment;
        Y[i] = arr[i].rightFragment;
        nontermIDs[i] = arr[i].nonterm;
        minWeights[i] = arr[i].minWeight;
    }
    space_efficient_vector<NonTerminalInfo> countArr;
    for (c_size_t i = 0; i < nonterms; i++){
        const RLSLPNonterm& nt = recomp_rlslp->nonterm[i];
        if (nt.type != '0'){
            countArr.push_back(NonTerminalInfo());
            const Node& firstNode = recomp_rlslp->firstNodes[i];
            c_size_t leftLength = recomp_rlslp->nonterm[nt.first].explen;
            Fragment leftFragment(firstNode.l, leftLength);
            countArr.back().leftFragment = leftFragment;
            if (nt.type == '1'){
                Fragment rightFragment(firstNode.l + leftLength, recomp_rlslp->nonterm[nt.second].explen);
                countArr.back().rightFragment = rightFragment;
                countArr.back().countWeight = recomp_rlslp->countNodes[i];
            }
            else{
                c_size_t k = nt.second;
                Fragment rightFragment(firstNode.l, leftLength);
                countArr.back().rightFragment = rightFragment; //use normal constructor instead of doing this later
                countArr.back().countWeight = recomp_rlslp->countNodes[i];
                countArr.push_back(NonTerminalInfo());
                countArr.back().leftFragment = Fragment(firstNode.l, leftLength);
                countArr.back().rightFragment = Fragment(firstNode.l, leftLength * 2);
                countArr.back().countWeight = (k - 2) * recomp_rlslp->countNodes[i];
            }
        }
    }
    countArr.sort(ntcomp);
    c_size_t count_sz = countArr.size();
    space_efficient_vector<Fragment> countX(count_sz), countY(count_sz);
    space_efficient_vector<c_size_t> countWeights(count_sz);
    for (c_size_t i = 0; i < count_sz; i++){
        countX[i] = countArr[i].leftFragment;
        countY[i] = countArr[i].rightFragment;
        countWeights[i] = countArr[i].countWeight;
    }
    RangeQuery rectangleQuery(recomp_rlslp.get(), X, Y, nontermIDs);
    RangeQuery minQuery(recomp_rlslp.get(), X, Y, minWeights);
    RangeQuery countQuery(recomp_rlslp.get(), countX, countY, countWeights);
    c_size_t sz = recomp_rlslp->nonterm.back().explen;
    for (c_size_t i = 0; i < rectangleQuery.X.size(); i++){
        rectangleQuery.X[i].index = sz - 1 - (rectangleQuery.X[i].index + rectangleQuery.X[i].length - 1);
        minQuery.X[i].index = sz - 1 - (minQuery.X[i].index + minQuery.X[i].length - 1);
    }
    for (c_size_t i = 0; i < countX.size(); i++){
        countQuery.X[i].index = sz - 1 - (countQuery.X[i].index + countQuery.X[i].length - 1);
    }
    ifstream ifs(query_file, ios::binary);
    if (!ifs) {
      cerr << "Error opening query file for reading: " << query_file << endl;
      exit(1);
    }
    ofstream ofs;
    ostream* out = &cout;
    if (!output_file.empty()){
        ofs.open(output_file);
        if (!ofs){
            cerr << "Error opening output file: " << output_file << endl;
            exit(1);
        }
        out = &ofs;
    }

    c_size_t type, index, length;
    while (ifs >> type >> index >> length){
        if (type == 1){
            space_efficient_vector<c_size_t> results = reportOccurrences(recomp_rlslp.get(), reverse_rlslp.get(), index, length, rectangleQuery);
            for (c_size_t i = 0; i < results.size(); i++){
                (*out) << results[i] << " ";
            }
            (*out) << '\n';
        }
        else if (type == 2){
            c_size_t mnOcc = getLeftMostOccurrence(recomp_rlslp.get(), reverse_rlslp.get(), index, length, minQuery);
            (*out) << mnOcc << '\n';
        }
        else if (type == 3){
            c_size_t countOccs = countOccurrences(recomp_rlslp.get(), reverse_rlslp.get(), index, length, sortedNonterms, countQuery);
            (*out) << countOccs << '\n';
        }
    }
    return 0;
}