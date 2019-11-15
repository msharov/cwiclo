// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#if __has_include(<curses.h>)
#include "widget.h"

namespace cwiclo {
namespace ui {

//{{{ Label

class Label : public Widget {
public:
		Label (const Msg::Link& l, const Layout& lay) : Widget(l,lay) {}
    void	on_draw (void) const override;
};
//}}}---------------------------------------------------------------
//{{{ Button

class Button : public Widget {
public:
		Button (const Msg::Link& l, const Layout& lay)
			: Widget(l,lay) { set_flag (f_CanFocus); }
    void	on_draw (void) const override;
    void	on_set_text (void) override;
};
//}}}---------------------------------------------------------------
//{{{ Listbox

class Listbox : public Widget {
public:
		Listbox (const Msg::Link& l, const Layout& lay)
			: Widget(l,lay),_n(),_top() { set_flag (f_CanFocus); }
    void	on_set_text (void) override;
    void	on_key (key_t k) override;
    void	on_draw (void) const override;
private:
    void	clip_sel (void)	{ set_selection (min (selection_start(), _n-1)); }
    void	set_n (dim_t n)	{ _n = n; clip_sel(); }
private:
    dim_t	_n;
    dim_t	_top;
};
//}}}---------------------------------------------------------------
//{{{ Editbox

class Editbox : public Widget {
public:
		Editbox (const Msg::Link& l, const Layout& lay);
    void	on_resize (void) override;
    void	on_set_text (void) override;
    void	on_key (key_t k) override;
    void	on_draw (void) const override;
private:
    void	posclip (void);
private:
    coord_t	_cpos;
    u_short	_fc;
};
//}}}---------------------------------------------------------------
//{{{ HSplitter

class HSplitter : public Widget {
public:
		HSplitter (const Msg::Link& l, const Layout& lay)
		    : Widget(l,lay) { set_size_hints (0, 1); }
    void	on_draw (void) const override {
		    Widget::on_draw();
		    whline (w(), 0, area().w);
		}
};
//}}}---------------------------------------------------------------
//{{{ VSplitter

class VSplitter : public Widget {
public:
		VSplitter (const Msg::Link& l, const Layout& lay)
		    : Widget(l,lay) { set_size_hints (1, 0); }
    void	on_draw (void) const override {
		    Widget::on_draw();
		    wvline (w(), 0, area().h);
		}
};
//}}}---------------------------------------------------------------
//{{{ GroupFrame

class GroupFrame : public Widget {
public:
		GroupFrame (const Msg::Link& l, const Layout& lay)
		    : Widget(l,lay) {}
    void	on_draw (void) const override {
		    Widget::on_draw();
		    box (w(), 0, 0);
		    auto tsz = min (text().size(), area().w-strlen("[  ]"));
		    if (tsz > 0) {
			mvwhline (w(), 0, (area().w-tsz)/2u-1, ' ', tsz+2);
			mvwaddnstr (w(), 0, (area().w-tsz)/2u, text().c_str(), tsz);
		    }
		}
};
//}}}---------------------------------------------------------------
//{{{ StatusLine

class StatusLine : public Widget {
public:
    enum { f_Modified = Widget::f_Last, f_Last };
public:
		StatusLine (const Msg::Link& l, const Layout& lay)
		    : Widget(l,lay) { set_size_hints (0, 1); }
    void	on_resize (void) override {
		    Widget::on_resize();
		    wbkgdset (w(), A_REVERSE);
		}
    void	on_draw (void) const override {
		    Widget::on_draw();
		    waddstr (w(), text().c_str());
		    if (is_modified())
			mvwaddstr (w(), 0, area().w-strlen(" *"), " *");
		}
};
//}}}---------------------------------------------------------------
//{{{ default_widget_factory

Widget* default_widget_factory (mrid_t owner, const Widget::Layout& l);

//}}}---------------------------------------------------------------

} // namespace ui
} // namespace cwiclo
#endif // __has_include(<curses.h>)
