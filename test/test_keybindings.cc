# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestKeybindings
# include <boost/test/unit_test.hpp>

# include "test_common.hh"
# include "glibmm.h"

# include "modes/keybindings.hh"
# include "utils/cmd.hh"

using namespace std;
using Astroid::ustring;


BOOST_AUTO_TEST_SUITE(Keybindings)

  BOOST_AUTO_TEST_CASE(loading_keybindings)
  {
    using namespace Astroid;
    using Astroid::log;
    setup ();

    Astroid::Keybindings::init ();

    Astroid::Keybindings keys;
    keys.set_prefix ("Test", "test");

    keys.register_key ("a", "test.a", "A", [&] (Key) { return true; });

    BOOST_CHECK_THROW (
      keys.register_key ("b", "test.b", "B", [&] (Key) { return true; }),
      duplicatekey_error);


    keys.register_key (UnboundKey (), "test.unbound", "U1", [&] (Key) { return true; });
    keys.register_key (UnboundKey (), "test.unbound2", "U2", [&] (Key) { return true; });

    /* check a bad key spec */
    BOOST_CHECK_THROW (
      keys.register_key ("1-a", "test.k", "bad keyspec", [&] (Key) { return true; }),
      keyspec_error);


    /* check for duplicate when defining */
    BOOST_CHECK_THROW (
      keys.register_key ("a", "test.a", "duplicate keyspec", [&] (Key) { return true; }),
      duplicatekey_error);


    /* test run hook */
    ustring test_thread = "001";

    auto f = [&] (Key k, ustring cmd) {
      log << test << "key: run-hook got back: " << cmd << endl;

      ustring final_cmd = ustring::compose (cmd, test_thread);
      log << test << "key: would run: " << final_cmd << endl;

      Cmd("test", final_cmd).run ();

      return true;
    };

    keys.register_run ("test.run", f);

    Key n ("n");
    GdkEventKey e;
    e.keyval = n.key;

    keys.handle (&e);


    teardown ();
  }


BOOST_AUTO_TEST_SUITE_END()

