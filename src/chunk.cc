# include <vector>
# include <iostream>
# include <atomic>

# include <glib.h>
# include <gmime/gmime.h>

# include "astroid.hh"
# include "chunk.hh"
# include "gmime_iostream.hh"
# include "log.hh"

namespace Astroid {

  atomic<uint> Chunk::nextid (0);

  Chunk::Chunk (GMimeObject * mp) : mime_object (mp) {
    id = nextid++;

    content_type = g_mime_object_get_content_type (mime_object);

    log << debug << "chunk: content-type: " << g_mime_content_type_to_string (content_type) << endl;

    if (GMIME_IS_PART (mime_object)) {
      // has no sub-parts

      string disposition = g_mime_object_get_disposition(mime_object) ? : string();
      viewable = !(disposition == "attachment");

      const char * cid = g_mime_part_get_content_id ((GMimePart *) mime_object);
      if (cid != NULL) {
        content_id = ustring(cid);
        log << debug << "chunk: part, id: " << content_id << endl;
      }

      if (content_type != NULL) {
        if (viewable) {
          /* check if we can show this type */
          viewable = false;

          for (auto &m : viewable_types) {
            if (g_mime_content_type_is_type (content_type,
                  g_mime_content_type_get_media_type (m.second),
                  g_mime_content_type_get_media_subtype (m.second))) {

              viewable = true;
              break;
            }
          }
        }
      } else {
        viewable = false;
      }

      attachment = !viewable;

      log << debug << "chunk: is part (viewable: " << viewable << ", attachment: " << attachment << ") " << endl;

    } else if GMIME_IS_MESSAGE_PART (mime_object) {
      log << debug << "chunk: message part" << endl;

      /* contains a GMimeMessage with a potential substructure */
      GMimeMessage * msg = g_mime_message_part_get_message ((GMimeMessagePart *) mime_object);
      kids.push_back (refptr<Chunk>(new Chunk((GMimeObject *) msg)));

    } else if GMIME_IS_MESSAGE_PARTIAL (mime_object) {
      log << debug << "chunk: partial" << endl;

      GMimeMessage * msg = g_mime_message_partial_reconstruct_message (
          (GMimeMessagePartial **) &mime_object,
          g_mime_message_partial_get_total ((GMimeMessagePartial *) mime_object)
          );

      kids.push_back (refptr<Chunk>(new Chunk((GMimeObject *) msg)));

    } else if GMIME_IS_MULTIPART (mime_object) {
      log << debug << "chunk: multi part" << endl;
      //  TODO: MultiPartEncrypted, MultiPartSigned

      bool alternative = (g_mime_content_type_is_type (content_type, "multipart", "alternative"));
      log << debug << "chunk: alternative: " << alternative << endl;

      int total = g_mime_multipart_get_count ((GMimeMultipart *) mime_object);

      for (int i = 0; i < total; i++) {
        GMimeObject * mo = g_mime_multipart_get_part (
            (GMimeMultipart *) mime_object,
            i);

        kids.push_back (refptr<Chunk>(new Chunk(mo)));
      }

      if (alternative) {
        for_each (
            kids.begin(),
            kids.end(),
            [&] (refptr<Chunk> c) {
              for_each (
                  kids.begin(),
                  kids.end(),
                  [&] (refptr<Chunk> cc) {
                    if (c != cc) {
                      log << debug << "chunk: multipart: added sibling" << endl;
                      c->siblings.push_back (cc);
                    }
                  }
                );

              if (g_mime_content_type_is_type (c->content_type,
                  g_mime_content_type_get_media_type (preferred_type),
                  g_mime_content_type_get_media_subtype (preferred_type)))
              {
                log << debug << "chunk: multipart: preferred." << endl;
                c->preferred = true;
              }
            }
          );
      }

      log << debug << "chunk: multi part end" << endl;

    } else if GMIME_IS_MESSAGE (mime_object) {
      log << debug << "chunk: mime message" << endl;

    }
  }

  ustring Chunk::viewable_text (bool html = true) {

    GMimeStream * content_stream = NULL;

    if (GMIME_IS_PART(mime_object)) {
      log << debug << "chunk: body: part" << endl;


      if (g_mime_content_type_is_type (content_type, "text", "plain")) {
        log << debug << "chunk: plain text (out html: " << html << ")" << endl;

        GMimeDataWrapper * content = g_mime_part_get_content_object (
            (GMimePart *) mime_object);

        const char * charset = g_mime_object_get_content_type_parameter(GMIME_OBJECT(mime_object), "charset");
        GMimeStream * stream = g_mime_data_wrapper_get_stream (content);

        GMimeStream * filter_stream = g_mime_stream_filter_new (stream);

        /* convert to html */
        guint32 cite_color = 0x1e1e1e;

        /* other filters:
         *
         * GMIME_FILTER_HTML_PRE ||
         */
        guint32 html_filter_flags = GMIME_FILTER_HTML_CONVERT_NL |
                                    GMIME_FILTER_HTML_CONVERT_SPACES |
                                    GMIME_FILTER_HTML_CONVERT_URLS |
                                    GMIME_FILTER_HTML_CONVERT_ADDRESSES ;

        /* convert encoding */
        GMimeContentEncoding enc = g_mime_data_wrapper_get_encoding (content);
        if (enc) {
          log << debug << "enc: " << g_mime_content_encoding_to_string(enc) << endl;
        }

        GMimeFilter * filter = g_mime_filter_basic_new(enc, false);
        g_mime_stream_filter_add(GMIME_STREAM_FILTER(filter_stream), filter);
        g_object_unref(filter);

        if (charset)
        {
          log << debug << "charset: " << charset << endl;
          if (string(charset) == "utf-8") {
            charset = "UTF-8";
          }

          GMimeFilter * filter = g_mime_filter_charset_new(charset, "UTF-8");
          g_mime_stream_filter_add(GMIME_STREAM_FILTER(filter_stream), filter);
          g_object_unref(filter);
        } else {
          log << warn << "charset: not defined." << endl;
        }

        /* CRLF to LF */
        GMimeFilter * crlf_filter = g_mime_filter_crlf_new (false, false);
        g_mime_stream_filter_add (GMIME_STREAM_FILTER (filter_stream),
            crlf_filter);
        g_object_unref (crlf_filter);


        GMimeFilter * html_filter;
        if (html) {

          html_filter = g_mime_filter_html_new (html_filter_flags, cite_color);
          g_mime_stream_filter_add (GMIME_STREAM_FILTER(filter_stream),
                                  html_filter);
          g_object_unref (html_filter);
        }

        g_mime_stream_reset (stream);

        content_stream = filter_stream;

      } else if (g_mime_content_type_is_type (content_type, "text", "html")) {
        log << debug << "chunk: html text" << endl;

        GMimeDataWrapper * content = g_mime_part_get_content_object (
            (GMimePart *) mime_object);

        const char * charset = g_mime_object_get_content_type_parameter(GMIME_OBJECT(mime_object), "charset");
        GMimeStream * stream = g_mime_data_wrapper_get_stream (content);

        GMimeStream * filter_stream = g_mime_stream_filter_new (stream);

        /* convert encoding */
        GMimeContentEncoding enc = g_mime_data_wrapper_get_encoding (content);
        if (enc) {
          log << debug << "enc: " << g_mime_content_encoding_to_string(enc) << endl;
        }

        GMimeFilter * filter = g_mime_filter_basic_new(enc, false);
        g_mime_stream_filter_add(GMIME_STREAM_FILTER(filter_stream), filter);
        g_object_unref(filter);

        if (charset)
        {
          log << debug << "charset: " << charset << endl;
          if (string(charset) == "utf-8") {
            charset = "UTF-8";
          }

          GMimeFilter * filter = g_mime_filter_charset_new(charset, "UTF-8");
          g_mime_stream_filter_add(GMIME_STREAM_FILTER(filter_stream), filter);
          g_object_unref(filter);
        } else {
          log << warn << "charset: not defined" << endl;
        }



        g_mime_stream_reset (stream);

        content_stream = filter_stream;
      }
    }

    if (content_stream != NULL) {
      GMimeIOStream io_stream (content_stream);
      g_object_unref (content_stream);

      stringstream sstr;
      sstr << io_stream.rdbuf();
      return ustring (sstr.str());
    } else {
      return ustring ("Error: Non-viewable part!");
      log << error << "chunk: tried to display non-viewable part." << endl;
      //throw runtime_error ("chunk: tried to display non-viewable part.");
    }
  }

  ustring Chunk::get_filename () {
    if (GMIME_IS_PART (mime_object)) {
      const char * s = g_mime_part_get_filename (GMIME_PART(mime_object));

      if (s != NULL) {
        ustring fname (s);
        delete s;
        return fname;
      }
    }
    // no filename specified
    return ustring ("");
  }

  size_t Chunk::get_file_size () {
    time_t t0 = clock ();

    // https://github.com/skx/lumail/blob/master/util/attachments.c

    size_t sz = 0;

    if (GMIME_IS_PART (mime_object)) {
      GMimeStream * mem = g_mime_stream_mem_new ();

      GMimeDataWrapper * content = g_mime_part_get_content_object (GMIME_PART (mime_object));

      g_mime_data_wrapper_write_to_stream (content, mem);

      //g_mime_stream_mem_set_owner (GMIME_STREAM (mem), false);

      GByteArray * res = g_mime_stream_mem_get_byte_array (GMIME_STREAM_MEM (mem));

      sz = res->len;

      /* mem owns the bytearray */
      g_object_unref (mem);
    }

    log << info << "chunk: file size: " << sz << " (time used to calculate: " << ( (clock () - t0) * 1000.0 / CLOCKS_PER_SEC ) << " ms.)" << endl;

    return sz;
  }

  refptr<Glib::ByteArray> Chunk::contents () {
    time_t t0 = clock ();

    // https://github.com/skx/lumail/blob/master/util/attachments.c

    if (GMIME_IS_PART (mime_object)) {
      GMimeStream * mem = g_mime_stream_mem_new ();

      GMimeDataWrapper * content = g_mime_part_get_content_object (GMIME_PART (mime_object));

      g_mime_data_wrapper_write_to_stream (content, mem);

      GByteArray * res = g_mime_stream_mem_get_byte_array (GMIME_STREAM_MEM (mem));

      auto data = Glib::ByteArray::create ();
      if (res != NULL) {
        data->append (res->data, res->len);
      }

      g_object_unref (mem);

      log << info << "chunk: attachment: loaded " << data->size () << " bytes in " << ( (clock () - t0) * 1000.0 / CLOCKS_PER_SEC ) << " ms." << endl;

      return data;
    } else {
      log << error << "Chunk::contents() not supported on non-part." << endl;
      throw runtime_error ("Chunk::contents() not supported on non-part.");
    }
  }

  Chunk::~Chunk () {
    //g_object_unref (mime_object); // TODO: not sure about this one..
    g_object_unref (content_type);
  }
}

