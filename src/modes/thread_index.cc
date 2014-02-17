# include <iostream>
# include <string>
# include <exception>

# include <gtkmm.h>
# include <gtkmm/label.h>

# include <notmuch.h>

# include "../gulp.hh"
# include "../db.hh"
# include "paned_mode.hh"
# include "thread_index.hh"
# include "thread_index_list_view.hh"
# include "thread_index_list_cell_renderer.hh"
# include "thread_view.hh"

using namespace std;

namespace Gulp {
  ThreadIndex::ThreadIndex (string _query) : query_string(_query){

    set_orientation (Gtk::Orientation::ORIENTATION_VERTICAL);
    tab_widget = new Gtk::Label (query_string);
    tab_widget->set_can_focus (false);

    /* set up notmuch query */
    query =  notmuch_query_create (gulp->db->nm_db, query_string.c_str ());

    cout << "index, query: " << notmuch_query_get_query_string (query) << ", approx: "
         << notmuch_query_count_messages (query) << " messages." << endl;


    /* set up treeview */
    list_store = Glib::RefPtr<ThreadIndexListStore>(new ThreadIndexListStore ());
    list_view  = new ThreadIndexListView (list_store);
    scroll     = new ThreadIndexScrolled (list_store, list_view);

    add_pane (*scroll);

    show_all ();

    /* add threads to model */
    add_threads ();

    /* select first */
    list_view->set_cursor (Gtk::TreePath("0"));
  }

  void ThreadIndex::add_threads () {
    notmuch_threads_t * threads;
    notmuch_thread_t  * thread;

    cout << "index: add threads.." << endl;

    int i = 0;

    for (threads = notmuch_query_search_threads (query);
         notmuch_threads_valid (threads);
         notmuch_threads_move_to_next (threads)) {

      thread = notmuch_threads_get (threads);

      NotmuchThread *t = new NotmuchThread (thread);

      auto iter = list_store->append ();
      Gtk::ListStore::Row row = *iter;

      row[list_store->columns.thread_id] = notmuch_thread_get_thread_id (thread);
      row[list_store->columns.thread]    = Glib::RefPtr<NotmuchThread>(t);

      i++;

      if (i >= initial_max_threads) {
        break;
      }
    }

    notmuch_threads_destroy (threads);
    notmuch_query_destroy (query);
  }

  void ThreadIndex::open_thread (ustring thread_id) {
    if (thread_view == NULL) {
      thread_view = new ThreadView ();
    }

    thread_view->load_thread (thread_id);
    thread_view->render ();

    if (!thread_view_visible) {
      add_pane (*thread_view);
      thread_view_visible = true;
    }

    // grab modal
    current = 1;
    grab_modal ();

  }

  ThreadIndex::~ThreadIndex () {

  }
}
