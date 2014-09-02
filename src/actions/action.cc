# include <iostream>

# include "astroid.hh"
# include "db.hh"

# include "action_manager.hh"
# include "action.hh"

using namespace std;

namespace Astroid {
  Action::Action (refptr<NotmuchThread> nmt) {

    thread = nmt;

  }

  bool Action::undoable () {
    return false;
  }

  void Action::emit (ActionManager * am, Db * db) {
    am->emit_thread_updated (db, thread->thread_id);
  }
}

