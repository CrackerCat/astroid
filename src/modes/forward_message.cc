# include <iostream>

# include "astroid.hh"
# include "log.hh"
# include "edit_message.hh"
# include "forward_message.hh"

# include "message_thread.hh"
# include "utils/address.hh"

using namespace std;

namespace Astroid {
  ForwardMessage::ForwardMessage (MainWindow * mw, refptr<Message> _msg) : EditMessage (mw) {
    msg = _msg;

    log << info << "fwd: forwarding message " << msg->mid << endl;

    /* set subject */
    subject = ustring::compose ("Fwd: %1", msg->subject);

    if (subject.size () > 0) {
      ((Gtk::Label*)tab_widget)->set_text ("New message: " + subject);
    }

    /* quote original message */
    ostringstream quoted;

    ustring quoting_a = ustring::compose ("Forwarding %1's message of %2:",
        Address(msg->sender.raw()).fail_safe_name(), msg->pretty_verbose_date());

    quoted  << quoting_a.raw ()
            << endl << endl;

    string vt = msg->viewable_text(false);
    quoted << vt;

    body = ustring(quoted.str());

    /* TODO: add attachments or other non-viewable parts */

    /* reload message */
    prepare_message ();
    read_edited_message ();

    activate_field (Thread);
  }
}

