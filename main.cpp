#include "Ship.h"

using namespace shipping;
using std::string;

int main() {
	// create grouping pairs
	Grouping<std::string> groupingFunctions = {
		{ "first_letter",
			[](const string& s){ return string(1, s[0]); }
		},
		{ "first_letter_toupper",
			[](const string& s){ return string(1, char(std::toupper(s[0]))); }
		}
	};
	// create restrictions
	std::vector<std::tuple<X, Y, Height>> restrictions = {
		std::tuple(X{2}, Y{6}, Height{4}),
		std::tuple(X{2}, Y{7}, Height{6}),
		std::tuple(X{0}, Y{0}, Height{2})
	};
	// create ship
	Ship<std::string> myShip{ X{5}, Y{12}, Height{8},
									restrictions,
									groupingFunctions };
	// load “containers”
	myShip.load(X{0}, Y{0}, "Hello");
	myShip.load(X{1}, Y{1}, "hey");
	myShip.load(X{1}, Y{1}, "bye");

	auto view00 = myShip.getContainersViewByPosition(X{0}, Y{0});
	auto view_h = myShip.getContainersViewByGroup("first_letter", "h");
	auto view_Hh = myShip.getContainersViewByGroup("first_letter_toupper", "H");

	myShip.load(X{0}, Y{0}, "hi");
	
	// loop on all “containers”: Hello, hi, hey, bye - in some undefined order
	for(const auto& container : myShip) { /*...*/}

	// loop on view00: hi, Hello - in this exact order
	for(const auto& container : view00) { /*...*/}

	// loop on view_h:	pair { tuple{X{0}, Y{0}, Height{1}}, hi },
	// 						pair { tuple{X{1}, Y{1}, Height{0}}, hey }
	// - in some undefined order
	for(const auto& container_tuple : view_h) { /*...*/}

	// loop on view_Hh:	pair { tuple{X{0}, Y{0}, Height{0}}, Hello },
	// 						pair { tuple{X{0}, Y{0}, Height{1}}, hi },
	//						pair { tuple{X{1}, Y{1}, Height{0}}, hey }
	// - in some undefined order
	for(const auto& container_tuple : view_Hh) { /*...*/}
}
