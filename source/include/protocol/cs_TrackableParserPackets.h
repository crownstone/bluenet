/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 2, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#ifndef SOURCE_INCLUDE_PROTOCOL_CS_TRACKABLEPARSERPACKETS_H_
#define SOURCE_INCLUDE_PROTOCOL_CS_TRACKABLEPARSERPACKETS_H_

// ------------------ Command values -----------------

struct __attribute__((__packed__)) trackable_parser_cmd_upload_filter_t {

};

struct __attribute__((__packed__)) trackable_parser_cmd_remove_filter_t {

};

struct __attribute__((__packed__)) trackable_parser_cmd_commit_filter_changes_t {

};

struct __attribute__((__packed__)) trackable_parser_cmd_get_filer_summaries_t {

};

// ------------------ Return values ------------------


struct __attribute__((__packed__)) trackable_parser_cmd_upload_filter_ret_t {

};

struct __attribute__((__packed__)) trackable_parser_cmd_remove_filter_ret_t {

};

struct __attribute__((__packed__)) trackable_parser_cmd_commit_filter_changes_ret_t {

};

struct __attribute__((__packed__)) trackable_parser_cmd_get_filer_summaries_ret_t {

};


#endif /* SOURCE_INCLUDE_PROTOCOL_CS_TRACKABLEPARSERPACKETS_H_ */
