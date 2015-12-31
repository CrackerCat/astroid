# include <iostream>

# include "main_window.hh"
# include "mode.hh"
# include "help_mode.hh"
# include "build_config.hh"

# include <vector>
# include <tuple>

using namespace std;

namespace Astroid {
  HelpMode::HelpMode (MainWindow * mw) : Mode (mw) {
    set_label ("Help");

    scroll.add (help_text);
    pack_start (scroll, true, true, 5);

    scroll.show_all ();

    /* register keys */
    keys.register_key ("j",
        { Key("C-j"),
          Key (true, false, (guint) GDK_KEY_Down),
          Key(GDK_KEY_Down) },
        "help.down",
        "Scroll down",
        [&] (Key k) {
          if (k.ctrl) {
            auto adj = scroll.get_vadjustment ();
            adj->set_value (adj->get_value() + adj->get_page_increment ());
          } else {
            auto adj = scroll.get_vadjustment ();
            adj->set_value (adj->get_value() + adj->get_step_increment ());
          }

          return true;
        });

    keys.register_key ("k",
        { Key ("C-k"),
          Key (true, false, (guint) GDK_KEY_Up),
          Key (GDK_KEY_Up) },
        "help.up",
        "Scroll up",
        [&] (Key k) {
          if (k.ctrl) {
            auto adj = scroll.get_vadjustment ();
            adj->set_value (adj->get_value() - adj->get_page_increment ());
          } else {
            auto adj = scroll.get_vadjustment ();
            adj->set_value (adj->get_value() - adj->get_step_increment ());
          }

          return true;
        });

    keys.register_key ("K", { Key (GDK_KEY_Page_Up) }, "help.page_up",
        "Page up",
        [&] (Key k) {
          auto adj = scroll.get_vadjustment ();
          adj->set_value (adj->get_value() - adj->get_page_increment ());
          return true;
        });

    keys.register_key ("J", { Key (GDK_KEY_Page_Down) }, "help.page_down",
        "Page down",
        [&] (Key k) {
          auto adj = scroll.get_vadjustment ();
          adj->set_value (adj->get_value() + adj->get_page_increment ());
          return true;
        });

    keys.register_key ("1", { Key (GDK_KEY_Home) }, "help.page_top",
        "Scroll to top",
        [&] (Key k) {
          auto adj = scroll.get_vadjustment ();
          adj->set_value (adj->get_lower ());
          return true;
        });

    keys.register_key ("0", { Key (GDK_KEY_End) }, "help.page_end",
        "Scroll to end",
        [&] (Key k) {
          auto adj = scroll.get_vadjustment ();
          adj->set_value (adj->get_upper ());
          return true;
        });

  }

  HelpMode::HelpMode (MainWindow *mw, Mode * m) : HelpMode (mw) {
    show_help (m);
  }

  void HelpMode::show_help (Mode * m) {
    ModeHelpInfo * mhelp = m->key_help ();

    set_label ("Help: " + mhelp->title);

    ustring header =
    "<b>Astroid</b> (" GIT_DESC ") \n"
    "\n"
    "Gaute Hope &lt;<a href=\"mailto:eg@gaute.vetsj.com\">eg@gaute.vetsj.com</a>&gt; (c) 2014"
    " (<i>Licenced under the GNU GPL v3</i>)\n"
    "<a href=\"https://github.com/gauteh/astroid\">https://github.com/gauteh/astroid</a> | <a href=\"mailto:astroidmail@googlegroups.com\">astroidmail@googlegroups.com</a>\n"
    "\n";

    ustring help = header + generate_help (m);

    delete mhelp;

    help_text.set_markup (help);
  }

  ustring HelpMode::generate_help (Gtk::Widget * w) {
    ustring h;

    MainWindow * mw = dynamic_cast<MainWindow *> (w);
    bool is_mw = (mw != NULL); // we stop here

    Mode * m   = dynamic_cast<Mode *> (w);
    bool is_mode = (m != NULL);


    if (!is_mw) {
      h = generate_help (w->get_parent ());
    }

    if (is_mode) {
      h += "---\n";
      h += "<i>" + m->mode_help_title + "</i>";

      h += m->keys.help ();
    }

    return h;
  }

  void HelpMode::grab_modal () {
    add_modal_grab ();
    grab_focus ();
  }

  void HelpMode::release_modal () {
    remove_modal_grab ();
  }
};

