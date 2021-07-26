Issue-1181676是由于在DeleteItemWithCommandId中如果删除的是最后一个item，那么他将会关闭掉整个menu，此时释放了ClipboardHistoryItemView，但它接着在Action的析构函数中被赋值，导致uaf产生。

首先当我们要删除clipboard content时，将会通过ExecuteCommand()去调用DeleteItemWithCommandId()函数。
```
void ClipboardHistoryItemView::Activate(Action action, int event_flags) {
  DCHECK(Action::kEmpty == action_ && action_ != action);

  base::AutoReset<Action> action_to_take(&action_, action);
  RecordButtonPressedHistogram();

  views::MenuDelegate* delegate = container_->GetDelegate();
  const int command_id = container_->GetCommand();
  DCHECK(delegate->IsCommandEnabled(command_id));
  delegate->ExecuteCommand(command_id, event_flags);
}
```

但是在DeleteItemWithCommandId中存在一个特殊情况：
```
void ClipboardHistoryControllerImpl::DeleteItemWithCommandId(int command_id) {
  DCHECK(context_menu_);

  // Pressing VKEY_DELETE is handled here via AcceleratorTarget because the
  // contextual menu consumes the key event. Record the "pressing the delete
  // button" histogram here because this action does the same thing as
  // activating the button directly via click/tap. There is no special handling
  // for pasting an item via VKEY_RETURN because in that case the menu does not
  // process the key event.
  const auto& to_be_deleted_item =
      context_menu_->GetItemFromCommandId(command_id);
  ClipboardHistoryUtil::RecordClipboardHistoryItemDeleted(to_be_deleted_item);
  clipboard_history_->RemoveItemForId(to_be_deleted_item.id());

  // If the item to be deleted is the last one, close the whole menu.
  if (context_menu_->GetMenuItemsCount() == 1) {
    context_menu_->Cancel();
    context_menu_.reset();
    return;
  }

  context_menu_->RemoveMenuItemWithCommandId(command_id);
}
```
即当要删除的item是最后一个的时候，将会关闭掉整个menu，对应代码如下：
```
if (context_menu_->GetMenuItemsCount() == 1) {
    context_menu_->Cancel();
    context_menu_.reset();
    return;
  }
```

这里简单介绍一下context_menu_，它的类型如下：
```
std::unique_ptr<ClipboardHistoryMenuModelAdapter> context_menu_;
```
当他被close时，将会触发下面的任务：
```
void ClipboardHistoryMenuModelAdapter::OnMenuClosed(views::MenuItemView* menu) {
  // Terminate alive asynchronous calls on `RemoveItemView()`. It is pointless
  // to update views when the menu is closed.
  // Note that data members related to the asynchronous calls, such as
  // `item_deletion_in_progress_count_` and `scoped_ignore_`, are not reset.
  // Because when hitting here, this instance is going to be destructed soon.
  weak_ptr_factory_.InvalidateWeakPtrs();

  ClipboardImageModelFactory::Get()->Deactivate();
  const base::TimeDelta user_journey_time =
      base::TimeTicks::Now() - menu_open_time_;
  UMA_HISTOGRAM_TIMES("Ash.ClipboardHistory.ContextMenu.UserJourneyTime",
                      user_journey_time);
  views::MenuModelAdapter::OnMenuClosed(menu);
  item_views_by_command_id_.clear();

  // This implementation of MenuModelAdapter does not have a widget so we need
  // to manually notify the accessibility side of the closed menu.
  aura::Window* active_window = window_util::GetActiveWindow();
  if (!active_window)
    return;
  views::Widget* active_widget =
      views::Widget::GetWidgetForNativeView(active_window);
  DCHECK(active_widget);
  views::View* focused_view =
      active_widget->GetFocusManager()->GetFocusedView();
  if (focused_view) {
    focused_view->NotifyAccessibilityEvent(ax::mojom::Event::kMenuEnd,
                                           /*send_native_event=*/true);
  }
}
```
可以看到在该函数中执行了item_views_by_command_id_.clear();
```
std::map<int, ClipboardHistoryItemView*> item_views_by_command_id_;
```
即释放掉了ClipboardHistoryItemView。
此时会连带执行Activate的析构函数,我们下来回忆一下它上面创建时的代码
```
base::AutoReset<Action> action_to_take(&action_, action);
```
并且action_是ClipboardHistoryItemView中的一个成员。
```
class ClipboardHistoryItemView : public views::View {
 public:
  static std::unique_ptr<ClipboardHistoryItemView>
  CreateFromClipboardHistoryItem(
      const ClipboardHistoryItem& item,
      const ClipboardHistoryResourceManager* resource_manager,
      views::MenuItemView* container);

  ......

  // Indicates whether the menu item is under the gesture long press.
  bool under_gesture_long_press_ = false;

  // Indicates the action to take. It is set when the menu item is activated
  // through `main_button_` or the delete button.
  ClipboardHistoryUtil::Action action_ = ClipboardHistoryUtil::Action::kEmpty;

  views::PropertyChangedSubscription subscription_;
};
```

最后我们来看他的析构函数：
```
  AutoReset(T* scoped_variable, U&& new_value)
      : scoped_variable_(scoped_variable),
        original_value_(
            std::exchange(*scoped_variable_, std::forward<U>(new_value))) {}

.....

  ~AutoReset() {
    if (scoped_variable_)
      *scoped_variable_ = std::move(original_value_);
  }
```
可以看到他对scoped_variable_，也就是我们上面提到的ClipboardHistoryItemView中的一个成员action_，此时ClipboardHistoryItemView已被释放，所以导致uaf产生。

