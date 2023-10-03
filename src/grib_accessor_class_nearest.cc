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
/*
   This is used by make_class.pl

   START_CLASS_DEF
   CLASS      = accessor
   SUPER      = grib_accessor_class_gen
   IMPLEMENTS = init;dump
   MEMBERS=grib_arguments* args
   END_CLASS_DEF

 */

/* START_CLASS_IMP */

/*

Don't edit anything between START_CLASS_IMP and END_CLASS_IMP
Instead edit values between START_CLASS_DEF and END_CLASS_DEF
or edit "accessor.class" and rerun ./make_class.pl

*/

static void dump(grib_accessor*, grib_dumper*);
static void init(grib_accessor*, const long, grib_arguments*);

typedef struct grib_accessor_nearest
{
    grib_accessor att;
    /* Members defined in gen */
    /* Members defined in nearest */
    grib_arguments* args;
} grib_accessor_nearest;

extern grib_accessor_class* grib_accessor_class_gen;

static grib_accessor_class _grib_accessor_class_nearest = {
    &grib_accessor_class_gen,                      /* super */
    "nearest",                      /* name */
    sizeof(grib_accessor_nearest),  /* size */
    0,                           /* inited */
    0,                           /* init_class */
    &init,                       /* init */
    0,                  /* post_init */
    0,                    /* destroy */
    &dump,                       /* dump */
    0,                /* next_offset */
    0,              /* get length of string */
    0,                /* get number of values */
    0,                 /* get number of bytes */
    0,                /* get offset to bytes */
    0,            /* get native type */
    0,                /* get sub_section */
    0,               /* pack_missing */
    0,                 /* is_missing */
    0,                  /* pack_long */
    0,                /* unpack_long */
    0,                /* pack_double */
    0,                 /* pack_float */
    0,              /* unpack_double */
    0,               /* unpack_float */
    0,                /* pack_string */
    0,              /* unpack_string */
    0,          /* pack_string_array */
    0,        /* unpack_string_array */
    0,                 /* pack_bytes */
    0,               /* unpack_bytes */
    0,            /* pack_expression */
    0,              /* notify_change */
    0,                /* update_size */
    0,             /* preferred_size */
    0,                     /* resize */
    0,      /* nearest_smaller_value */
    0,                       /* next accessor */
    0,                    /* compare vs. another accessor */
    0,      /* unpack only ith value (double) */
    0,       /* unpack only ith value (float) */
    0,  /* unpack a given set of elements (double) */
    0,   /* unpack a given set of elements (float) */
    0,     /* unpack a subarray */
    0,                      /* clear */
    0,                 /* clone accessor */
};


grib_accessor_class* grib_accessor_class_nearest = &_grib_accessor_class_nearest;

/* END_CLASS_IMP */

static void init(grib_accessor* a, const long l, grib_arguments* args)
{
    grib_accessor_nearest* self = (grib_accessor_nearest*)a;
    self->args                  = args;
}

static void dump(grib_accessor* a, grib_dumper* dumper)
{
    /* TODO: pass args */
    grib_dump_label(dumper, a, NULL);
}

grib_nearest* grib_nearest_new(const grib_handle* ch, int* error)
{
    grib_handle* h            = (grib_handle*)ch;
    grib_accessor* a          = NULL;
    grib_accessor_nearest* na = NULL;
    grib_nearest* n           = NULL;
    *error                    = GRIB_NOT_IMPLEMENTED;
    a                         = grib_find_accessor(h, "NEAREST");
    na                        = (grib_accessor_nearest*)a;

    if (!a)
        return NULL;

    n = grib_nearest_factory(h, na->args, error);

    if (n)
        *error = GRIB_SUCCESS;

    return n;
}
