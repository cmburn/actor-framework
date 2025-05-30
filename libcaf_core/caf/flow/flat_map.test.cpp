// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/main/LICENSE.

#include "caf/test/fixture/deterministic.hpp"
#include "caf/test/fixture/flow.hpp"
#include "caf/test/scenario.hpp"
#include "caf/test/test.hpp"

#include "caf/actor_from_state.hpp"
#include "caf/defaults.hpp"
#include "caf/event_based_actor.hpp"
#include "caf/scheduled_actor/flow.hpp"
#include "caf/stateful_actor.hpp"

using namespace caf;

namespace {

class flat_adder_state {
public:
  static inline const char* name = "adder";

  explicit flat_adder_state(int32_t x) : x(x) {
    // nop
  }

  caf::behavior make_behavior() {
    return {
      [this](int32_t y) { return x + y; },
    };
  }

  int32_t x;
};

struct fixture : test::fixture::flow, test::fixture::deterministic {};

} // namespace

WITH_FIXTURE(fixture) {

SCENARIO("flat_map merges multiple observables") {
  using i32_list = std::vector<int32_t>;
  GIVEN("a generation that emits lists for flat_map") {
    WHEN("lifting each list to an observable with flat_map") {
      THEN("the observer receives values from all observables") {
        auto inputs = std::vector<i32_list>{
          i32_list{1},
          i32_list{2, 2},
          i32_list{3, 3, 3},
        };
        auto outputs
          = collect(make_observable().from_container(inputs).flat_map(
            [this](const i32_list& x) {
              return make_observable().from_container(x);
            }));
        if (check(outputs.has_value())) {
          std::sort((*outputs).begin(), (*outputs).end());
          auto expected_outputs = i32_list{1, 2, 2, 3, 3, 3};
          check_eq(outputs, expected_outputs);
        }
      }
    }
  }
  GIVEN("a generation that emits 10 integers for flat_map") {
    WHEN("sending a request for each each integer for flat_map") {
      THEN("flat_map merges the responses") {
        auto outputs = i32_list{};
        auto inputs = i32_list(10);
        std::iota(inputs.begin(), inputs.end(), 0);
        auto adder = sys.spawn(actor_from_state<flat_adder_state>, 1);
        auto [self, launch] = sys.spawn_inactive();
        self->make_observable()
          .from_container(inputs)
          .flat_map([self = self, adder](int32_t x) {
            return self->mail(x)
              .request(adder, infinite)
              .as_observable<int32_t>();
          })
          .for_each([&outputs](int32_t x) { outputs.emplace_back(x); });
        launch();
        dispatch_messages();
        std::sort(outputs.begin(), outputs.end());
        auto expected_outputs = i32_list{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        check_eq(outputs, expected_outputs);
      }
    }
  }
}

TEST("the merge operator allows setting a maximum concurrency") {
  using snk_t = flow::auto_observer<int>;
  auto to_iota = [this](int start) { return make_observable().iota(start); };
  auto in = make_observable().iota(1);
  auto snk = coordinator()->add_child(std::in_place_type<snk_t>);
  SECTION("merging multiple observables") {
    auto uut = std::move(in).flat_map(to_iota);
    auto sub = uut.subscribe(snk->as_observer());
    auto* ptr = dynamic_cast<caf::flow::op::merge_sub<int>*>(sub.ptr());
    check_eq(ptr->max_concurrent(), defaults::flow::max_concurrent);
    sub.dispose();
  }
  SECTION("merging multiple observables") {
    auto uut = std::move(in).flat_map(to_iota, 17u);
    auto sub = uut.subscribe(snk->as_observer());
    auto* ptr = dynamic_cast<caf::flow::op::merge_sub<int>*>(sub.ptr());
    check_eq(ptr->max_concurrent(), 17u);
    sub.dispose();
  }
  run_flows();
}

} // WITH_FIXTURE(fixture)
