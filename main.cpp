#include <iostream>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <math.h>
//#include <map>
#include <boost/functional/hash.hpp>
#include <unordered_map>

using namespace std;
using namespace boost;

template <typename Container>
struct container_hash {
  std::size_t operator()(Container const& c) const {
    return boost::hash_range(c.begin(), c.end());
  }
};

vector<vector<int>> data;
vector<int> labels;

//map<vector<int>, vector<double>> rules_to_probs;
std::unordered_map<vector<int>, vector<double>, container_hash<vector<int>>> rules_to_probs;
int min_num_rules = 30;

void loadData(char* filename) {
  cout << "Loading Data..." << endl;
  ifstream in(filename);
  typedef tokenizer< escaped_list_separator<char> > Tokenizer;

  string line;

  while (getline(in,line)) {

    Tokenizer tok(line);
    vector<string> vec;
    vec.assign(tok.begin(),tok.end());

    labels.push_back(stoi(vec[0]));
    vec.erase(vec.begin());
    vector<int> img;
    for (auto num : vec) {
      img.push_back(stoi(num));
    }
    data.push_back(img);
    
  }
  cout <<"Finish Loading Data..." << endl;
}
//p == 1
//updates rules_to_prob with one img
void extractRules(const vector<int>& data, int label) {
  //cout <<"Extracting Rules..." << endl;
  for (int i = 0; i < data.size(); ++i) {
    vector<int> rule{i, data[i]};
    auto iter = rules_to_probs.find(rule);
    if (iter == rules_to_probs.end()) {
      vector<double> probs(10, 0);
      probs[label] += 1;
      rules_to_probs[rule] = probs;
    } else {
      auto probs = iter->second;
      probs[label] += 1;
      iter->second = probs; //update count
    }
  }
}

//p == 2
void extractRules2(const vector<int>& data, int label) {
  for (int i = 0; i < data.size() - 1; ++i) { //go over all the combinations of each two pixels
    for (int j = i + 1; j < data.size(); ++j) {
      vector<int> rule{i, data[i], j, data[j]}; //rules will be a vector of position followed by value
      auto iter = rules_to_probs.find(rule);
      if (iter == rules_to_probs.end()) {
        vector<double> probs(10, 0);
        probs[label] += 1;
        rules_to_probs[rule] = probs;
      } else {
        auto probs = iter->second;
        probs[label] += 1;
        iter->second = probs; //update count
      }
    }
  }
}

// remove rules that the sum of counts is smaller 30;
void filterRules() {
  cout << "Filtering Rules..." << endl;
  vector<vector<int>> keys;
  for (auto iter = rules_to_probs.begin(); iter != rules_to_probs.end(); ++iter) {
    auto probs= iter->second;
    int sum = accumulate(probs.begin(), probs.end(), 0);
    if (sum < min_num_rules) keys.push_back(iter->first);
  }
  int num_erased = 0;
  for (auto key: keys) {
    rules_to_probs.erase(rules_to_probs.find(key));
    ++num_erased;
  }
  cout << "Remove rules:" << num_erased << endl;
}

//update count to prob
void logCount() {
  cout << "Update count to log prob..." << endl;
  for (auto iter = rules_to_probs.begin(); iter != rules_to_probs.end(); ++iter) {
    auto probs = iter->second;
    for (int i = 0; i < probs.size(); ++i) {
        probs[i] = log(probs[i]/5000);
    }
    iter->second = probs;
  }
  cout << "Finish updating count to log prob..." << endl;
}

int predictOneData(const vector<int>& data) {
  vector<vector<double>> matched_prob;

  // go over the rules and find those rules can be applied to this data
  for (auto iter = rules_to_probs.begin(); iter != rules_to_probs.end(); ++iter) {
    auto rule = iter->first;
    bool matched = true;
    for (int i = 0; i < rule.size(); i+=2) {
      int index = rule[i];
      int value = rule[i+1];
      if (data[index] != value) {
        //doesn't match
        matched = false;
        break;
      }
    }
    if (matched) {
      matched_prob.push_back(iter->second);
    }
  }

  vector<double> res(10, 0.0);

  for (int i = 0; i < matched_prob.size(); ++i) {
    for (int j = 0; j < 10; ++j) { //sum up the log prob for each digit
      res[j] += matched_prob[i][j];
    }
  }

  int predict = 0;
  for (int i = 0; i < 10; ++i) {
    if (res[predict] < res[i]) {
      predict = i;
    }
  }
  return predict;
}

int main(int argc, char* argv[]) {
  
  if (argc != 3) {
    cout <<"usage: " << argv[0] <<" <filename> pvalue\n";
    return 0;
  }

  loadData(argv[1]);

  int p = atoi(argv[2]);
  if (p != 1 && p != 2) {
    cout << "only support p = 1 or 2 for now" << endl;
    return 0;
  }
  int N = data.size();
  cout <<"Extracting Rules..." << endl;
  if (p == 1) {
    for (int i = 0; i < N; ++i) {
      extractRules(data[i], labels[i]);

      if (i % 5000 == 0)
        cout << "Finish "<< i << endl;
    }
  } else {
    for (int i = 0; i < N; ++i) {
      extractRules2(data[i], labels[i]);
      
      if (i % 5000 == 0)
        cout << "Finish "<< i << endl;
    }
  }
  cout << "Finish Extracting Rules..." << endl;
  filterRules();
  logCount();

  //print results
  cout << "\t";
  for (int i = 0; i < 10; ++i) {
    cout << i << "\t";
  }
  cout << endl;

  double acc = 0;
  double num_corr = 0;
  
  for (int i = 0; i < 50000; i+=5000) {
    vector<int> res(10, 0);
    for (int j = 0; j < 5000; ++j) {
      int p = predictOneData(data[i + j]);
      res[p] += 1;
      if (p == labels[i + j]) ++num_corr;
    }
    cout << i / 5000 << "\t";
    for (auto num : res) {
      cout << num << "\t";
    }
    cout << "5000" << endl;
  }

  cout <<"====================="<< endl;
  cout <<"Total accuracy: "<< num_corr / data.size() << endl;
  cout <<"====================="<< endl;
  /*
  //per digit
  cout << "\t";
  for (int i = 0; i < 10; ++i) {
    cout << i << "\t";
  }
  cout << endl;
  for (int i = 0; i < 50000; i+=5000) {
    vector<int> res(10, 0);
    for (int j = 0; j < 5000; ++j) {
      res[predictOneData(data[i + j])] += 1;
    }
    cout << i / 5000 << "\t";
    for (auto num : res) {
      cout << 1.0 * num / 5000 << "\t";
    }
    cout << "5000" << endl;
  }
  */
  return 0;
}
