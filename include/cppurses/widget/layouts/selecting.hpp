#ifndef CPPURSES_WIDGET_LAYOUTS_SELECTING_HPP
#define CPPURSES_WIDGET_LAYOUTS_SELECTING_HPP
#include <algorithm>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <cppurses/system/events/key.hpp>
#include <cppurses/system/system.hpp>
#include <cppurses/widget/pipe.hpp>

namespace cppurses::layout {

/// Adds 'Selected Child' concept to a layout.
/** Keyboard and mouse selecting ability, scrolls when selection is off screeen.
 *  Depends on the Child Widgets of Layout_t to have a select() and unselect()
 *  method. Inherit from this and override key_press_event to perform actions on
 *  the selected_child(). Scroll action also moves the selected child index. */
template <typename Layout_t, bool unselect_on_focus_out = true>
class Selecting : public Layout_t {
   private:
    using Key_codes = std::vector<Key::Code>;

    template <typename Unary_predicate>
    using Enable_if_invocable_with_child_t = std::enable_if_t<
        std::is_invocable_v<Unary_predicate,
                            std::add_const_t<std::add_lvalue_reference_t<
                                typename Layout_t::Child_t>>>,
        int>;

   public:
    Selecting() { *this | pipe::strong_focus(); }

    Selecting(Key_codes increment_selection_keys,
              Key_codes decrement_selection_keys,
              Key_codes increment_scroll_keys,
              Key_codes decrement_scroll_keys)
        : increment_selection_keys_{increment_selection_keys},
          decrement_selection_keys_{decrement_selection_keys},
          increment_scroll_keys_{increment_scroll_keys},
          decrement_scroll_keys_{decrement_scroll_keys}
    {
        *this | pipe::strong_focus();
    }

   public:
    void set_increment_selection_keys(Key_codes keys)
    {
        increment_selection_keys_ = std::move(keys);
    }

    void set_decrement_selection_keys(Key_codes keys)
    {
        decrement_selection_keys_ = std::move(keys);
    }

    void set_increment_scroll_keys(Key_codes keys)
    {
        increment_scroll_keys_ = std::move(keys);
    }

    void set_decrement_scroll_keys(Key_codes keys)
    {
        decrement_scroll_keys_ = std::move(keys);
    }

    /// Return the currently selected child, UB if no children in Layout.
    auto selected_child() const -> typename Layout_t::Child_t const&
    {
        return this->get_children()[selected_];
    }

    /// Return the currently selected child, UB if no children in Layout.
    auto selected_child() -> typename Layout_t::Child_t&
    {
        if (selected_ >= this->child_count()) {
            // Getting intermitent crash because of outside access.
            throw std::runtime_error{
                "Selecting::selected_child();" + std::to_string(selected_) +
                ">= " + std::to_string(this->child_count())};
        }
        return this->get_children()[selected_];
    }

    /// Return the index into get_children() corresponding to the selected child
    auto selected_row() const -> std::size_t { return selected_; }

    /// Erase first element that satisfies \p pred. Return true if erase happens
    template <typename Unary_predicate,
              Enable_if_invocable_with_child_t<Unary_predicate> = 0>
    auto erase(Unary_predicate&& pred) -> bool
    {
        auto child =
            this->get_children().find(std::forward<Unary_predicate>(pred));
        if (child == nullptr)
            return false;
        this->erase(child);
        return true;
    }

    /// Erase the given widget and reset selected to the Layout's offset.
    void erase(Widget const* child)
    {
        auto const was_selected = &this->selected_child() == child;
        this->Layout_t::erase(child);
        if (was_selected && this->child_count() > 0)
            this->set_selected(this->children_.get_offset());
    }

    /// Erase child at \p index and reset selected to the Layout's offset.
    void erase(std::size_t index)
    {
        auto const was_selected = selected_ == index;
        this->Layout_t::erase(index);
        if (was_selected && this->child_count() > 0)
            this->set_selected(this->children_.get_offset());
    }

   protected:
    auto key_press_event(Key::State const& keyboard) -> bool override
    {
        if (contains(keyboard.key, increment_selection_keys_))
            this->increment_selected_and_scroll_if_necessary();
        else if (contains(keyboard.key, decrement_selection_keys_))
            this->decrement_selected_and_scroll_if_necessary();
        else if (contains(keyboard.key, increment_scroll_keys_))
            this->increment_offset_and_increment_selected();
        else if (contains(keyboard.key, decrement_scroll_keys_))
            this->decrement_offset_and_decrement_selected();
        return Layout_t::key_press_event(keyboard);
    }

    /// Reset the selected child if needed.
    auto resize_event(cppurses::Area new_size, cppurses::Area old_size)
        -> bool override
    {
        auto const base_result = Layout_t::resize_event(new_size, old_size);
        this->reset_selected_if_necessary();
        return base_result;
    }

    /// If selected_child is off the screen, select() the last displayed widget.
    void reset_selected_if_necessary()
    {
        if (this->Layout_t::child_count() == 0 ||
            this->selected_child().is_enabled()) {
            return;
        }
        this->set_selected(this->find_bottom_row());
    }

    auto focus_in_event() -> bool override
    {
        this->reset_selected_if_necessary();
        if (this->child_count() != 0)
            this->selected_child().select();
        return Layout_t::focus_in_event();
    }

    auto focus_out_event() -> bool override
    {
        if constexpr (unselect_on_focus_out) {
            if (this->child_count() != 0)
                this->selected_child().unselect();
        }
        return Layout_t::focus_out_event();
    }

    auto disable_event() -> bool override
    {
        if (this->child_count() != 0)
            this->selected_child().unselect();
        return Layout_t::disable_event();
    }

    auto enable_event() -> bool override
    {
        if (this->child_count() != 0 && System::focus_widget() == this)
            this->selected_child().select();
        return Layout_t::enable_event();
    }

   private:
    std::size_t selected_ = 0uL;
    Key_codes increment_selection_keys_;
    Key_codes decrement_selection_keys_;
    Key_codes increment_scroll_keys_;
    Key_codes decrement_scroll_keys_;

   private:
    void increment_selected()
    {
        if (this->child_count() == 0 || selected_ + 1 == this->child_count())
            return;
        this->set_selected(selected_ + 1);
    }

    void increment_selected_and_scroll_if_necessary()
    {
        this->increment_selected();
        while (!this->selected_child().is_enabled()) {
            this->increment_offset();
            this->update_geometry();
        }
    }

    void decrement_selected()
    {
        if (this->child_count() == 0 || selected_ == 0)
            return;
        this->set_selected(selected_ - 1);
    }

    void decrement_selected_and_scroll_if_necessary()
    {
        this->decrement_selected();
        if (!this->selected_child().is_enabled())
            this->decrement_offset();
    }

    /// Scroll down or right.
    void increment_offset()
    {
        auto const child_n = this->child_count();
        if (child_n == 0)
            return;
        if (auto const offset = this->child_offset(); offset + 1 != child_n)
            this->set_offset(offset + 1);
    }

    void increment_offset_and_increment_selected()
    {
        this->increment_offset();
        this->increment_selected();
    }

    /// Scroll up or left.
    void decrement_offset()
    {
        if (this->child_count() == 0)
            return;
        if (auto const offset = this->child_offset(); offset != 0)
            this->set_offset(offset - 1);
    }

    void decrement_offset_and_decrement_selected()
    {
        if (this->child_offset() == 0)
            return;
        this->decrement_offset();
        this->decrement_selected();
    }

    /// unselect() the currently selected child, select() the child at \p index.
    void set_selected(std::size_t index)
    {
        if (selected_ < this->child_count())
            this->selected_child().unselect();
        selected_ = index;
        this->selected_child().select();
    }

    /// Find the child index of the last displayed Data_row.
    /** Assumes child_count() > 0. Returns child_offset if all are disabled. */
    auto find_bottom_row() const -> std::size_t
    {
        auto const children = this->Widget::get_children();
        auto const count    = this->child_count();
        auto const offset   = this->child_offset();
        for (auto i = offset + 1; i < count; ++i) {
            if (children[i].is_enabled())
                continue;
            return i - 1;
        }
        return offset;
    }

    /// Return true if \p codes contains the value \p key.
    static auto contains(Key::Code key, Key_codes const& codes) -> bool
    {
        return std::any_of(std::begin(codes), std::end(codes),
                           [=](auto k) { return k == key; });
    }
};

}  // namespace cppurses::layout
#endif  // CPPURSES_WIDGET_LAYOUTS_SELECTING_HPP
