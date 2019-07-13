#include <cstdio>
#include <cstdlib>
#include <tuple>
#include <variant>

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...)->overloaded<Ts...>;

struct EventA {
  const char* msg{nullptr};
};
struct EventB {
  int number{0};
};

using Event = std::variant<EventA, EventB>;

struct Context {
  Context Inc() const { return Context{counter + 1}; }
  int counter = 0;
};

template <class T>
struct identity {
  using type = T;
};

template <class T>
using identity_t = typename identity<T>::type;

template <template <class T> class Base = identity_t, class... Args>
struct SelfReturning {
  using RetType = Base<SelfReturning>;
  using FuncType = RetType (*)(Args... args);

  SelfReturning(FuncType func) : _func{func} {}
  RetType operator()(Args... args) {
    return _func(std::forward<Args>(args)...);
  }

  FuncType _func;

  template <class... AltArgs>
  using WithArgs = SelfReturning<Base, AltArgs...>;
};

template <class T>
using PairWithCtx = std::pair<T, const Context>;

using State = SelfReturning<PairWithCtx>::WithArgs<const Context&, Event>;

State::RetType A(const Context&, Event);
State::RetType B(const Context&, Event);

State::RetType A(const Context& ctx, Event evt) {
  printf("State A, counter = %d\n", ctx.counter);
  return std::visit(overloaded{[&](EventA e) {
                                 if (e.msg != nullptr) {
                                   printf("A message = \"%s\"", e.msg);
                                 } else {
                                   puts("A message = nullptr");
                                 }
                                 return make_pair(A, ctx);
                               },
                               [&](EventB) { return make_pair(B, ctx.Inc()); }},
                    evt);
}

State::RetType B(const Context& ctx, Event evt) {
  printf("State B, counter = %d\n", ctx.counter);
  return std::visit(
      overloaded{[&](EventA e) { return make_pair(A, ctx.Inc()); },
                 [&](EventB e) {
                   printf("B number = %d\n", e.number);
                   return make_pair(B, ctx);
                 }},
      evt);
}

int main() {
  State state = A;
  Context ctx{};
  Event events[] = {EventB{},   EventA{}, EventB{},
                    EventB{10}, EventA{}, EventA{"Hello, world!"}};

  for (auto evt : events) {
    std::tie(state, ctx) = state(ctx, evt);
  }

  return 0;
}
