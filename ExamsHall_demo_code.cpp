//=====================================
// ExamHall exercise
// @author: Amir Kirsh
//=====================================

//-------------------------------------
// ExamHall.h
//-------------------------------------
#include <vector>
#include <unordered_map>
#include <tuple>
#include <functional>
#include <string>
#include <optional>
#include <iostream>

namespace exams {
    template<typename T> class NamedType {
        T t;
    public:
        explicit NamedType(T t): t(t) {}
        operator T() const {
            return t;
        }
    };

    struct X : NamedType<int> {
        using NamedType<int>::NamedType;
    };

    struct Y : NamedType<int> {
        using NamedType<int>::NamedType;
    };

    using Position = std::tuple<exams::X, exams::Y>;
}

// a short pause from exams to define hash for Position
namespace std
{
    template<> struct hash<exams::Position>
    {
        std::size_t operator()(const exams::Position& pos) const noexcept
        {
            return std::get<0>(pos) ^ (std::get<1>(pos) << 1);
        }
    };
}

// back to the exams business 
namespace exams {
    class BadPositionException {
        X x;
        Y y;
        std::string msg;
    public:
        BadPositionException(X x, Y y, std::string msg)
            : x(x), y(y), msg(std::move(msg)) {}
        void print() const {
            std::cout << msg << " : X {" << x << "}, Y {" << y << "}\n";
        }
    };

    template<typename Examinee>
    using Grouping = std::unordered_map<std::string, std::function<std::string(const Examinee&)>>;

    template<typename Examinee>
    class ExamHall {
        class GroupView {
            const std::unordered_map<Position, const Examinee&>* p_group = nullptr;
            using iterator_type = typename std::unordered_map<Position, const Examinee&>::const_iterator;
        public:
            GroupView(const std::unordered_map<Position, const Examinee&>& group): p_group(&group) {}
            GroupView(int) {}
            auto begin() const { 
                return p_group? p_group->begin(): iterator_type{};
            } 
            auto end() const {
                return p_group? p_group->end(): iterator_type{};
            } 
        };
        class iterator {
            using ExamineesItr = typename std::vector<std::optional<Examinee>>::const_iterator;
            ExamineesItr examinees_itr;
            ExamineesItr examinees_end;
            void set_itr_to_occupied_sit() {
                while(examinees_itr != examinees_end && !(*examinees_itr)) {
                    ++examinees_itr;
                }
            }
        public:
            iterator(ExamineesItr examinees_itr, ExamineesItr examinees_end)
            : examinees_itr(examinees_itr), examinees_end(examinees_end) { set_itr_to_occupied_sit(); }
            iterator operator++() {
                ++examinees_itr;
                set_itr_to_occupied_sit();
                return *this;
            }
            const Examinee& operator*() const {
                return examinees_itr->value();
            }
            bool operator!=(iterator other) const {
                return examinees_itr != other.examinees_itr;
            }
        };

        std::vector<std::optional<Examinee>> examinees;
        int x_size;
        int y_size;
        Grouping<Examinee> groupingFunctions;
        using Pos2Examinee = std::unordered_map<Position, const Examinee&>;
        using Group = std::unordered_map<std::string, Pos2Examinee>;
        // all groupings by their grouping name
        mutable std::unordered_map<std::string, Group> groups;
        // private method
        int pos_index(X x, Y y) const { 
            if(x >= 0 && x < x_size && y >= 0 && y < y_size) {
                return y * x_size + x;
            }
            throw BadPositionException(x, y, "index out of range");
        }
        Examinee& get_examinee(X x, Y y) {
            return examinees[pos_index(x, y)].value();
        }
        void addExamineeToGroups(X x, Y y) {
            Examinee& e = get_examinee(x, y);
            for(auto& group_pair: groupingFunctions) {
                groups[group_pair.first][group_pair.second(e)].insert( { std::tuple{x, y}, e } );
            }
        }
        void removeExamineeFromGroups(X x, Y y) {
            Examinee& e = get_examinee(x, y);
            for(auto& group_pair: groupingFunctions) {
                groups[group_pair.first][group_pair.second(e)].erase(std::tuple{x, y});
            }            
        }
    public:
        ExamHall(X x, Y y, Grouping<Examinee> groupingFunctions) noexcept
        : examinees(x*y), x_size(x), y_size(y), groupingFunctions(std::move(groupingFunctions)) {}

        void sit(X x, Y y, Examinee e) noexcept(false) {
            auto& seat = examinees[pos_index(x, y)];
            if(seat) {
                throw BadPositionException(x, y, "occupied sit");
            }
            seat = std::move(e);
            addExamineeToGroups(x, y);
        }

        Examinee unsit(X x, Y y) noexcept(false) {
            auto& seat = examinees[pos_index(x, y)];
            if(!seat) {
                throw BadPositionException(x, y, "no examinee to unsit");
            }
            removeExamineeFromGroups(x, y);
            auto examinee = std::optional<Examinee> {}; // empty seat
            // swap empty seat with real examinee and return the real examinee
            // might be more efficient than creating a temp copy
            std::swap(seat, examinee); // seat would become empty, examinee would get the real examinee
            return examinee.value();
        }
        void move(X from_x, Y from_y, X to_x, Y to_y) noexcept(false) {
            // note that this implementation is problematic - if to "to" location is bad
            // an excpetion would be thrown after examinee already unsitted from original seat
            // leaving us in an invalidated state
            // TODO: fix above issue
            sit(to_x, to_y, unsit(from_x, from_y));
        }

        GroupView getExamineesViewByGroup(const std::string& groupingName, const std::string& groupName) const {
            auto itr = groups.find(groupingName);
            if(itr == groups.end() && groupingFunctions.find(groupingName) != groupingFunctions.end()) {
                // for use of insert and tie, see:
                // 1. https://en.cppreference.com/w/cpp/utility/tuple/tie
                // 2. https://en.cppreference.com/w/cpp/container/unordered_map/insert
                std::tie(itr, std::ignore) = groups.insert({groupingName, Group{}});
                //--------------------------------------------------------------------
                // OR, with auto tuple unpack
                // see: https://en.cppreference.com/w/cpp/language/structured_binding
                //--------------------------------------------------------------------
                // auto [insert_itr, _] = groups.insert({groupingName, Group{}});
                // itr = insert_itr;
            }
            if(itr != groups.end()) {
                const auto& grouping = itr->second;
                auto itr2 = grouping.find(groupName);
                if(itr2 == grouping.end()) {
                    std::tie(itr2, std::ignore) = itr->second.insert({groupName, Pos2Examinee{}});
                    //--------------------------------------------------------------------
                    // OR, with auto tuple unpack
                    //--------------------------------------------------------------------
                    // auto [insert_itr, _] = itr->second.insert({groupName, Pos2Examinee{}});
                    // itr2 = insert_itr;
                }
                return GroupView { itr2->second };
            }
            return GroupView { 0 };
        }

        iterator begin() const {
            return { examinees.begin(), examinees.end() };
        }
        iterator end() const {
            return { examinees.end(), examinees.end() };
        }
        
        //-------------------------------------------------------
        // Note:
        //-------------------------------------------------------
        // relying on move to keep validity of references to items in container
        // this is not guarenteed by the spec (yet) but there is almost no way
        // not to support that, as swap(vec1, vec2) must guarentee validity of references
        // see: https://stackoverflow.com/questions/11021764/does-moving-a-vector-invalidate-iterators
        // and: https://stackoverflow.com/questions/25347599/am-i-guaranteed-that-pointers-to-stdvector-elements-are-valid-after-the-vector
        // see also - spec open proposal: http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-active.html#2321
        //-------------------------------------------------------
        ExamHall(const ExamHall&) = delete;
        ExamHall& operator=(const ExamHall&) = delete;
        ExamHall(ExamHall&&) = default;
        ExamHall& operator=(ExamHall&&) = default;
        //-------------------------------------------------------
    };
}

//-------------------------------------
// TESTS
//-------------------------------------

// #include "ExamHall.h" -- if separated to different files
#include <string>
#include <iostream>
#include <type_traits>

using namespace exams;

template<typename ExamHallType>
void test_move_assignment_operator() {
	// create simple ExamHall with no groupings
	ExamHallType examHall{ X{4}, Y{8}, {} };

    // inner scope
    {
    	// create ExamHall2
    	Grouping<std::string> groupingFunctions = {
    		{ "first_letter",
    			[](const std::string& s){ return std::string(1, s[0]); }
    		}
    	};
    	ExamHallType examHall2{ X{5}, Y{12}, groupingFunctions };
    	examHall2.sit(X{1}, Y{2}, "galit");
        examHall2.unsit(X{1}, Y{2}); // unsit galit
	    examHall2.sit(X{1}, Y{2}, "goni");
    	// move assignment - do not use examHall2 after next line:
        examHall = std::move(examHall2);
        examHall.unsit(X{1}, Y{2}); // unsit goni
	    examHall.sit(X{1}, Y{2}, "gideon");
    }
    
	auto view_g = examHall.getExamineesViewByGroup("first_letter", "g");

    auto& from_hall = *examHall.begin();
    auto& from_view = view_g.begin()->second;
    if(from_hall != from_view) {
        std::cout << "move assignment test failed: view returned '" << from_view << "' expecting: '" << from_hall << "'" << std::endl;
    }
    else if(&from_hall != &from_view) {
        std::cout << "move assignment test failed: address from view: " << &from_view << " expecting: " << &from_hall << "'" << std::endl;
    }
}

template<typename ExamHallType>
void test_assignment_operator() {
	// create simple ExamHall with no groupings
	ExamHallType examHall{ X{4}, Y{8}, {} };

    // inner scope
    {
    	// create ExamHall2
    	Grouping<std::string> groupingFunctions = {
    		{ "first_letter",
    			[](const std::string& s){ return std::string(1, s[0]); }
    		}
    	};
	    ExamHallType examHall2{ X{5}, Y{12}, groupingFunctions };
    	examHall2.sit(X{1}, Y{2}, "galit");
    	// assignment:
        examHall = examHall2;        
        examHall2.unsit(X{1}, Y{2}); // unsit galit
    	examHall2.sit(X{1}, Y{2}, "goni");
    }
    
	auto view_g = examHall.getExamineesViewByGroup("first_letter", "g");

    auto& from_hall = *examHall.begin();
    auto& from_view = view_g.begin()->second;
    if(from_hall != from_view) {
        std::cout << "assignment test failed: view returned '" << from_view << "' expecting: '" << from_hall << "'" << std::endl;
    }
    else if(&from_hall != &from_view) {
        std::cout << "assignment test failed: address from view: " << &from_view << " expecting: " << &from_hall << "'" << std::endl;
    }
}

template<typename ExamHallType>
void test_copy_constructor() {
	// create simple ExamHall with no groupings
	Grouping<std::string> groupingFunctions = {
		{ "first_letter",
			[](const std::string& s){ return std::string(1, s[0]); }
		}
	};
    ExamHallType examHall{ X{5}, Y{12}, groupingFunctions };
	examHall.sit(X{1}, Y{2}, "galit");

	// create ExamHall2 with copy ctor
	ExamHallType examHall2 (examHall);

    examHall.unsit(X{1}, Y{2}); // unsit galit
	examHall.sit(X{1}, Y{2}, "goni");
    
	auto view_g = examHall2.getExamineesViewByGroup("first_letter", "g");

    auto& from_hall = *examHall2.begin();
    auto& from_view = view_g.begin()->second;
    if(from_hall != from_view) {
        std::cout << "copy ctor test failed: view returned '" << from_view << "' expecting: '" << from_hall << "'" << std::endl;
    }
    else if(&from_hall != &from_view) {
        std::cout << "copy ctor test failed: address from view: " << &from_view << " expecting: " << &from_hall << "'" << std::endl;
    }
}

template<typename ExamHallType>
void test_move_constructor() {
	// create simple ExamHall with no groupings
	Grouping<std::string> groupingFunctions = {
		{ "first_letter",
			[](const std::string& s){ return std::string(1, s[0]); }
		}
	};
    ExamHallType examHall{ X{5}, Y{12}, groupingFunctions };
	examHall.sit(X{1}, Y{2}, "galit");
    examHall.unsit(X{1}, Y{2}); // unsit galit
	examHall.sit(X{1}, Y{2}, "goni");
    
	// create ExamHall2 with move ctor - do not use examHall after next line:
	ExamHallType examHall2 (std::move(examHall));
    examHall2.unsit(X{1}, Y{2}); // unsit goni
	examHall2.sit(X{1}, Y{2}, "gideon");

	auto view_g = examHall2.getExamineesViewByGroup("first_letter", "g");

    auto& from_hall = *examHall2.begin();
    auto& from_view = view_g.begin()->second;
    if(from_hall != from_view) {
        std::cout << "move ctor test failed: view returned '" << from_view << "' expecting: '" << from_hall << "'" << std::endl;
    }
    else if(&from_hall != &from_view) {
        std::cout << "move ctor test failed: address from view: " << &from_view << " expecting: " << &from_hall << "'" << std::endl;
    }
}

void bad_cases() {
	// create simple ExamHall with no groupings
	ExamHall<std::string> examHall{ X{4}, Y{8}, {} };
	// sit a student
	examHall.sit(X{2}, Y{6}, "Danit");

	// bad path - sitting on an occupied sit
	bool passed_occupied_sit = false;
	try {
        // sit already occupied
		examHall.sit(X{2}, Y{6}, "David");
	} catch(BadPositionException& e) {
    	passed_occupied_sit = true;
	}
	if(!passed_occupied_sit) {
        std::cout << "failed occupied sit test";
	}

	// occupy a sit, to have one examinee in
	examHall.sit(X{2}, Y{7}, "David");

	// bad unsit - no examinee at location
	bool passed_bad_unsit = false;
	try {
		std::string examinee = examHall.unsit(X{1}, Y{1});
	} catch(BadPositionException& e) {
        passed_bad_unsit = true;
	}
	if(!passed_bad_unsit) {
        std::cout << "failed bad unsit test";
	}

	// bad sit - wrong index
	bool passed_wrong_index = false;
	try {
		examHall.sit(X{1}, Y{8}, "Yossi");
	} catch(BadPositionException& e) {
        passed_wrong_index = true;
	}
	if(!passed_wrong_index) {
        std::cout << "failed wrong index test";
	}
}

void simple_happy_path() {
	// create grouping pairs
	Grouping<std::string> groupingFunctions = {
		{ "first_letter",
			[](const std::string& s){ return std::string(1, s[0]); }
		},
		{ "first_letter_toupper",
			[](const std::string& s){ return std::string(1, char(std::toupper(s[0]))); }
		}
	};
	// create ExamHall
	ExamHall<std::string> examHall2{ X{5}, Y{12}, groupingFunctions };
	// sit examinees
	examHall2.sit(X{0}, Y{0}, "John");
	examHall2.sit(X{1}, Y{1}, "deer");
	examHall2.sit(X{1}, Y{2}, "Dove");

	auto view_d = examHall2.getExamineesViewByGroup("first_letter", "d");
	auto view_Dd = examHall2.getExamineesViewByGroup("first_letter_toupper", "D");

	examHall2.sit(X{2}, Y{2}, "david");
	
	// loop on all examinees - John, deer, Dove, david - in some undefined order
	std::cout << "expected: John, deer, Dove, david - in some undefined order -- ";
	for(const auto& examinee : examHall2) {
        std::cout << examinee << ' ';
    }
    std::cout << std::endl;
    std::cout << std::endl;

	// loop on view_d:	{X{1}, Y{1}, deer},
	// 					{X{2}, Y{2}, david}
	// - in some undefined order
	for(const auto& examinee_data : view_d) {
        const std::tuple<X, Y>& pos = examinee_data.first;
        std::cout << "X{" << std::get<0>(pos) << "}, "
                  << "Y{" << std::get<1>(pos) << "}, "
                  << examinee_data.second << std::endl;
    }
    std::cout << std::endl;

	// loop on view_Dd:	{X{1}, Y{1}, deer},
	// 					{X{2}, Y{2}, david}
	// 					{X{1}, Y{2}, Dove}
	// - in some undefined order
	for(const auto& examinee_data : view_Dd) {
        const std::tuple<X, Y>& pos = examinee_data.first;
        std::cout << "X{" << std::get<0>(pos) << "}, "
                  << "Y{" << std::get<1>(pos) << "}, "
                  << examinee_data.second << std::endl;
    }
    std::cout << std::endl;
}

void creating_view_on_empty_hall() {
	// create simple ExamHall with no groupings
	Grouping<std::string> groupingFunctions = {
		{ "first_letter",
			[](const std::string& s){ return std::string(1, s[0]); }
		}
	};
    ExamHall<std::string> examHall{ X{5}, Y{12}, groupingFunctions };
    
	auto view_g = examHall.getExamineesViewByGroup("first_letter", "g");
	examHall.sit(X{1}, Y{1}, "galit");
	examHall.sit(X{1}, Y{2}, "goni");

    for(const auto& data : view_g) {
        std::cout << data.second << std::endl;
    }
}

void test_copy_ctor_assignment_move() {
    if constexpr(!std::is_copy_constructible_v<ExamHall<std::string>>) {
        std::cout << "copy ctor is blocked - check if there is a reason for that" << std::endl;
    }
    else {
        test_copy_constructor<ExamHall<std::string>>();
        std::cout << "passed test_copy_constructor" << std::endl;
    }
    if constexpr(!std::is_assignable_v<ExamHall<std::string>&, ExamHall<std::string>&>) {
        std::cout << "assignment operator is blocked - check if there is a reason for that" << std::endl;
    }
    else {
        test_assignment_operator<ExamHall<std::string>>();
        std::cout << "passed test_assignment_operator" << std::endl;
    }
    if constexpr(!std::is_move_constructible_v<ExamHall<std::string>>) {
        std::cout << "move ctor is blocked - check if there is a reason for that" << std::endl;
    }
    else {
        test_move_constructor<ExamHall<std::string>>();
        std::cout << "passed test_move_constructor" << std::endl;
    }
    if constexpr(!std::is_move_assignable_v<ExamHall<std::string>>) {
        std::cout << "move assignment operator is blocked - check if there is a reason for that" << std::endl;
    }
    else {
        test_move_assignment_operator<ExamHall<std::string>>();
        std::cout << "passed test_move_assignment_operator" << std::endl;
    }
}

//-------------------------------------
// main.cpp
//-------------------------------------

// #include "ExamHall.h" -- if separated to different files
#include <string>
#include <iostream>

using namespace exams;

//-------------------------------------
// M A I N
//-------------------------------------
int main() {
	bad_cases();
	simple_happy_path();	
	creating_view_on_empty_hall();
	test_copy_ctor_assignment_move();
}
