// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "widget.h"

namespace cwiclo {
namespace ui {

//{{{ Label

class Label : public Widget {
public:
		Label (Window* w, const Layout& lay) : Widget(w,lay) {}
private:
    DECLARE_WIDGET_WRITE_DRAWLIST (Drawlist);
};
//}}}---------------------------------------------------------------
//{{{ Button

class Button : public Widget {
public:
		Button (Window* w, const Layout& lay)
			: Widget(w,lay) { set_flag (f_CanFocus); }
protected:
    void	on_set_text (void) override;
private:
    DECLARE_WIDGET_WRITE_DRAWLIST (Drawlist);
};
//}}}---------------------------------------------------------------
//{{{ Listbox

class Listbox : public Widget {
public:
		Listbox (Window* w, const Layout& lay)
			: Widget(w,lay),_n(),_top() { set_flag (f_CanFocus); }
    void	on_key (key_t k) override;
protected:
    void	on_set_text (void) override;
private:
    void	clip_sel (void)	{ set_selection (min (selection_start(), _n-1)); }
    void	set_n (dim_t n)	{ _n = n; clip_sel(); }
    DECLARE_WIDGET_WRITE_DRAWLIST (Drawlist);
private:
    dim_t	_n;
    dim_t	_top;
};
//}}}---------------------------------------------------------------
//{{{ Editbox

class Editbox : public Widget {
public:
		Editbox (Window* w, const Layout& lay);
    void	on_key (key_t k) override;
    void	on_resize (void) override;
protected:
    void	on_set_text (void) override;
private:
    void	posclip (void);
    DECLARE_WIDGET_WRITE_DRAWLIST (Drawlist);
private:
    coord_t	_cpos;
    u_short	_fc;
};
//}}}---------------------------------------------------------------
//{{{ HSplitter

class HSplitter : public Widget {
public:
		HSplitter (Window* w, const Layout& lay)
		    : Widget(w,lay) { set_size_hints (0, 1); }
private:
    DECLARE_WIDGET_WRITE_DRAWLIST (Drawlist);
};
//}}}---------------------------------------------------------------
//{{{ VSplitter

class VSplitter : public Widget {
public:
		VSplitter (Window* w, const Layout& lay)
		    : Widget(w,lay) { set_size_hints (1, 0); }
private:
    DECLARE_WIDGET_WRITE_DRAWLIST (Drawlist);
};
//}}}---------------------------------------------------------------
//{{{ GroupFrame

class GroupFrame : public Widget {
public:
		GroupFrame (Window* w, const Layout& lay)
		    : Widget(w,lay) {}
private:
    DECLARE_WIDGET_WRITE_DRAWLIST (Drawlist);
};
//}}}---------------------------------------------------------------
//{{{ StatusLine

class StatusLine : public Widget {
public:
    enum { f_Modified = Widget::f_Last, f_Last };
public:
		StatusLine (Window* w, const Layout& lay)
		    : Widget(w,lay) { set_size_hints (0, 1); }
private:
    DECLARE_WIDGET_WRITE_DRAWLIST (Drawlist);
};
//}}}---------------------------------------------------------------
//{{{ default_widget_factory

Widget* default_widget_factory (mrid_t owner, const Widget::Layout& l);

//}}}---------------------------------------------------------------

} // namespace ui
} // namespace cwiclo
