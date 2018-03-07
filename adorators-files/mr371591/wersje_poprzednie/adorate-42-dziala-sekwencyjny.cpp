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
set<unsigned long> graph_prim;
map<unsigned long, set<unsigned long>> who_i_adorate;
map<unsigned long, set<unsigned long>> who_i_dont_adorate;
map<unsigned long, set<unsigned long>> who_adorate_me;
map<unsigned long, unsigned long> db;
map<unsigned long, unsigned long> db_temp;

mutex mutex_to_get_vertex_to_process;
mutex mutex_to_add_to_graph_prim;

mutex mutex_variable_security;
volatile unsigned long how_many_read = 0;
volatile unsigned long how_many_write = 0;
mutex mutex_read_security;
mutex mutex_write_security;

set<unsigned long> vertex_to_process;
map<int, mutex*> mutex_on_vertex;

bool debug = false;
bool debug2 = false;
bool debug_num = true;

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
			if(debug)cout << "Linia " << x << " : " << line << endl;
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

						set<unsigned long> new_set;
						new_set.insert(v2);
						who_i_dont_adorate.insert(make_pair(v1, new_set));

						set<unsigned long> empty_set;
						who_i_adorate.insert(make_pair(v1, empty_set));
						who_adorate_me.insert(make_pair(v1, empty_set));

						db.insert(make_pair(v1, 0));
						db_temp.insert(make_pair(v1, 0));

						mutex* new_mutex = new mutex;
						mutex_on_vertex.insert(make_pair(v1, new_mutex));

					}
					else {
						if(graph[v1].count(v2) == 0) {
							graph[v1].insert(make_pair(v2, k));
							who_i_dont_adorate[v1].insert(v2);
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

						set<unsigned long> new_set;
						new_set.insert(v1);
						who_i_dont_adorate.insert(make_pair(v2, new_set));

						set<unsigned long> empty_set;
						who_i_adorate.insert(make_pair(v2, empty_set));
						who_adorate_me.insert(make_pair(v2, empty_set));

						db.insert(make_pair(v2, 0));
						db_temp.insert(make_pair(v2, 0));

						mutex* new_mutex = new mutex;
						mutex_on_vertex.insert(make_pair(v2, new_mutex));

					}
					else {
						if(graph[v2].count(v1) == 0) {
							graph[v2].insert(make_pair(v1, k));
							who_i_dont_adorate[v2].insert(v1);
						}
						else if(graph[v2][v1] < k) {
							graph[v2][v1] = k; // ustawiam wagę krawędzi na maksymalną
						}
						else {
//							cout << "Olewam tą linijkę :-D" << endl;
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
	if(who_adorate_me[v].size() < (unsigned long)bvalue(method, v)) return make_pair(false, 0);
	else { // who_adorate_me[v].size() == bvalue(method, v)
		unsigned long min, minVertex;
		bool noMinVertex = true;
		for(unsigned long adorator : who_adorate_me[v]) {
			if(noMinVertex) {
				noMinVertex = false;
				min = graph[v][adorator];
				minVertex = adorator;
			}
			else if(graph[v][adorator] < min) {
				min = graph[v][adorator];
				minVertex = adorator;
			}
		}
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
	unsigned long maxEdgeValue, maxVertex;
	bool noMaxVertex = true;
//	x = arg max(v) { W(u, v) : v in N(u) - T(u) && (W(u, v) > W(v, S(v).last)) };

	for(unsigned long v2 : who_i_dont_adorate[v1]) { // v2 to v
		if(debug) cout << "--- Nie adoruję jeszcze " << v2 << endl;
		pair<unsigned long, unsigned long> W_v_Sv_last_result = W_v_Sv_last(method, v2); // para <wierzchołek, wartość krawędzi>
		if(debug) cout << "-----> W_v_Sv_last_result(" << v2 << ") = (" << W_v_Sv_last_result.first << ", " << W_v_Sv_last_result.second << ")" << endl;
		if(W_v_Sv_last_result.second == 0) {
			// waga = 0 więc kto = NULL
			// więc na pewno W(v1, v2) :>: W(v2, S[v2].last)
			if(noMaxVertex) {
				noMaxVertex = false;
				maxVertex = v2;
				maxEdgeValue = graph[v1][v2];
			}
			else {
				if(graph[v1][v2] > maxEdgeValue || (graph[v1][v2] == maxEdgeValue && v2 > maxVertex) ) {
					maxVertex = v2;
					maxEdgeValue = graph[v1][v2];
				}
			}
		}
		else {
			// W_v_Sv_last_result.second > 0 więc istnieje S[v2].last
//			if(graph[v1][v2] > W_v_Sv_last_result.second || (graph[v1][v2] == W_v_Sv_last_result.second && v2 > W_v_Sv_last_result.first)) {
			if(graph[v1][v2] > W_v_Sv_last_result.second || (graph[v1][v2] == W_v_Sv_last_result.second && v1 > W_v_Sv_last_result.first)) {
				if(noMaxVertex) {
					noMaxVertex = false;
					maxVertex = v2;
					maxEdgeValue = graph[v1][v2];
				}
				else {
					if(graph[v1][v2] > maxEdgeValue || (graph[v1][v2] == maxEdgeValue && v2 > maxVertex) ) {
						maxVertex = v2;
						maxEdgeValue = graph[v1][v2];
					}
				}

			}
			else if(debug) cout << "NIE1 bo graph[v1][v2] = graph[" << v1 << "][" << v2 << "] = " << graph[v1][v2] << endl;
		}
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
	// y = S(v2).last ---> adorator węzła v2 z najmniejszą wagą gdy S[v].size() == b[v], w przeciwnym razie NULL
	// S(v2) ----> lista adoratrów węzła v2
	// T(v1) ----> lista węzłów które v1 adoruje

	// ----- N (v2) - sąsiedzi węzła v2,
	// ----- T (v2) - wezły które są adorowane przez v2
	// ----- S (v2) - węzły które adorują v2


//	 S[x] is a priority queue limited to maximally b[x] nodes.
//	 So, if y != null, S[x].insert(u) implicitly removes y, a worse adorator, from S[x].
//	 In other words, if y != null, after adding u, y is no longer in S[x].

	if(debug) cout << "Jestem w make_suitor (method = " << method << ", v1 = " << v1 << ", v2 = " << v2 << ")" << endl;
	pair<bool, unsigned long> y = Sv_last(method, v2);
	// S(x).insert(u);
	who_adorate_me[v2].insert(v1);
	// T(u).insert(x);
	who_i_adorate[v1].insert(v2);
	who_i_dont_adorate[v1].erase(v2);

	if(y.first == false) {
		if(debug) cout << "Nikogo nie usuwam" << endl;
	}
	else if(y.first == true) {
		if(debug) cout << "Kogos usuwam" << endl;
		// S(x).remove(y) bo S(x).size()
		who_adorate_me[v2].erase(y.second); // ?? na pewno ??? --> Tak, bo te zbiory to koleki priorytetowe, i musimy usunąc y bo jest najmniejszy
//		annuled_proposals[v2].insert(y);

		// T(y).remove(x)
		who_i_adorate[y.second].erase(v2); // y przestaje adorować v2 //
		who_i_dont_adorate[y.second].insert(v2);

		db_temp[y.second]++; // y będzie musiał sobie jeszcze kogoś znaleźć

		pair<bool, unsigned long> z = argmax(method, y.second);
		if(z.first == true) {
			if(debug) cout << "Rekurencja" << endl; // tu się zapętla !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			return make_suitor(method, y.second, z.second); // if u annuls the proposal of a vertex v then ... ? co mam z tym zrobić ??
		}
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

			mutex_variable_security.lock(); // P(ochrona) - w zwiazku z tym że jakieś  v2 może zmienic who_i_dont_adorate[v1] w funkcji make_suitor

			// pomijam wierzchołki v, t że b(v) = 0
			set<unsigned long> to_remove;
			for(unsigned long who : who_i_dont_adorate[v1]) {
				if(bvalue(b_method, who) <= 0) {
					to_remove.insert(who);
				}
			}
			for(unsigned long who : to_remove) {
				who_i_dont_adorate[v1].erase(who);
			}

			unsigned long i = 1;
			if(true) {
				if(debug) cout << "Uwaga, zaraz będzie while ale teraz bvalue(b_method, v1)-db[v1]=" << ((unsigned long) bvalue(b_method, v1) - db[v1]) << endl;
				if(debug) cout << "... bo bvalue = " << (unsigned long) bvalue(b_method, v1) << " i db[v1] = " << db[v1] << endl;
				while(i + db[v1] <= (unsigned long) bvalue(b_method, v1) && who_i_dont_adorate[v1].empty() == false) { // while i <= b(u) - db[u] i mam jeszcze jakiegos sąsiada ( N(u) != exhausted)
					if(debug) {
						cout << "Wątek " << thread_count << " : dopuki i=" << i << " + db[v1] = " << db[v1] << " <= bvalue(b_method, v1)="
						     << (unsigned long) bvalue(b_method, v1) << endl;
						cout << "W pętli while , who...size() = " << who_i_dont_adorate[v1].size() << endl;
					}

					pair<bool, unsigned long> best = argmax(b_method, v1); // to jest to samo co "Let best be an eligible partner of v1"
					if(debug)
						cout << "Wątek " << thread_count << " : best = (" << best.first << ", " << best.second << ")"
						     << endl;
					if(best.first == true) { // best != NULL

//					if(best == argmax(b_method, v1)) {
						if(true) {
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
//					cout << "Wątek " << thread_count << " : zrobi V( " << best.second << ")" << endl;
//					mutex_on_vertex[best.second]->unlock();
					}
					else {
						who_i_dont_adorate[v1].clear();
					}
					mutex_variable_security.unlock();
					mutex_variable_security.lock();
				}
			}
			mutex_variable_security.unlock();
		}
	}
	if(thread_count > 1) {
		handle.get();
	}
	if(debug)cout << "Wątek " << thread_count << " kończy " << endl;
}


void b_suitor_algorithm(int thread_count, unsigned int b_method) {
	for(auto pair : graph) {
		if(bvalue(b_method, pair.first) > 0) {
			vertex_to_process.insert(pair.first);
		}
	}

	if(debug)cout << "GŁÓWNY WĄTEK : Będę odpalał " << thread_count << " watkow " << endl;

	while(not vertex_to_process.empty()) {
		if(debug)cout << "GŁÓWNY WĄTEK : zadania rozpoczynają działanie ##########################" << endl;
		if(thread_count > 0) {
			task(b_method, thread_count);
		}
		if(debug)cout << "GŁÓWNY WĄTEK : zadania kończą działanie       ##########################" << endl;

		// Update Q using Q'
		vertex_to_process.clear();
		for(unsigned long vertex : graph_prim) {
			if(bvalue(b_method, vertex) > 0) {
				vertex_to_process.insert(vertex);
			}
		}
		if(debug)cout << "Następnym razem trzeba będzie przerobić " << vertex_to_process.size() << " wierzchołków" << endl;
		graph_prim.clear();
		map<unsigned long, unsigned long>::iterator it;
		for(it = db_temp.begin(); it != db_temp.end(); it++) {
			db[it->first] += db_temp[it->first];
			db_temp[it->first] = 0;
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
	for(pair<unsigned long, set<unsigned long>> para : who_i_adorate) {
		if(debug)cout << para.first << " adoruje : " << endl;
		for(unsigned long kogo : para.second) {
			if(debug)cout << "--- " << kogo << " z wagą " << graph[para.first][kogo] << endl;
			suma += graph[para.first][kogo];
		}
	}
	cout << "WYNIK = " << suma/2 << endl;
}

void recover_data() {

	map<unsigned long, set<unsigned long>>::iterator it;
	for(it = who_i_adorate.begin(); it != who_i_adorate.end(); it++) { it->second.clear(); }
	for(it = who_adorate_me.begin(); it != who_adorate_me.end(); it++) { it->second.clear(); }
	for(it = who_i_dont_adorate.begin(); it != who_i_dont_adorate.end(); it++) { it->second.clear(); }

	for(auto pair_int_map : graph) {
		// wystarczy że dla każdej pary zrobię pojedyńczo bo w graph'ie jesli jest map<3, <7, 21>> to jest tez map<7, <3, 21>>
		for(auto pair_int_int : pair_int_map.second) {
			who_i_dont_adorate[pair_int_map.first].insert(pair_int_int.first);
		}
		db[pair_int_map.first] = 0;
	}
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

	clock_t czas_przed_wczytywaniem = clock();

	read_file(input_filename);

	clock_t czas_po_wczytywaniu = clock();
	cout << "Czas wczytywania : " << (czas_po_wczytywaniu - czas_przed_wczytywaniem) * 1000.0 / CLOCKS_PER_SEC << "ms" << endl;

	cout << "Wczytałem plik" << endl;
	cout << "Zostało " << graph.size() << " wierzcholkow do przerobienia" << endl;

	for (unsigned int b_method = 0; b_method < b_limit + 1; b_method++) {
//	for (unsigned int b_method = 2; b_method < 2 + 1; b_method++) {
		clock_t czas_przed_wykonaniem_testu = clock();
		cout << "----------------------------- Przeprowadzam test dla b_method = " << b_method << endl;
		b_suitor_algorithm(thread_count, b_method);
		give_result();
		recover_data();
		cout << "Czas wykonania testu : " << (clock() - czas_przed_wykonaniem_testu) * 1000.0 / CLOCKS_PER_SEC << "ms" << endl;
	}
	cout << "Czas wykonania wszystkich testów : " << (clock()  - czas_po_wczytywaniu) * 1000.0 / CLOCKS_PER_SEC << "ms" << endl;
}
