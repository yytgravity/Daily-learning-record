LoadEliminationPhase:删除不必要的读取和检查

## LoadEliminationPhase

```
struct LoadEliminationPhase {
  DECL_PIPELINE_PHASE_CONSTANTS(LoadElimination)

  void Run(PipelineData* data, Zone* temp_zone) {
    GraphReducer graph_reducer(
        temp_zone, data->graph(), &data->info()->tick_counter(), data->broker(),
        data->jsgraph()->Dead(), data->observe_node_manager());
    BranchElimination branch_condition_elimination(&graph_reducer,
                                                   data->jsgraph(), temp_zone,
                                                   BranchElimination::kEARLY);
    DeadCodeElimination dead_code_elimination(&graph_reducer, data->graph(),
                                              data->common(), temp_zone);
    RedundancyElimination redundancy_elimination(&graph_reducer, temp_zone);
    LoadElimination load_elimination(&graph_reducer, data->jsgraph(),
                                     temp_zone);
    CheckpointElimination checkpoint_elimination(&graph_reducer);
    ValueNumberingReducer value_numbering(temp_zone, data->graph()->zone());
    CommonOperatorReducer common_reducer(&graph_reducer, data->graph(),
                                         data->broker(), data->common(),
                                         data->machine(), temp_zone);
    TypedOptimization typed_optimization(&graph_reducer, data->dependencies(),
                                         data->jsgraph(), data->broker());
    ConstantFoldingReducer constant_folding_reducer(
        &graph_reducer, data->jsgraph(), data->broker());
    TypeNarrowingReducer type_narrowing_reducer(&graph_reducer, data->jsgraph(),
                                                data->broker());

    AddReducer(data, &graph_reducer, &branch_condition_elimination);
    AddReducer(data, &graph_reducer, &dead_code_elimination);
    AddReducer(data, &graph_reducer, &redundancy_elimination);
    AddReducer(data, &graph_reducer, &load_elimination);
    AddReducer(data, &graph_reducer, &type_narrowing_reducer);
    AddReducer(data, &graph_reducer, &constant_folding_reducer);
    AddReducer(data, &graph_reducer, &typed_optimization);
    AddReducer(data, &graph_reducer, &checkpoint_elimination);
    AddReducer(data, &graph_reducer, &common_reducer);
    AddReducer(data, &graph_reducer, &value_numbering);

    // ConstantFoldingReducer and TypedOptimization access the heap.
    UnparkedScopeIfNeeded scope(data->broker());

    graph_reducer.ReduceGraph();
  }
};
```

### BranchElimination

```
Reduction BranchElimination::Reduce(Node* node) {
  switch (node->opcode()) {
    case IrOpcode::kDead:
      return NoChange();
    case IrOpcode::kDeoptimizeIf:
    case IrOpcode::kDeoptimizeUnless:
      return ReduceDeoptimizeConditional(node);
    case IrOpcode::kMerge:
      return ReduceMerge(node);
    case IrOpcode::kLoop:
      return ReduceLoop(node);
    case IrOpcode::kBranch:
      return ReduceBranch(node);
    case IrOpcode::kIfFalse:
      return ReduceIf(node, false);
    case IrOpcode::kIfTrue:
      return ReduceIf(node, true);
    case IrOpcode::kStart:
      return ReduceStart(node);
    default:
      if (node->op()->ControlOutputCount() > 0) {
        return ReduceOtherControl(node);
      }
      break;
  }
  return NoChange();
}
```
reduce主要是通过switch，对不同的node进行处理，我们这里就简单举几个例子：

##### ReduceBranch

```
Reduction BranchElimination::ReduceBranch(Node* node) {
  Node* condition = node->InputAt(0); //value_input
  Node* control_input = NodeProperties::GetControlInput(node, 0); //control_input
  ControlPathConditions from_input = node_conditions_.Get(control_input); //Condition list
  Node* branch;
  bool condition_value;
  // If we know the condition we can discard the branch.
  if (from_input.LookupCondition(condition, &branch, &condition_value)) {
    MarkAsSafetyCheckIfNeeded(branch, node);
    for (Node* const use : node->uses()) {
      switch (use->opcode()) {  //根据LookupCondition判断的结果消除对应的结点
        case IrOpcode::kIfTrue:
          Replace(use, condition_value ? control_input : dead());
          break;
        case IrOpcode::kIfFalse:
          Replace(use, condition_value ? dead() : control_input);
          break;
        default:
          UNREACHABLE();
      }
    }
    return Replace(dead());
  }
  SimplifyBranchCondition(node);
  // Trigger revisits of the IfTrue/IfFalse projections, since they depend on
  // the branch condition.
  for (Node* const use : node->uses()) {
    Revisit(use);
  }
  return TakeConditionsFromFirstControl(node);
}

 ----------------------------

bool BranchElimination::ControlPathConditions::LookupCondition(
    Node* condition, Node** branch, bool* is_true) const {
  for (BranchCondition element : *this) { //遍历condition list，判断ControlInput的Condition list中是否有和value_input相同的情况。
    if (element.condition == condition) {
      *is_true = element.is_true;
      *branch = element.branch;
      return true;
    }
  }
  return false;
}
```
ReduceBranch主要作用是：通过比较control和value结点的condition，在相等的时候去消除对应不满足condition的分支。
 
##### ReduceMerge
```
Reduction BranchElimination::ReduceMerge(Node* node) {
  // Shortcut for the case when we do not know anything about some
  // input.
  Node::Inputs inputs = node->inputs(); //获得merage结点的所有input结点
  for (Node* input : inputs) {
    if (!reduced_.Get(input)) {
      return NoChange();
    }
  }

  auto input_it = inputs.begin();

  DCHECK_GT(inputs.count(), 0);

  ControlPathConditions conditions = node_conditions_.Get(*input_it); //第一个input结点的condition list
  ++input_it;
  // Merge the first input's conditions with the conditions from the other
  // inputs.
  auto input_end = inputs.end(); 
  for (; input_it != input_end; ++input_it) { //迭代遍历
    // Change the current condition list to a longest common tail
    // of this condition list and the other list. (The common tail
    // should correspond to the list from the common dominator.)
    conditions.ResetToCommonAncestor(node_conditions_.Get(*input_it));
  }
  return UpdateConditions(node, conditions);
}


 ----------------------------

 
  // Drop elements until the current stack is equal to the tail shared with
  // {other}. The shared tail must not only be equal, but also refer to the
  // same memory.
  //共享尾部不仅必须相等，而且还必须引用相同的内存。
  void ResetToCommonAncestor(FunctionalList other) {
    while (other.Size() > Size()) other.DropFront(); //当other的长度大于this（conditions）的长度时，other丢弃头部
    while (other.Size() < Size()) DropFront(); //当this（conditions）的长度大于other的长度，this丢弃头部
    while (elements_ != other.elements_) { //other和this的elements必须相同，也就是满足shared tail的要求
      DropFront(); //不想等时依次丢弃this和other的头部
      other.DropFront();
    }
  }
  
  ----------------------------
  
    void DropFront() {
    CHECK_GT(Size(), 0);
    elements_ = elements_->rest; //类似于list中的next
  } 
```
ReduceMerge主要作用：将merage结点的condition list更新为所有input结点的longest common tail。

###  DeadCodeElimination

```
Reduction DeadCodeElimination::Reduce(Node* node) {
  DisallowHeapAccessIf no_heap_access(!FLAG_turbo_direct_heap_access);
  switch (node->opcode()) {
    case IrOpcode::kEnd:
      return ReduceEnd(node);
    case IrOpcode::kLoop:
    case IrOpcode::kMerge:
      return ReduceLoopOrMerge(node);
    case IrOpcode::kLoopExit:
      return ReduceLoopExit(node);
    case IrOpcode::kUnreachable:
    case IrOpcode::kIfException:
      return ReduceUnreachableOrIfException(node);
    case IrOpcode::kPhi:
      return ReducePhi(node);
    case IrOpcode::kEffectPhi:
      return ReduceEffectPhi(node);
    case IrOpcode::kDeoptimize:
    case IrOpcode::kReturn:
    case IrOpcode::kTerminate:
    case IrOpcode::kTailCall:
      return ReduceDeoptimizeOrReturnOrTerminateOrTailCall(node);
    case IrOpcode::kThrow:
      return PropagateDeadControl(node);
    case IrOpcode::kBranch:
    case IrOpcode::kSwitch:
      return ReduceBranchOrSwitch(node);
    default:
      return ReduceNode(node);
  }
  UNREACHABLE();
}
```
老规矩举几个例子：

##### ReduceUnreachableOrIfException

```
Reduction DeadCodeElimination::ReduceUnreachableOrIfException(Node* node) {
  DCHECK(node->opcode() == IrOpcode::kUnreachable ||
         node->opcode() == IrOpcode::kIfException);
  Reduction reduction = PropagateDeadControl(node); //判断是否消除control结点
  if (reduction.Changed()) return reduction;
  Node* effect = NodeProperties::GetEffectInput(node, 0); //获取effect结点
  if (effect->opcode() == IrOpcode::kDead) { //如果effect为dead
    return Replace(effect); //清除effect
  }
  if (effect->opcode() == IrOpcode::kUnreachable) { //如果effect为Unreachable
    return Replace(effect); //清除effect
  }
  return NoChange();
}

  ----------------------------

Reduction DeadCodeElimination::PropagateDeadControl(Node* node) {
  DCHECK_EQ(1, node->op()->ControlInputCount());
  Node* control = NodeProperties::GetControlInput(node); //获取结点的control结点
  if (control->opcode() == IrOpcode::kDead) return Replace(control); //如果该control结点是一个dead结点，那么将其消除。
  return NoChange();
}
```

##### ReducePhi

```
Reduction DeadCodeElimination::ReducePhi(Node* node) {
  DCHECK_EQ(IrOpcode::kPhi, node->opcode());
  Reduction reduction = PropagateDeadControl(node); //判断是否需要消除control结点
  if (reduction.Changed()) return reduction;
  MachineRepresentation rep = PhiRepresentationOf(node->op()); //获得node结点的MachineRepresentation
  if (rep == MachineRepresentation::kNone ||
      NodeProperties::GetTypeOrAny(node).IsNone()) { //如果node是KNone或者NodeProperties为None，则消除node结点。
    return Replace(DeadValue(node, rep));
  }
  int input_count = node->op()->ValueInputCount(); //获取node结点的input数量
  for (int i = 0; i < input_count; ++i) { //遍历input结点
    Node* input = NodeProperties::GetValueInput(node, i);
    if (input->opcode() == IrOpcode::kDeadValue &&
        DeadValueRepresentationOf(input->op()) != rep) {
      NodeProperties::ReplaceValueInput(node, DeadValue(input, rep), i); //替换node结点的input结点。
    }
  }
  return NoChange();
}

  ----------------------------
  
  
  Node* DeadCodeElimination::DeadValue(Node* node, MachineRepresentation rep) {
  if (node->opcode() == IrOpcode::kDeadValue) { //如果node结点为deadvalue
    if (rep == DeadValueRepresentationOf(node->op())) return node; //如果node的use结点的MachineRepresentation（rep）为DeadValue，返回node
    node = NodeProperties::GetValueInput(node, 0); //将node赋值为input结点
  }
  Node* dead_value = graph()->NewNode(common()->DeadValue(rep), node); //创建一个dead结点
  NodeProperties::SetType(dead_value, Type::None()); set dead结点为None
  return dead_value;
}
  
  
  ----------------------------

 
enum class MachineRepresentation : uint8_t {
  kNone,
  kBit,
  // Integral representations must be consecutive, in order of increasing order.
  kWord8,
  kWord16,
  kWord32,
  kWord64,
  kTaggedSigned,       // (uncompressed) Smi
  kTaggedPointer,      // (uncompressed) HeapObject
  kTagged,             // (uncompressed) Object (Smi or HeapObject)
  kCompressedPointer,  // (compressed) HeapObject
  kCompressed,         // (compressed) Object (Smi or HeapObject)
  // FP and SIMD representations must be last, and in order of increasing size.
  kFloat32,
  kFloat64,
  kSimd128,
  kFirstFPRepresentation = kFloat32,
  kLastRepresentation = kSimd128
};
```

### RedundancyElimination

```
Reduction RedundancyElimination::Reduce(Node* node) {
  if (node_checks_.Get(node)) return NoChange();
  switch (node->opcode()) {
    case IrOpcode::kCheckBigInt:
    case IrOpcode::kCheckBounds:
    case IrOpcode::kCheckClosure:
    case IrOpcode::kCheckEqualsInternalizedString:
    case IrOpcode::kCheckEqualsSymbol:
    case IrOpcode::kCheckFloat64Hole:
    case IrOpcode::kCheckHeapObject:
    case IrOpcode::kCheckIf:
    case IrOpcode::kCheckInternalizedString:
    case IrOpcode::kCheckNotTaggedHole:
    case IrOpcode::kCheckNumber:
    case IrOpcode::kCheckReceiver:
    case IrOpcode::kCheckReceiverOrNullOrUndefined:
    case IrOpcode::kCheckSmi:
    case IrOpcode::kCheckString:
    case IrOpcode::kCheckSymbol:
#define SIMPLIFIED_CHECKED_OP(Opcode) case IrOpcode::k##Opcode:
      SIMPLIFIED_CHECKED_OP_LIST(SIMPLIFIED_CHECKED_OP)
#undef SIMPLIFIED_CHECKED_OP
      return ReduceCheckNode(node);
    case IrOpcode::kSpeculativeNumberEqual:
    case IrOpcode::kSpeculativeNumberLessThan:
    case IrOpcode::kSpeculativeNumberLessThanOrEqual:
      return ReduceSpeculativeNumberComparison(node);
    case IrOpcode::kSpeculativeNumberAdd:
    case IrOpcode::kSpeculativeNumberSubtract:
    case IrOpcode::kSpeculativeSafeIntegerAdd:
    case IrOpcode::kSpeculativeSafeIntegerSubtract:
    case IrOpcode::kSpeculativeToNumber:
      return ReduceSpeculativeNumberOperation(node);
    case IrOpcode::kEffectPhi:
      return ReduceEffectPhi(node);
    case IrOpcode::kDead:
      break;
    case IrOpcode::kStart:
      return ReduceStart(node);
    default:
      return ReduceOtherNode(node);
  }
  return NoChange();
}
```

主要是一个switch，根据不同的opcode来选择对应的reduce

我们先看ReduceCheckNode

##### ReduceCheckNode
```
Reduction RedundancyElimination::ReduceCheckNode(Node* node) {
  Node* const effect = NodeProperties::GetEffectInput(node); //获取node的effect结点
  EffectPathChecks const* checks = node_checks_.Get(effect); //获得effect结点的EffectPathChecks
  // If we do not know anything about the predecessor, do not propagate just yet
  // because we will have to recompute anyway once we compute the predecessor.
  if (checks == nullptr) return NoChange(); //判断checks不为空
  // See if we have another check that dominates us.
  if (Node* check = checks->LookupCheck(node)) { //如果存在支配当前check的check
    ReplaceWithValue(node, check);  //将当前的node替换为check
    return Replace(check);  //将check结点消除
  }

  // Learn from this check.
  return UpdateChecks(node, checks->AddCheck(zone(), node));
}

  ----------------------------

Node* RedundancyElimination::EffectPathChecks::LookupCheck(Node* node) const {
  for (Check const* check = head_; check != nullptr; check = check->next) {
    if (CheckSubsumes(check->node, node) && TypeSubsumes(node, check->node)) { //遍历判断checks中的check是否包含当前的node（当前的node就是被reduce的结点）
      DCHECK(!check->node->IsDead());
      return check->node;
    }
  }
  return nullptr;
}
```
总结一下ReduceCheckNode：
首先获取effect input的check list，如果存在包含当前check（设为结点a）结点的check（设为结点b），那我们就将a替换为b，并消除b。
简单来说就是当结点b支配结点a时，如果a check包含b check，就可以将a check转移到b check，从而省去一个check。

##### ReduceSpeculativeNumberOperation
```
Reduction RedundancyElimination::ReduceSpeculativeNumberOperation(Node* node) {
  DCHECK(node->opcode() == IrOpcode::kSpeculativeNumberAdd ||
         node->opcode() == IrOpcode::kSpeculativeNumberSubtract ||
         node->opcode() == IrOpcode::kSpeculativeSafeIntegerAdd ||
         node->opcode() == IrOpcode::kSpeculativeSafeIntegerSubtract ||
         node->opcode() == IrOpcode::kSpeculativeToNumber);
  DCHECK_EQ(1, node->op()->EffectInputCount());
  DCHECK_EQ(1, node->op()->EffectOutputCount());

  Node* const first = NodeProperties::GetValueInput(node, 0); //获得value input
  Node* const effect = NodeProperties::GetEffectInput(node); //获得effect input
  EffectPathChecks const* checks = node_checks_.Get(effect); //获得effect结点的check list
  // If we do not know anything about the predecessor, do not propagate just yet
  // because we will have to recompute anyway once we compute the predecessor.
  if (checks == nullptr) return NoChange();

  // Check if there's a CheckBounds operation on {first}
  // in the graph already, which we might be able to
  // reuse here to improve the representation selection
  // for the {node} later on.
  if (Node* check = checks->LookupBoundsCheckFor(first)) {  //判断是否存在checkbounds，它的input为node。
    // Only use the bounds {check} if its type is better
    // than the type of the {first} node, otherwise we
    // would end up replacing NumberConstant inputs with
    // CheckBounds operations, which is kind of pointless.
    if (!NodeProperties::GetType(first).Is(NodeProperties::GetType(check))) { //判断frist结点的type是否优于check结点，避免出现将NumberConstant优化为CheckBounds的浪费行为。
      NodeProperties::ReplaceValueInput(node, check, 0); //将node结点的input替换为check。
    }
  }

  return UpdateChecks(node, checks);
}

  ----------------------------
  
  Node* RedundancyElimination::EffectPathChecks::LookupBoundsCheckFor(
    Node* node) const {
  for (Check const* check = head_; check != nullptr; check = check->next) {
    if (check->node->opcode() == IrOpcode::kCheckBounds &&
        check->node->InputAt(0) == node && TypeSubsumes(node, check->node) &&
        !(CheckBoundsParametersOf(check->node->op()).flags() &
          CheckBoundsFlag::kConvertStringAndMinusZero)) {
      return check->node;
    }
  }
  return nullptr;
}

  ----------------------------
  
  //对typer进行判断，相同或者that优于this都可满足判断。
    bool Is(Type that) const {
    return payload_ == that.payload_ || this->SlowIs(that); //SlowIs: Check if [this] <= [that].
  }  
```
它主要的作用就是，判断SpeculativeNumberOperation的input是在之前有过checkbounds检查，以及是否可以重用此checkbounds。

### CheckpointElimination

冗余消除漏洞中常见的优化阶段，丢失checkpoint将会导致类型混淆。

```
Reduction CheckpointElimination::Reduce(Node* node) {
  DisallowHeapAccess no_heap_access;
  switch (node->opcode()) {
    case IrOpcode::kCheckpoint:
      return ReduceCheckpoint(node);
    default:
      break;
  }
  return NoChange();
}

  ----------------------------

Reduction CheckpointElimination::ReduceCheckpoint(Node* node) {
  DCHECK_EQ(IrOpcode::kCheckpoint, node->opcode());
  if (IsRedundantCheckpoint(node)) {
    return Replace(NodeProperties::GetEffectInput(node));
  }
  return NoChange();
}

  ----------------------------

bool IsRedundantCheckpoint(Node* node) {
  FrameStateFunctionInfo const* function_info = GetFunctionInfo(node);
  if (function_info == nullptr) return false; //如果没有functioninfo，不能消除checkpoint（具体可以回忆framestate的作用）
  Node* effect = NodeProperties::GetEffectInput(node); //获得effect input结点
  while (effect->op()->HasProperty(Operator::kNoWrite) &&
         effect->op()->EffectInputCount() == 1) { //如果checkpoint只有一个effect input，并且他是KNoWrite（无副作用）
          if (effect->opcode() == IrOpcode::kCheckpoint) { //如果effect为checkpoint，表示当前的checkpoint冗余
      return GetFunctionInfo(effect) == function_info; //将effect的functioninfo更新为冗余的checkpoint的functioninfo（具体可以回忆framestate的作用）
    }
    effect = NodeProperties::GetEffectInput(effect);
  }
  return false;
}

  ----------------------------
  
FrameStateFunctionInfo const* GetFunctionInfo(Node* checkpoint) {
  DCHECK_EQ(IrOpcode::kCheckpoint, checkpoint->opcode());
  Node* frame_state = NodeProperties::GetFrameStateInput(checkpoint); //获得framestate input
  return frame_state->opcode() == IrOpcode::kFrameState
             ? FrameStateInfoOf(frame_state->op()).function_info()
             : nullptr;
}
```

### TypeNarrowingReducer

```
Reduction TypeNarrowingReducer::Reduce(Node* node) {
  DisallowHeapAccessIf no_heap_access(!FLAG_turbo_direct_heap_access);

  Type new_type = Type::Any();

  switch (node->opcode()) {
    case IrOpcode::kNumberLessThan: {
      // TODO(turbofan) Reuse the logic from typer.cc (by integrating relational
      // comparisons with the operation typer).
      Type left_type = NodeProperties::GetType(node->InputAt(0)); //获取左操作数类型
      Type right_type = NodeProperties::GetType(node->InputAt(1)); //获取右操作数类型
      if (left_type.Is(Type::PlainNumber()) &&
          right_type.Is(Type::PlainNumber())) { //如果左右操作数都为PlainNumber
        if (left_type.Max() < right_type.Min()) { //如果左操作数最大值小于右操作数最小值
          new_type = op_typer_.singleton_true();
        } else if (left_type.Min() >= right_type.Max()) { //如果左操作数最大值大于等于右操作数最小值
          new_type = op_typer_.singleton_false();
        }
      }
      break;
    }

    case IrOpcode::kTypeGuard: {
      new_type = op_typer_.TypeTypeGuard(
          node->op(), NodeProperties::GetType(node->InputAt(0)));
      break;
    }

#define DECLARE_CASE(Name)                                                \
  case IrOpcode::k##Name: {                                               \
    new_type = op_typer_.Name(NodeProperties::GetType(node->InputAt(0)),  \
                              NodeProperties::GetType(node->InputAt(1))); \
    break;                                                                \
  }
      SIMPLIFIED_NUMBER_BINOP_LIST(DECLARE_CASE)
      DECLARE_CASE(SameValue)
#undef DECLARE_CASE

#define DECLARE_CASE(Name)                                                \
  case IrOpcode::k##Name: {                                               \
    new_type = op_typer_.Name(NodeProperties::GetType(node->InputAt(0))); \
    break;                                                                \
  }
      SIMPLIFIED_NUMBER_UNOP_LIST(DECLARE_CASE)
      DECLARE_CASE(ToBoolean)
#undef DECLARE_CASE

    default:
      return NoChange();
  }

  Type original_type = NodeProperties::GetType(node);
  Type restricted = Type::Intersect(new_type, original_type, zone());
  if (!original_type.Is(restricted)) {
    NodeProperties::SetType(node, restricted);
    return Changed(node);
  }
  return NoChange();
}
```

### TypedOptimization

代码和typerlowering阶段的typed_optimization基本相同。

还是举几个例子

##### ReduceCheckBounds
```
Reduction TypedOptimization::ReduceCheckBounds(Node* node) {
  CheckBoundsParameters const& p = CheckBoundsParametersOf(node->op()); //获得CheckBounds操作的参数
  Node* const input = NodeProperties::GetValueInput(node, 0);
  Type const input_type = NodeProperties::GetType(input);
  if (p.flags() & CheckBoundsFlag::kConvertStringAndMinusZero &&
      !input_type.Maybe(Type::String()) &&
      !input_type.Maybe(Type::MinusZero())) { //如果input的类型不为string和-0
    NodeProperties::ChangeOp(
        node,
        simplified()->CheckBounds(
            p.check_parameters().feedback(),
            p.flags().without(CheckBoundsFlag::kConvertStringAndMinusZero)));
    return Changed(node);
  }
  return NoChange();
}

----------------------------

CheckBoundsParameters const& CheckBoundsParametersOf(Operator const* op) {
  DCHECK(op->opcode() == IrOpcode::kCheckBounds ||
         op->opcode() == IrOpcode::kCheckedUint32Bounds ||
         op->opcode() == IrOpcode::kCheckedUint64Bounds);
  return OpParameter<CheckBoundsParameters>(op);
}

----------------------------

// For IrOpcode::kCheckBounds, we allow additional flags:
const Operator* SimplifiedOperatorBuilder::CheckBounds(
    const FeedbackSource& feedback, CheckBoundsFlags flags) {
  if (!feedback.IsValid()) {
    if (flags & CheckBoundsFlag::kAbortOnOutOfBounds) {
      if (flags & CheckBoundsFlag::kConvertStringAndMinusZero) {
        return &cache_.kCheckBoundsAbortingAndConverting;
      } else {
        return &cache_.kCheckBoundsAborting;
      }
    } else {
      if (flags & CheckBoundsFlag::kConvertStringAndMinusZero) {
        return &cache_.kCheckBoundsConverting;
      } else {
        return &cache_.kCheckBounds;
      }
    }
  }
  return zone()->New<SimplifiedOperatorGlobalCache::CheckBoundsOperator>(
      feedback, flags);
}
```
如果CheckBounds的input的类型不为string和-0，则更新Op并附加一些flags


##### ReduceCheckMaps
判断是否可以消除CheckMaps
```
Reduction TypedOptimization::ReduceCheckMaps(Node* node) {
  // The CheckMaps(o, ...map...) can be eliminated if map is stable,
  // o has type Constant(object) and map == object->map, and either
  //  (1) map cannot transition further, or
  //  (2) we can add a code dependency on the stability of map
  //      (to guard the Constant type information).
  // CheckMaps(o，…map…)可以消除，如果map是稳定的，
  // o具有类型常量(object)和map == object->map，下面任意条件满足一个即可
  // (1)map不能进一步转换，或
  // (2)我们可以添加一个依赖于map稳定性的代码(用来保护常量类型信息)。
  Node* const object = NodeProperties::GetValueInput(node, 0);
  Type const object_type = NodeProperties::GetType(object);
  Node* const effect = NodeProperties::GetEffectInput(node);
  base::Optional<MapRef> object_map =
      GetStableMapFromObjectType(broker(), object_type); 
  if (object_map.has_value()) {  //如果当前结点的map是stable map
    for (int i = 1; i < node->op()->ValueInputCount(); ++i) { 对node的input结点做优化
      Node* const map = NodeProperties::GetValueInput(node, i);
      Type const map_type = NodeProperties::GetType(map);
      if (map_type.IsHeapConstant() &&
          map_type.AsHeapConstant()->Ref().equals(*object_map)) {  //input结点的map是HeapConstant并且是stable map
        if (object_map->CanTransition()) { //如果不能进一步转换满足条件（1）
          dependencies()->DependOnStableMap(*object_map); //添加一个依赖于map稳定性的代码，条件（2）
        }
        return Replace(effect); //消除check
      }
    }
  }
  return NoChange();
}

----------------------------

base::Optional<MapRef> GetStableMapFromObjectType(JSHeapBroker* broker,
                                                  Type object_type) {
  if (object_type.IsHeapConstant()) { //如果type为HeapConstant，并且map is stable返回map，否则返回空
    HeapObjectRef object = object_type.AsHeapConstant()->Ref();
    MapRef object_map = object.map();
    if (object_map.is_stable()) return object_map;
  }
  return {};
}
```





