#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include <termox/widgets/widget.hpp>

namespace ox::widgets {
struct Divider;

/**
 * Policy for how a widget should be sized by its parent layout.
 */
struct SizePolicy {
    int minimum       = 0;
    int maximum       = std::numeric_limits<int>::max();
    float flexibility = 1.f;

    [[nodiscard]] static auto fixed(int size) -> SizePolicy
    {
        return {.minimum = size, .maximum = size, .flexibility = 0.f};
    }

    [[nodiscard]] static auto flex(float flex = 1.f) -> SizePolicy
    {
        return {.minimum     = 0,
                .maximum     = std::numeric_limits<int>::max(),
                .flexibility = flex};
    }

    [[nodiscard]] static auto bounded(int min, int max) -> SizePolicy
    {
        return {.minimum = min, .maximum = max, .flexibility = 1.f};
    }

    [[nodiscard]] static auto min(int min) -> SizePolicy
    {
        return {.minimum     = min,
                .maximum     = std::numeric_limits<int>::max(),
                .flexibility = 1.f};
    }

    [[nodiscard]] static auto max(int max) -> SizePolicy
    {
        return {.minimum = 0, .maximum = max, .flexibility = 1.f};
    }
};

/**
 * A layout that arranges its children in a line, either horizontally or
 * vertically. Do not use directly, instead use HLayout or VLayout.
 */
struct LinearLayout {
    template <typename... Widgets>
    explicit LinearLayout(Widgets&&... children_)
    {
        static_assert(
            (!std::is_same_v<std::remove_cvref_t<Widgets>, Widget> && ...),
            "`Widget` type should not be passed as an argument");
        size_policies.resize(sizeof...(children_), SizePolicy{});
        (children.emplace_back(std::forward<Widgets>(children_)), ...);
    }

    std::vector<Widget> children          = {};
    std::vector<SizePolicy> size_policies = {};
};

inline auto children(LinearLayout& w) -> std::span<Widget>
{
    return w.children;
}

inline auto children(LinearLayout const& w) -> std::span<Widget const>
{
    return w.children;
}

// -----------------------------------------------------------------------------

/**
 * Append a Widget to the LinearLayout.
 *
 * @param t The Widget to append.
 * @param size_policy The size policy to apply to the Widget.
 * @param focus_policy The focus policy to apply to the Widget.
 * @return A reference to the appended Widget. This reference will remain
 * valid until the Widget is destroyed.
 */
template <typename T>
auto append(LinearLayout& layout,
            T t,
            SizePolicy size_policy   = {},
            FocusPolicy focus_policy = FocusPolicy::None) -> T&
{
    static_assert(!std::is_same_v<std::remove_cvref_t<T>, Widget>);

    layout.size_policies.push_back(size_policy);

    return layout.children.emplace_back(std::move(t), focus_policy)
        .template data<T>();
}

/**
 * Inserts a Widget into the LinearLayout at the given index.
 *
 * @param index The index to insert the Widget at. If this is greater than the
 * current number of children, the Widget will be appended to the end of the
 * layout.
 * @param t The Widget to insert.
 * @param size_policy The size policy to apply to the Widget.
 * @param focus_policy The focus policy to apply to the Widget.
 * @return A reference to the inserted Widget. This reference will remain
 * valid until the Widget is destroyed.
 */
template <typename T>
auto insert_at(LinearLayout& layout,
               std::size_t index,
               T t,
               SizePolicy size_policy   = {},
               FocusPolicy focus_policy = FocusPolicy::None) -> T&
{
    static_assert(!std::is_same_v<std::remove_cvref_t<T>, Widget>);

    if (index >= layout.children.size()) {
        return append(layout, std::move(t), size_policy, focus_policy);
    }

    layout.size_policies.insert(
        std::next(std::begin(layout.size_policies), (std::ptrdiff_t)index),
        size_policy);

    return layout.children
        .emplace(std::next(std::begin(layout.children), index), std::move(t),
                 focus_policy)
        ->template data<T>();
}

/**
 * Removes and returns the given Widget from the LinearLayout.
 *
 * @details Find \p w with find_if.
 * @param layout The LinearLayout to remove \p w from.
 * @param w The Widget to remove.
 * @return The removed Widget.
 * @throws std::out_of_range If \p w is not found in \p layout.
 */
inline auto remove(LinearLayout& layout, Widget const& w) -> Widget
{
    auto const iter = std::ranges::find_if(
        layout.children, [&w](Widget const* child) { return &w == child; },
        [](Widget const& child) { return &child; });

    if (iter == std::end(layout.children)) {
        throw std::out_of_range{"remove: Widget not found in layout"};
    }
    auto const index = std::distance(std::begin(layout.children), iter);
    layout.size_policies.erase(
        std::next(std::begin(layout.size_policies), index));
    auto removed = std::move(*iter);
    layout.children.erase(iter);
    return removed;
}

inline auto remove_at(LinearLayout& layout, std::size_t index) -> Widget
{
    if (index >= layout.children.size()) {
        throw std::out_of_range{"remove_at: index out of range"};
    }

    layout.size_policies.erase(
        std::next(std::begin(layout.size_policies), (std::ptrdiff_t)index));
    auto iter = std::next(std::begin(layout.children), (std::ptrdiff_t)index);
    auto removed = std::move(*iter);
    layout.children.erase(iter);
    return removed;
}

inline auto remove_all(LinearLayout& layout) -> std::vector<Widget>
{
    auto removed = std::move(layout.children);
    layout.children.clear();
    layout.size_policies.clear();
    return removed;
}

// -----------------------------------------------------------------------------

auto paint(LinearLayout const&, ox::Canvas) -> void;

auto timer(LinearLayout&, int id) -> void;

// -----------------------------------------------------------------------------

struct HLayout : LinearLayout {
    using LinearLayout::LinearLayout;
};

// TODO can these be made generic in LinearLayout? Is direction really that
// important?
auto mouse_press(HLayout& layout, Mouse m) -> void;

auto mouse_release(HLayout& layout, Mouse m) -> void;

auto mouse_wheel(HLayout& layout, Mouse m) -> void;

auto mouse_move(HLayout& layout, Mouse m) -> void;

auto resize(HLayout& layout, Area a) -> void;

auto append_divider(HLayout& layout, Glyph line = {U'│'}) -> Divider&;

// -----------------------------------------------------------------------------

struct VLayout : LinearLayout {
    using LinearLayout::LinearLayout;
};

auto mouse_press(VLayout& layout, Mouse m) -> void;

auto mouse_release(VLayout& layout, Mouse m) -> void;

auto mouse_wheel(VLayout& layout, Mouse m) -> void;

auto mouse_move(VLayout& layout, Mouse m) -> void;

auto resize(VLayout& layout, ox::Area a) -> void;

auto append_divider(VLayout& layout, Glyph line = {U'─'}) -> Divider&;

// -----------------------------------------------------------------------------

// TODO
struct GridLayout {};

}  // namespace ox::widgets