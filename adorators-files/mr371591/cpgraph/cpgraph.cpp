#include<cstdio>
#include<iostream>
#include<fstream>

using namespace std;

void wczytaj(ifstream& input, ofstream& output) {
	string s;
	int ile, ile_ominac;
	cout << "Ile linijek ominac : ";
	cin >> ile_ominac;
	cout << "Ile linijek wczytać : ";
	cin >> ile;
	for(int i = 0; i < ile_ominac; i++) {
		getline(input, s);
	}
	for(int i = 0; i < ile; i++) {
		getline(input, s);
		output << s << "\n";
	}
}

int main() {
	ifstream input;
	input.open("road");
	ofstream output;
	output.open("graf");
	if(output.is_open() && input.is_open()) {
		cout << "Są otwarte" << endl;
		wczytaj(input, output);
		wczytaj(input, output);
	}
	input.close();
	output.close();
	return 0;
}
