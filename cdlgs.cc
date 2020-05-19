// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "cdlgs.h"

namespace cwiclo {
namespace ui {

//----------------------------------------------------------------------

DEFINE_INTERFACE (MessageBox)
DEFINE_INTERFACE (MessageBoxR)

//----------------------------------------------------------------------

MessageBox::MessageBox (const Msg::Link& l)
: Window(l)
,_prompt()
,_type()
,_reply(l)
{
}

bool MessageBox::dispatch (Msg& msg)
{
    return PMessageBox::dispatch (this, msg)
	|| Window::dispatch (msg);
}

void MessageBox::MessageBox_ask (const string_view& prompt, Type type, uint16_t)
{
    _prompt = prompt;
    _type = type;
    destroy_widgets();

    // Each type of box has different number of buttons, which are last
    // in the list. Subtracting unneeded buttons generates the layout.
    if (_type == Type::Ok)
	create_widgets (begin(c_layout), end(c_layout)-2);
    else if (_type == Type::OkCancel || _type == Type::YesNo)
	create_widgets (begin(c_layout), end(c_layout)-1);
    else
	create_widgets (c_layout);

    set_widget_text (wid_Message, prompt);

    // Setting labels by type
    if (_type == Type::RetryCancelIgnore)
	set_widget_text (wid_OK, "Retry");
    else if (_type == Type::YesNo || _type == Type::YesNoCancel)
	set_widget_text (wid_OK, "Yes");
    else
	set_widget_text (wid_OK, "Ok");

    if (_type == Type::YesNo)
	set_widget_text (wid_Cancel, "No");
    else
	set_widget_text (wid_Cancel, "Cancel");

    if (_type == Type::YesNoCancel)
	set_widget_text (wid_Ignore, "No");
    else
	set_widget_text (wid_Ignore, "Ignore");
}

void MessageBox::done (Answer answer)
{
    _reply.reply (answer);
    close();
}

void MessageBox::on_key (key_t key)
{
    if (key == 'y' || key == 'r')
	done (Answer::Ok);
    else if (key == 'n' || key == 'i')
	done (Answer::No);
    else if (key == Key::Escape || key == 'c')
	done (Answer::Cancel);
    else if (key == Key::Left || key == 'h')
	focus_prev();
    else if (key == Key::Right || key == 'l')
	focus_next();
    else if (key == Key::Enter) {
	if (focused_widget_id() == wid_Cancel)
	    done (Answer::Cancel);
	else if (focused_widget_id() == wid_Ignore)
	    done (Answer::Ignore);
	else if (focused_widget_id() == wid_OK)
	    done (Answer::Ok);
	else
	    Window::on_key (key);
    } else
	Window::on_key (key);
}

} // namespace ui
} // namespace cwiclo
