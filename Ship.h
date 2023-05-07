// =====================================
// Ship - Assignment 4
// Sukesh Cheripalli, Puneet Udupi
// =====================================
#include <functional>
#include <iostream>
#include <list>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace shipping {
template <typename T> class NamedType {
  T t;

public:
  explicit NamedType(T t) : t(t) {}
  operator T() const { return t; }
};

struct X : NamedType<int> {
  using NamedType<int>::NamedType;
};

struct Y : NamedType<int> {
  using NamedType<int>::NamedType;
};

struct Height : NamedType<int> {
  using NamedType<int>::NamedType;
};

using Position = std::tuple<shipping::X, shipping::Y>;
using Position3D = std::tuple<shipping::X, shipping::Y, shipping::Height>;
} // namespace shipping

// a short pause from shipping to define hash for Position
namespace std {
template <> struct hash<shipping::Position> {
  std::size_t operator()(const shipping::Position &pos) const noexcept {
    return std::get<0>(pos) ^ (std::get<1>(pos) << 1);
  }
};
template <> struct hash<shipping::Position3D> {
  std::size_t operator()(const shipping::Position3D &pos) const noexcept {
    return std::get<0>(pos) ^ (std::get<1>(pos) << 1) ^ (std::get<2>(pos) << 2);
  }
};

} // namespace std

// back to the shipping business
namespace shipping {
class BadShipOperationException {
  // X x;
  // Y y;
  std::string msg;

public:
  BadShipOperationException(std::string msg) : msg(std::move(msg)) {}
  void print() const { std::cout << msg << std::endl; }
};

template <typename Container>
using Grouping =
    std::unordered_map<std::string,
                       std::function<std::string(const Container &)>>;

template <typename Container> class Ship {
  class GroupView {
    const std::unordered_map<Position3D, const Container &> *p_group = nullptr;
    using IteratorType =
        typename std::unordered_map<Position3D,
                                    const Container &>::const_iterator;

  public:
    GroupView(const std::unordered_map<Position3D, const Container &> &group)
        : p_group(&group) {}
    GroupView(int) {}
    auto begin() const { return p_group ? p_group->begin() : IteratorType{}; }
    auto end() const { return p_group ? p_group->end() : IteratorType{}; }
  };

  class GroupIterator {
    using ContainersItr =
        typename std::vector<std::optional<Container>>::const_iterator;
    ContainersItr containers_itr;
    ContainersItr containers_end;
    void set_itr_to_occupied_load() {
      while (containers_itr != containers_end && !(*containers_itr)) {
        ++containers_itr;
      }
    }

  public:
    GroupIterator(ContainersItr containers_itr, ContainersItr containers_end)
        : containers_itr(containers_itr), containers_end(containers_end) {
      set_itr_to_occupied_load();
    }
    GroupIterator operator++() {
      ++containers_itr;
      set_itr_to_occupied_load();
      return *this;
    }
    const Container &operator*() const { return containers_itr->value(); }
    bool operator!=(GroupIterator other) const {
      return containers_itr != other.containers_itr;
    }
  };

  class PositionIterator {
    using ContainersItr = typename std::list<
        std::reference_wrapper<const Container>>::const_iterator;
    ContainersItr containers_itr_;

  public:
    PositionIterator(){};
    PositionIterator(ContainersItr containers_itr)
        : containers_itr_(containers_itr) {}
    PositionIterator operator++(int) {
      PositionIterator temp_obj = *this;
      ++containers_itr_;
      return temp_obj;
    }
    PositionIterator &operator++() {
      ++containers_itr_;
      return *this;
    }
    const Container &operator*() const {
      return (const Container &)(*containers_itr_);
    }
    bool operator!=(PositionIterator other) const {
      return containers_itr_ != other.containers_itr_;
    }
  };

  class PositionView {
    std::list<std::reference_wrapper<const Container>> *pview_list_ = nullptr;
    using IteratorType = typename std::list<
        std::reference_wrapper<const Container>>::const_iterator;

  public:
    PositionView(std::list<std::reference_wrapper<const Container>> &pview_list)
        : pview_list_(&pview_list) {}
    PositionView(int) {}
    auto begin() const {
      return pview_list_ ? PositionIterator{pview_list_->begin()}
                         : PositionIterator{};
    }
    auto end() const {
      return pview_list_ ? PositionIterator{pview_list_->end()}
                         : PositionIterator{};
    }
  };

  std::vector<std::optional<Container>> stacked_containers;
  std::vector<size_t> stacked_compartment_sizes;
  std::unordered_map<shipping::Position, int> restrictions_;
  int x_size;
  int y_size;
  int h_size;

  Grouping<Container> groupingFunctions_;
  using Pos3D2Container = std::unordered_map<Position3D, const Container &>;
  using Group = std::unordered_map<std::string, Pos3D2Container>;
  // all groupings by their grouping name
  mutable std::unordered_map<std::string, Group> groups;
  mutable std::vector<std::list<std::reference_wrapper<const Container>>>
      position_list_;
  //   private method
  int pos_index(X x, Y y, Height z) const {
    if (x >= 0 && x < x_size && y >= 0 && y < y_size && z >= 0 && z < h_size) {
      return (z * x_size * y_size) + y * x_size + x;
    }
    throw BadShipOperationException(
        std::to_string(__LINE__) + " : " + std::to_string(x) + "," +
        std::to_string(y) + "," + std::to_string(z) + ": index out of range");
  }
  int pos_index(X x, Y y) const {
    if (x >= 0 && x < x_size && y >= 0 && y < y_size) {
      return y * x_size + x;
    }
    throw BadShipOperationException(std::to_string(__LINE__) + " : " +
                                    std::to_string(x) + "," +
                                    std::to_string(y) + ": index out of range");
  }
  Container &get_container(X x, Y y) {
    return stacked_containers
        [pos_index(x, y,
                   (Height)(stacked_compartment_sizes[pos_index(x, y)] - 1))]
            .value();
  }
  Container &get_container(X x, Y y, Height z) {
    return stacked_containers[pos_index(x, y, z)].value();
  }
  void addContainerToGroups(X x, Y y, Height z) {
    Container &e = get_container(x, y, z);
    // std::cout << groupingFunctions_.size() << std::endl;
    for (auto &group_pair : groupingFunctions_) {
      groups[group_pair.first][group_pair.second(e)].insert(
          {std::tuple{x, y, z}, e});
    }
  }
  void removeContainerFromGroups(X x, Y y, Height z) {
    Container &e = get_container(x, y, z);
    for (auto &group_pair : groupingFunctions_) {
      groups[group_pair.first][group_pair.second(e)].erase(std::tuple{x, y, z});
    }
  }

  void addContainerToPList(X x, Y y, Height z) {
    Container &c = get_container(x, y, z);
    position_list_[pos_index(x, y)].push_front(c);
    // for (auto &test : position_list_[pos_index(x, y)])
    //   std::cout << (std::string)test << " ";
    // std::cout << std::endl;
  }

  void removeContainerFromPList(X x, Y y, Height z) {
    position_list_[pos_index(x, y)].pop_front();
  }

public:
  // TODO: (3) create containers for x*y*h in ctors
  // TODO: (4) implement restrictions

  Ship(X x, Y y, Height max_height) noexcept
      : x_size(x), y_size(y), h_size(max_height),
        stacked_containers(
            std::vector<std::optional<Container>>(x * y * max_height)),
        stacked_compartment_sizes(std::vector<size_t>(x * y, 0)),
        position_list_(
            std::vector<std::list<std::reference_wrapper<const Container>>>(
                x * y)) {}

  Ship(X x, Y y, Height max_height,
       std::vector<std::tuple<X, Y, Height>> restrictions) noexcept(false)
      : Ship(x, y, max_height) {
    for (const auto &restriction : restrictions) {
      shipping::Position current_pos = shipping::Position(
          std::get<0>(restriction), std::get<1>(restriction));
      if (restrictions_.find(current_pos) != restrictions_.end()) {
        throw BadShipOperationException(
            std::to_string(__LINE__) + " : " +
            std::to_string(std::get<0>(restriction)) + "," +
            std::to_string(std::get<1>(restriction)) +
            ": duplicate restrictions");
      }
      if (std::get<0>(restriction) >= x || std::get<1>(restriction) >= y ||
          std::get<2>(restriction) >= max_height) {
        throw BadShipOperationException(
            std::to_string(__LINE__) + " : " +
            std::to_string(std::get<0>(restriction)) + "," +
            std::to_string(std::get<1>(restriction)) + "," +
            std::to_string(std::get<2>(restriction)) + ": Bad restrictions");
      }
      restrictions_[current_pos] = Height{std::get<2>(restriction)};
    }
  }

  Ship(X x, Y y, Height max_height,
       std::vector<std::tuple<X, Y, Height>> restrictions,
       Grouping<Container> groupingFunctions) noexcept(false)
      : Ship(x, y, max_height, restrictions) {
    groupingFunctions_ = std::move(groupingFunctions);
  }

  void load(X x, Y y, Container c) noexcept(false) {
    // TODO: (5) handle height of the container
    shipping::Position current_pos = shipping::Position(x, y);
    auto &current_compartment_size = stacked_compartment_sizes[pos_index(x, y)];
    if (restrictions_.find(current_pos) != restrictions_.end() &&
        restrictions_[current_pos] <= current_compartment_size) {
      throw BadShipOperationException(
          std::to_string(__LINE__) + " : " + std::to_string(x) + "," +
          std::to_string(y) +
          ": has restriction : " + std::to_string(restrictions_[current_pos]));
    }
    // std::cout << "Load:" << __LINE__ << std::endl;
    if (current_compartment_size == h_size) {
      throw BadShipOperationException(
          std::to_string(__LINE__) + " : " + std::to_string(x) + "," +
          std::to_string(y) + ": occupied compartment");
    }

    auto &container =
        stacked_containers[pos_index(x, y, (Height)(current_compartment_size))];

    container = std::move(c);
    addContainerToGroups(x, y, (Height)current_compartment_size);
    addContainerToPList(x, y, (Height)current_compartment_size);
    current_compartment_size++;
  }

  Container unload(X x, Y y) noexcept(false) {
    auto &current_compartment_size = stacked_compartment_sizes[pos_index(x, y)];
    auto unload_index = current_compartment_size - 1;
    if (unload_index == -1) {
      throw BadShipOperationException(
          std::to_string(__LINE__) + " : " + std::to_string(x) + "," +
          std::to_string(y) + ": no container to unload");
    }
    removeContainerFromGroups(x, y, (Height)unload_index);
    removeContainerFromPList(x, y, (Height)unload_index);
    auto &unload_container =
        stacked_containers[pos_index(x, y, (Height)unload_index)];
    auto empty_container = std::optional<Container>{};
    std::swap(unload_container, empty_container);
    current_compartment_size--;
    return empty_container.value();
  }
  void move(X from_x, Y from_y, X to_x, Y to_y) noexcept(false) {
    auto from_container = unload(from_x, from_y);
    try {
      load(to_x, to_y, from_container);
    } catch (const BadShipOperationException &e) {
      load(from_x, from_y, from_container);
      throw e;
    }
  }

  // TODO: (8) verify if this works out of box from ExamHall
  GroupView getContainersViewByGroup(const std::string &groupingName,
                                     const std::string &groupName) const {
    auto itr = groups.find(groupingName);
    if (itr == groups.end() &&
        groupingFunctions_.find(groupingName) != groupingFunctions_.end()) {
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
    if (itr != groups.end()) {
      const auto &grouping = itr->second;
      auto itr2 = grouping.find(groupName);
      if (itr2 == grouping.end()) {
        std::tie(itr2, std::ignore) =
            itr->second.insert({groupName, Pos3D2Container{}});
        //--------------------------------------------------------------------
        // OR, with auto tuple unpack
        //--------------------------------------------------------------------
        // auto [insert_itr, _] = itr->second.insert({groupName,
        // Pos2Container{}}); itr2 = insert_itr;
      }
      return GroupView{itr2->second};
    }
    return GroupView{0};
  }
  // TODO: (9) implement API
  PositionView getContainersViewByPosition(X x, Y y) const {
    try {
      return PositionView{position_list_[pos_index(x, y)]};
    } catch (...) {
      ;
    }
    return PositionView{0};
  }

  GroupIterator begin() const {
    return {stacked_containers.begin(), stacked_containers.end()};
  }
  GroupIterator end() const {
    return {stacked_containers.end(), stacked_containers.end()};
  }

  //-------------------------------------------------------
  // Note:
  //-------------------------------------------------------
  // relying on move to keep validity of references to items in container
  // this is not guarenteed by the spec (yet) but there is almost no way
  // not to support that, as swap(vec1, vec2) must guarentee validity of
  // references see:
  // https://stackoverflow.com/questions/11021764/does-moving-a-vector-invalidate-iterators
  // and:
  // https://stackoverflow.com/questions/25347599/am-i-guaranteed-that-pointers-to-stdvector-elements-are-valid-after-the-vector
  // see also - spec open proposal:
  // http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-active.html#2321
  //-------------------------------------------------------
  Ship(const Ship &) = delete;
  Ship &operator=(const Ship &) = delete;
  Ship(Ship &&) = default;
  Ship &operator=(Ship &&) = default;
  //-------------------------------------------------------
};
} // namespace shipping
