#include "blimit.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <list>
#include <set>
#include <future>
#include <condition_variable>

using namespace std;

/*
 * graph[3] = map <nr, waga> gdzie :
 * nr to wierzchołek z którym połączony jest wierzchołek(3)
 * waga to waga krawedzi pomiędzy wierzcholek(3) i wierzcholek(nr)
*/
/*  !!! Graf jest nieskierowany. !!!  */


/*
 * S(v) 		- adoratorzy węzła v
 * S(v).last 	- if S(v).size() < b(v)
 * 						then return NULL,
 * 				  else
 * 				  		return nr adoratora węzła v z najmniejszą wagą // bo S(v).size() == b(v)
 * T(v)			- węzły które v adoruje // "vertices that v has proposed to"
 * N(v)			- sąsiedzi węzła v
 * W(u, v)		- waga krawędzi u <----> v
 * Q, R			- kolejki węzłów, Q - unsaturated vertices,
 * Q'           - węzły które zostaną przerobione jeszcze raz podczas kolejnej iteracji
 *
 * b(v)			- maksymalna liczba adoratorów węzła v
 * db(v)        - ile razy propozycje v były anulowane przez jego sąsiadów
 */

map<unsigned long, map<unsigned long, unsigned long>> graph;

class my_comparator_min_max {
private:
	unsigned long v1;
public:
	explicit my_comparator_min_max(unsigned long& v) : v1(v) {};
	bool operator()(const unsigned long v2_1, const unsigned long v2_2) {
		return (graph[v1][v2_1] < graph[v1][v2_2] || (graph[v1][v2_1] == graph[v1][v2_2] && v2_1 < v2_2));
	}
};

class my_comparator_max_min {
private:
	unsigned long v1;
public:
	explicit my_comparator_max_min(unsigned long& v) : v1(v) {};
	bool operator()(const unsigned long v2_1, const unsigned long v2_2) {
		return (graph[v1][v2_1] > graph[v1][v2_2] || (graph[v1][v2_1] == graph[v1][v2_2] && v2_1 > v2_2));
	}
};

set<unsigned long> graph_prim;
map<unsigned long, set<unsigned long, my_comparator_min_max>> who_adorate_me;
map<unsigned long, set<unsigned long, my_comparator_min_max>> who_i_adorate;
map<unsigned long, set<unsigned long, my_comparator_max_min>> who_i_dont_adorate;
map<unsigned long, set<unsigned long, my_comparator_max_min>> who_i_dont_adorate_next;
map<unsigned long, unsigned long> db;
map<unsigned long, unsigned long> db_temp;

mutex mutex_to_get_vertex_to_process;
mutex mutex_to_who_i_dont_adorate_next;
mutex mutex_variable_security;

set<unsigned long> vertex_to_process;
map<int, mutex*> mutex_on_vertex;

bool debug = false;
bool debug2 = false;
bool debug_num = false;

void read_file(string &input_filename) {
	if(debug2) cout << "*********** Wczytywanie pliku ***********" << endl;
	ifstream file;
	file.open(input_filename);
	if(debug) cout << "Czy jest otwarty  : " << file.is_open() << endl;
	if(file.is_open()) {
		string line;
		int x = 0;
		while(getline(file, line)) {
			if(debug) if(line[0] == '#') cout << "Pominięta ";
			if(debug) cout << "Linia " << x << " : " << line << endl;
			if(line[0] != '#') {
				istringstream iss(line);
				unsigned long v1, v2, k;
				iss >> v1 >> v2 >> k;
				if(debug) cout << "krawedz (" << v1 << ", " << v2 << ") waga = " << k << endl;
				if(v1 != v2) {
					if(graph.count(v1) == 0) {
						map<unsigned long, unsigned long> new_map;
						new_map.insert(make_pair(v2, k));
						graph.insert(make_pair(v1, new_map));

						my_comparator_max_min mc_max_min(v1);
						my_comparator_min_max mc_min_max(v1);
						set<unsigned long, my_comparator_max_min> new_set(mc_max_min);
						new_set.insert(v2);
						who_i_dont_adorate.insert(make_pair(v1, new_set));

						set<unsigned long, my_comparator_max_min> empty_set1(mc_max_min);
						who_i_dont_adorate_next.insert(make_pair(v1, empty_set1));

						set<unsigned long, my_comparator_min_max> empty_set2(mc_min_max);
						who_i_adorate.insert(make_pair(v1, empty_set2));
						who_adorate_me.insert(make_pair(v1, empty_set2));

						db.insert(make_pair(v1, 0));
						db_temp.insert(make_pair(v1, 0));

						mutex* new_mutex = new mutex;
						mutex_on_vertex.insert(make_pair(v1, new_mutex));

					}
					else {
						if(graph[v1].count(v2) == 0) {
							graph[v1].insert(make_pair(v2, k));
							who_i_dont_adorate.at(v1).insert(v2);
						}
						else if(graph[v1][v2] < k) {
							graph[v1][v2] = k; // ustawiam wagę krawędzi na maksymalną
						}
						else {
//							cout << "Olewam tą linijkę :-D" << endl;
						}
					}

					if(graph.count(v2) == 0) {
						map<unsigned long, unsigned long> new_map;
						new_map.insert(make_pair(v1, k));
						graph.insert(make_pair(v2, new_map));

						my_comparator_max_min mc_max_min(v2);
						my_comparator_min_max mc_min_max(v2);
						set<unsigned long, my_comparator_max_min> new_set(mc_max_min);
						new_set.insert(v1);
						who_i_dont_adorate.insert(make_pair(v2, new_set));

						set<unsigned long, my_comparator_max_min> empty_set1(mc_max_min);
						who_i_dont_adorate_next.insert(make_pair(v2, empty_set1));

						set<unsigned long, my_comparator_min_max> empty_set2(mc_min_max);
						who_i_adorate.insert(make_pair(v2, empty_set2));
						who_adorate_me.insert(make_pair(v2, empty_set2));

						db.insert(make_pair(v2, 0));
						db_temp.insert(make_pair(v2, 0));

						mutex* new_mutex = new mutex;
						mutex_on_vertex.insert(make_pair(v2, new_mutex));

					}
					else {
						if(graph[v2].count(v1) == 0) {
							graph[v2].insert(make_pair(v1, k));
							who_i_dont_adorate.at(v2).insert(v1);
						}
						else if(graph[v2][v1] < k) {
							graph[v2][v1] = k; // ustawiam wagę krawędzi na maksymalną
						}
						else {
//							cout << "Olewam tą linijkę 2 :-D" << endl;
						}
					}
				}
			}
			x++;
		}
	}
	if(debug2) cout << "*********** Koniec wczytywania pliku ***********" << endl << endl;
}


pair<bool, unsigned long> Sv_last(unsigned int method, unsigned long v) {
	if(who_adorate_me.at(v).size() < (unsigned long)bvalue(method, v)) return make_pair(false, 0);
	else { // who_adorate_me[v].size() == bvalue(method, v)
		auto it = who_adorate_me.at(v).begin();
		unsigned long minVertex = *it;
		return make_pair(true, minVertex);
	}
}

// zwraca parę <kto, waga>
// ale jesli waga = 0, to znaczy że kto = NULL
pair<unsigned long, unsigned long> W_v_Sv_last(unsigned int method, unsigned long v) {
	pair<unsigned long, unsigned long> Sv_last_value = Sv_last(method, v);
	if(debug) cout << "------ S(" << v << ").last = (" << Sv_last_value.first << ", " << Sv_last_value.second << ")" << endl;
	if(Sv_last_value.first == false) return make_pair(0, 0);
	else return make_pair(Sv_last_value.second, graph[v][Sv_last_value.second]);
}

pair<bool, unsigned long> argmax(unsigned int method, unsigned long v1) { // v1 to u
	if(debug) cout << "--- Jestem w argmax(" << v1 << ")" << endl;
	unsigned long maxVertex;
	bool noMaxVertex = true;
//	x = arg max(v) { W(u, v) : v in N(u) - T(u) && (W(u, v) > W(v, S(v).last)) };

	set<unsigned long, my_comparator_max_min>::iterator it = who_i_dont_adorate.at(v1).begin();
	while(noMaxVertex && it != who_i_dont_adorate.at(v1).end()) {
		unsigned long v2 = *it;
		if(debug) cout << "Kandydat " << v2 << endl;
		pair<unsigned long, unsigned long> W_v_Sv_last_result = W_v_Sv_last(method, v2);
		if(W_v_Sv_last_result.second == 0) {
			// waga = 0 więc kto = NULL
			// więc na pewno W(v1, v2) :>: W(v2, S[v2].last)
			noMaxVertex = false;
			maxVertex = v2;
		}
		else {
			// W_v_Sv_last_result.second > 0 więc istnieje S[v2].last
			if(graph[v1][v2] > W_v_Sv_last_result.second || (graph[v1][v2] == W_v_Sv_last_result.second && v1 > W_v_Sv_last_result.first)) {
				noMaxVertex = false;
				maxVertex = v2;
			}
		}
		it++;
	}
	if(noMaxVertex) {
		if(debug) cout << "--- Nie wybrałem nikogo" << endl;
		return make_pair(false, 0);
	}
	else {
		if(debug) cout << "--- Wybrałem : " << maxVertex << endl;
		return make_pair(true, maxVertex);
	}
}

// v1 staje się adoratorem v2
pair<bool, unsigned long> make_suitor(unsigned int method, unsigned long v1, unsigned long v2) {

	if(debug) cout << "Jestem w make_suitor (method = " << method << ", v1 = " << v1 << ", v2 = " << v2 << ")" << endl;
	pair<bool, unsigned long> y = Sv_last(method, v2);
	if(debug) cout << "Sv_last(" << v2 << ") = (" << y.first << ", " << y.second << ")" << endl;

	// S(x).insert(u);
	who_adorate_me.at(v2).insert(v1);
	// T(u).insert(x);
	who_i_adorate.at(v1).insert(v2);
	who_i_dont_adorate.at(v1).erase(v2);

	if(y.first == false) {
		if(debug) cout << "Nikogo nie usuwam" << endl;
	}
	else if(y.first == true) {
		if(debug) cout << "Kogos usuwam" << endl;
		// S(x).remove(y) bo S(x).size()
		who_adorate_me.at(v2).erase(y.second); // ?? na pewno ??? --> Tak, bo te zbiory to koleki priorytetowe, i musimy usunąc y bo jest najmniejszy
		if(debug) cout << "Udało się usunąc " << endl;

		// T(y).remove(x)
		who_i_adorate.at(y.second).erase(v2); // y przestaje adorować v2 //
//		cout << "Uwaga : dla y.second = " << y.second << ", mamy " << (who_i_dont_adorate_next.find(y.second) != who_i_dont_adorate_next.end()) << endl;

		mutex_to_who_i_dont_adorate_next.lock();
		who_i_dont_adorate_next.at(y.second).insert(v2);
		mutex_to_who_i_dont_adorate_next.unlock();

		db_temp[y.second]++; // y będzie musiał sobie jeszcze kogoś znaleźć
	}
	return y;
}

void task(unsigned int b_method, int thread_count) {

	if(debug2)cout << "Wątek " << thread_count << " startuje " << endl;
	future<void> handle;
	if(thread_count > 1) {
		handle = async(launch::async, task, b_method, thread_count - 1);
	}
	bool end_of_work = false;
	while(not end_of_work) {

		mutex_to_get_vertex_to_process.lock();
		if(vertex_to_process.empty()) {
			if(debug2)cout << "Wątek " << thread_count << " : nie ma wierzchołków do wzięcia" << endl;
			mutex_to_get_vertex_to_process.unlock();
			end_of_work = true;
		}
		else {
			// biorę jakiś wierzcholek
			set<unsigned long>::iterator it = vertex_to_process.begin();
			unsigned long v1 = *it;
			if(debug2)cout << "Wątek " << thread_count << " : biore wierzcholek " << v1 << endl;
			if(debug_num) {
				if(
						vertex_to_process.size() % 100000 == 0 ||
						(vertex_to_process.size() < 100000 && vertex_to_process.size() % 10000 == 0) ||
						(vertex_to_process.size() < 10000 && vertex_to_process.size() % 2000 == 0)
						) {
					cout << "Zostało " << vertex_to_process.size() << " wierzcholkow do przerobienia" << endl;
				}
			}

			vertex_to_process.erase(it);
			if(debug2)cout << "Wątek " << thread_count << " : usunąłem" << endl;
			mutex_to_get_vertex_to_process.unlock();
			// skończyłem brać wierzchołek

			mutex_on_vertex[v1]->lock();

//			mutex_variable_security.lock(); // P(ochrona) - w zwiazku z tym że jakieś  v2 może zmienic who_i_dont_adorate[v1] w funkcji make_suitor
			/* ZMIANA */
			unsigned long i = 1;
			bool end_of_loop = false;

//			mutex_variable_security.lock();
			if(debug) cout << "Zrobiłem lock" << endl;
			while(i <= db[v1] && who_i_dont_adorate.at(v1).empty() == false && end_of_loop == false) { // while i <= b(u) - db[u] i mam jeszcze jakiegos sąsiada ( N(u) != exhausted)
				if(debug) {
					cout << "Wątek " << thread_count << " : dopuki i=" << i << " <= db[v1]=" << db[v1];
					cout << " && who_i_dont_adorate.at(" << v1 << ").empty()=" << who_i_dont_adorate.at(v1).empty();
					cout << " == false && end_of_loop == false" << endl;
					cout << "W pętli while , who_i_dont_adorate.at(v1).size() = " << who_i_dont_adorate.at(v1).size() << endl;
				}

				pair<bool, unsigned long> best = argmax(b_method, v1); // to jest to samo co "Let best be an eligible partner of v1"
				if(debug)
					cout << "Wątek " << thread_count << " : best = (" << best.first << ", " << best.second << ")"
						 << endl;
				if(best.first == true) { // best != NULL
//					bool czy_udalo_sie = true;
//					mutex_on_vertex[best.second]->lock();

					bool czy_udalo_sie = mutex_on_vertex[best.second]->try_lock();
					if(czy_udalo_sie) {
						if(best == argmax(b_method, v1)) {
							i++;
							if(debug)
								cout << "Wątek " << thread_count << " : zaraz " << v1 << " będzie suitor'em of "
									 << best.second << endl;

							pair<bool, unsigned long> whom_i_annulled = make_suitor(b_method, v1, best.second);

							if(debug) cout << "Wątek " << thread_count << " : skończyłem make_suitor i anulowałem : ";
							if(debug)
								cout << "(" << whom_i_annulled.first << ", " << whom_i_annulled.second << ")" << endl;
							// niezmiennik --> whom_i_annulled != NULL
							if(whom_i_annulled.first == true) { // jesli kogoś anulowałem to dodaję go do graph_prim
								graph_prim.insert(whom_i_annulled.second);
							} // Q'.insert(v)
							// nie musimy wykonywać update db, do
						}
						mutex_on_vertex[best.second]->unlock();
					}
				}
				else {
					end_of_loop = true;
				}
				mutex_on_vertex[v1]->unlock();
				mutex_on_vertex[v1]->lock();
			}
			mutex_on_vertex[v1]->unlock();
		}
	}
	if(thread_count > 1) {
		if(debug) cout << "Wątek " << thread_count << " : skonczylem, czekam na " << thread_count - 1 << endl;
		handle.get();
	}
	else if(debug) cout << "Wątek " << thread_count << " : skonczylem" << endl;
	if(debug) cout << "Wątek " << thread_count << " kończy " << endl;
}


void b_suitor_algorithm(int thread_count, unsigned int b_method) {

	if(debug)cout << "GŁÓWNY WĄTEK : Będę odpalał " << thread_count << " watkow " << endl;

	while(not vertex_to_process.empty()) {
		if(debug)cout << "GŁÓWNY WĄTEK : zadania rozpoczynają działanie ##########################" << endl;
		if(thread_count > 0) {
			task(b_method, thread_count);
		}
		if(debug)cout << "GŁÓWNY WĄTEK : zadania kończą działanie       ##########################" << endl;

		// Update Q using Q'
		for(unsigned long vertex : graph_prim) {
			if(bvalue(b_method, vertex) > 0) {
				vertex_to_process.insert(vertex);
			}
		}
		if(debug)cout << "Następnym razem trzeba będzie przerobić " << vertex_to_process.size() << " wierzchołków" << endl;
		graph_prim.clear();

		// dodajemy z who_i_dont_adorate_next do who_i_dont_adorate
		for(pair<unsigned long, set<unsigned long, my_comparator_max_min>> para : who_i_dont_adorate_next) {
			for(unsigned long who : para.second) {
				who_i_dont_adorate.at(para.first).insert(who);
			}
		}
		map<unsigned long, set<unsigned long, my_comparator_max_min>>::iterator it;
		for(it = who_i_dont_adorate_next.begin(); it != who_i_dont_adorate_next.end(); it++) {
			it->second.clear();
		}

		map<unsigned long, unsigned long>::iterator it2;
		for(it2 = db_temp.begin(); it2 != db_temp.end(); it2++) {
			db[it2->first] = db_temp[it2->first];
			db_temp[it2->first] = 0;
		}
//		Update b using db // O co tu chodzi :
//
//		We update Q to be the set of vertices in Q'
//		and the vector b to reflect the number of additional partners we need to find for each vertex u using db(u),
//		the number of times u's proposal was annulled by a neighbor
//
//		A już chyba rozumiem!! -> chodzi o to aby pamiętać o tym, żeby zwiekszyc b(v) bo anulowaniu propozycji
	}
}

void give_result() {
	if(debug)cout << "Ostatecznie wynik jest następujący : " << endl;
	unsigned long suma = 0;
	for(pair<unsigned long, set<unsigned long, my_comparator_min_max>> para : who_i_adorate) {
		if(debug)cout << para.first << " adoruje : " << endl;
		for(unsigned long kogo : para.second) {
			if(debug)cout << "--- " << kogo << " z wagą " << graph[para.first][kogo] << endl;
			suma += graph[para.first][kogo];
		}
	}
	cout << "WYNIK = " << suma/2 << endl;
}

void prepare_who_i_dont_adorate(unsigned int b_method) {
	for(auto pair_int_map : graph) {
		// wystarczy że dla każdej pary zrobię pojedyńczo bo w graph'ie jesli jest map<3, <7, 21>> to jest tez map<7, <3, 21>>
		if(bvalue(b_method, pair_int_map.first) > 0) {
			for(auto pair_int_int : pair_int_map.second) {
				if(bvalue(b_method, pair_int_int.first)) {
					who_i_dont_adorate.at(pair_int_map.first).insert(pair_int_int.first);
				}
			}
			vertex_to_process.insert(pair_int_map.first);
		}
		db[pair_int_map.first] = bvalue(b_method, pair_int_map.first);
	}
}

void recover_data() {
	map<unsigned long, set<unsigned long, my_comparator_min_max>>::iterator it_min_max;
	map<unsigned long, set<unsigned long, my_comparator_max_min>>::iterator it_max_min;
	for(it_min_max = who_i_adorate.begin(); it_min_max != who_i_adorate.end(); it_min_max++) { it_min_max->second.clear(); }
	for(it_min_max = who_adorate_me.begin(); it_min_max != who_adorate_me.end(); it_min_max++) { it_min_max->second.clear(); }
	for(it_max_min = who_i_dont_adorate.begin(); it_max_min != who_i_dont_adorate.end(); it_max_min++) { it_max_min->second.clear(); }
}



int main(int argc, char* argv[]) {

	srand (time(NULL));

	if (argc != 4) {
		std::cerr << "usage: "<<argv[0]<<" thread-count inputfile b-limit"<< std::endl;
		return 1;
	}

	int thread_count = std::stoi(argv[1]);
	int b_limit = std::stoi(argv[3]);
	std::string input_filename{argv[2]};

	read_file(input_filename);

	cout << "Wczytałem plik" << endl;
	cout << "Zostało " << graph.size() << " wierzcholkow do przerobienia" << endl;

	for (unsigned int b_method = 0; b_method < b_limit + 1; b_method++) {
//	for (unsigned int b_method = 2; b_method < 2 + 1; b_method++) {
		cout << "----------------------------- Przeprowadzam test dla b_method = " << b_method << endl;
		prepare_who_i_dont_adorate(b_method);
		b_suitor_algorithm(thread_count, b_method);
		give_result();
		recover_data();
	}
}
