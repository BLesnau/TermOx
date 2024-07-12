#pragma once

#include <ox/core/core.hpp>
#include <ox/label.hpp>
#include <ox/widget.hpp>

namespace ox {

struct Border {
    Painter::Box box = {};
    Label label = {};

    /// Light border: ┌┐└┘─│
    [[nodiscard]] static auto light(std::string label = "") -> Border;

    /// Round border: ╭╮╰╯─│
    [[nodiscard]] static auto round(std::string label = "") -> Border;

    /// Double-line border: ╔╗╚╝═║
    [[nodiscard]] static auto double_line(std::string label = "") -> Border;

    /// Bold border: ┏┓┗┛━┃
    [[nodiscard]] static auto bold(std::string label = "") -> Border;

    /// Dashed border: ┌┐└┘╌╎
    [[nodiscard]] static auto dashed(std::string label = "") -> Border;

    /// Dotted border: ┌┐└┘┄┆
    [[nodiscard]] static auto dotted(std::string label = "") -> Border;

    /// ASCII border: ++++++-|
    [[nodiscard]] static auto ascii(std::string label = "") -> Border;

    /// Double horizontal border: ╒╕╘╛═│
    [[nodiscard]] static auto double_horizontal(std::string label = "") -> Border;

    /// Double vertical border: ╓╖╙╜─║
    [[nodiscard]] static auto double_vertical(std::string label = "") -> Border;
};

/**
 * A Widget that wraps another Widget with a Border display.
 */
template <WidgetDerived ChildWidget>
class Bordered : public Widget {
   public:
    ChildWidget child;
    Border border;

   public:
    Bordered(ChildWidget child_ = {}, Border border_ = {})
        : child{std::move(child_)}, border{std::move(border_)}
    {}

   public:
    void resize(Area new_size) override
    {
        border.box.size = new_size;
        child.at = {1, 1};
        child.size = {
            .width = std::max(0, new_size.width - 2),
            .height = std::max(0, new_size.height - 2),
        };
        child.resize(child.size);
    }

    void paint(Canvas c) override
    {
        // Box
        Painter{c}[{0, 0}] << border.box;

        // Label
        auto& label = border.label;
        auto const width = std::max(0, c.size.width - 2);
        auto const glyphs =
            std::string_view{label.text}.substr(0, (std::size_t)width) | label.brush;

        auto const at = [&]() -> Point {
            switch (label.align) {
                case Label::Align::Left:
                    return {
                        .x = 1,
                        .y = 0,
                    };
                case Label::Align::Center:
                    return {
                        .x = 1 + (width - (int)glyphs.size()) / 2,
                        .y = 0,
                    };
                case Label::Align::Right:
                    return {
                        .x = 1 + width - (int)glyphs.size(),
                        .y = 0,
                    };
            }
        }();

        Painter{c}[at] << glyphs;
    }

    auto get_children() -> Generator<Widget&> override { co_yield child; }

    auto get_children() const -> Generator<Widget const&> override { co_yield child; }
};

// Moves \p w into a Bordered object
template <WidgetDerived WidgetType>
[[nodiscard]] auto operator|(WidgetType w, Border b) -> Bordered<WidgetType>
{
    return {std::move(w), std::move(b)};
}

// -------------------------------------------------------------------------------------

// struct Divider {
//     Glyph line;
// };

}  // namespace ox