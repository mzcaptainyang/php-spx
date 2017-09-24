#include <stdio.h>
#include <stdlib.h>

#include "spx_fmt.h"

#define ROW_MAX_CELLS 16

struct spx_fmt_row_t {
    size_t cell_count;
    struct {
        size_t span;
        const char * text;
        int num;
        double num_value;
        spx_fmt_value_type_t num_type;
    } cells[ROW_MAX_CELLS];
};

static void resolve_time_format(double * value, const char ** format);
static void resolve_mem_format(double * value, const char ** format);
static void resolve_pct_format(double * value, const char ** format);
static void resolve_qty_format(double * value, const char ** format);

void spx_fmt_format_value(
    char * str,
    size_t size,
    spx_fmt_value_type_t type,
    double value
) {
    const char * format;
    switch (type) {
        case SPX_FMT_TIME:
            resolve_time_format(&value, &format);
            
            break;

        case SPX_FMT_MEMORY:
            resolve_mem_format(&value, &format);
            
            break;

        case SPX_FMT_PERCENTAGE:
            resolve_pct_format(&value, &format);
            
            break;

        case SPX_FMT_QUANTITY:
        default:
            resolve_qty_format(&value, &format);
    }

    snprintf(
        str,
        size,
        format,
        value
    );
}

void spx_fmt_print_value(
    spx_output_stream_t * output,
    spx_fmt_value_type_t type,
    double value
) {
    char tmp[16];

    spx_fmt_format_value(
        tmp,
        sizeof(tmp),
        type,
        value
    );

    spx_output_stream_print(output, tmp);
}

spx_fmt_row_t * spx_fmt_row_create(void)
{
    spx_fmt_row_t * row = malloc(sizeof(*row));
    if (!row) {
        return NULL;
    }

    row->cell_count = 0;

    return row;
}

void spx_fmt_row_destroy(spx_fmt_row_t * row)
{
    free(row);
}

void spx_fmt_row_add_tcell(
    spx_fmt_row_t * row,
    size_t span,
    const char * text
) {
    if (row->cell_count == ROW_MAX_CELLS) {
        fprintf(stderr, "ROW_MAX_CELLS exceeded\n");

        exit(EXIT_FAILURE);
    }

    row->cells[row->cell_count].span = span;
    row->cells[row->cell_count].num = 0;
    row->cells[row->cell_count].text = text;

    row->cell_count++;
}

void spx_fmt_row_add_ncell(
    spx_fmt_row_t * row,
    size_t span,
    spx_fmt_value_type_t type,
    double value
) {
    if (row->cell_count == ROW_MAX_CELLS) {
        fprintf(stderr, "ROW_MAX_CELLS exceeded\n");

        exit(EXIT_FAILURE);
    }

    row->cells[row->cell_count].span = span;
    row->cells[row->cell_count].num = 1;
    row->cells[row->cell_count].num_type = type;
    row->cells[row->cell_count].num_value = value;

    row->cell_count++;
}

void spx_fmt_row_print(const spx_fmt_row_t * row, spx_output_stream_t * output)
{
    size_t i;
    char format[32], num_str[32];
    for (i = 0; i < row->cell_count; i++) {
        spx_output_stream_print(output, " ");

        const char * text;
        if (row->cells[i].num) {
            spx_fmt_format_value(
                num_str,
                sizeof(num_str),
                row->cells[i].num_type,
                row->cells[i].num_value
            );

            text = num_str;
        } else {
            text = row->cells[i].text;
        }

        if (row->cells[i].span == 0) {
            spx_output_stream_print(output, text);

            break;
        }

        const size_t cell_width = 
            row->cells[i].span * 8 +
            (row->cells[i].span - 1) * 3
        ;

        snprintf(
            format,
            sizeof(format),
            "%%-%lu.%lus",
            cell_width,
            cell_width
        );

        spx_output_stream_printf(output, format, text);

        spx_output_stream_print(output, " |");
    }

    spx_output_stream_print(output, "\n");
}

void spx_fmt_row_print_sep(const spx_fmt_row_t * row, spx_output_stream_t * output)
{
    size_t i;
    for (i = 0; i < row->cell_count; i++) {
        if (row->cells[i].span == 0) {
            spx_output_stream_print(output, "----------");

            break;
        }

        size_t j;
        for (j = 0; j < row->cells[i].span; j++) {
            spx_output_stream_print(output, "----------+");
        }
    }

    spx_output_stream_print(output, "\n");
}

void spx_fmt_row_reset(spx_fmt_row_t * row)
{
    row->cell_count = 0;
}

static void resolve_time_format(double * value, const char ** format)
{
    if (*value >= 1000 * 1000) {
        *format = "%7.2fs";
        *value /= 1000 * 1000;
    } else if (*value >= 1000) {
        *format = "%6.1fms";
        *value /= 1000;
    } else {
        *format = "%6.fus";
    }
}

static void resolve_mem_format(double * value, const char ** format)
{
    int neg = *value < 0;
    if (neg) {
        *value *= -1;
    }

    if (*value >= 1000 * 1000 * 1000) {
        *format = "%6.1fGB";
        *value /= 1 << 30;
    } else if (*value >= 1000 * 1000) {
        *format = "%6.1fMB";
        *value /= 1 << 20;
    } else if (*value >= 1000) {
        *format = "%6.1fKB";
        *value /= 1 << 10;
    } else {
        *format = "%7.fB";
    }

    if (neg) {
        *value *= -1;
    }
}

static void resolve_pct_format(double * value, const char ** format)
{
    *value *= 100;
    *format = "%7.3f%%";
}

static void resolve_qty_format(double * value, const char ** format)
{
    if (*value >= 1000 * 1000 * 1000) {
        *format = "%7.1fG";
        *value /= 1000 * 1000 * 1000;
    } else if (*value >= 1000 * 1000) {
        *format = "%7.1fM";
        *value /= 1000 * 1000;
    } else if (*value >= 1000) {
        *format = "%7.1fK";
        *value /= 1000;
    } else {
        *format = "%8.f";
    }
}