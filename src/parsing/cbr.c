#include "cbr.h"
#include "src/ctx.h"

unsigned int cbr_mime;
unsigned int cbz_mime;

void cbr_init() {
    cbr_mime = mime_get_mime_by_string(ScanCtx.mime_table, "application/x-cbr");
    cbz_mime = mime_get_mime_by_string(ScanCtx.mime_table, "application/x-cbz");
}

int is_cbr(unsigned int mime) {
    return mime == cbr_mime;
}

void parse_cbr(void *buf, size_t buf_len, document_t *doc) {
    char *out_buf = malloc(buf_len * 2);
    size_t out_buf_used = 0;

    struct archive *rar_in = archive_read_new();
    archive_read_support_filter_none(rar_in);
    archive_read_support_format_rar(rar_in);

    archive_read_open_memory(rar_in, buf, buf_len);

    struct archive *zip_out = archive_write_new();
    archive_write_set_format_zip(zip_out);
    archive_write_open_memory(zip_out, out_buf, buf_len * 2, &out_buf_used);

    struct archive_entry *entry;
    while (archive_read_next_header(rar_in, &entry) == ARCHIVE_OK) {
        archive_write_header(zip_out, entry);

        char arc_buf[ARC_BUF_SIZE];
        int len = archive_read_data(rar_in, arc_buf, ARC_BUF_SIZE);
        while (len > 0) {
            archive_write_data(zip_out, arc_buf, len);
            len = archive_read_data(rar_in, arc_buf, ARC_BUF_SIZE);
        }
    }

    archive_write_close(zip_out);
    archive_write_free(zip_out);

    archive_read_close(rar_in);
    archive_read_free(rar_in);

    doc->mime = cbz_mime;
    parse_pdf(out_buf, out_buf_used, doc);
    doc->mime = cbr_mime;
    free(out_buf);
}
