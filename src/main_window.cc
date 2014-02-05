# include <iostream>

# include <gtkmm.h>
# include <gtkmm/widget.h>

# include "gulp.hh"
# include "main_window.hh"
# include "modes/mode.hh"
# include "modes/thread_index.hh"

using namespace std;

namespace Gulp {
  MainWindow::MainWindow () {
    cout << "mw: init.." << endl;

    set_title ("Gulp");
    set_default_size (800, 400);

    add (notebook);

    show_all ();

    /* connect keys */
    add_events (Gdk::KEY_PRESS_MASK);
    signal_key_press_event ().connect (
        sigc::mem_fun(*this, &MainWindow::on_key_press));

  }

  bool MainWindow::on_key_press (GdkEventKey * event) {
    switch (event->keyval) {
      case GDK_KEY_q:
      case GDK_KEY_Q:
        gulp->quit ();
    }
    return true;
  }

  MainWindow::~MainWindow () {
    cout << "mw: done." << endl;
  }

  void MainWindow::add_mode (Mode * m) {
    modes.push_back (m);
    Gtk::Widget * w = m;
    notebook.append_page ((*w), *(m->tab_widget));
    notebook.show_all ();
  }
}

