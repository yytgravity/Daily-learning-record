``` c++
  void MarkObjectByPointer(Object** p) {    if (!(*p)->IsHeapObject()) return;    HeapObject* object = HeapObject::cast(*p);    if (!collector_->heap()->InNewSpace(object)) return;    if (marking_state_->WhiteToGrey(object)) {      collector_->main_marking_visitor()->Visit(object);      collector_->ProcessMarkingWorklist();    }  }
```

``` c++
  bool Visit(HeapObject* obj, int size) override {    if (marking_state_->IsBlack(obj)) {      live_collector_.CollectStatistics(obj);    } else {      DCHECK(!marking_state_->IsGrey(obj));      dead_collector_.CollectStatistics(obj);    }    return true;  }
```

``` c++
void MarkCompactCollector::ProcessMarkingWorklist() {  HeapObject* object;  MarkCompactMarkingVisitor visitor(this);  while ((object = marking_worklist()->Pop()) != nullptr) {    DCHECK(!object->IsFiller());    DCHECK(object->IsHeapObject());    DCHECK(heap()->Contains(object));    DCHECK(!(atomic_marking_state()->IsWhite(object)));    atomic_marking_state()->GreyToBlack(object);    Map* map = object->map();    MarkObject(object, map);    visitor.Visit(map, object);  }  DCHECK(marking_worklist()->IsBailoutEmpty());}
```
``` c++
  inline bool Visit(HeapObject* object, int size) {    RecordMigratedSlotVisitor visitor(heap_->mark_compact_collector());    object->IterateBody(&visitor);    return true;  }
```
