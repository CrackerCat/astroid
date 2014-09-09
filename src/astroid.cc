# include <iostream>
# include <vector>
# include <memory>

# include <gtkmm.h>
# include <gtkmm/window.h>

/* program options */
# include <boost/program_options.hpp>

# include "astroid.hh"
# include "version.hh"
# include "db.hh"
# include "config.hh"
# include "account_manager.hh"
# include "actions/action_manager.hh"
# include "contacts.hh"
# include "utils/date_utils.hh"
# include "log.hh"
# include "poll.hh"

/* UI */
# include "main_window.hh"
# include "modes/thread_index.hh"

/* gmime */
# include <gmime/gmime.h>

using namespace std;

/* globally available static instance of the Astroid */
Astroid::Astroid * (Astroid::astroid);
Astroid::Log Astroid::log;

int main (int argc, char **argv) {
  Astroid::astroid = new Astroid::Astroid ();
  return Astroid::astroid->main (argc, argv);
}

namespace Astroid {

  Astroid::Astroid () {
  }

  int Astroid::main (int argc, char **argv) {
    Glib::init ();

    log.add_out_stream (&cout);

    log << info << "welcome to astroid! - " << GIT_DESC << endl;

    /* set up gtk */
    int aargc = 1; // TODO: allow GTK to get some options aswell.
    app = Gtk::Application::create (aargc, argv, "org.astroid");

    app->register_application ();

    if (app->is_remote ()) {
      log << warn << "astroid: instance already running, opening new window.." << endl;

      app->activate ();
      return 0;
    } else {
      /* we are the main instance */
      app->signal_activate().connect (sigc::mem_fun (this,
            &Astroid::on_signal_activate));

    }

    /* user agent */
    user_agent = ustring::compose ("astroid/v%1 (https://github.com/gauteh/astroid)", GIT_DESC);

    /* options */
    namespace po = boost::program_options;
    po::options_description desc ("options");
    desc.add_options ()
      ( "help,h", "print this help message")
      ( "config,c", po::value<ustring>(), "config file, default: $XDG_CONFIG_HOME/astroid/config");

    po::variables_map vm;
    po::store ( po::command_line_parser (argc, argv).options(desc).run(), vm );

    if (vm.count ("help")) {
      cout << desc << endl;

      exit (0);
    }

    /* load config */
    if (vm.count("config")) {
      config = new Config (vm["config"].as<ustring>().c_str());
    } else {
      config = new Config ();
    }

    /* gmime settings */
    g_mime_init (0); // utf-8 is default

    /* set up static classes */
    Date::init ();

    /* set up accounts */
    accounts = new AccountManager ();

    /* set up contacts */
    contacts = new Contacts ();

    /* set up global actions */
    global_actions = new GlobalActions ();

    /* set up poller */
    poll = new Poll ();

    open_new_window ();
    app->run ();

    /* clean up and exit */
    delete accounts;
    delete contacts;
    delete config;

    log << info << "astroid: goodbye!" << endl;

    return 0;
  }

  void Astroid::quit () {
    app->quit ();

    log << info << "astroid: goodbye!" << endl;
  }

  MainWindow * Astroid::open_new_window () {
    log << warn << "astroid: starting a new window.." << endl;

    /* set up a new main window */

    /* start up default window with default buffers */
    MainWindow * mw = new MainWindow (); // is freed / destroyed by application

    ptree qpt = config->config.get_child ("startup.queries");

    for (const auto &kv : qpt) {
      ustring name = kv.first;
      ustring query = kv.second.data();

      log << info << "astroid: got query: " << name << ": " << query << endl;
      mw->add_mode (new ThreadIndex (mw, query));
    }

    mw->set_active (0);

    app->add_window (*mw);
    mw->show_all ();

    return mw;
  }

  void Astroid::on_signal_activate () {
    if (activated) {
      open_new_window ();
    } else {
      /* we'll get one activated signal from ourselves first */
      activated = true;
    }
  }

}

