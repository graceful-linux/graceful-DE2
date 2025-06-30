//
// Created by dingjing on 25-6-28.
//
#include "gd-output-info-private.h"

#include "gd-edid-private.h"


G_DEFINE_BOXED_TYPE(GDOutputInfo, gd_output_info, gd_output_info_ref, gd_output_info_unref)

GDOutputInfo*
gd_output_info_new(void)
{
    GDOutputInfo* self;

    self = g_new0(GDOutputInfo, 1);
    g_ref_count_init(&self->ref_count);

    return self;
}

GDOutputInfo*
gd_output_info_ref(GDOutputInfo* self)
{
    g_ref_count_inc(&self->ref_count);

    return self;
}

void
gd_output_info_unref(GDOutputInfo* self)
{
    if (g_ref_count_dec(&self->ref_count)) {
        g_free(self->name);
        g_free(self->vendor);
        g_free(self->product);
        g_free(self->serial);
        g_free(self->modes);
        g_free(self->possible_crtcs);
        g_free(self->possible_clones);
        g_free(self);
    }
}

void
gd_output_info_parse_edid(GDOutputInfo* output_info, GBytes* edid)
{
    GDEdidInfo* parsed_edid;
    gsize len;

    if (!edid) {
        output_info->vendor = g_strdup("unknown");
        output_info->product = g_strdup("unknown");
        output_info->serial = g_strdup("unknown");
        return;
    }

    parsed_edid = gd_edid_info_new_parse(g_bytes_get_data(edid, &len));
    if (parsed_edid) {
        output_info->vendor = g_strndup(parsed_edid->manufacturer_code, 4);
        if (!g_utf8_validate(output_info->vendor, -1, NULL))
            g_clear_pointer(&output_info->vendor, g_free);

        output_info->product = g_strndup(parsed_edid->dsc_product_name, 14);
        if (!g_utf8_validate(output_info->product, -1, NULL) || output_info->product[0] == '\0') {
            g_clear_pointer(&output_info->product, g_free);
            output_info->product = g_strdup_printf("0x%04x", (unsigned)parsed_edid->product_code);
        }

        output_info->serial = g_strndup(parsed_edid->dsc_serial_number, 14);
        if (!g_utf8_validate(output_info->serial, -1, NULL) || output_info->serial[0] == '\0') {
            g_clear_pointer(&output_info->serial, g_free);
            output_info->serial = g_strdup_printf("0x%08x", parsed_edid->serial_number);
        }

        g_free(parsed_edid);
    }

    if (!output_info->vendor) output_info->vendor = g_strdup("unknown");

    if (!output_info->product) output_info->product = g_strdup("unknown");

    if (!output_info->serial) output_info->serial = g_strdup("unknown");
}
