## Typerphase

在Typerphase中会尽可能的推测各节点的类型，它由OptimizeGraph运行。
```
bool PipelineImpl::OptimizeGraph(Linkage* linkage) {
  PipelineData* data = this->data_;

  data->BeginPhaseKind("V8.TFLowering");

  // Type the graph and keep the Typer running such that new nodes get
  // automatically typed when they are created.
  Run<TyperPhase>(data->CreateTyper());
  RunPrintAndVerify(TyperPhase::phase_name());
```

我们直接去看TyperPhase：

```
struct TyperPhase {
  DECL_PIPELINE_PHASE_CONSTANTS(Typer)

  void Run(PipelineData* data, Zone* temp_zone, Typer* typer) {
    NodeVector roots(temp_zone);
    data->jsgraph()->GetCachedNodes(&roots);

    // Make sure we always type True and False. Needed for escape analysis.
    roots.push_back(data->jsgraph()->TrueConstant());
    roots.push_back(data->jsgraph()->FalseConstant());

    LoopVariableOptimizer induction_vars(data->jsgraph()->graph(),
                                         data->common(), temp_zone);
    if (FLAG_turbo_loop_variable) induction_vars.Run();

    // The typer inspects heap objects, so we need to unpark the local heap.
    UnparkedScopeIfNeeded scope(data->broker());
    typer->Run(roots, &induction_vars); //对图中各结点做处理
  }
};
```

我们来看看typer->Run
```
void Typer::Run(const NodeVector& roots,
                LoopVariableOptimizer* induction_vars) {
  if (induction_vars != nullptr) { //对于可优化的loop结点，将其处理为phi结点
    induction_vars->ChangeToInductionVariablePhis();
  }
  Visitor visitor(this, induction_vars);
  GraphReducer graph_reducer(zone(), graph(), tick_counter_, broker());
  graph_reducer.AddReducer(&visitor); //将visitor加入到reducers_向量
  for (Node* const root : roots) graph_reducer.ReduceNode(root); //访问图形的每一个节点，并试图减少他们
  graph_reducer.ReduceGraph(); //对图进行优化，深度优先从end结点开始

  if (induction_vars != nullptr) {
    // Validate the types computed by TypeInductionVariablePhi.
    for (auto entry : induction_vars->induction_variables()) {
      InductionVariable* induction_var = entry.second;
      if (induction_var->phi()->opcode() == IrOpcode::kInductionVariablePhi) {
        CHECK(visitor.InductionVariablePhiTypeIsPrefixedPoint(induction_var));
      }
    }

    induction_vars->ChangeToPhisAndInsertGuards();
  }
}

---------------------

void GraphReducer::AddReducer(Reducer* reducer) {
  reducers_.push_back(reducer);
}
```
graph_reducer在上篇文章中有过详细分析了，这里就不重复了。

```
void GraphReducer::ReduceTop() {
  
  ....
  
  // All inputs should be visited or on stack. Apply reductions to node.
  Reduction reduction = Reduce(node);

  ....
}
```
我们直接跳转到ReduceTop中真正优化的地方Reduce(node)。（这里不明白的去看上篇文章）。


```
Reduction GraphReducer::Reduce(Node* const node) {
  auto skip = reducers_.end();
  for (auto i = reducers_.begin(); i != reducers_.end();) {
    if (i != skip) {
      tick_counter_->TickAndMaybeEnterSafepoint();
      Reduction reduction = (*i)->Reduce(node, observe_node_manager_);
      
      ....

---------------------

class Typer::Visitor : public Reducer {
    Reduction Reduce(Node* node) override {
        if (node->op()->ValueOutputCount() == 0) return NoChange();
            return UpdateType(node, TypeNode(node));
     }

---------------------

  Type TypeNode(Node* node) {
    switch (node->opcode()) {
#define DECLARE_UNARY_CASE(x, ...) \
  case IrOpcode::k##x:             \
    return Type##x(Operand(node, 0));
      JS_SIMPLE_UNOP_LIST(DECLARE_UNARY_CASE)
      SIMPLIFIED_NUMBER_UNOP_LIST(DECLARE_UNARY_CASE)
      SIMPLIFIED_BIGINT_UNOP_LIST(DECLARE_UNARY_CASE)
      SIMPLIFIED_SPECULATIVE_NUMBER_UNOP_LIST(DECLARE_UNARY_CASE)
      SIMPLIFIED_SPECULATIVE_BIGINT_UNOP_LIST(DECLARE_UNARY_CASE)
#undef DECLARE_UNARY_CASE
#define DECLARE_BINARY_CASE(x, ...) \
  case IrOpcode::k##x:              \
    return Type##x(Operand(node, 0), Operand(node, 1));
      JS_SIMPLE_BINOP_LIST(DECLARE_BINARY_CASE)
      SIMPLIFIED_NUMBER_BINOP_LIST(DECLARE_BINARY_CASE)
      SIMPLIFIED_BIGINT_BINOP_LIST(DECLARE_BINARY_CASE)
      SIMPLIFIED_SPECULATIVE_NUMBER_BINOP_LIST(DECLARE_BINARY_CASE)
      SIMPLIFIED_SPECULATIVE_BIGINT_BINOP_LIST(DECLARE_BINARY_CASE)
#undef DECLARE_BINARY_CASE

....

```
Reduce会调用储存在reducers_向量中的Reducer来进行优化，此处是Visitor。
UpdateType(node, TypeNode(node));最终会调用TypeNode，可以看到里面是很多宏定义的switch，这里就不展开了，我们拿几个比较常见的来举个例子：

#### JSCall
```
Type Typer::Visitor::TypeJSCall(Node* node) {
  // TODO(bmeurer): We could infer better types if we wouldn't ignore the
  // argument types for the JSCallTyper above.
  return TypeUnaryOp(node, JSCallTyper);
}

---------------------

Type Typer::Visitor::JSCallTyper(Type fun, Typer* t) {
  if (!fun.IsHeapConstant() || !fun.AsHeapConstant()->Ref().IsJSFunction()) { //如果不是HeapConstant或者不是JSFunction
    return Type::NonInternal(); //返回非内置
  } 
  JSFunctionRef function = fun.AsHeapConstant()->Ref().AsJSFunction();
  if (!function.serialized()) { //如果func非序列化
    TRACE_BROKER_MISSING(t->broker(), "data for function " << function);
    return Type::NonInternal();  //返回非内置
  }
  if (!function.shared().HasBuiltinId()) { //如果func非内置函数
    return Type::NonInternal();  //返回非内置
  }
  switch (function.shared().builtin_id()) { //func为内置函数，根据不同的内置函数返回对应的type
    case Builtins::kMathRandom:
      return Type::PlainNumber();
    case Builtins::kMathFloor:
    case Builtins::kMathCeil:
    case Builtins::kMathRound:
    case Builtins::kMathTrunc:
      return t->cache_->kIntegerOrMinusZeroOrNaN;
    ....
    default:
      return Type::NonInternal();
  }
}
```
由上面的代码可知：如果调用的函数是一个内置函数时，他就会将一个类型与它关联起来，比如调用了MathRandom内置函数，就会返回Type::PlainNumber类型。


#### NumberConstant

```
Type Typer::Visitor::TypeNumberConstant(Node* node) {
  double number = OpParameter<double>(node->op());
  return Type::Constant(number, zone());
}

---------------------

Type Type::Constant(double value, Zone* zone) {
  if (RangeType::IsInteger(value)) {  //如果是整数，返回range
    return Range(value, value, zone);
  } else if (IsMinusZero(value)) {  //如果是MinusZero(-0)，返回MinusZero
    return Type::MinusZero();
  } else if (std::isnan(value)) {  //如果是nan，返回NAN
    return Type::NaN();
  }

  DCHECK(OtherNumberConstantType::IsOtherNumberConstant(value));
  return OtherNumberConstant(value, zone);
}
```

#### SpeculativeNumberAdd

```
#define SPECULATIVE_NUMBER_BINOP(Name)                         \
  Type OperationTyper::Speculative##Name(Type lhs, Type rhs) { \
    lhs = SpeculativeToNumber(lhs);                            \
    rhs = SpeculativeToNumber(rhs);                            \
    return Name(lhs, rhs);                                     \
  }
  
  ---------------------
  
  Type OperationTyper::SpeculativeToNumber(Type type) {
  return ToNumber(Type::Intersect(type, Type::NumberOrOddball(), zone()));
}
```
在OperationTyper::Speculative##Name中，通过SpeculativeToNumber来获得操作的左右值，为了保持简单，任何类型的Type::Number将保持相同的类型(一个PlainNumber就是一个数字，它将保持为一个PlainNumber)。Range(n,n)类型也将成为一个数字，因此我们最终调用NumberAdd对两个数字进行操作。

OperationTyper::Speculative##Name最终将SpeculativeNumberAdd处理为了NumberAdd(lhs, rhs)。
```
Type OperationTyper::NumberAdd(Type lhs, Type rhs) {
  DCHECK(lhs.Is(Type::Number()));
  DCHECK(rhs.Is(Type::Number()));

  if (lhs.IsNone() || rhs.IsNone()) return Type::None(); //判断左右值是否为空

  // Addition can return NaN if either input can be NaN or we try to compute
  // the sum of two infinities of opposite sign.
  bool maybe_nan = lhs.Maybe(Type::NaN()) || rhs.Maybe(Type::NaN()); //判断左右值是否为NAN

  // Addition can yield minus zero only if both inputs can be minus zero.
  //只有当两个输入都为负零时，加法才能产生负零时。
  bool maybe_minuszero = true;
  if (lhs.Maybe(Type::MinusZero())) {  //左值是否为-0
    lhs = Type::Union(lhs, cache_->kSingletonZero, zone());
  } else {
    maybe_minuszero = false;
  }
  if (rhs.Maybe(Type::MinusZero())) {  //右值是否为-0
    rhs = Type::Union(rhs, cache_->kSingletonZero, zone());
  } else {
    maybe_minuszero = false;
  }

  // We can give more precise types for integers.
  Type type = Type::None();
  lhs = Type::Intersect(lhs, Type::PlainNumber(), zone());
  rhs = Type::Intersect(rhs, Type::PlainNumber(), zone());
  if (!lhs.IsNone() && !rhs.IsNone()) { //左右值不为空
    if (lhs.Is(cache_->kInteger) && rhs.Is(cache_->kInteger)) { //左右值都为整数
      type = AddRanger(lhs.Min(), lhs.Max(), rhs.Min(), rhs.Max());
    } else {
      if ((lhs.Maybe(minus_infinity_) && rhs.Maybe(infinity_)) ||
          (rhs.Maybe(minus_infinity_) && lhs.Maybe(infinity_))) { //左右值为正负无穷
        maybe_nan = true;
      }
      type = Type::PlainNumber(); //无特殊情况时的返回值
    }
  }

  // Take into account the -0 and NaN information computed earlier.
  if (maybe_minuszero) type = Type::Union(type, Type::MinusZero(), zone());
  if (maybe_nan) type = Type::Union(type, Type::NaN(), zone());
  return type;
}
```
在左右值都为整数时，通过AddRanger来计算范围：
```
Type OperationTyper::AddRanger(double lhs_min, double lhs_max, double rhs_min,
                               double rhs_max) {
  double results[4];
  results[0] = lhs_min + rhs_min;
  results[1] = lhs_min + rhs_max;
  results[2] = lhs_max + rhs_min;
  results[3] = lhs_max + rhs_max;
  // Since none of the inputs can be -0, the result cannot be -0 either.
  // However, it can be nan (the sum of two infinities of opposite sign).
  // On the other hand, if none of the "results" above is nan, then the
  // actual result cannot be nan either.
  int nans = 0;
  for (int i = 0; i < 4; ++i) {
    if (std::isnan(results[i])) ++nans;
  }
  if (nans == 4) return Type::NaN();
  Type type = Type::Range(array_min(results, 4), array_max(results, 4), zone());
  if (nans > 0) type = Type::Union(type, Type::NaN(), zone());
  // Examples:
  //   [-inf, -inf] + [+inf, +inf] = NaN
  //   [-inf, -inf] + [n, +inf] = [-inf, -inf] \/ NaN
  //   [-inf, +inf] + [n, +inf] = [-inf, +inf] \/ NaN
  //   [-inf, m] + [n, +inf] = [-inf, +inf] \/ NaN
  return type;
}
```

#### TypeInductionVariablePhi

他和循环变量(induction variables)有关，这部分请移步之前的cve分析：

https://github.com/yytgravity/Daily-learning-record/tree/master/cve/v8/issue%202013

