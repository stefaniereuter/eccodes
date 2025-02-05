/*
 * (C) Copyright 2005- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities granted to it by
 * virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
 */

#include "grib_api_internal.h"
#include <cctype>
/*
   This is used by make_class.pl

   START_CLASS_DEF
   CLASS      = dumper
   IMPLEMENTS = dump_long;dump_bits
   IMPLEMENTS = dump_double;dump_string;dump_string_array
   IMPLEMENTS = dump_bytes;dump_values
   IMPLEMENTS = dump_label;dump_section
   IMPLEMENTS = init;destroy
   MEMBERS = long section_offset
   MEMBERS = long begin
   MEMBERS = long theEnd
   END_CLASS_DEF

 */


/* START_CLASS_IMP */

/*

Don't edit anything between START_CLASS_IMP and END_CLASS_IMP
Instead edit values between START_CLASS_DEF and END_CLASS_DEF
or edit "dumper.class" and rerun ./make_class.pl

*/

static void init_class      (grib_dumper_class*);
static int init            (grib_dumper* d);
static int destroy         (grib_dumper*);
static void dump_long       (grib_dumper* d, grib_accessor* a,const char* comment);
static void dump_bits       (grib_dumper* d, grib_accessor* a,const char* comment);
static void dump_double     (grib_dumper* d, grib_accessor* a,const char* comment);
static void dump_string     (grib_dumper* d, grib_accessor* a,const char* comment);
static void dump_string_array     (grib_dumper* d, grib_accessor* a,const char* comment);
static void dump_bytes      (grib_dumper* d, grib_accessor* a,const char* comment);
static void dump_values     (grib_dumper* d, grib_accessor* a);
static void dump_label      (grib_dumper* d, grib_accessor* a,const char* comment);
static void dump_section    (grib_dumper* d, grib_accessor* a,grib_block_of_accessors* block);

typedef struct grib_dumper_debug {
    grib_dumper          dumper;
    /* Members defined in debug */
    long section_offset;
    long begin;
    long theEnd;
} grib_dumper_debug;


static grib_dumper_class _grib_dumper_class_debug = {
    0,                              /* super                     */
    "debug",                              /* name                      */
    sizeof(grib_dumper_debug),     /* size                      */
    0,                                   /* inited */
    &init_class,                         /* init_class */
    &init,                               /* init                      */
    &destroy,                            /* free mem                       */
    &dump_long,                          /* dump long         */
    &dump_double,                        /* dump double    */
    &dump_string,                        /* dump string    */
    &dump_string_array,                        /* dump string array   */
    &dump_label,                         /* dump labels  */
    &dump_bytes,                         /* dump bytes  */
    &dump_bits,                          /* dump bits   */
    &dump_section,                       /* dump section      */
    &dump_values,                        /* dump values   */
    0,                             /* header   */
    0,                             /* footer   */
};

grib_dumper_class* grib_dumper_class_debug = &_grib_dumper_class_debug;

/* END_CLASS_IMP */
static void set_begin_end(grib_dumper* d, grib_accessor* a);

static void init_class(grib_dumper_class* c) {}

static int init(grib_dumper* d)
{
    grib_dumper_debug* self = (grib_dumper_debug*)d;
    self->section_offset    = 0;

    return GRIB_SUCCESS;
}

static int destroy(grib_dumper* d)
{
    return GRIB_SUCCESS;
}

static void default_long_value(grib_dumper* d, grib_accessor* a, long actualValue)
{
    grib_dumper_debug* self = (grib_dumper_debug*)d;
    grib_action* act = a->creator;
    if (act->default_value == NULL)
        return;

    grib_handle* h = grib_handle_of_accessor(a);
    grib_expression* expression = grib_arguments_get_expression(h, act->default_value, 0);
    if (!expression)
        return;

    const int type = grib_expression_native_type(h, expression);
    if (type == GRIB_TYPE_LONG) {
        long defaultValue = 0;
        if (grib_expression_evaluate_long(h, expression, &defaultValue) == GRIB_SUCCESS && defaultValue != actualValue) {
            if (defaultValue == GRIB_MISSING_LONG)
                fprintf(self->dumper.out, " (default=MISSING)");
            else
                fprintf(self->dumper.out, " (default=%ld)",defaultValue);
        }
    }
}

// static void default_string_value(grib_dumper* d, grib_accessor* a, const char* actualValue)
// {
//     grib_dumper_debug* self = (grib_dumper_debug*)d;
//     grib_action* act = a->creator;
//     if (act->default_value == NULL)
//         return;

//     grib_handle* h = grib_handle_of_accessor(a);
//     grib_expression* expression = grib_arguments_get_expression(h, act->default_value, 0);
//     if (!expression)
//         return;

//     const int type = grib_expression_native_type(h, expression);
//     DEBUG_ASSERT(type == GRIB_TYPE_STRING);
//     if (type == GRIB_TYPE_STRING) {
//         char tmp[1024] = {0,};
//         size_t s_len = sizeof(tmp);
//         int err = 0;
//         const char* p = grib_expression_evaluate_string(h, expression, tmp, &s_len, &err);
//         if (!err && !STR_EQUAL(p, actualValue)) {
//             fprintf(self->dumper.out, " (default=%s)", p);
//         }
//     }
// }

static void aliases(grib_dumper* d, grib_accessor* a)
{
    int i;
    grib_dumper_debug* self = (grib_dumper_debug*)d;

    if (a->all_names[1]) {
        const char* sep = "";
        fprintf(self->dumper.out, " [");

        for (i = 1; i < MAX_ACCESSOR_NAMES; i++) {
            if (a->all_names[i]) {
                if (a->all_name_spaces[i])
                    fprintf(self->dumper.out, "%s%s.%s", sep, a->all_name_spaces[i], a->all_names[i]);
                else
                    fprintf(self->dumper.out, "%s%s", sep, a->all_names[i]);
            }
            sep = ", ";
        }
        fprintf(self->dumper.out, "]");
    }
}

static void dump_long(grib_dumper* d, grib_accessor* a, const char* comment)
{
    grib_dumper_debug* self = (grib_dumper_debug*)d;
    long value              = 0;
    size_t size             = 0;
    size_t more             = 0;
    long* values            = NULL; /* array of long */
    long count              = 0;
    int err = 0, i = 0;

    if (a->length == 0 && (d->option_flags & GRIB_DUMP_FLAG_CODED) != 0)
        return;

    if ((a->flags & GRIB_ACCESSOR_FLAG_READ_ONLY) != 0 &&
        (d->option_flags & GRIB_DUMP_FLAG_READ_ONLY) == 0)
        return;

    a->value_count(&count);
    size = count;
    if (size > 1) {
        values = (long*)grib_context_malloc_clear(a->context, sizeof(long) * size);
        err    = a->unpack_long(values, &size);
    }
    else {
        err = a->unpack_long(&value, &size);
    }

    set_begin_end(d, a);

    for (i = 0; i < d->depth; i++)
        fprintf(self->dumper.out, " ");

    if (size > 1) {
        fprintf(self->dumper.out, "%ld-%ld %s %s = {\n", self->begin, self->theEnd, a->creator->op, a->name);
        if (values) {
            int k = 0;
            if (size > 100) {
                more = size - 100;
                size = 100;
            }
            while (k < size) {
                int j;
                for (i = 0; i < d->depth + 3; i++)
                    fprintf(self->dumper.out, " ");
                for (j = 0; j < 8 && k < size; j++, k++) {
                    fprintf(self->dumper.out, "%ld", values[k]);
                    if (k != size - 1)
                        fprintf(self->dumper.out, ", ");
                }
                fprintf(self->dumper.out, "\n");
            }
            if (more) {
                for (i = 0; i < d->depth + 3; i++)
                    fprintf(self->dumper.out, " ");
                fprintf(self->dumper.out, "... %lu more values\n", (unsigned long)more);
            }
            for (i = 0; i < d->depth; i++)
                fprintf(self->dumper.out, " ");
            fprintf(self->dumper.out, "} # %s %s \n", a->creator->op, a->name);
            grib_context_free(a->context, values);
        }
    }
    else {
        if (((a->flags & GRIB_ACCESSOR_FLAG_CAN_BE_MISSING) != 0) && a->is_missing_internal())
            fprintf(self->dumper.out, "%ld-%ld %s %s = MISSING", self->begin, self->theEnd, a->creator->op, a->name);
        else
            fprintf(self->dumper.out, "%ld-%ld %s %s = %ld", self->begin, self->theEnd, a->creator->op, a->name, value);
        if (comment)
            fprintf(self->dumper.out, " [%s]", comment);
        if ((d->option_flags & GRIB_DUMP_FLAG_TYPE) != 0)
            fprintf(self->dumper.out, " (%s)", grib_get_type_name(a->get_native_type()));
        if ((a->flags & GRIB_ACCESSOR_FLAG_CAN_BE_MISSING) != 0)
            fprintf(self->dumper.out, " %s", "(can be missing)");
        if ((a->flags & GRIB_ACCESSOR_FLAG_READ_ONLY) != 0)
            fprintf(self->dumper.out, " %s", "(read-only)");
    }
    if (err)
        fprintf(self->dumper.out, " *** ERR=%d (%s) [grib_dumper_debug::dump_long]", err, grib_get_error_message(err));

    aliases(d, a);
    default_long_value(d, a, value);

    fprintf(self->dumper.out, "\n");
}

static int test_bit(long a, long b)
{
    return a & (1 << b);
}

static void dump_bits(grib_dumper* d, grib_accessor* a, const char* comment)
{
    grib_dumper_debug* self = (grib_dumper_debug*)d;

    if (a->length == 0 &&
        (d->option_flags & GRIB_DUMP_FLAG_CODED) != 0)
        return;

    size_t size = 1;
    long value = 0;
    int err = a->unpack_long(&value, &size);
    set_begin_end(d, a);

    for (int i = 0; i < d->depth; i++)
        fprintf(self->dumper.out, " ");
    fprintf(self->dumper.out, "%ld-%ld %s %s = %ld [", self->begin, self->theEnd, a->creator->op, a->name, value);

    for (long i = 0; i < (a->length * 8); i++) {
        if (test_bit(value, a->length * 8 - i - 1))
            fprintf(self->dumper.out, "1");
        else
            fprintf(self->dumper.out, "0");
    }

    if (comment)
        fprintf(self->dumper.out, ":%s]", comment);
    else
        fprintf(self->dumper.out, "]");

    if (err)
        fprintf(self->dumper.out, " *** ERR=%d (%s) [grib_dumper_debug::dump_bits]", err, grib_get_error_message(err));

    aliases(d, a);
    fprintf(self->dumper.out, "\n");
}

static void dump_double(grib_dumper* d, grib_accessor* a, const char* comment)
{
    grib_dumper_debug* self = (grib_dumper_debug*)d;
    double value            = 0;
    size_t size             = 1;
    int err                 = a->unpack_double(&value, &size);
    int i;

    if (a->length == 0 &&
        (d->option_flags & GRIB_DUMP_FLAG_CODED) != 0)
        return;

    set_begin_end(d, a);

    for (i = 0; i < d->depth; i++)
        fprintf(self->dumper.out, " ");

    if (((a->flags & GRIB_ACCESSOR_FLAG_CAN_BE_MISSING) != 0) && a->is_missing_internal())
        fprintf(self->dumper.out, "%ld-%ld %s %s = MISSING", self->begin, self->theEnd, a->creator->op, a->name);
    else
        fprintf(self->dumper.out, "%ld-%ld %s %s = %g", self->begin, self->theEnd, a->creator->op, a->name, value);
    if (comment)
        fprintf(self->dumper.out, " [%s]", comment);
    if ((d->option_flags & GRIB_DUMP_FLAG_TYPE) != 0)
        fprintf(self->dumper.out, " (%s)", grib_get_type_name(a->get_native_type()));
    if (err)
        fprintf(self->dumper.out, " *** ERR=%d (%s) [grib_dumper_debug::dump_double]", err, grib_get_error_message(err));
    aliases(d, a);
    fprintf(self->dumper.out, "\n");
}

static void dump_string(grib_dumper* d, grib_accessor* a, const char* comment)
{
    grib_dumper_debug* self = (grib_dumper_debug*)d;
    int err                 = 0;
    int i;
    size_t size = 0;
    char* value = NULL;
    char* p     = NULL;

    if (a->length == 0 && (d->option_flags & GRIB_DUMP_FLAG_CODED) != 0)
        return;

    grib_get_string_length_acc(a, &size);
    if ((size < 2) && a->is_missing_internal()) {
        /* GRIB-302: transients and missing keys. Need to re-adjust the size */
        size = 10; /* big enough to hold the string "missing" */
    }

    value = (char*)grib_context_malloc_clear(a->context, size);
    if (!value)
        return;
    err = a->unpack_string(value, &size);

    if (err)
        strcpy(value, "<error>");

    p = value;

    set_begin_end(d, a);

    while (*p) {
        if (!isprint(*p))
            *p = '.';
        p++;
    }

    for (i = 0; i < d->depth; i++)
        fprintf(self->dumper.out, " ");
    fprintf(self->dumper.out, "%ld-%ld %s %s = %s", self->begin, self->theEnd, a->creator->op, a->name, value);

    if (comment)
        fprintf(self->dumper.out, " [%s]", comment);
    if ((d->option_flags & GRIB_DUMP_FLAG_TYPE) != 0)
        fprintf(self->dumper.out, " (%s)", grib_get_type_name(a->get_native_type()));

    if (err)
        fprintf(self->dumper.out, " *** ERR=%d (%s) [grib_dumper_debug::dump_string]", err, grib_get_error_message(err));
    aliases(d, a);
    fprintf(self->dumper.out, "\n");

    grib_context_free(a->context, value);
}

static void dump_string_array(grib_dumper* d, grib_accessor* a, const char* comment)
{
    grib_dumper_debug* self = (grib_dumper_debug*)d;

    char** values;
    size_t size = 0, i = 0;
    grib_context* c = NULL;
    int err         = 0;
    int tab         = 0;
    long count      = 0;

    if ((a->flags & GRIB_ACCESSOR_FLAG_DUMP) == 0)
        return;

    c = a->context;
    a->value_count(&count);
    if (count == 0)
        return;
    size = count;
    if (size == 1) {
        dump_string(d, a, comment);
        return;
    }

    values = (char**)grib_context_malloc_clear(c, size * sizeof(char*));
    if (!values) {
        grib_context_log(c, GRIB_LOG_ERROR, "unable to allocate %zu bytes", size);
        return;
    }

    err = a->unpack_string_array(values, &size);

    // print_offset(self->dumper.out,d,a);
    //print_offset(self->dumper.out, self->begin, self->theEnd);

    if ((d->option_flags & GRIB_DUMP_FLAG_TYPE) != 0) {
        fprintf(self->dumper.out, "  ");
        fprintf(self->dumper.out, "# type %s (str) \n", a->creator->op);
    }

    aliases(d, a);
    if (comment) {
        fprintf(self->dumper.out, "  ");
        fprintf(self->dumper.out, "# %s \n", comment);
    }
    if (a->flags & GRIB_ACCESSOR_FLAG_READ_ONLY) {
        fprintf(self->dumper.out, "  ");
        fprintf(self->dumper.out, "#-READ ONLY- ");
        tab = 13;
    }
    else
        fprintf(self->dumper.out, "  ");

    tab++;
    fprintf(self->dumper.out, "%s = {\n", a->name);
    for (i = 0; i < size; i++) {
        fprintf(self->dumper.out, "%-*s\"%s\",\n", (int)(tab + strlen(a->name) + 4), " ", values[i]);
    }
    fprintf(self->dumper.out, "  }");

    if (err) {
        fprintf(self->dumper.out, "  ");
        fprintf(self->dumper.out, "# *** ERR=%d (%s)", err, grib_get_error_message(err));
    }

    fprintf(self->dumper.out, "\n");
    for (i=0; i<size; ++i) grib_context_free(c, values[i]);
    grib_context_free(c, values);
}

static void dump_bytes(grib_dumper* d, grib_accessor* a, const char* comment)
{
    grib_dumper_debug* self = (grib_dumper_debug*)d;
    int i, k, err = 0;
    size_t more        = 0;
    size_t size        = a->length;
    unsigned char* buf = (unsigned char*)grib_context_malloc(d->context, size);

    if (a->length == 0 &&
        (d->option_flags & GRIB_DUMP_FLAG_CODED) != 0)
        return;

    set_begin_end(d, a);

    for (i = 0; i < d->depth; i++)
        fprintf(self->dumper.out, " ");
    fprintf(self->dumper.out, "%ld-%ld %s %s = %ld", self->begin, self->theEnd, a->creator->op, a->name, a->length);
    aliases(d, a);
    fprintf(self->dumper.out, " {");

    if (!buf) {
        if (size == 0)
            fprintf(self->dumper.out, "}\n");
        else
            fprintf(self->dumper.out, " *** ERR cannot malloc(%zu) }\n", size);
        return;
    }

    fprintf(self->dumper.out, "\n");

    err = a->unpack_bytes(buf, &size);
    if (err) {
        grib_context_free(d->context, buf);
        fprintf(self->dumper.out, " *** ERR=%d (%s) [grib_dumper_debug::dump_bytes]\n}", err, grib_get_error_message(err));
        return;
    }

    if (size > 100) {
        more = size - 100;
        size = 100;
    }

    k = 0;
    /* if(size > 100) size = 100;  */
    while (k < size) {
        int j;
        for (i = 0; i < d->depth + 3; i++)
            fprintf(self->dumper.out, " ");
        for (j = 0; j < 16 && k < size; j++, k++) {
            fprintf(self->dumper.out, "%02x", buf[k]);
            if (k != size - 1)
                fprintf(self->dumper.out, ", ");
        }
        fprintf(self->dumper.out, "\n");
    }

    if (more) {
        for (i = 0; i < d->depth + 3; i++)
            fprintf(self->dumper.out, " ");
        fprintf(self->dumper.out, "... %lu more values\n", (unsigned long)more);
    }

    for (i = 0; i < d->depth; i++)
        fprintf(self->dumper.out, " ");
    fprintf(self->dumper.out, "} # %s %s \n", a->creator->op, a->name);
    grib_context_free(d->context, buf);
}

static void dump_values(grib_dumper* d, grib_accessor* a)
{
    grib_dumper_debug* self = (grib_dumper_debug*)d;
    int i, k, err = 0;
    size_t more = 0;
    double* buf = NULL;
    size_t size = 0;
    long count  = 0;

    if (a->length == 0 &&
        (d->option_flags & GRIB_DUMP_FLAG_CODED) != 0)
        return;

    a->value_count(&count);
    size = count;
    if (size == 1) {
        dump_double(d, a, NULL);
        return;
    }
    buf = (double*)grib_context_malloc_clear(d->context, size * sizeof(double));

    set_begin_end(d, a);

    for (i = 0; i < d->depth; i++)
        fprintf(self->dumper.out, " ");
    fprintf(self->dumper.out, "%ld-%ld %s %s = (%ld,%ld)", self->begin, self->theEnd, a->creator->op, a->name, (long)size, a->length);
    aliases(d, a);
    fprintf(self->dumper.out, " {");

    if (!buf) {
        if (size == 0)
            fprintf(self->dumper.out, "}\n");
        else
            fprintf(self->dumper.out, " *** ERR cannot malloc(%zu) }\n", size);
        return;
    }

    fprintf(self->dumper.out, "\n");

    err = a->unpack_double(buf, &size);
    if (err) {
        grib_context_free(d->context, buf);
        fprintf(self->dumper.out, " *** ERR=%d (%s) [grib_dumper_debug::dump_values]\n}", err, grib_get_error_message(err));
        return;
    }

    if (size > 100) {
        more = size - 100;
        size = 100;
    }

    k = 0;
    while (k < size) {
        int j;
        for (i = 0; i < d->depth + 3; i++)
            fprintf(self->dumper.out, " ");
        for (j = 0; j < 8 && k < size; j++, k++) {
            fprintf(self->dumper.out, "%10g", buf[k]);
            if (k != size - 1)
                fprintf(self->dumper.out, ", ");
        }
        fprintf(self->dumper.out, "\n");
    }
    if (more) {
        for (i = 0; i < d->depth + 3; i++)
            fprintf(self->dumper.out, " ");
        fprintf(self->dumper.out, "... %lu more values\n", (unsigned long)more);
    }

    for (i = 0; i < d->depth; i++)
        fprintf(self->dumper.out, " ");
    fprintf(self->dumper.out, "} # %s %s \n", a->creator->op, a->name);
    grib_context_free(d->context, buf);
}

static void dump_label(grib_dumper* d, grib_accessor* a, const char* comment)
{
    grib_dumper_debug* self = (grib_dumper_debug*)d;
    int i;
    for (i = 0; i < d->depth; i++)
        fprintf(self->dumper.out, " ");
    fprintf(self->dumper.out, "----> %s %s %s\n", a->creator->op, a->name, comment ? comment : "");
}

static void dump_section(grib_dumper* d, grib_accessor* a, grib_block_of_accessors* block)
{
    grib_dumper_debug* self = (grib_dumper_debug*)d;
    int i;
    /* grib_section* s = grib_get_sub_section(a); */
    grib_section* s = a->sub_section;

    if (a->name[0] == '_') {
        grib_dump_accessors_block(d, block);
        return;
    }

    for (i = 0; i < d->depth; i++)
        fprintf(self->dumper.out, " ");
    fprintf(self->dumper.out, "======> %s %s (%ld,%ld,%ld)\n", a->creator->op,
            a->name, a->length, (long)s->length, (long)s->padding);
    if (!strncmp(a->name, "section", 7))
        self->section_offset = a->offset;
    /*printf("------------- section_offset = %ld\n",self->section_offset);*/
    d->depth += 3;
    grib_dump_accessors_block(d, block);
    d->depth -= 3;

    for (i = 0; i < d->depth; i++)
        fprintf(self->dumper.out, " ");
    fprintf(self->dumper.out, "<===== %s %s\n", a->creator->op, a->name);
}

static void set_begin_end(grib_dumper* d, grib_accessor* a)
{
    grib_dumper_debug* self = (grib_dumper_debug*)d;
    if ((d->option_flags & GRIB_DUMP_FLAG_OCTET) != 0) {
        self->begin  = a->offset - self->section_offset + 1;
        self->theEnd = a->get_next_position_offset() - self->section_offset;
    }
    else {
        self->begin  = a->offset;
        self->theEnd = a->get_next_position_offset();
    }
}
