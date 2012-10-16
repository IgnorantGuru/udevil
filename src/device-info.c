/* *************************************************************************
 * device-info.c    GPL v3  Copyright 2012
 * contains code excerpts from udisks v1.0.4
************************************************************************** */

#include "device-info.h"

static char *
_dupv8 (const char *s)
{
  const char *end_valid;

  if (!g_utf8_validate (s, -1, &end_valid))
    {
      g_print ("**** NOTE: The string '%s' is not valid UTF-8. Invalid characters begins at '%s'\n", s, end_valid);
      return g_strndup (s, end_valid - s);
    }
  else
    {
      return g_strdup (s);
    }
}

/* unescapes things like \x20 to " " and ensures the returned string is valid UTF-8.
 *
 * see volume_id_encode_string() in extras/volume_id/lib/volume_id.c in the
 * udev tree for the encoder
 */
static gchar *
decode_udev_encoded_string (const gchar *str)
{
  GString *s;
  gchar *ret;
  const gchar *end_valid;
  guint n;

  s = g_string_new (NULL);
  for (n = 0; str[n] != '\0'; n++)
    {
      if (str[n] == '\\')
        {
          gint val;

          if (str[n + 1] != 'x' || str[n + 2] == '\0' || str[n + 3] == '\0')
            {
              g_print ("**** NOTE: malformed encoded string '%s'\n", str);
              break;
            }

          val = (g_ascii_xdigit_value (str[n + 2]) << 4) | g_ascii_xdigit_value (str[n + 3]);

          g_string_append_c (s, val);

          n += 3;
        }
      else
        {
          g_string_append_c (s, str[n]);
        }
    }

  if (!g_utf8_validate (s->str, -1, &end_valid))
    {
      g_print ("**** NOTE: The string '%s' is not valid UTF-8. Invalid characters begins at '%s'\n", s->str, end_valid);
      ret = g_strndup (s->str, end_valid - s->str);
      g_string_free (s, TRUE);
    }
  else
    {
      ret = g_string_free (s, FALSE);
    }

  return ret;
}

static gint
ptr_str_array_compare (const gchar **a,
                       const gchar **b)
{
  return g_strcmp0 (*a, *b);
}

static double
sysfs_get_double (const char *dir,
                  const char *attribute)
{
  double result;
  char *contents;
  char *filename;

  result = 0.0;
  filename = g_build_filename (dir, attribute, NULL);
  if (g_file_get_contents (filename, &contents, NULL, NULL))
    {
      result = atof (contents);
      g_free (contents);
    }
  g_free (filename);

  return result;
}

static char *
sysfs_get_string (const char *dir,
                  const char *attribute)
{
  char *result;
  char *filename;

  result = NULL;
  filename = g_build_filename (dir, attribute, NULL);
  if (!g_file_get_contents (filename, &result, NULL, NULL))
    {
      result = g_strdup ("");
    }
  g_free (filename);

  return result;
}

static int
sysfs_get_int (const char *dir,
               const char *attribute)
{
  int result;
  char *contents;
  char *filename;

  result = 0;
  filename = g_build_filename (dir, attribute, NULL);
  if (g_file_get_contents (filename, &contents, NULL, NULL))
    {
      result = strtol (contents, NULL, 0);
      g_free (contents);
    }
  g_free (filename);

  return result;
}

static guint64
sysfs_get_uint64 (const char *dir,
                  const char *attribute)
{
  guint64 result;
  char *contents;
  char *filename;

  result = 0;
  filename = g_build_filename (dir, attribute, NULL);
  if (g_file_get_contents (filename, &contents, NULL, NULL))
    {
      result = strtoll (contents, NULL, 0);
      g_free (contents);
    }
  g_free (filename);

  return result;
}

static gboolean
sysfs_file_exists (const char *dir,
                   const char *attribute)
{
  gboolean result;
  char *filename;

  result = FALSE;
  filename = g_build_filename (dir, attribute, NULL);
  if (g_file_test (filename, G_FILE_TEST_EXISTS))
    {
      result = TRUE;
    }
  g_free (filename);

  return result;
}

static char *
sysfs_resolve_link (const char *sysfs_path,
                    const char *name)
{
  char *full_path;
  char link_path[PATH_MAX];
  char resolved_path[PATH_MAX];
  ssize_t num;
  gboolean found_it;

  found_it = FALSE;

  full_path = g_build_filename (sysfs_path, name, NULL);

  //g_debug ("name='%s'", name);
  //g_debug ("full_path='%s'", full_path);
  num = readlink (full_path, link_path, sizeof(link_path) - 1);
  if (num != -1)
    {
      char *absolute_path;

      link_path[num] = '\0';

      //g_debug ("link_path='%s'", link_path);
      absolute_path = g_build_filename (sysfs_path, link_path, NULL);
      //g_debug ("absolute_path='%s'", absolute_path);
      if (realpath (absolute_path, resolved_path) != NULL)
        {
          //g_debug ("resolved_path='%s'", resolved_path);
          found_it = TRUE;
        }
      g_free (absolute_path);
    }
  g_free (full_path);

  if (found_it)
    return g_strdup (resolved_path);
  else
    return NULL;
}

gboolean info_is_system_internal( device_t *device )
{
    const char *value;

    if ( value = udev_device_get_property_value( device->udevice, "UDISKS_SYSTEM_INTERNAL" ) )
        return atoi( value ) != 0;

    /* A Linux MD device is system internal if, and only if
    *
    * - a single component is system internal
    * - there are no components
    * SKIP THIS TEST
    */

    /* a partition is system internal only if the drive it belongs to is system internal */
    //TODO

    /* a LUKS cleartext device is system internal only if the underlying crypto-text
    * device is system internal
    * SKIP THIS TEST
    */

    // devices with removable media are never system internal
    if ( device->device_is_removable )
        return FALSE;

    /* devices on certain buses are never system internal */
    if ( device->drive_connection_interface != NULL )
    {
        if (strcmp (device->drive_connection_interface, "ata_serial_esata") == 0
          || strcmp (device->drive_connection_interface, "sdio") == 0
          || strcmp (device->drive_connection_interface, "usb") == 0
          || strcmp (device->drive_connection_interface, "firewire") == 0)
            return FALSE;
    }
    return TRUE;
}

void info_drive_connection( device_t *device )
{
  char *s;
  char *p;
  char *q;
  char *model;
  char *vendor;
  char *subsystem;
  char *serial;
  char *revision;
  const char *connection_interface;
  guint64 connection_speed;

  connection_interface = NULL;
  connection_speed = 0;

  /* walk up the device tree to figure out the subsystem */
  s = g_strdup (device->native_path);
  do
    {
      p = sysfs_resolve_link (s, "subsystem");
      if ( !device->device_is_removable && sysfs_get_int( s, "removable") != 0 )
            device->device_is_removable = TRUE;
      if (p != NULL)
        {
          subsystem = g_path_get_basename (p);
          g_free (p);

          if (strcmp (subsystem, "scsi") == 0)
            {
              connection_interface = "scsi";
              connection_speed = 0;

              /* continue walking up the chain; we just use scsi as a fallback */

              /* grab the names from SCSI since the names from udev currently
               *  - replaces whitespace with _
               *  - is missing for e.g. Firewire
               */
              vendor = sysfs_get_string (s, "vendor");
              if (vendor != NULL)
                {
                  g_strstrip (vendor);
                  /* Don't overwrite what we set earlier from ID_VENDOR */
                  if (device->drive_vendor == NULL)
                    {
                      device->drive_vendor = _dupv8 (vendor);
                    }
                  g_free (vendor);
                }

              model = sysfs_get_string (s, "model");
              if (model != NULL)
                {
                  g_strstrip (model);
                  /* Don't overwrite what we set earlier from ID_MODEL */
                  if (device->drive_model == NULL)
                    {
                      device->drive_model = _dupv8 (model);
                    }
                  g_free (model);
                }

              /* TODO: need to improve this code; we probably need the kernel to export more
               *       information before we can properly get the type and speed.
               */

              if (device->drive_vendor != NULL && strcmp (device->drive_vendor, "ATA") == 0)
                {
                  connection_interface = "ata";
                  break;
                }

            }
          else if (strcmp (subsystem, "usb") == 0)
            {
              double usb_speed;

              /* both the interface and the device will be 'usb'. However only
               * the device will have the 'speed' property.
               */
              usb_speed = sysfs_get_double (s, "speed");
              if (usb_speed > 0)
                {
                  connection_interface = "usb";
                  connection_speed = usb_speed * (1000 * 1000);
                  break;

                }
            }
          else if (strcmp (subsystem, "firewire") == 0 || strcmp (subsystem, "ieee1394") == 0)
            {

              /* TODO: krh has promised a speed file in sysfs; theoretically, the speed can
               *       be anything from 100, 200, 400, 800 and 3200. Till then we just hardcode
               *       a resonable default of 400 Mbit/s.
               */

              connection_interface = "firewire";
              connection_speed = 400 * (1000 * 1000);
              break;

            }
          else if (strcmp (subsystem, "mmc") == 0)
            {

              /* TODO: what about non-SD, e.g. MMC? Is that another bus? */
              connection_interface = "sdio";

              /* Set vendor name. According to this MMC document
               *
               * http://www.mmca.org/membership/IAA_Agreement_10_12_06.pdf
               *
               *  - manfid: the manufacturer id
               *  - oemid: the customer of the manufacturer
               *
               * Apparently these numbers are kept secret. It would be nice
               * to map these into names for setting the manufacturer of the drive,
               * e.g. Panasonic, Sandisk etc.
               */

              model = sysfs_get_string (s, "name");
              if (model != NULL)
                {
                  g_strstrip (model);
                  /* Don't overwrite what we set earlier from ID_MODEL */
                  if (device->drive_model == NULL)
                    {
                      device->drive_model = _dupv8 (model);
                    }
                  g_free (model);
                }

              serial = sysfs_get_string (s, "serial");
              if (serial != NULL)
                {
                  g_strstrip (serial);
                  /* Don't overwrite what we set earlier from ID_SERIAL */
                  if (device->drive_serial == NULL)
                    {
                      /* this is formatted as a hexnumber; drop the leading 0x */
                      device->drive_serial = _dupv8 (serial + 2);
                    }
                  g_free (serial);
                }

              /* TODO: use hwrev and fwrev files? */
              revision = sysfs_get_string (s, "date");
              if (revision != NULL)
                {
                  g_strstrip (revision);
                  /* Don't overwrite what we set earlier from ID_REVISION */
                  if (device->drive_revision == NULL)
                    {
                      device->drive_revision = _dupv8 (revision);
                    }
                  g_free (revision);
                }

              /* TODO: interface speed; the kernel driver knows; would be nice
               * if it could export it */

            }
          else if (strcmp (subsystem, "platform") == 0)
            {
              const gchar *sysfs_name;

              sysfs_name = g_strrstr (s, "/");
              if (g_str_has_prefix (sysfs_name + 1, "floppy.")
                                            && device->drive_vendor == NULL )
                {
                  device->drive_vendor = g_strdup( "Floppy Drive" );
                  connection_interface = "platform";
                }
            }

          g_free (subsystem);
        }

      /* advance up the chain */
      p = g_strrstr (s, "/");
      if (p == NULL)
        break;
      *p = '\0';

      /* but stop at the root */
      if (strcmp (s, "/sys/devices") == 0)
        break;

    }
  while (TRUE);

  if (connection_interface != NULL)
    {
        device->drive_connection_interface = g_strdup( connection_interface );
        device->drive_connection_speed = connection_speed;
    }

  g_free (s);
}

static const struct
{
  const char *udev_property;
  const char *media_name;
} drive_media_mapping[] =
  {
    { "ID_DRIVE_FLASH", "flash" },
    { "ID_DRIVE_FLASH_CF", "flash_cf" },
    { "ID_DRIVE_FLASH_MS", "flash_ms" },
    { "ID_DRIVE_FLASH_SM", "flash_sm" },
    { "ID_DRIVE_FLASH_SD", "flash_sd" },
    { "ID_DRIVE_FLASH_SDHC", "flash_sdhc" },
    { "ID_DRIVE_FLASH_MMC", "flash_mmc" },
    { "ID_DRIVE_FLOPPY", "floppy" },
    { "ID_DRIVE_FLOPPY_ZIP", "floppy_zip" },
    { "ID_DRIVE_FLOPPY_JAZ", "floppy_jaz" },
    { "ID_CDROM", "optical_cd" },
    { "ID_CDROM_CD_R", "optical_cd_r" },
    { "ID_CDROM_CD_RW", "optical_cd_rw" },
    { "ID_CDROM_DVD", "optical_dvd" },
    { "ID_CDROM_DVD_R", "optical_dvd_r" },
    { "ID_CDROM_DVD_RW", "optical_dvd_rw" },
    { "ID_CDROM_DVD_RAM", "optical_dvd_ram" },
    { "ID_CDROM_DVD_PLUS_R", "optical_dvd_plus_r" },
    { "ID_CDROM_DVD_PLUS_RW", "optical_dvd_plus_rw" },
    { "ID_CDROM_DVD_PLUS_R_DL", "optical_dvd_plus_r_dl" },
    { "ID_CDROM_DVD_PLUS_RW_DL", "optical_dvd_plus_rw_dl" },
    { "ID_CDROM_BD", "optical_bd" },
    { "ID_CDROM_BD_R", "optical_bd_r" },
    { "ID_CDROM_BD_RE", "optical_bd_re" },
    { "ID_CDROM_HDDVD", "optical_hddvd" },
    { "ID_CDROM_HDDVD_R", "optical_hddvd_r" },
    { "ID_CDROM_HDDVD_RW", "optical_hddvd_rw" },
    { "ID_CDROM_MO", "optical_mo" },
    { "ID_CDROM_MRW", "optical_mrw" },
    { "ID_CDROM_MRW_W", "optical_mrw_w" },
    { NULL, NULL }, };

static const struct
{
  const char *udev_property;
  const char *media_name;
} media_mapping[] =
  {
    { "ID_DRIVE_MEDIA_FLASH", "flash" },
    { "ID_DRIVE_MEDIA_FLASH_CF", "flash_cf" },
    { "ID_DRIVE_MEDIA_FLASH_MS", "flash_ms" },
    { "ID_DRIVE_MEDIA_FLASH_SM", "flash_sm" },
    { "ID_DRIVE_MEDIA_FLASH_SD", "flash_sd" },
    { "ID_DRIVE_MEDIA_FLASH_SDHC", "flash_sdhc" },
    { "ID_DRIVE_MEDIA_FLASH_MMC", "flash_mmc" },
    { "ID_DRIVE_MEDIA_FLOPPY", "floppy" },
    { "ID_DRIVE_MEDIA_FLOPPY_ZIP", "floppy_zip" },
    { "ID_DRIVE_MEDIA_FLOPPY_JAZ", "floppy_jaz" },
    { "ID_CDROM_MEDIA_CD", "optical_cd" },
    { "ID_CDROM_MEDIA_CD_R", "optical_cd_r" },
    { "ID_CDROM_MEDIA_CD_RW", "optical_cd_rw" },
    { "ID_CDROM_MEDIA_DVD", "optical_dvd" },
    { "ID_CDROM_MEDIA_DVD_R", "optical_dvd_r" },
    { "ID_CDROM_MEDIA_DVD_RW", "optical_dvd_rw" },
    { "ID_CDROM_MEDIA_DVD_RAM", "optical_dvd_ram" },
    { "ID_CDROM_MEDIA_DVD_PLUS_R", "optical_dvd_plus_r" },
    { "ID_CDROM_MEDIA_DVD_PLUS_RW", "optical_dvd_plus_rw" },
    { "ID_CDROM_MEDIA_DVD_PLUS_R_DL", "optical_dvd_plus_r_dl" },
    { "ID_CDROM_MEDIA_DVD_PLUS_RW_DL", "optical_dvd_plus_rw_dl" },
    { "ID_CDROM_MEDIA_BD", "optical_bd" },
    { "ID_CDROM_MEDIA_BD_R", "optical_bd_r" },
    { "ID_CDROM_MEDIA_BD_RE", "optical_bd_re" },
    { "ID_CDROM_MEDIA_HDDVD", "optical_hddvd" },
    { "ID_CDROM_MEDIA_HDDVD_R", "optical_hddvd_r" },
    { "ID_CDROM_MEDIA_HDDVD_RW", "optical_hddvd_rw" },
    { "ID_CDROM_MEDIA_MO", "optical_mo" },
    { "ID_CDROM_MEDIA_MRW", "optical_mrw" },
    { "ID_CDROM_MEDIA_MRW_W", "optical_mrw_w" },
    { NULL, NULL }, };

void info_drive_properties ( device_t *device )
{
    GPtrArray *media_compat_array;
    const char *media_in_drive;
    gboolean drive_is_ejectable;
    gboolean drive_can_detach;
    char *decoded_string;
    guint n;
    const char *value;

    // drive identification
    device->device_is_drive = sysfs_file_exists( device->native_path, "range" );

    // vendor
    if ( value = udev_device_get_property_value( device->udevice, "ID_VENDOR_ENC" ) )
    {
        decoded_string = decode_udev_encoded_string ( value );
        g_strstrip (decoded_string);
        device->drive_vendor = decoded_string;
    }
    else if ( value = udev_device_get_property_value( device->udevice, "ID_VENDOR" ) )
    {
        device->drive_vendor = g_strdup( value );
    }

    // model
    if ( value = udev_device_get_property_value( device->udevice, "ID_MODEL_ENC" ) )
    {
        decoded_string = decode_udev_encoded_string ( value );
        g_strstrip (decoded_string);
        device->drive_model = decoded_string;
    }
    else if ( value = udev_device_get_property_value( device->udevice, "ID_MODEL" ) )
    {
        device->drive_model = g_strdup( value );
    }

    // revision
    device->drive_revision = g_strdup( udev_device_get_property_value(
                                                    device->udevice, "ID_REVISION" ) );

    // serial
    if ( value = udev_device_get_property_value( device->udevice, "ID_SCSI_SERIAL" ) )
    {
        /* scsi_id sometimes use the WWN as the serial - annoying - see
        * http://git.kernel.org/?p=linux/hotplug/udev.git;a=commit;h=4e9fdfccbdd16f0cfdb5c8fa8484a8ba0f2e69d3
        * for details
        */
        device->drive_serial = g_strdup( value );
    }
    else if ( value = udev_device_get_property_value( device->udevice, "ID_SERIAL_SHORT" ) )
    {
        device->drive_serial = g_strdup( value );
    }

    // wwn
    if ( value = udev_device_get_property_value( device->udevice, "ID_WWN_WITH_EXTENSION" ) )
    {
        device->drive_wwn = g_strdup( value + 2 );
    }
    else if ( value = udev_device_get_property_value( device->udevice, "ID_WWN" ) )
    {
        device->drive_wwn = g_strdup( value + 2 );
    }

    /* pick up some things (vendor, model, connection_interface, connection_speed)
    * not (yet) exported by udev helpers
    */
    //update_drive_properties_from_sysfs (device);
    info_drive_connection( device );

    // is_ejectable
    if ( value = udev_device_get_property_value( device->udevice, "ID_DRIVE_EJECTABLE" ) )
    {
        drive_is_ejectable = atoi( value ) != 0;
    }
    else
    {
      drive_is_ejectable = FALSE;
      drive_is_ejectable |= ( udev_device_get_property_value(
                                    device->udevice, "ID_CDROM" ) != NULL );
      drive_is_ejectable |= ( udev_device_get_property_value(
                                    device->udevice, "ID_DRIVE_FLOPPY_ZIP" ) != NULL );
      drive_is_ejectable |= ( udev_device_get_property_value(
                                    device->udevice, "ID_DRIVE_FLOPPY_JAZ" ) != NULL );
    }
    device->drive_is_media_ejectable = drive_is_ejectable;

    // drive_media_compatibility
    media_compat_array = g_ptr_array_new ();
    for (n = 0; drive_media_mapping[n].udev_property != NULL; n++)
    {
        if ( udev_device_get_property_value( device->udevice,
                                drive_media_mapping[n].udev_property ) == NULL )
            continue;

      g_ptr_array_add (media_compat_array, (gpointer) drive_media_mapping[n].media_name);
    }
    /* special handling for SDIO since we don't yet have a sdio_id helper in udev to set properties */
    if (g_strcmp0 (device->drive_connection_interface, "sdio") == 0)
    {
      gchar *type;

      type = sysfs_get_string (device->native_path, "../../type");
      g_strstrip (type);
      if (g_strcmp0 (type, "MMC") == 0)
        {
          g_ptr_array_add (media_compat_array, "flash_mmc");
        }
      else if (g_strcmp0 (type, "SD") == 0)
        {
          g_ptr_array_add (media_compat_array, "flash_sd");
        }
      else if (g_strcmp0 (type, "SDHC") == 0)
        {
          g_ptr_array_add (media_compat_array, "flash_sdhc");
        }
      g_free (type);
    }
    g_ptr_array_sort (media_compat_array, (GCompareFunc) ptr_str_array_compare);
    g_ptr_array_add (media_compat_array, NULL);
    device->drive_media_compatibility = g_strjoinv( " ", (gchar**)media_compat_array->pdata );

    // drive_media
    media_in_drive = NULL;
    if (device->device_is_media_available)
    {
        for (n = 0; media_mapping[n].udev_property != NULL; n++)
        {
            if ( udev_device_get_property_value( device->udevice,
                                    media_mapping[n].udev_property ) == NULL )
                continue;
            // should this be media_mapping[n] ?  doesn't matter, same?
            media_in_drive = drive_media_mapping[n].media_name;
            break;
        }
      /* If the media isn't set (from e.g. udev rules), just pick the first one in media_compat - note
       * that this may be NULL (if we don't know what media is compatible with the drive) which is OK.
       */
        if (media_in_drive == NULL)
            media_in_drive = ((const gchar **) media_compat_array->pdata)[0];
    }
    device->drive_media = g_strdup( media_in_drive );
    g_ptr_array_free (media_compat_array, TRUE);

    // drive_can_detach
    // right now, we only offer to detach USB devices
    drive_can_detach = FALSE;
    if (g_strcmp0 (device->drive_connection_interface, "usb") == 0)
    {
      drive_can_detach = TRUE;
    }
    if ( value = udev_device_get_property_value( device->udevice, "ID_DRIVE_DETACHABLE" ) )
    {
        drive_can_detach = atoi( value ) != 0;
    }
    device->drive_can_detach = drive_can_detach;
}

void info_device_properties( device_t *device )
{
    const char* value;

    device->native_path = g_strdup( udev_device_get_syspath( device->udevice ) );
    device->devnode = g_strdup( udev_device_get_devnode( device->udevice ) );
    device->major = g_strdup( udev_device_get_property_value( device->udevice, "MAJOR") );
    device->minor = g_strdup( udev_device_get_property_value( device->udevice, "MINOR") );
    if ( !device->native_path || !device->devnode || !device->major || !device->minor )
    {
        if ( device->native_path )
            g_free( device->native_path );
        device->native_path = NULL;
        return;
    }

    //by id - would need to read symlinks in /dev/disk/by-id

    // is_removable may also be set in info_drive_connection walking up sys tree
    device->device_is_removable = sysfs_get_int( device->native_path, "removable");

    device->device_presentation_hide = g_strdup( udev_device_get_property_value(
                                            device->udevice, "UDISKS_PRESENTATION_HIDE") );
    device->device_presentation_nopolicy = g_strdup( udev_device_get_property_value(
                                            device->udevice, "UDISKS_PRESENTATION_NOPOLICY") );
    device->device_presentation_name = g_strdup( udev_device_get_property_value(
                                            device->udevice, "UDISKS_PRESENTATION_NAME") );
    device->device_presentation_icon_name = g_strdup( udev_device_get_property_value(
                                            device->udevice, "UDISKS_PRESENTATION_ICON_NAME") );
    device->device_automount_hint = g_strdup( udev_device_get_property_value(
                                            device->udevice, "UDISKS_AUTOMOUNT_HINT") );

    // filesystem properties
    gchar *decoded_string;
    const gchar *partition_scheme;
    gint partition_type = 0;

    partition_scheme = udev_device_get_property_value( device->udevice, "UDISKS_PARTITION_SCHEME");
    if ( value = udev_device_get_property_value( device->udevice, "UDISKS_PARTITION_TYPE") )
        partition_type = atoi( value );
    if (g_strcmp0 (partition_scheme, "mbr") == 0 && (partition_type == 0x05 ||
                                                   partition_type == 0x0f ||
                                                   partition_type == 0x85))
    {
    }
    else
    {
        device->id_usage = g_strdup( udev_device_get_property_value( device->udevice,
                                                        "ID_FS_USAGE" ) );
        device->id_type = g_strdup( udev_device_get_property_value( device->udevice,
                                                        "ID_FS_TYPE" ) );
        device->id_version = g_strdup( udev_device_get_property_value( device->udevice,
                                                        "ID_FS_VERSION" ) );
        device->id_uuid = g_strdup( udev_device_get_property_value( device->udevice,
                                                        "ID_FS_UUID" ) );

        if ( value = udev_device_get_property_value( device->udevice, "ID_FS_LABEL_ENC" ) )
        {
            decoded_string = decode_udev_encoded_string ( value );
            g_strstrip (decoded_string);
            device->id_label = decoded_string;
        }
        else if ( value = udev_device_get_property_value( device->udevice, "ID_FS_LABEL" ) )
        {
            device->id_label = g_strdup( value );
        }
    }

    // device_is_media_available
    gboolean media_available = FALSE;

    if ( ( device->id_usage && device->id_usage[0] != '\0' ) ||
         ( device->id_type  && device->id_type[0]  != '\0' ) ||
         ( device->id_uuid  && device->id_uuid[0]  != '\0' ) ||
         ( device->id_label && device->id_label[0] != '\0' ) )
    {
         media_available = TRUE;
    }
    else if ( g_str_has_prefix( device->devnode, "/dev/loop" ) )
        media_available = FALSE;
    else if ( device->device_is_removable )
    {
        gboolean is_cd, is_floppy;
        if ( value = udev_device_get_property_value( device->udevice, "ID_CDROM" ) )
            is_cd = atoi( value ) != 0;
        else
            is_cd = FALSE;

        if ( value = udev_device_get_property_value( device->udevice, "ID_DRIVE_FLOPPY" ) )
            is_floppy = atoi( value ) != 0;
        else
            is_floppy = FALSE;

        if ( !is_cd && !is_floppy )
        {
            // this test is limited for non-root - user may not have read
            // access to device file even if media is present
            int fd;
            fd = open( device->devnode, O_RDONLY );
            if ( fd >= 0 )
            {
                media_available = TRUE;
                close( fd );
            }
        }
        else if ( value = udev_device_get_property_value( device->udevice, "ID_CDROM_MEDIA" ) )
            media_available = ( atoi( value ) == 1 );
    }
    else if ( value = udev_device_get_property_value( device->udevice, "ID_CDROM_MEDIA" ) )
        media_available = ( atoi( value ) == 1 );
    else
        media_available = TRUE;
    device->device_is_media_available = media_available;

    /* device_size, device_block_size and device_is_read_only properties */
    if (device->device_is_media_available)
    {
        guint64 block_size;

        device->device_size = sysfs_get_uint64( device->native_path, "size")
                                                                * ((guint64) 512);
        device->device_is_read_only = (sysfs_get_int (device->native_path,
                                                                    "ro") != 0);
        /* This is not available on all devices so fall back to 512 if unavailable.
        *
        * Another way to get this information is the BLKSSZGET ioctl but we don't want
        * to open the device. Ideally vol_id would export it.
        */
        block_size = sysfs_get_uint64 (device->native_path, "queue/hw_sector_size");
        if (block_size == 0)
            block_size = 512;
        device->device_block_size = block_size;
    }
    else
    {
        device->device_size = device->device_block_size = 0;
        device->device_is_read_only = FALSE;
    }

    // links
    struct udev_list_entry *entry = udev_device_get_devlinks_list_entry(
                                                                device->udevice );
    while ( entry )
    {
        const char *entry_name = udev_list_entry_get_name( entry );
        if ( entry_name && ( g_str_has_prefix( entry_name, "/dev/disk/by-id/" )
                || g_str_has_prefix( entry_name, "/dev/disk/by-uuid/" ) ) )
        {
            device->device_by_id = g_strdup( entry_name );
            break;
        }
        entry = udev_list_entry_get_next( entry );
    }
}

gchar* info_mount_points( device_t *device, GList* devmounts )
{
    gchar *contents;
    gchar **lines;
    GError *error;
    guint n;
    GList* mounts = NULL;

    if ( !device->major || !device->minor )
        return NULL;
    guint dmajor = atoi( device->major );
    guint dminor = atoi( device->minor );

    // if we have the mount point list, use this instead of reading mountinfo
    if ( devmounts )
    {
        GList* l;
        for ( l = devmounts; l; l = l->next )
        {
            if ( ((devmount_t*)l->data)->major == dmajor &&
                                        ((devmount_t*)l->data)->minor == dminor )
            {
                return g_strdup( ((devmount_t*)l->data)->mount_points );
            }
        }
        return NULL;
    }

    contents = NULL;
    lines = NULL;

    error = NULL;
    if (!g_file_get_contents ("/proc/self/mountinfo", &contents, NULL, &error))
    {
        g_warning ("Error reading /proc/self/mountinfo: %s", error->message);
        g_error_free (error);
        return NULL;
    }

    /* See Documentation/filesystems/proc.txt for the format of /proc/self/mountinfo
    *
    * Note that things like space are encoded as \020.
    */

  lines = g_strsplit (contents, "\n", 0);
  for (n = 0; lines[n] != NULL; n++)
    {
      guint mount_id;
      guint parent_id;
      guint major, minor;
      gchar encoded_root[PATH_MAX];
      gchar encoded_mount_point[PATH_MAX];
      gchar *mount_point;
      //dev_t dev;

      if (strlen (lines[n]) == 0)
        continue;

      if (sscanf (lines[n],
                  "%d %d %d:%d %s %s",
                  &mount_id,
                  &parent_id,
                  &major,
                  &minor,
                  encoded_root,
                  encoded_mount_point) != 6)
        {
          g_warning ("Error reading /proc/self/mountinfo: Error parsing line '%s'", lines[n]);
          continue;
        }

        /* ignore mounts where only a subtree of a filesystem is mounted */
        if (g_strcmp0 (encoded_root, "/") != 0)
            continue;

        /* Temporary work-around for btrfs, see
        *
        *  https://github.com/IgnorantGuru/spacefm/issues/165
        *  http://article.gmane.org/gmane.comp.file-systems.btrfs/2851
        *  https://bugzilla.redhat.com/show_bug.cgi?id=495152#c31
        */
        if ( major == 0 )
        {
            const gchar *sep;
            sep = strstr( lines[n], " - " );
            if ( sep != NULL )
            {
                gchar typebuf[PATH_MAX];
                gchar mount_source[PATH_MAX];
                struct stat statbuf;

                if ( sscanf( sep + 3, "%s %s", typebuf, mount_source ) == 2 &&
                                !g_strcmp0( typebuf, "btrfs" ) &&
                                g_str_has_prefix( mount_source, "/dev/" ) &&
                                stat( mount_source, &statbuf ) == 0 &&
                                S_ISBLK( statbuf.st_mode ) )
                {
                    major = major( statbuf.st_rdev );
                    minor = minor( statbuf.st_rdev );
                }
            }
        }

        if ( major != dmajor || minor != dminor )
            continue;

      mount_point = g_strcompress (encoded_mount_point);
      if ( mount_point && mount_point[0] != '\0' )
      {
        if ( !g_list_find( mounts, mount_point ) )
        {
            mounts = g_list_prepend( mounts, mount_point );
        }
        else
            g_free (mount_point);
      }

    }
  g_free (contents);
  g_strfreev (lines);

    if ( mounts )
    {
        gchar *points, *old_points;
        GList* l;
        // Sort the list to ensure that shortest mount paths appear first
        mounts = g_list_sort( mounts, (GCompareFunc) g_strcmp0 );
        points = g_strdup( (gchar*)mounts->data );
        l = mounts;
        while ( l = l->next )
        {
            old_points = points;
            points = g_strdup_printf( "%s, %s", old_points, (gchar*)l->data );
            g_free( old_points );
        }
        g_list_foreach( mounts, (GFunc)g_free, NULL );
        g_list_free( mounts );
        return points;
    }
    else
        return NULL;
}

void info_partition_table( device_t *device )
{
  gboolean is_partition_table = FALSE;
  const char* value;

    /* Check if udisks-part-id identified the device as a partition table.. this includes
    * identifying partition tables set up by kpartx for multipath etc.
    */
    if ( ( value = udev_device_get_property_value( device->udevice, "UDISKS_PARTITION_TABLE" ) )
                                                    && atoi( value ) == 1 )
    {
        device->partition_table_scheme = g_strdup( udev_device_get_property_value(
                                                device->udevice,
                                                "UDISKS_PARTITION_TABLE_SCHEME" ) );
        device->partition_table_count = g_strdup( udev_device_get_property_value(
                                                device->udevice,
                                                "UDISKS_PARTITION_TABLE_COUNT" ) );
        is_partition_table = TRUE;
    }

  /* Note that udisks-part-id might not detect all partition table
   * formats.. so in the negative case, also double check with
   * information in sysfs.
   *
   * The kernel guarantees that all childs are created before the
   * uevent for the parent is created. So if we have childs, we must
   * be a partition table.
   *
   * To detect a child we check for the existance of a subdir that has
   * the parents name as a prefix (e.g. for parent sda then sda1,
   * sda2, sda3 ditto md0, md0p1 etc. etc. will work).
   */
  if (!is_partition_table)
    {
      gchar *s;
      GDir *dir;

      s = g_path_get_basename (device->native_path);
      if ((dir = g_dir_open (device->native_path, 0, NULL)) != NULL)
        {
          guint partition_count;
          const gchar *name;

          partition_count = 0;
          while ((name = g_dir_read_name (dir)) != NULL)
            {
              if (g_str_has_prefix (name, s))
                {
                  partition_count++;
                }
            }
          g_dir_close (dir);

          if (partition_count > 0)
            {
              device->partition_table_scheme = g_strdup( "" );
              device->partition_table_count = g_strdup_printf( "%d", partition_count );
              is_partition_table = TRUE;
            }
        }
      g_free (s);
    }

  device->device_is_partition_table = is_partition_table;
  if (!is_partition_table)
    {
        if ( device->partition_table_scheme )
            g_free( device->partition_table_scheme );
        device->partition_table_scheme = NULL;
        if ( device->partition_table_count )
            g_free( device->partition_table_count );
        device->partition_table_count = NULL;
    }
}

void info_partition( device_t *device )
{
  gboolean is_partition = FALSE;

  /* Check if udisks-part-id identified the device as a partition.. this includes
   * identifying partitions set up by kpartx for multipath
   */
  if ( udev_device_get_property_value( device->udevice,"UDISKS_PARTITION" ) )
    {
      const gchar *size;
      const gchar *scheme;
      const gchar *type;
      const gchar *label;
      const gchar *uuid;
      const gchar *flags;
      const gchar *offset;
      const gchar *alignment_offset;
      const gchar *slave_sysfs_path;
      const gchar *number;

      scheme = udev_device_get_property_value( device->udevice, "UDISKS_PARTITION_SCHEME");
      size = udev_device_get_property_value( device->udevice, "UDISKS_PARTITION_SIZE");
      type = udev_device_get_property_value( device->udevice, "UDISKS_PARTITION_TYPE");
      label = udev_device_get_property_value( device->udevice, "UDISKS_PARTITION_LABEL");
      uuid = udev_device_get_property_value( device->udevice, "UDISKS_PARTITION_UUID");
      flags = udev_device_get_property_value( device->udevice, "UDISKS_PARTITION_FLAGS");
      offset = udev_device_get_property_value( device->udevice, "UDISKS_PARTITION_OFFSET");
      alignment_offset = udev_device_get_property_value( device->udevice,
                                                "UDISKS_PARTITION_ALIGNMENT_OFFSET");
      number = udev_device_get_property_value( device->udevice, "UDISKS_PARTITION_NUMBER");
      slave_sysfs_path = udev_device_get_property_value( device->udevice, "UDISKS_PARTITION_SLAVE");

      if (slave_sysfs_path != NULL && scheme != NULL && number != NULL && atoi( number ) > 0)
        {
          device->partition_scheme = g_strdup( scheme );
          device->partition_size = g_strdup( size );
          device->partition_type = g_strdup( type );
          device->partition_label = g_strdup( label );
          device->partition_uuid = g_strdup( uuid );
          device->partition_flags = g_strdup( flags );
          device->partition_offset = g_strdup( offset );
          device->partition_alignment_offset = g_strdup( alignment_offset );
          device->partition_number = g_strdup( number );
          is_partition = TRUE;
        }
    }

  /* Also handle the case where we are partitioned by the kernel and don't have
   * any UDISKS_PARTITION_* properties.
   *
   * This works without any udev UDISKS_PARTITION_* properties and is
   * there for maximum compatibility since udisks-part-id only knows a
   * limited set of partition table formats.
   */
  if (!is_partition && sysfs_file_exists (device->native_path, "start"))
    {
      guint64 size;
      guint64 offset;
      guint64 alignment_offset;
      gchar *s;
      guint n;

      size = sysfs_get_uint64 (device->native_path, "size");
      alignment_offset = sysfs_get_uint64 (device->native_path, "alignment_offset");

      device->partition_size = g_strdup_printf( "%d", size * 512 );
      device->partition_alignment_offset = g_strdup_printf( "%d", alignment_offset );

      offset = sysfs_get_uint64 (device->native_path, "start") * device->device_block_size;
      device->partition_offset = g_strdup_printf( "%d", offset );

      s = device->native_path;
      for (n = strlen (s) - 1; n >= 0 && g_ascii_isdigit (s[n]); n--)
        ;
      device->partition_number = g_strdup_printf( "%d", strtol (s + n + 1, NULL, 0) );
        /*
      s = g_strdup (device->priv->native_path);
      for (n = strlen (s) - 1; n >= 0 && s[n] != '/'; n--)
        s[n] = '\0';
      s[n] = '\0';
      device_set_partition_slave (device, compute_object_path (s));
      g_free (s);
        */
      is_partition = TRUE;
    }

    device->device_is_partition = is_partition;

  if (!is_partition)
    {
      device->partition_scheme = NULL;
      device->partition_size = NULL;
      device->partition_type = NULL;
      device->partition_label = NULL;
      device->partition_uuid = NULL;
      device->partition_flags = NULL;
      device->partition_offset = NULL;
      device->partition_alignment_offset = NULL;
      device->partition_number = NULL;
    }
  else
    {
        device->device_is_drive = FALSE;
    }
}

void info_optical_disc( device_t *device )
{
    const char *cdrom_disc_state;

    if ( udev_device_get_property_value( device->udevice, "ID_CDROM_MEDIA") )
    {
        device->device_is_optical_disc = TRUE;

        device->optical_disc_num_tracks = g_strdup( udev_device_get_property_value(
                                    device->udevice, "ID_CDROM_MEDIA_TRACK_COUNT") );
        device->optical_disc_num_audio_tracks = g_strdup( udev_device_get_property_value(
                                    device->udevice, "ID_CDROM_MEDIA_TRACK_COUNT_AUDIO") );
        device->optical_disc_num_sessions = g_strdup( udev_device_get_property_value(
                                    device->udevice, "ID_CDROM_MEDIA_SESSION_COUNT") );

        cdrom_disc_state = udev_device_get_property_value( device->udevice, "ID_CDROM_MEDIA_STATE");

        device->optical_disc_is_blank = ( g_strcmp0( cdrom_disc_state,
                                                            "blank" ) == 0);
        device->optical_disc_is_appendable = ( g_strcmp0( cdrom_disc_state,
                                                            "appendable" ) == 0);
        device->optical_disc_is_closed = ( g_strcmp0( cdrom_disc_state,
                                                            "complete" ) == 0);
    }
    else
        device->device_is_optical_disc = FALSE;
}

void device_free( device_t *device )
{
    if ( !device )
        return;

    g_free( device->native_path );
    g_free( device->major );
    g_free( device->minor );
    g_free( device->mount_points );
    g_free( device->devnode );

    g_free( device->device_presentation_hide );
    g_free( device->device_presentation_nopolicy );
    g_free( device->device_presentation_name );
    g_free( device->device_presentation_icon_name );
    g_free( device->device_automount_hint );
    g_free( device->device_by_id );
    g_free( device->id_usage );
    g_free( device->id_type );
    g_free( device->id_version );
    g_free( device->id_uuid );
    g_free( device->id_label );

    g_free( device->drive_vendor );
    g_free( device->drive_model );
    g_free( device->drive_revision );
    g_free( device->drive_serial );
    g_free( device->drive_wwn );
    g_free( device->drive_connection_interface );
    g_free( device->drive_media_compatibility );
    g_free( device->drive_media );

    g_free( device->partition_scheme );
    g_free( device->partition_number );
    g_free( device->partition_type );
    g_free( device->partition_label );
    g_free( device->partition_uuid );
    g_free( device->partition_flags );
    g_free( device->partition_offset );
    g_free( device->partition_size );
    g_free( device->partition_alignment_offset );

    g_free( device->partition_table_scheme );
    g_free( device->partition_table_count );

    g_free( device->optical_disc_num_tracks );
    g_free( device->optical_disc_num_audio_tracks );
    g_free( device->optical_disc_num_sessions );
    g_slice_free( device_t, device );
}

device_t *device_alloc( struct udev_device *udevice )
{
    device_t *device = g_slice_new0( device_t );
    device->udevice = udevice;

    device->native_path = NULL;
    device->major = NULL;
    device->minor = NULL;
    device->mount_points = NULL;
    device->devnode = NULL;

    device->device_is_system_internal = TRUE;
    device->device_is_partition = FALSE;
    device->device_is_partition_table = FALSE;
    device->device_is_removable = FALSE;
    device->device_is_media_available = FALSE;
    device->device_is_read_only = FALSE;
    device->device_is_drive = FALSE;
    device->device_is_optical_disc = FALSE;
    device->device_is_mounted = FALSE;
    device->device_presentation_hide = NULL;
    device->device_presentation_nopolicy = NULL;
    device->device_presentation_name = NULL;
    device->device_presentation_icon_name = NULL;
    device->device_automount_hint = NULL;
    device->device_by_id = NULL;
    device->device_size = 0;
    device->device_block_size = 0;
    device->id_usage = NULL;
    device->id_type = NULL;
    device->id_version = NULL;
    device->id_uuid = NULL;
    device->id_label = NULL;

    device->drive_vendor = NULL;
    device->drive_model = NULL;
    device->drive_revision = NULL;
    device->drive_serial = NULL;
    device->drive_wwn = NULL;
    device->drive_connection_interface = NULL;
    device->drive_connection_speed = 0;
    device->drive_media_compatibility = NULL;
    device->drive_media = NULL;
    device->drive_is_media_ejectable = FALSE;
    device->drive_can_detach = FALSE;

    device->partition_scheme = NULL;
    device->partition_number = NULL;
    device->partition_type = NULL;
    device->partition_label = NULL;
    device->partition_uuid = NULL;
    device->partition_flags = NULL;
    device->partition_offset = NULL;
    device->partition_size = NULL;
    device->partition_alignment_offset = NULL;

    device->partition_table_scheme = NULL;
    device->partition_table_count = NULL;

    device->optical_disc_is_blank = FALSE;
    device->optical_disc_is_appendable = FALSE;
    device->optical_disc_is_closed = FALSE;
    device->optical_disc_num_tracks = NULL;
    device->optical_disc_num_audio_tracks = NULL;
    device->optical_disc_num_sessions = NULL;

    return device;
}

gboolean device_get_info( device_t *device, GList* devmounts )
{
    info_device_properties( device );
    if ( !device->native_path )
        return FALSE;
    info_drive_properties( device );
    device->device_is_system_internal = info_is_system_internal( device );
    device->mount_points = info_mount_points( device, devmounts );
    device->device_is_mounted = ( device->mount_points != NULL );
    info_partition_table( device );
    info_partition( device );
    info_optical_disc( device );
    return TRUE;
}

char* device_show_info( device_t *device )
{   // no translate
    gchar* line[140];
    int i = 0;

    //line[i++] = g_strdup_printf("Showing information for %s\n", device->devnode );
    char* bdev = g_path_get_basename( device->devnode );
    line[i++] = g_strdup_printf("Showing information for /org/freedesktop/UDisks/devices/%s\n", bdev );
    g_free( bdev );
    line[i++] = g_strdup_printf("  native-path:                 %s\n", device->native_path );
    line[i++] = g_strdup_printf("  device:                      %s:%s\n", device->major, device->minor );
    line[i++] = g_strdup_printf("  device-file:                 %s\n", device->devnode );
    line[i++] = g_strdup_printf("    presentation:              %s\n", device->devnode );
    if ( device->device_by_id )
        line[i++] = g_strdup_printf("    by-id:                     %s\n", device->device_by_id );
    line[i++] = g_strdup_printf("  system internal:             %d\n", device->device_is_system_internal );
    line[i++] = g_strdup_printf("  removable:                   %d\n", device->device_is_removable);
    line[i++] = g_strdup_printf("  has media:                   %d\n", device->device_is_media_available);
    line[i++] = g_strdup_printf("  is read only:                %d\n", device->device_is_read_only );
    line[i++] = g_strdup_printf("  is mounted:                  %d\n", device->device_is_mounted );
    line[i++] = g_strdup_printf("  mount paths:                 %s\n", device->mount_points ? device->mount_points : "" );
    line[i++] = g_strdup_printf("  presentation hide:           %s\n", device->device_presentation_hide ?
                                                device->device_presentation_hide : "0" );
    line[i++] = g_strdup_printf("  presentation nopolicy:       %s\n", device->device_presentation_nopolicy ?
                                                device->device_presentation_nopolicy : "0" );
    line[i++] = g_strdup_printf("  presentation name:           %s\n", device->device_presentation_name ?
                                                device->device_presentation_name : "" );
    line[i++] = g_strdup_printf("  presentation icon:           %s\n", device->device_presentation_icon_name ?
                                                device->device_presentation_icon_name : "" );
    line[i++] = g_strdup_printf("  automount hint:              %s\n", device->device_automount_hint ?
                                                device->device_automount_hint : "" );
    line[i++] = g_strdup_printf("  size:                        %" G_GUINT64_FORMAT "\n", device->device_size);
    line[i++] = g_strdup_printf("  block size:                  %" G_GUINT64_FORMAT "\n", device->device_block_size);
    line[i++] = g_strdup_printf("  usage:                       %s\n", device->id_usage ? device->id_usage : "" );
    line[i++] = g_strdup_printf("  type:                        %s\n", device->id_type ? device->id_type : "" );
    line[i++] = g_strdup_printf("  version:                     %s\n", device->id_version ? device->id_version : "" );
    line[i++] = g_strdup_printf("  uuid:                        %s\n", device->id_uuid ? device->id_uuid : "" );
    line[i++] = g_strdup_printf("  label:                       %s\n", device->id_label ? device->id_label : "" );
    if (device->device_is_partition_table)
    {
        line[i++] = g_strdup_printf("  partition table:\n");
        line[i++] = g_strdup_printf("    scheme:                    %s\n", device->partition_table_scheme ?
                                                    device->partition_table_scheme : "" );
        line[i++] = g_strdup_printf("    count:                     %s\n", device->partition_table_count ?
                                                    device->partition_table_count : "0" );
    }
    if (device->device_is_partition)
    {
        line[i++] = g_strdup_printf("  partition:\n");
        line[i++] = g_strdup_printf("    scheme:                    %s\n", device->partition_scheme ?
                                                    device->partition_scheme : "" );
        line[i++] = g_strdup_printf("    number:                    %d\n", device->partition_number ?
                                                    device->partition_number : "" );
        line[i++] = g_strdup_printf("    type:                      %s\n", device->partition_type ?
                                                    device->partition_type : "" );
        line[i++] = g_strdup_printf("    flags:                     %s\n", device->partition_flags ?
                                                    device->partition_flags : "" );
        line[i++] = g_strdup_printf("    offset:                    %s\n", device->partition_offset ?
                                                    device->partition_offset : "" );
        line[i++] = g_strdup_printf("    alignment offset:          %s\n", device->partition_alignment_offset ?
                                                    device->partition_alignment_offset : "" );
        line[i++] = g_strdup_printf("    size:                      %s\n", device->partition_size ?
                                                    device->partition_size : "" );
        line[i++] = g_strdup_printf("    label:                     %s\n", device->partition_label ?
                                                    device->partition_label : "" );
        line[i++] = g_strdup_printf("    uuid:                      %s\n", device->partition_uuid ?
                                                    device->partition_uuid : "" );
    }
    if (device->device_is_optical_disc)
    {
        line[i++] = g_strdup_printf("  optical disc:\n");
        line[i++] = g_strdup_printf("    blank:                     %d\n", device->optical_disc_is_blank);
        line[i++] = g_strdup_printf("    appendable:                %d\n", device->optical_disc_is_appendable);
        line[i++] = g_strdup_printf("    closed:                    %d\n", device->optical_disc_is_closed);
        line[i++] = g_strdup_printf("    num tracks:                %s\n", device->optical_disc_num_tracks ?
                                                    device->optical_disc_num_tracks : "0" );
        line[i++] = g_strdup_printf("    num audio tracks:          %s\n", device->optical_disc_num_audio_tracks ?
                                                    device->optical_disc_num_audio_tracks : "0" );
        line[i++] = g_strdup_printf("    num sessions:              %s\n", device->optical_disc_num_sessions ?
                                                    device->optical_disc_num_sessions : "0" );
    }
    if (device->device_is_drive)
    {
        line[i++] = g_strdup_printf("  drive:\n");
        line[i++] = g_strdup_printf("    vendor:                    %s\n", device->drive_vendor ?
                                                    device->drive_vendor : "" );
        line[i++] = g_strdup_printf("    model:                     %s\n", device->drive_model ?
                                                    device->drive_model : "" );
        line[i++] = g_strdup_printf("    revision:                  %s\n", device->drive_revision ?
                                                    device->drive_revision : "" );
        line[i++] = g_strdup_printf("    serial:                    %s\n", device->drive_serial ?
                                                    device->drive_serial : "" );
        line[i++] = g_strdup_printf("    WWN:                       %s\n", device->drive_wwn ?
                                                    device->drive_wwn : "" );
        line[i++] = g_strdup_printf("    detachable:                %d\n", device->drive_can_detach);
        line[i++] = g_strdup_printf("    ejectable:                 %d\n", device->drive_is_media_ejectable);
        line[i++] = g_strdup_printf("    media:                     %s\n", device->drive_media ?
                                                    device->drive_media : "" );
        line[i++] = g_strdup_printf("      compat:                  %s\n", device->drive_media_compatibility ?
                                                    device->drive_media_compatibility : "" );
        if ( device->drive_connection_interface == NULL ||
                                strlen (device->drive_connection_interface) == 0 )
            line[i++] = g_strdup_printf("    interface:                 (unknown)\n");
        else
            line[i++] = g_strdup_printf("    interface:                 %s\n", device->drive_connection_interface);
        if (device->drive_connection_speed == 0)
            line[i++] = g_strdup_printf("    if speed:                  (unknown)\n");
        else
            line[i++] = g_strdup_printf("    if speed:                  %" G_GINT64_FORMAT " bits/s\n",
                                                    device->drive_connection_speed);
    }
    line[i] = NULL;
    gchar* output = g_strjoinv( NULL, line );
    i = 0;
    while ( line[i] )
        g_free( line[i++] );
    return output;
}

