#include<cstdio>
#include<iostream>
#include<set>
#include<map>

using namespace std;

class my_comparator {
private:
	unsigned long v1;
public:
	my_comparator(const unsigned long& v) : v1(v) {};
	bool operator()(const unsigned long& v2_1, const unsigned long& v2_2) {
		return (v2_1%v1 < v2_2%v1 || (v2_1%v1 == v2_2%v1 && v2_1 < v2_2));
	}
};

map<unsigned long, set<unsigned long, my_comparator> > graph;

int main() {
	unsigned long x = 5;
	my_comparator mc(x);
	set<unsigned long, my_comparator> s(mc);
	s.insert(25);
	s.insert(17);
	s.insert(22);
	graph.insert(make_pair(1, s));
//	set<unsigned long, my_comparator> s2(mc);
//	s2 = graph.at(1);
//	s2.insert(33);
//	graph.erase(1);
//	graph.insert(make_pair(1, s2));
//	cout << graph.count(1) << endl;
	graph.at(1).insert(33);



//	x.insert(31);
//	x = 6;
//	set<unsigned long, my_comparator> s2(my_comparator(x));
//	s2.insert(17);
//	s2.insert(10);
//	s2.insert(23);
//	graph.insert(make_pair(2, s2));
	for(pair<unsigned long, set<unsigned long, my_comparator>> para : graph) {
		cout << "Dla x = " << para.first << " : {";
		for(auto x : para.second) {
			cout << x << ", ";
		}
		cout << "}" << endl;
	}
	return 0;
}