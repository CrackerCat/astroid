# include <iostream>
# include <boost/filesystem.hpp>

# include <glibmm.h>

# include <notmuch.h>

# include "db.hh"

using namespace std;
using namespace boost::filesystem;

namespace Gulp {
  Db::Db () {
    cout << "db: opening db..";

    const char * home = getenv ("HOME");
    if (home == NULL) {
      cerr << "db: error: HOME variable not set." << endl;
      exit (1);
    }

    path path_db (home);
    path_db /= ".mail";


    notmuch_status_t s = notmuch_database_open (path_db.c_str(),
        notmuch_database_mode_t::NOTMUCH_DATABASE_MODE_READ_ONLY,
        &nm_db);

    if (s != 0) {
      cerr << "db: error: failed opening database." << endl;
      exit (1);
    }

    cout << "ok." << endl;

    test_query ();
  }

  void Db::test_query () {
    cout << "db: running test query.." << endl;
    auto q = notmuch_query_create (nm_db, "label:inbox");

    cout << "query: " << notmuch_query_get_query_string (q) << ", approx: "
         << notmuch_query_count_messages (q) << " messages." << endl;

    notmuch_messages_t * messages;
    notmuch_message_t  * message;

    for (messages = notmuch_query_search_messages (q);
         notmuch_messages_valid (messages);
         notmuch_messages_move_to_next (messages)) {
      message = notmuch_messages_get (messages);

      cout << "thread:" << notmuch_message_get_thread_id (message) << ", message: " << notmuch_message_get_header (message, "Subject") << endl;

      notmuch_message_destroy (message);
    }
  }

  Db::~Db () {
    cout << "db: closing notmuch database." << endl << flush;
    if (nm_db != NULL) notmuch_database_close (nm_db);
  }

  /* notmuch thread */
  NotmuchThread::NotmuchThread () { };
  NotmuchThread::NotmuchThread (notmuch_thread_t * t) {
    nm_thread = t;
    thread_id = notmuch_thread_get_thread_id (t);
  }

  /* get subject of thread */
  ustring * NotmuchThread::get_subject () {
    return new ustring ("hey" + thread_id);
    /*
    if (notmuch_thread_valid (nm_thread)) {
      return new ustring (notmuch_thread_get_subject (nm_thread));
    } else {
      throw invalid_argument ("Notmuch Thread is no longer valid."); 
    }
    */
  }
}

