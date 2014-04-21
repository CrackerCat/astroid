# include <iostream>

# include <boost/tokenizer.hpp>

# include <gtkmm.h>
# include <gtkmm/widget.h>

# include "command_bar.hh"
# include "astroid.hh"
# include "main_window.hh"
# include "modes/mode.hh"
# include "modes/thread_index.hh"
# include "modes/help_mode.hh"

using namespace std;

namespace Astroid {
  CommandBar::CommandBar () {
    set_show_close_button ();
    connect_entry (entry);

    hbox.set_orientation (Gtk::ORIENTATION_HORIZONTAL);
    hbox.set_size_request (400, -1);

    hbox.pack_start (mode_label, false, false, 5);
    hbox.pack_start (entry, true, true, 5);
    add(hbox);

    entry.signal_activate ().connect (
        sigc::mem_fun (*this, &CommandBar::on_entry_activated)
        );

  }

  void CommandBar::on_entry_activated () {
    /* handle input */
    ustring cmd = get_text ();
    cout << "cb: cmd (in mode: " << mode << "): " << cmd << endl;
    set_search_mode (false); // emits changed -> disables search

    switch (mode) {
      case CommandMode::Search:
        if (cmd.size() > 0) {
          Mode * m = new ThreadIndex (main_window, cmd);

          main_window->add_mode (m);
        }
        break;

      case CommandMode::Generic:
        {
          handle_command (cmd);
        }
        break;
    }
  }

  ustring CommandBar::get_text () {
    return entry.get_text ();
  }

  void CommandBar::set_text (ustring txt) {
    entry.set_text (txt);
  }

  void CommandBar::enable_command (CommandMode m, ustring cmd = "") {
    mode = m;

    switch (mode) {

      case CommandMode::Search:
        {
          mode_label.set_text ("search >");
          entry.set_icon_from_icon_name ("edit-find-symbolic");
        }
        break;

      case CommandMode::Generic:
        {
          mode_label.set_text (">");
          entry.set_icon_from_icon_name ("system-run-symbolic");
        }
        break;

    }

    entry.set_text (cmd);
    entry.set_position (-1);
    set_search_mode (true);
  }

  void CommandBar::handle_command (ustring cmd) {
    cout << "cb: command: " << cmd << endl;

    /* extract tokens
     *
     * ustring tokenizer: http://lists.boost.org/boost-users/2007/01/24698.php
     */
    typedef boost::tokenizer<
      boost::char_delimiters_separator< Glib::ustring::value_type > ,
      Glib::ustring::const_iterator ,
      Glib::ustring > utokenizer;

    utokenizer tok (cmd);

    auto it = tok.begin ();

    if (it == tok.end ()) return;

    if (*it == "help") {
      main_window->add_mode (new HelpMode ());

    } else if (*it == "archive") {
      it++;
      if (it == tok.end ()) return;

      if (*it == "thread") {
        it++;
        if (it == tok.end ()) return;

        ustring thread_id = *it;

        cout << "cb: toggle archive on thread: " << thread_id << endl;
      }

    } else {
      cout  << "cb: unknown command: " << cmd << endl;
    }
  }

  void CommandBar::disable_command () {

  }

  bool CommandBar::command_handle_event (GdkEventKey * event) {
    return handle_event (event);
  }
}

