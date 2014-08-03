# pragma once

# include <iostream>
# include <atomic>

# include <gtkmm.h>
# include <webkit/webkit.h>

# include "astroid.hh"
# include "proto.hh"
# include "mode.hh"

using namespace std;

namespace Astroid {
  extern "C" bool ThreadView_on_load_changed (
          GtkWidget *,
          GParamSpec *,
          gpointer );

  extern "C" WebKitWebView * ThreadView_activate_inspector (
      WebKitWebInspector *,
      WebKitWebView *,
      gpointer );

  extern "C" bool ThreadView_show_inspector (
      WebKitWebInspector *,
      gpointer);


  class ThreadView : public Mode {
    /*
    friend bool ThreadView_on_load_changed (
          GObject *,
          WebKitLoadEvent,
          gchar *,
          gpointer,
          gpointer );
    */
    public:
      ThreadView (MainWindow *);
      ~ThreadView ();
      void load_thread (refptr<NotmuchThread>);
      void load_message_thread (refptr<MessageThread>);

      refptr<NotmuchThread> thread;
      refptr<MessageThread> mthread;

      Gtk::ScrolledWindow scroll;

      const int MAX_TAB_SUBJECT_LEN = 15;

      MainWindow  * main_window;
    private:
      /* message manipulation and location */
      refptr<Message> get_current_message ();
      refptr<Message> get_next_message (refptr<Message>);
      void scroll_to_message (refptr<Message>);
      void update_focus_to_view ();
      void update_focus_status ();
      void focus_next ();
      void focus_previous ();

      /* focused message */
      refptr<Message> focused_message;

      /* webkit (using C api) */
      WebKitWebView     * webview;
      WebKitWebSettings * websettings;
      WebKitDOMHTMLDivElement * container = NULL;
      WebKitWebInspector * web_inspector;

      static bool theme_loaded;
      static const char *  thread_view_html_f;
      static const char *  thread_view_css_f;
      static ustring       thread_view_html;
      static ustring       thread_view_css;
      const char * STYLE_NAME = "STYLE";

      atomic<bool> wk_loaded;

      /* rendering */
      void render ();
      void render_post ();
      void add_message (refptr<Message>);
      WebKitDOMHTMLElement * make_message_div ();

      /* message loading setup */
      void set_message_html (refptr<Message>, WebKitDOMHTMLElement*);
      void insert_header_address (ustring &, ustring, ustring, bool);
      ustring create_header_row (ustring, ustring, bool);


      /* webkit dom utils */
      WebKitDOMHTMLElement * clone_select (
          WebKitDOMNode * node,
          ustring         selector,
          bool            deep = true);

      WebKitDOMHTMLElement * clone_node (
          WebKitDOMNode * node,
          bool            deep = true);

      WebKitDOMHTMLElement * select (
          WebKitDOMNode * node,
          ustring         selector);

    public:
      /* event wrappers */
      bool on_load_changed (
          GtkWidget *,
          GParamSpec *);

      Gtk::Window * inspector_window;
      Gtk::ScrolledWindow inspector_scroll;
      WebKitWebView * activate_inspector (
          WebKitWebInspector *,
          WebKitWebView *);
      bool show_inspector (WebKitWebInspector *);


      /* mode */
      virtual void grab_modal () override;
      virtual void release_modal () override;

    protected:
      virtual bool on_key_press_event (GdkEventKey *) override;
  };
}

