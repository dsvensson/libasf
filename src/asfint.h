
#ifndef ASFINT_H
#define ASFINT_H

#include "asf.h"
#include "guid.h"

struct asf_packet_int_s {
	/* This has to be exactly the same with asf.h asf_packet_s */
	uint8_t ec_length;
	uint8_t *ec_data;

	uint32_t length;
	uint32_t padding_length;
	uint32_t send_time;
	uint16_t duration;

	uint16_t payload_count;
	asf_payload_t *payloads;

	uint32_t datalen;
	uint8_t *payload_data;

	/* Hidden size data only available for internal use */
	uint8_t ec_data_size;
	uint16_t payloads_size;
	uint32_t payload_data_size;
};
typedef struct asf_packet_int_s asf_packet_int_t;

#define ASF_OBJECT_COMMON      \
	guid_t       guid;     \
	guid_type_t  type;     \
	uint64_t     size;     \
	uint64_t     datalen;  \
	uint8_t      *data;    \
	struct asf_object_s *next;

struct asf_object_s {
	ASF_OBJECT_COMMON
};
typedef struct asf_object_s asf_object_t;

struct asf_object_headerext_s {
	ASF_OBJECT_COMMON
	guid_t       reserved1;
	uint16_t     reserved2;
	struct asf_object_s *first;
	struct asf_object_s *last;
};
typedef struct asf_object_headerext_s asf_object_headerext_t;

struct asf_object_header_s {
	ASF_OBJECT_COMMON
	uint16_t     subobjects;
	uint8_t      reserved1; /* 0x01, but could be safely ignored */
	uint8_t      reserved2; /* 0x02, if not must failed to source the contain */
	asf_object_headerext_t *ext; /* this is here just for convenience */
	struct asf_object_s    *first;
	struct asf_object_s    *last;
};
typedef struct asf_object_header_s asf_object_header_t;

struct asf_object_data_s {
	ASF_OBJECT_COMMON
	guid_t   file_id;
	uint64_t total_data_packets;
	uint16_t reserved;
	uint64_t packets_position;
};
typedef struct asf_object_data_s asf_object_data_t;

struct asf_index_entry_s {
	uint32_t packet_index;
	uint16_t packet_count;
};
typedef struct asf_index_entry_s asf_index_entry_t;

struct asf_object_index_s {
	ASF_OBJECT_COMMON
	guid_t   file_id;
	uint64_t entry_time_interval;
	uint32_t max_packet_count;
	uint32_t entry_count;
	asf_index_entry_t *entries;
};
typedef struct asf_object_index_s asf_object_index_t;

#define ASF_MAX_STREAMS 128

#define ASF_FLAG_BROADCAST 1
#define ASF_FLAG_SEEKABLE  2

struct asf_file_s {
	const char *filename;
	asf_stream_t stream;

	uint64_t position;
	uint64_t packet;

	/* Top level objects */
	struct asf_object_header_s *header;
	struct asf_object_data_s   *data;
	struct asf_object_index_s  *index;

	/* position (in bytes) of data and index objects */
	uint64_t data_position;
	uint64_t index_position;

	guid_t   file_id;
	uint64_t file_size;
	uint64_t creation_date;
	uint64_t data_packets_count;
	uint64_t play_duration;
	uint64_t send_duration;
	uint64_t preroll;
	uint16_t flags;
	uint32_t packet_size;
	uint32_t max_bitrate;

	asf_stream_type_t streams[ASF_MAX_STREAMS];
};

#endif
