#ifndef _DEVICE_INFO_H_
#define _DEVICE_INFO_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

// glib
#include <glib.h>

// udev
#include <libudev.h>
#include <fcntl.h>
#include <errno.h>

// intltool
#include <glib/gi18n.h>



typedef struct device_t  {
    struct udev_device *udevice;
    char *devnode;
    char *native_path;
    char *major;
    char *minor;
    char *mount_points;

    gboolean device_is_system_internal;
    gboolean device_is_partition;
    gboolean device_is_partition_table;
    gboolean device_is_removable;
    gboolean device_is_media_available;
    gboolean device_is_read_only;
    gboolean device_is_drive;
    gboolean device_is_optical_disc;
    gboolean device_is_mounted;
    char *device_presentation_hide;
    char *device_presentation_nopolicy;
    char *device_presentation_name;
    char *device_presentation_icon_name;
    char *device_automount_hint;
    char *device_by_id;
    guint64 device_size;
    guint64 device_block_size;
    char *id_usage;
    char *id_type;
    char *id_version;
    char *id_uuid;
    char *id_label;

    char *drive_vendor;
    char *drive_model;
    char *drive_revision;
    char *drive_serial;
    char *drive_wwn;
    char *drive_connection_interface;
    guint64 drive_connection_speed;
    char *drive_media_compatibility;
    char *drive_media;
    gboolean drive_is_media_ejectable;
    gboolean drive_can_detach;

    char *partition_scheme;
    char *partition_number;
    char *partition_type;
    char *partition_label;
    char *partition_uuid;
    char *partition_flags;
    char *partition_offset;
    char *partition_size;
    char *partition_alignment_offset;

    char *partition_table_scheme;
    char *partition_table_count;

    gboolean optical_disc_is_blank;
    gboolean optical_disc_is_appendable;
    gboolean optical_disc_is_closed;
    char *optical_disc_num_tracks;
    char *optical_disc_num_audio_tracks;
    char *optical_disc_num_sessions;
} device_t;

typedef struct devmount_t {
    guint major;
    guint minor;
    char *mount_points;
    GList* mounts;
} devmount_t;

device_t *device_alloc( struct udev_device *udevice );
void device_free( device_t *device );
gboolean device_get_info( device_t *device, GList* devmounts );
char* device_show_info( device_t *device );

#endif
