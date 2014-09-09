# include <iostream>

# include <gtkmm.h>
# include <gtkmm/widget.h>

# include "astroid.hh"
# include "version.hh"
# include "poll.hh"
# include "main_window.hh"
# include "modes/mode.hh"
# include "modes/thread_index.hh"
# include "modes/help_mode.hh"
# include "modes/edit_message.hh"
# include "modes/log_view.hh"
# include "command_bar.hh"
# include "actions/action.hh"
# include "actions/action_manager.hh"

using namespace std;

namespace Astroid {
  MainWindow::MainWindow () {
    log << debug << "mw: init.." << endl;

    set_title ("Astroid - " GIT_DESC);
    set_default_size (1040, 800);

    actions.main_window = this;

    Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = theme->load_icon (
        "mail-send-symbolic", 42, Gtk::ICON_LOOKUP_USE_BUILTIN );
    set_icon (pixbuf);

    vbox.set_orientation (Gtk::ORIENTATION_VERTICAL);

    command.main_window = this;

    command.property_search_mode_enabled().signal_changed().connect(
        sigc::mem_fun (*this, &MainWindow::on_command_mode_changed)
        );

    vbox.pack_start (command, false, true, 0);
    vbox.pack_start (notebook, Gtk::PACK_EXPAND_WIDGET, 0);

    add (vbox);

    show_all_children ();

    /* connect keys */
    add_events (Gdk::KEY_PRESS_MASK);
    signal_key_press_event ().connect (
        sigc::mem_fun(*this, &MainWindow::on_key_press));

    /* got focus, grab keys */
    signal_focus_in_event ().connect (
        sigc::mem_fun (this, &MainWindow::on_my_focus_in_event));
    signal_focus_out_event ().connect (
        sigc::mem_fun (this, &MainWindow::on_my_focus_out_event));
  }

  void MainWindow::enable_command (CommandBar::CommandMode m, ustring cmd, function<void(ustring)> f) {
    unset_active ();
    command.enable_command (m, cmd, f);
    is_command = true;
    command.add_modal_grab ();
  }

  void MainWindow::disable_command () {
    // hides itself
    command.disable_command ();
    command.remove_modal_grab();
    set_active (lastcurrent);
    is_command = false;
  }

  void MainWindow::on_command_mode_changed () {
    if (!command.get_search_mode()) {
      disable_command ();
    }
  }

  bool MainWindow::on_key_press (GdkEventKey * event) {
    if (is_command) {
      command.command_handle_event (event);
      return true;
    }

    switch (event->keyval) {
      case GDK_KEY_q:
      case GDK_KEY_Q:
        log << info << "mw: quit." << endl;
        astroid->app->remove_window (*this);
        close ();
        return true;

      /* page through notebook */
      case GDK_KEY_b:
        if (notebook.get_current_page () == (notebook.get_n_pages () - 1))
          set_active (0);
        else
          set_active (notebook.get_current_page() + 1);
        return true;

      case GDK_KEY_B:
        if (notebook.get_current_page() == 0)
          set_active (notebook.get_n_pages()-1);
        else
          set_active (notebook.get_current_page() - 1);

        return true;

      /* close page */
      case GDK_KEY_x:
        {
          if (modes.size() > 1) {
            int c = notebook.get_current_page ();
            del_mode (c);
          } else {
            /* if there are more windows, close this one */
            if (astroid->app->get_windows().size () > 1) {
              log << info << "mw: more windows available, closing this one." << endl;
              astroid->app->remove_window (*this);
              close ();
            }
          }
        }
        return true;

      /* search */
      case GDK_KEY_F:
        enable_command (CommandBar::CommandMode::Search, "", NULL);
        return true;

      /* short cut for searching for label */
      case GDK_KEY_L:
        enable_command (CommandBar::CommandMode::Search, "tag:", NULL);
        return true;

      /* command */
      case GDK_KEY_colon:
        enable_command (CommandBar::CommandMode::Generic, "", NULL);
        return true;

      /* help */
      case GDK_KEY_question:
        add_mode (new HelpMode ());
        return true;

      /* log window */
      case GDK_KEY_z:
        add_mode (new LogView ());
        return true;

      /* undo */
      case GDK_KEY_u:
        {
          actions.undo ();
          return true;
        }

      case GDK_KEY_m:
        {
          add_mode (new EditMessage (this));
          return true;
        }

      case GDK_KEY_P:
        {
          astroid->poll->poll ();
          return true;
        }

      case GDK_KEY_O:
        {
          astroid->open_new_window ();
          return true;
        }

    }
    return false;
  }

  MainWindow::~MainWindow () {
    modes.clear (); // the modes themselves should be freed upon
                    // widget desctruction
    log << debug << "mw: done." << endl;
  }

  void MainWindow::add_mode (Mode * m) {
    m = Gtk::manage (m);
    auto it = modes.begin ();
    advance (it, current +1);
    modes.insert (it, m);
    Gtk::Widget * w = m;
    int n = notebook.insert_page ((*w), *(m->tab_widget), current+1);
    notebook.show_all ();

    set_active (n);
  }

  void MainWindow::del_mode (int c) {
    // log << debug << "mw: del mode: " << c << endl;
    if (c >= 0) {
      if (c == 0) {
        set_active (c + 1);
      } else {
        set_active (c - 1);
      }

      notebook.remove_page (c); // this should free the widget (?)

      if (modes.size() > 0) {
        auto it = modes.begin () + c;
        modes.erase (it);

        // mode is deleted by Gtk::manage.
      }
    } else {
      log << warn << "mw: attempt to remove negative page" << endl;
    }
  }

  void MainWindow::remove_all_modes () {
    // used by Astroid::quit to deconstruct all modes before
    // exiting.

    for (int n = modes.size ()-1; n >= 0; n--)
      del_mode (n);

  }

  void MainWindow::unset_active () {
    if (current >= 0) {
      if (static_cast<int>(modes.size()) > current) {
        //log << debug << "mw: release modal, from: " << current << endl;
        modes[current]->release_modal ();
        lastcurrent = current;
        current = -1;
      }
    }
  }

  void MainWindow::set_active (int n) {
    //log << debug << "mw: set active: " << n << ", current: " << current << endl;

    if (n >= 0 && n <= notebook.get_n_pages()-1) {

      if (notebook.get_current_page() != n) {
        notebook.set_current_page (n);
      }

      unset_active ();

      //log << debug << "mw: grab modal to: " << n << endl;
      modes[n]->grab_modal ();
      current = n;
    } else {
      // log << debug << "mw: set active: page is out of range: " << n << endl;
    }
  }

  bool MainWindow::on_my_focus_in_event (GdkEventFocus *event) {
    set_active (current);
    return false;
  }

  bool MainWindow::on_my_focus_out_event (GdkEventFocus *event) {
    unset_active ();
    return false;
  }
}

