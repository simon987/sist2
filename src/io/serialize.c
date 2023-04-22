#include "src/ctx.h"
#include "serialize.h"
#include "src/parsing/mime.h"


char *get_meta_key_text(enum metakey meta_key) {

    switch (meta_key) {
        case MetaContent:
            return "content";
        case MetaWidth:
            return "width";
        case MetaHeight:
            return "height";
        case MetaMediaDuration:
            return "duration";
        case MetaMediaAudioCodec:
            return "audioc";
        case MetaMediaVideoCodec:
            return "videoc";
        case MetaMediaBitrate:
            return "bitrate";
        case MetaArtist:
            return "artist";
        case MetaAlbum:
            return "album";
        case MetaAlbumArtist:
            return "album_artist";
        case MetaGenre:
            return "genre";
        case MetaTitle:
            return "title";
        case MetaFontName:
            return "font_name";
        case MetaParent:
            return "parent";
        case MetaExifMake:
            return "exif_make";
        case MetaExifDescription:
            return "exif_description";
        case MetaExifSoftware:
            return "exif_software";
        case MetaExifExposureTime:
            return "exif_exposure_time";
        case MetaExifFNumber:
            return "exif_fnumber";
        case MetaExifFocalLength:
            return "exif_focal_length";
        case MetaExifUserComment:
            return "exif_user_comment";
        case MetaExifIsoSpeedRatings:
            return "exif_iso_speed_ratings";
        case MetaExifModel:
            return "exif_model";
        case MetaExifDateTime:
            return "exif_datetime";
        case MetaAuthor:
            return "author";
        case MetaModifiedBy:
            return "modified_by";
        case MetaThumbnail:
            return "thumbnail";
        case MetaPages:
            return "pages";
        case MetaExifGpsLongitudeRef:
            return "exif_gps_longitude_ref";
        case MetaExifGpsLongitudeDMS:
            return "exif_gps_longitude_dms";
        case MetaExifGpsLongitudeDec:
            return "exif_gps_longitude_dec";
        case MetaExifGpsLatitudeRef:
            return "exif_gps_latitude_ref";
        case MetaExifGpsLatitudeDMS:
            return "exif_gps_latitude_dms";
        case MetaExifGpsLatitudeDec:
            return "exif_gps_latitude_dec";
        case MetaChecksum:
            return "checksum";
        default:
        LOG_FATALF("serialize.c", "FIXME: Unknown meta key: %d", meta_key);
    }
}

char *build_json_string(document_t *doc) {
    cJSON *json = cJSON_CreateObject();
    int buffer_size_guess = 8192;

    const char *mime_text = mime_get_mime_text(doc->mime);
    if (mime_text == NULL) {
        cJSON_AddNullToObject(json, "mime");
    } else {
        cJSON_AddStringToObject(json, "mime", mime_text);
    }

    // Ignore root directory in the file path
    doc->ext = (short) (doc->ext - ScanCtx.index.desc.root_len);
    doc->base = (short) (doc->base - ScanCtx.index.desc.root_len);
    char *filepath = doc->filepath + ScanCtx.index.desc.root_len;

    cJSON_AddStringToObject(json, "extension", filepath + doc->ext);

    // Remove extension
    if (*(filepath + doc->ext - 1) == '.') {
        *(filepath + doc->ext - 1) = '\0';
    } else {
        *(filepath + doc->ext) = '\0';
    }

    char filepath_escaped[PATH_MAX * 3];
    str_escape(filepath_escaped, filepath + doc->base);

    cJSON_AddStringToObject(json, "name", filepath_escaped);

    if (doc->base > 0) {
        *(filepath + doc->base - 1) = '\0';

        str_escape(filepath_escaped, filepath);
        cJSON_AddStringToObject(json, "path", filepath_escaped);
    } else {
        cJSON_AddStringToObject(json, "path", "");
    }

    // Metadata
    meta_line_t *meta = doc->meta_head;
    while (meta != NULL) {

        switch (meta->key) {
            case MetaThumbnail:
            case MetaPages:
            case MetaWidth:
            case MetaHeight:
            case MetaMediaDuration:
            case MetaMediaBitrate: {
                cJSON_AddNumberToObject(json, get_meta_key_text(meta->key), (double) meta->long_val);
                buffer_size_guess += 20;
                break;
            }
            case MetaMediaAudioCodec:
            case MetaMediaVideoCodec:
            case MetaContent:
            case MetaArtist:
            case MetaAlbum:
            case MetaAlbumArtist:
            case MetaGenre:
            case MetaFontName:
            case MetaParent:
            case MetaExifMake:
            case MetaExifDescription:
            case MetaExifSoftware:
            case MetaExifExposureTime:
            case MetaExifFNumber:
            case MetaExifFocalLength:
            case MetaExifUserComment:
            case MetaExifIsoSpeedRatings:
            case MetaExifDateTime:
            case MetaExifModel:
            case MetaAuthor:
            case MetaModifiedBy:
            case MetaExifGpsLongitudeDMS:
            case MetaExifGpsLongitudeDec:
            case MetaExifGpsLongitudeRef:
            case MetaExifGpsLatitudeDMS:
            case MetaExifGpsLatitudeDec:
            case MetaExifGpsLatitudeRef:
            case MetaChecksum:
            case MetaTitle: {
                cJSON_AddStringToObject(json, get_meta_key_text(meta->key), meta->str_val);
                buffer_size_guess += (int) strlen(meta->str_val);
                break;
            }
            default:
            LOG_FATALF("serialize.c", "Invalid meta key: %x %s", meta->key, get_meta_key_text(meta->key));
        }

        meta_line_t *tmp = meta;
        meta = meta->next;
        free(tmp);
    }

    char *json_str = cJSON_PrintBuffered(json, buffer_size_guess, FALSE);
    cJSON_Delete(json);

    return json_str;
}

void write_document(document_t *doc) {
    char *json_str = build_json_string(doc);

    database_write_document(ProcData.index_db, doc, json_str);
    free(doc);
    free(json_str);
}