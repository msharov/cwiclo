// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "../app.h"
#include "../window.h"
#include "../termscr.h"
using namespace cwiclo;
using namespace cwiclo::ui;

class UITWindow : public Window {
public:
    UITWindow (const Msg::Link& l);
private:
    DECLARE_WIDGET_WRITE_DRAWLIST (Drawlist);
    void on_key (key_t k) override;
    void on_resize (void) override;
private:
    enum : widgetid_t {
	wid_ScreenHole = wid_First,
	wid_Label,
	wid_Edit,
	wid_OK,
	wid_Cancel,
	wid_List,
	wid_Status
    };
    static constexpr const Layout c_layout[] = {
	WL_(VBox),
	WL___(None, wid_ScreenHole),
	WL___(HSplitter),
	WL___(GroupFrame),
	WL_____(HBox),
	WL_______(VBox),
	WL_________(Label,    wid_Label),
	WL_________(Editbox,  wid_Edit),
	WL_________(HBox, HAlign::Center),
	WL___________(Button, wid_OK),
	WL___________(Button, wid_Cancel),
	WL_______(VSplitter),
	WL_______(Listbox,    wid_List),
	WL___(StatusLine,     wid_Status),
    };
};

UITWindow::UITWindow (const Msg::Link& l)
: Window(l)
{
    create_widgets (c_layout);
    set_widget_text (wid_Label, "Test label above edit box");
    set_widget_text (wid_OK, "OK");
    set_widget_text (wid_Cancel, "Cancel");
    set_widget_text (wid_List, string_view (ARRAY_BLOCK ("Line one\0Line two\0Three\0 Long line four and ffff gggg dddd aaaa\0Seventy five")));
    set_widget_text (wid_Status, "Status line text\0Indicators");
}

void UITWindow::on_resize (void)
{
    set_widget_size_hints (wid_ScreenHole, 0, area().h-11);
    Window::on_resize();
}

DEFINE_WIDGET_WRITE_DRAWLIST (UITWindow, Drawlist, dlw)
{
    dlw.move_to (area().w/2, area().h/2);
    dlw.text ("Hello world!", HAlign::Center, VAlign::Center);
}

void UITWindow::on_key (key_t k)
{
    if (k == Key::Escape)
	close();
    else
	Window::on_key (k);
}

//{{{ TestApp ----------------------------------------------------------

class TestApp : public App {
public:
    static auto& instance (void) { static TestApp s_app; return s_app; }
    int run (void) { _uitwp.create_dest_as<UITWindow>(); return App::run(); }
    inline void process_args (argc_t argc, argv_t argv);
private:
    TestApp (void) : App(),_uitwp (mrid_App) {}
private:
    Proxy	_uitwp;
};

void TestApp::process_args (argc_t argc [[maybe_unused]], argv_t argv [[maybe_unused]])
{
    #ifndef NDEBUG
	for (int opt; 0 < (opt = getopt (argc, argv, "d"));)
	    if (opt == 'd')
		set_flag (f_DebugMsgTrace);
    #endif
}

BEGIN_CWICLO_APP (TestApp)
    REGISTER_MSGER (PTimer, App::Timer)
    REGISTER_MSGER (PScreen, TerminalScreenWindow)
END_CWICLO_APP

SET_WIDGET_FACTORY (Widget::default_factory)

//}}}-------------------------------------------------------------------
