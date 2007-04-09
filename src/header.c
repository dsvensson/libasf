
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asf.h"
#include "asfint.h"
#include "utf.h"
#include "header.h"
#include "guid.h"
#include "byteio.h"

static asf_object_t *
asf_header_get_object(asf_object_header_t *header, const guid_type_t type)
{
	asf_object_t *current;

	current = header->first;
	while (current) {
		if (current->type == type) {
			return current;
		}
		current = current->next;
	}

	return NULL;
}

int
asf_parse_header_validate(asf_file_t *file, asf_object_header_t *header)
{
	asf_object_t *current;
	int fileprop = 0, streamprop = 0;

	if (header->first) {
		current = header->first;
		while (current) {
			uint64_t size = current->size;

			switch (current->type) {
			case GUID_FILE_PROPERTIES:
			{
				uint32_t max_packet_size;
				if (size < 104)
					return ASF_ERROR_OBJECT_SIZE;

				if (fileprop) {
					/* multiple file properties objects not allowed */
					return ASF_ERROR_INVALID_OBJECT;
				}

				fileprop = 1;
				asf_byteio_getGUID(&file->file_id, current->data);
				file->file_size = asf_byteio_getQWLE(current->data + 16);
				file->creation_date = asf_byteio_getQWLE(current->data + 24);
				file->data_packets_count = asf_byteio_getQWLE(current->data + 32);
				file->play_duration = asf_byteio_getQWLE(current->data + 40);
				file->send_duration = asf_byteio_getQWLE(current->data + 48);
				file->preroll = asf_byteio_getQWLE(current->data + 56);
				file->flags = asf_byteio_getDWLE(current->data + 64);
				file->packet_size = asf_byteio_getDWLE(current->data + 68);
				file->max_bitrate = asf_byteio_getQWLE(current->data + 76);

				max_packet_size = asf_byteio_getDWLE(current->data + 72);
				if (file->packet_size != max_packet_size) {
					/* in ASF file minimum packet size and maximum
					 * packet size have to be same apparently...
					 * stupid, eh? */
					return ASF_ERROR_INVALID_VALUE;
				}
				break;
			}
			case GUID_STREAM_PROPERTIES:
			{
				uint16_t flags;

				if (size < 78)
					return ASF_ERROR_OBJECT_SIZE;

				streamprop = 1;
				flags = asf_byteio_getWLE(current->data + 48);
				if (file->streams[flags & 0x7f].type) {
					/* only one stream object per stream allowed */
					return ASF_ERROR_INVALID_OBJECT;
				} else {
					guid_t guid;
					guid_type_t type;
					asf_stream_type_t *stype;

					stype = file->streams + (flags & 0x7f);

					asf_byteio_getGUID(&guid, current->data);
					type = asf_guid_get_stream_type(&guid);

					switch (type) {
					case GUID_STREAM_TYPE_AUDIO:
						stype->type = ASF_STREAM_TYPE_AUDIO;
						break;
					case GUID_STREAM_TYPE_VIDEO:
						stype->type = ASF_STREAM_TYPE_VIDEO;
						break;
					case GUID_STREAM_TYPE_COMMAND:
						stype->type = ASF_STREAM_TYPE_COMMAND;
						break;
					default:
						stype->type = ASF_STREAM_TYPE_UNKNOWN;
						break;
					}

					stype->datalen = asf_byteio_getDWLE(current->data + 40);
					stype->data = current->data + 54;
				}
				break;
			}
			case GUID_CONTENT_DESCRIPTION:
			{
				uint32_t stringlen = 0;

				if (size < 34)
					return ASF_ERROR_OBJECT_SIZE;

				stringlen += asf_byteio_getWLE(current->data);
				stringlen += asf_byteio_getWLE(current->data + 2);
				stringlen += asf_byteio_getWLE(current->data + 4);
				stringlen += asf_byteio_getWLE(current->data + 6);
				stringlen += asf_byteio_getWLE(current->data + 8);

				if (size < stringlen + 34) {
					/* invalid string length values */
					return ASF_ERROR_INVALID_LENGTH;
				}
				break;
			}
			case GUID_MARKER:
				break;
			case GUID_CODEC_LIST:
				if (size < 44)
					return ASF_ERROR_OBJECT_SIZE;
				break;
			case GUID_STREAM_BITRATE_PROPERTIES:
				if (size < 26)
					return ASF_ERROR_OBJECT_SIZE;
				break;
			case GUID_PADDING:
				break;
			case GUID_EXTENDED_CONTENT_DESCRIPTION:
				if (size < 26)
					return ASF_ERROR_OBJECT_SIZE;
				break;
			case GUID_UNKNOWN:
				/* unknown guid type */
				break;
			default:
				/* identified type in wrong place */
				return ASF_ERROR_INVALID_OBJECT;
			}

			current = current->next;
		}
	}

	if (header->ext) {
		current = header->ext->first;
		while (current) {
			uint64_t size = current->size;

			switch (current->type) {
			case GUID_METADATA:
				if (size < 26)
					return ASF_ERROR_OBJECT_SIZE;
				break;
			case GUID_LANGUAGE_LIST:
				if (size < 26)
					return ASF_ERROR_OBJECT_SIZE;
				break;
			case GUID_EXTENDED_STREAM_PROPERTIES:
				/* FIXME check for hidden stream properties object */
				if (size < 88)
					return ASF_ERROR_OBJECT_SIZE;
				break;
			case GUID_ADVANCED_MUTUAL_EXCLUSION:
				if (size < 42)
					return ASF_ERROR_OBJECT_SIZE;
				break;
			case GUID_STREAM_PRIORITIZATION:
				if (size < 26)
					return ASF_ERROR_OBJECT_SIZE;
				break;
			case GUID_UNKNOWN:
				/* unknown guid type */
				break;
			default:
				/* identified type in wrong place */
				break;
			}

			current = current->next;
		}
	}

	if (!fileprop || !streamprop || !header->ext) {
		/* mandatory subobject missing */
		return ASF_ERROR_INVALID_OBJECT;
	}

	return 1;
}


void
asf_header_destroy(asf_object_header_t *header)
{
	if (!header)
		return;

	if (header->first) {
		asf_object_t *current = header->first, *next;
		while (current) {
			next = current->next;
			free(current);
			current = next;
		}
	}

	if (header->ext) {
		asf_object_t *current = header->ext->first, *next;
		while (current) {
			next = current->next;
			free(current);
			current = next;
		}
	}
	free(header->data);
	free(header->ext);
	free(header);
}


asf_metadata_t *
asf_header_get_metadata(asf_object_header_t *header)
{
	asf_object_t *current;
	asf_metadata_t *ret;

	ret = calloc(1, sizeof(asf_metadata_t));
	if (!ret) {
		return NULL;
	}

	current = asf_header_get_object(header, GUID_CONTENT_DESCRIPTION);
	if (current) {
		char *str;
		uint16_t strlen;
		int i, read = 0;

		/* FIXME: better error handling */
		for (i=0; i<5; i++) {
			strlen = asf_byteio_getWLE(current->data + i*2);
			if (strlen == 0)
				continue;

			str = asf_utf8_from_utf16le(current->data + 10 + read, strlen);
			read += strlen;

			switch (i) {
			case 0:
				ret->title = str;
				break;
			case 1:
				ret->artist = str;
				break;
			case 2:
				ret->copyright = str;
				break;
			case 3:
				ret->description = str;
				break;
			case 4:
				ret->rating = str;
				break;
			default:
				free(str);
				break;
			}
		}
	}

	current = asf_header_get_object(header, GUID_EXTENDED_CONTENT_DESCRIPTION);
	if (current) {
		int i, j, position;

		ret->extended_count = asf_byteio_getWLE(current->data);
		ret->extended = calloc(ret->extended_count, sizeof(asf_metadata_entry_t));
		if (!ret->extended) {
			/* FIXME: cleanup */
			return NULL;
		}

		position = 2;
		for (i=0; i<ret->extended_count; i++) {
			uint16_t length, type;

			length = asf_byteio_getWLE(current->data + position);
			position += 2;

			ret->extended[i].key = asf_utf8_from_utf16le(current->data + position, length);
			position += length;

			type = asf_byteio_getWLE(current->data + position);
			position += 2;

			length = asf_byteio_getWLE(current->data + position);
			position += 2;

			switch (type) {
			case 0:
				ret->extended[i].value = asf_utf8_from_utf16le(current->data + position, length);
				break;
			case 1:
				ret->extended[i].value = malloc((length*2 + 1) * sizeof(char));
				for (j=0; j<length; j++) {
					static const char hex[16] = "0123456789ABCDEF";
					ret->extended[i].value[j*2+0] = hex[current->data[position]>>4];
					ret->extended[i].value[j*2+1] = hex[current->data[position]&0x0f];
				}
				ret->extended[i].value[j*2] = '\0';
				break;
			case 2:
				ret->extended[i].value = malloc(6 * sizeof(char));
				sprintf(ret->extended[i].value, "%s",
				        *current->data ? "true" : "false");
				break;
			case 3:
				ret->extended[i].value = malloc(11 * sizeof(char));
				sprintf(ret->extended[i].value, "%u",
				        asf_byteio_getDWLE(current->data + position));
				break;
			case 4:
				/* FIXME: This doesn't print whole 64-bit integer */
				ret->extended[i].value = malloc(21 * sizeof(char));
				sprintf(ret->extended[i].value, "%u",
				        (uint32_t) asf_byteio_getQWLE(current->data + position));
				break;
			case 5:
				ret->extended[i].value = malloc(6 * sizeof(char));
				sprintf(ret->extended[i].value, "%u",
				        asf_byteio_getWLE(current->data + position));
				break;
			default:
				/* Unknown value type... */
				ret->extended[i].value = NULL;
				break;
			}
			position += length;
		}
	}

	return ret;
}

void
asf_header_metadata_destroy(asf_metadata_t *metadata)
{
	int i;

	free(metadata->title);
	free(metadata->artist);
	free(metadata->copyright);
	free(metadata->description);
	free(metadata->rating);
	for (i=0; i<metadata->extended_count; i++) {
		free(metadata->extended[i].key);
		free(metadata->extended[i].value);
	}
	free(metadata->extended);
	free(metadata);
}
