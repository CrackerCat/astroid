# pragma once

# include <glibmm.h>
# include <vector>

# include "proto.hh"

namespace Astroid {
  class Action : public Glib::Object {
    public:
      virtual bool doit (Db *) = 0;
      virtual bool undo (Db *) = 0;
      virtual bool undoable ();

      /* used when undoing, the action_worker will undo the action
       * without adding it to the doneactions */
      bool in_undo = false;

      /* generally actions need a read-write db */
      bool need_db    = true; /* only this will give ro-db */
      bool need_db_rw = true; /* only this will lock db-rw */

      virtual void emit (Db *) = 0;
  };
}
