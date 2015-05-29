# include <iostream>
# include <string>

# include <notmuch.h>
# include <gmime/gmime.h>
# include <boost/filesystem.hpp>

# include "astroid.hh"
# include "db.hh"
# include "log.hh"
# include "message_thread.hh"
# include "chunk.hh"
# include "utils/date_utils.hh"
# include "utils/address.hh"

using namespace std;
using namespace boost::filesystem;

namespace Astroid {
  /* --------
   * Message
   * --------
   */
  Message::Message () {
    in_notmuch = false;
    has_file   = false;
  }

  Message::Message (ustring _fname) : fname (_fname) {

    log << info << "msg: loading message from file: " << fname << endl;
    in_notmuch = false;
    has_file   = true;
    load_message_from_file (fname);
  }

  Message::Message (ustring _mid, ustring _fname) {
    mid = _mid;
    fname = _fname;
    log << info << "msg: loading message from file (mid supplied): " << fname << endl;
    in_notmuch = false;
    has_file   = true;
    load_message_from_file (fname);
  }

  Message::Message (notmuch_message_t *message) {
    /* The caller must make sure the message pointer
     * is valid and not destroyed while initializing */

    mid = notmuch_message_get_message_id (message);
    in_notmuch = true;
    has_file   = true;

    log << info << "msg: loading mid: " << mid << endl;

    fname = notmuch_message_get_filename (message);
    log << info << "msg: filename: " << fname << endl;

    load_message_from_file (fname);
    load_tags (message);
  }

  Message::Message (GMimeMessage * _msg) {
    log << info << "msg: loading message from GMimeMessage." << endl;
    in_notmuch = false;
    has_file   = false;

    load_message (_msg);
  }

  Message::~Message () {
    //g_object_unref (message);
  }

  void Message::load_tags (Db * db) {
    if (!in_notmuch) {
      log << error << "mt: error: message not in database." << endl;
      throw invalid_argument ("mt: load_tags on message not in database.");
    }

    if (mid == "") {
      log << error << "mt: error: mid not defined, no tags" << endl;
      throw invalid_argument ("mt: load_tags on message without message id.");
    } else {
      /* get tags from nm db */

      lock_guard<Db> grd (*db);
      db->on_message (mid, [&](notmuch_message_t * msg)
        {
          load_tags (msg);
        });
    }
  }

  void Message::load_tags (notmuch_message_t * msg) {

    tags.clear ();

    notmuch_tags_t * ntags;
    for (ntags = notmuch_message_get_tags (msg);
         notmuch_tags_valid (ntags);
         notmuch_tags_move_to_next (ntags)) {

      tags.push_back (ustring(notmuch_tags_get (ntags)));
    }

  }

  void Message::load_message_from_file (ustring fname) {
    GMimeStream   * stream  = g_mime_stream_file_new_for_path (fname.c_str(), "r");
    if (stream == NULL) {
      if (!exists (fname.c_str())) {
        log << error << "failed to open file: " << fname << ", it does not exist!" << endl;
      } else {
        log << error << "failed to open file: " << fname << " (unspecified error)" << endl;
      }

      string error_s = "failed to open file: " + fname;
      throw message_error (error_s.c_str());

    }

    GMimeParser   * parser  = g_mime_parser_new_with_stream (stream);

    GMimeMessage * _message = g_mime_parser_construct_message (parser);

    load_message (_message);

    g_object_unref (stream); // reffed from parser
    g_object_unref (parser); // reffed from message
  }

  void Message::load_message (GMimeMessage * _msg) {

    /* Load message with parts.
     *
     * example:
     * https://git.gnome.org/browse/gmime/tree/examples/basic-example.c
     *
     *
     * Do this a bit like sup:
     * - build up a tree/list of chunks that are viewable, except siblings.
     * - show text and html parts
     * - show a gallery of attachments at the bottom
     *
     */

    message = _msg;

    if (mid == "") {
      mid = g_mime_message_get_message_id (message);
    }

    /* read header fields */
    const char *c;
    c  = g_mime_message_get_sender (message);
    if (c != NULL) sender = ustring (c);
    else sender = "";

    c = g_mime_message_get_subject (message);
    if (c != NULL) subject = ustring (c);
    else subject = "";

    c = g_mime_object_get_header (GMIME_OBJECT(message), "In-Reply-To");
    if (c != NULL) inreplyto = ustring (c);
    else inreplyto = "";

    c = g_mime_object_get_header (GMIME_OBJECT(message), "References");
    if (c != NULL) references = ustring (c);
    else references = "";

    c = g_mime_object_get_header (GMIME_OBJECT(message), "Reply-To");
    if (c != NULL) reply_to = ustring (c);
    else reply_to = "";


    root = refptr<Chunk>(new Chunk (g_mime_message_get_mime_part (message)));

    g_object_ref (message);  // TODO: a little bit at loss here -> change to
                             //       std::something.
  }

  ustring Message::viewable_text (bool html, bool fallback_html) {
    /* build message body:
     * html:      output html (using gmimes html filter)
     *
     */

    if (html && fallback_html) {
      throw runtime_error ("message: html implies fallback_html");
    }

    ustring body;

    function< void (refptr<Chunk>) > app_body =
      [&] (refptr<Chunk> c)
    {
      /* check if we're the preferred sibling */
      bool use = false;

      if (c->siblings.size() >= 1) {
        if (c->preferred) {
          use = true;
        } else {
          /* check if there are any other preferred */
          if (all_of (c->siblings.begin (),
                      c->siblings.end (),
                      [](refptr<Chunk> c) { return (!c->preferred); })) {
            use = true;
          } else {
            use = false;
          }
        }
      } else {
        use = true;
      }

      if (use) {
        if (c->viewable && (c->preferred || html || fallback_html)) {
          body += c->viewable_text (html);
        }

        for_each (c->kids.begin(),
                  c->kids.end (),
                  app_body);
      }
    };

    app_body (root);

    return body;
  }

  vector<refptr<Chunk>> Message::attachments () {
    /* return a flat vector of attachments */

    vector<refptr<Chunk>> attachments;

    function< void (refptr<Chunk>) > app_attachment =
      [&] (refptr<Chunk> c)
    {
      if (c->attachment)
        attachments.push_back (c);

      for_each (c->kids.begin(),
                c->kids.end (),
                app_attachment);
    };

    app_attachment (root);

    return attachments;
  }

  refptr<Chunk> Message::get_chunk_by_id (int id) {
    if (root->id == id) {
      return root;
    } else {
      return root->get_by_id (id);
    }
  }

  vector<refptr<Chunk>> Message::mime_messages () {
    /* return a flat vector of mime messages */

    vector<refptr<Chunk>> mime_messages;

    function< void (refptr<Chunk>) > app_mm =
      [&] (refptr<Chunk> c)
    {
      if (c->mime_message)
        mime_messages.push_back (c);

      for_each (c->kids.begin(),
                c->kids.end (),
                app_mm);
    };

    app_mm (root);

    return mime_messages;
  }

  ustring Message::date () {
    return ustring (g_mime_message_get_date_as_string (message));
  }

  ustring Message::pretty_date () {
    time_t t;
    g_mime_message_get_date (message, &t, NULL);

    return Date::pretty_print (t);
  }

  ustring Message::pretty_verbose_date () {
    time_t t;
    g_mime_message_get_date (message, &t, NULL);

    return Date::pretty_print_verbose (t);
  }

  InternetAddressList * Message::to () {
    return g_mime_message_get_recipients (message, GMIME_RECIPIENT_TYPE_TO);
  }

  InternetAddressList * Message::cc () {
    return g_mime_message_get_recipients (message, GMIME_RECIPIENT_TYPE_CC);
  }

  InternetAddressList * Message::bcc () {
    return g_mime_message_get_recipients (message, GMIME_RECIPIENT_TYPE_BCC);
  }

  AddressList Message::all_to_from () {
    return ( AddressList(to()) + AddressList(cc()) + AddressList(bcc()) + Address(sender) );
  }

  void Message::save () {
    Gtk::FileChooserDialog dialog ("Save message..",
        Gtk::FILE_CHOOSER_ACTION_SAVE);

    dialog.add_button ("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button ("_Select", Gtk::RESPONSE_OK);
    dialog.set_do_overwrite_confirmation (true);

    ustring _f = root->get_filename ();
    if (_f.size () == 0) {
      if (is_patch ()) {
        _f = subject;

        auto pattern = Glib::Regex::create ("[^0-9A-Za-z.\\-]");
        _f = pattern->replace (_f, 0, "_", static_cast<Glib::RegexMatchFlags>(0));

        auto pattern2 = Glib::Regex::create ("^_+");
        _f = pattern2->replace (_f, 0, "", static_cast<Glib::RegexMatchFlags>(0));

        if (_f.substr(0,5).uppercase() == "PATCH")
          _f.erase (0, 5);

        _f = pattern2->replace (_f, 0, "", static_cast<Glib::RegexMatchFlags>(0));

        _f += ".patch";
      } else {
        _f = subject + ".eml";
      }
    }

    dialog.set_current_name (_f);

    int result = dialog.run ();

    switch (result) {
      case (Gtk::RESPONSE_OK):
        {
          string fname = dialog.get_filename ();

          save_to (fname);

          break;
        }

      default:
        {
          log << debug << "msg: save: cancelled." << endl;
        }
    }
  }

  void Message::save_to (ustring tofname) {
    log << info << "msg: saving to: " << tofname << endl;
    // apparently boost needs to be compiled with -std=c++0x
    // https://svn.boost.org/trac/boost/ticket/6124
    // copy_file ( path (fname), path (tofname) );

    if (has_file)
    {
      ifstream src (fname, ios::binary);
      ofstream dst (tofname, ios::binary);

      dst << src.rdbuf ();
    } else {
      /* write GMimeMessage */

      FILE * MessageFile = fopen(tofname.c_str(), "w");
      GMimeStream * stream = g_mime_stream_file_new(MessageFile);
      g_mime_object_write_to_stream(GMIME_OBJECT(message), stream);
      g_object_unref(stream);

    }
  }

  refptr<Glib::ByteArray> Message::contents () {
    return root->contents ();
  }

  bool Message::is_patch () {
    return (
        (subject.substr(0,3).uppercase() != "RE:") &&
         Glib::Regex::match_simple ("\\[PATCH.*\\]", subject));
  }

  /************
   * exceptions
   * **********
   */

  message_error::message_error (const char * w) : runtime_error (w)
  {
  }

  /* --------
   * MessageThread
   * --------
   */

  MessageThread::MessageThread () {
    in_notmuch = false;
  }

  MessageThread::MessageThread (refptr<NotmuchThread> _nmt) : thread (_nmt) {
    in_notmuch = true;

  }

  void MessageThread::load_messages (Db * db) {
    /* update values */
    subject     = thread->subject;
    /*
    newest_date = notmuch_thread_get_newest_date (nm_thread);
    unread      = check_unread (nm_thread);
    attachment  = check_attachment (nm_thread);
    */


    /* get messages from thread */
    db->on_thread (thread->thread_id, [&](notmuch_thread_t * nm_thread)
      {

        notmuch_messages_t * qmessages;
        notmuch_message_t  * message;

        for (qmessages = notmuch_thread_get_messages (nm_thread);
             notmuch_messages_valid (qmessages);
             notmuch_messages_move_to_next (qmessages)) {

          message = notmuch_messages_get (qmessages);

          messages.push_back (refptr<Message>(new Message (message)));
        }
      });
  }

  void MessageThread::add_message (ustring fname) {
    messages.push_back (refptr<Message>(new Message (fname)));
  }

  void MessageThread::add_message (refptr<Chunk> c) {
    if (!c->mime_message) {
      log << error << "mt: can only add message chunks that are GMimeMessages." << endl;
      throw runtime_error ("mt: can only add message chunks that are GMimeMessages");
    }

    messages.push_back (c->get_mime_message ());

    if (subject == "") {
      subject = (*(--messages.end()))->subject;
    }
  }


  void MessageThread::reload_messages () {
  }

}

