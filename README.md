## Wstęp :
  
  Problem b-skojarzeń (b-matching) uogólnia problem skojarzeń dopuszczając skojarzenie każdego wierzchołka \(v\) w grafie \(G=(V,E)\) z co najwyżej \(b(v)\) wierzchołkami (czyli standardowy problem skojarzeń to problem b-skojarzeń z b(v)=1 dla każdego \(v\)). Zajmiemy się problemem b-skojarzeń w nieskierowanym grafie ważonym \(w(E)\). Celem jest znalezienie skojarzenia o maksymalnej sumarycznej wadze krawędzi. Algorytmy dokładne dla tego problemu istnieją, ale mają dużą złożoność lub są trudne w implementacji.

## Algorytm b-adoratorów :
  
  Czym jest? Algorytm b-adoratorów (KHAN, Arif, et al, 2016) został opisany tutaj (https://www.cs.purdue.edu/homes/apothen/Papers/bMatching-SISC-2016.pdf). Jest on szybkim i prostym w implementacji rozwiązaniem problemu b-skojarzeń. Algorytm ten jest 2-aproksymacją: w najgorszym przypadku zwróci rozwiązanie co najwyżej dwa razy gorsze od optymalnego. Prosimy o zaimplementowanie równoległej wersji algorytmu b-adoratorów w C++ 14.
  Pseudokod sekwencyjnego algorytmu b-adoratorów przedstawiamy poniżej; dalsze wyjaśnienia w (KHAN, Arif, et al, 2016). Algorytm używa następujących zmiennych: Q, R: kolejki węzłow; S[v]: adoratorzy węzła v (maksymalnie b(v)); T[v] węzły, które v adoruje; N[v]: sąsiedzi węzła v; W(u,v) waga krawędzi (u,v); S[v].last: adorator węzła v z najmniejszą wagą gdy S[v].size()==b[v], w przeciwnym wypadku null (zakładamy, że W(v, null)==0). Żeby uzyskać deterministyczne rozwiązanie, algorytm wymaga ostrego porządku :<: wśród krawędzi wychodzących z każdego wierzchołka. Porządek ten definiujemy następująco: W(v,w) :<: W(v,x) <=> (W(v,w) < W(v,x)) || (W(v,w)==W(v,x) && w < x).
  Żeby uzyskać deterministyczne rozwiązanie, algorytm wymaga ostrego porządku :<: wśród krawędzi wychodzących z każdego wierzchołka. Porządek ten definiujemy następująco: W(v,w) :<: W(v,x) <=> (W(v,w) < W(v,x)) || (W(v,w)==W(v,x) && w < x).

## Pseudokod: 

```
Q = V
while (!Q.empty()) {
  for u : Q {
    while (T[u].size() < b(u)) {
       x = arg max(v) {W(u,v) : (v in N[u] - T[u]) && (W(u,v) :>: W(v, S[v].last))};
       if (x == null)
          break;
       else { // u will adorate x
          y = S[x].last;
          S[x].insert(u);
          T[u].insert(x);
          if (y != null) { // see also the FAQ
             T[y].remove(x);
             R.add(y);
          }
       }
    }
  }
  Q=R; R={};
}
```

## Sposób uruchomienia programu :

unzip xx123456.zip; cd xx123456; cp $TESTROOT$/blimit.cpp .; mkdir build; cd build; cmake -DCMAKE_BUILD_TYPE=Release ..; make
./adorate liczba-wątków plik-grafu limit-b
Program powinien przeprowadzać obliczenia używając liczba-wątków wątków. W testach wydajnościowych będzie zdecydowanie więcej wierzchołków niż wątków, ale musisz zapewnić poprawne działanie programu dla liczba-wątków < 100 niezależnie od liczby wierzchołków.

## Wejście programu :

Plik grafu opisuje krawędzie w grafie w formacie <id wierzchołek1> <id wierzchołek2> <waga>. Graf jest nieskierowany. Krawędzie nie są posortowane. Wierzchołki nie muszą być numerowane sekwencyjnie (tzn. poprawny jest graf bez np. wierzchołka o numerze 0). Graf nie musi być spójny. Plik może zaczynać się od kilku linii komentarza: każda z takich linii zaczyna się symbolem #.
Możesz przyjąć, że id wierzchołka \(\in [0, 10^9]\); waga \(\in [1, 10^6]\); a waga najlepszego b-dopasowania (rozwiązanie) nie przekroczy \(10^9\).
Możesz przyjąć, że cały graf zmieści się w pamięci operacyjnej.
Możesz przyjąć, że plik grafu jest poprawnie sformatowany.
Maksymalną liczbę dopasowanych wierzchołków \(b(v)\) zwraca funkcja bvalue o następującej sygnaturze (blimit.hpp):
  unsigned int bvalue(unsigned int method, unsigned long node_id);
Gdzie method to metoda generowania tej liczby. Przykładową implementację tej funkcji znajdziesz w blimit.cpp. Możesz założyć, że koszt obliczeniowy bvalue() jest niski (kilka instrukcji arytmetycznych).
Uwaga: w czasie testów będziemy podmieniać implementację blimit.cpp na nasze wersje używając cp nasz_katalog_testowy/nasz_plik_blimit.cpp twój_katalog/blimit.cpp. Po takim kopiowaniu twój program musi poprawnie kompilować się i następnie używać tej podmienionej wersji.

Twój program powinien wczytać graf wejściowy. Następnie, kolejno dla każdego metoda_b \(\in\) [0, limit_b]: (1) uruchomić algorytm adoratorów z \(b(v)\) wygenerowanymi funkcją bvalue(metoda_b, ...); i (2) wypisać na standardowym wyjściu sumę wag dopasowanych krawędzi.

## Przykładowe wejście: 
plik grafu "graf.txt"
```
#this graph is adopted from Khan et al, 2016
0 2 8
0 3 6
0 4 4
0 5 2
1 2 3
1 3 7
1 4 1
1 5 9
```
Funkcja bvalue (blimit.cpp):
```
unsigned int bvalue(unsigned int method, unsigned long node_id) {
  switch (method) {
    case 0: return 1;
    default: switch (node_id) {
        case 0: return 2;
        case 1: return 2;
        default: return 1;
    }
  }
}
```

Program uruchomiony przez: ./adorate liczba-watkow graf.txt 1 powinien zwrócić:
```
17
28
```
