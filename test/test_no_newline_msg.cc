# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestNoNewline
# include <boost/test/unit_test.hpp>

# include "test_common.hh"
# include "db.hh"
# include "message_thread.hh"
# include "glibmm.h"

using namespace std;


BOOST_AUTO_TEST_SUITE(Reading)


  BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES( reading_no_new_line_error, 1 )
  BOOST_AUTO_TEST_CASE(reading_no_new_line_error)
  {
    setup ();

    ustring fname = "test/mail/test_mail/no-nl.eml";

    Message m (fname);

    ustring text =  m.viewable_text(false);
    BOOST_CHECK (text.find ("line-ignored") != ustring::npos);

    ustring html = m.viewable_text(true);
    BOOST_CHECK (html.find ("line-ignored") != ustring::npos);

    teardown ();
  }

BOOST_AUTO_TEST_SUITE_END()

