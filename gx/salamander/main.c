/* RetroArch - A frontend for libretro.
 * Copyright (C) 2010-2012 - Hans-Kristian Arntzen
 * Copyright (C) 2011-2012 - Daniel De Matteis
 *
 * RetroArch is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with RetroArch.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include "../../boolean.h"
#include <stddef.h>
#include <string.h>

#include <sdcard/wiisd_io.h>
#include <sdcard/gcsd.h>
#include <fat.h>

#include "../../console/rarch_console.h"
#include "../../console/rarch_console_exec.h"
#include "../../console/rarch_console_libretro_mgmt.h"
#include "../../console/rarch_console_input.h"
#include "../../console/rarch_console_config.h"
#include "../../console/rarch_console_settings.h"
#include "../../console/rarch_console_main_wrap.h"
#include "../../conf/config_file.h"
#include "../../conf/config_file_macros.h"
#include "../../general.h"
#include "../../file.h"

char libretro_path[512];

default_paths_t default_paths;

static void find_and_set_first_file(void)
{
   //Last fallback - we'll need to start the first executable file 
   // we can find in the RetroArch cores directory

   char first_file[512] = {0};
   rarch_manage_libretro_set_first_file(first_file, sizeof(first_file),
   default_paths.core_dir, "dol");

   if(first_file[0])
      strlcpy(libretro_path, first_file, sizeof(libretro_path));
   else
      RARCH_ERR("Failed last fallback - RetroArch Salamander will exit.\n");
}

static void init_settings(void)
{
   char tmp_str[512];
   bool config_file_exists;

   if(!path_file_exists(default_paths.config_file))
   {
      FILE * f;
      config_file_exists = false;
      RARCH_ERR("Config file \"%s\" doesn't exist. Creating...\n", default_paths.config_file);
      f = fopen(default_paths.config_file, "w");
      fclose(f);
   }
   else
      config_file_exists = true;

   //try to find CORE executable
   char core_executable[1024];
   snprintf(core_executable, sizeof(core_executable), "%s/CORE.dol", default_paths.core_dir);

   if(path_file_exists(core_executable))
   {
      //Start CORE executable
      snprintf(libretro_path, sizeof(libretro_path), core_executable);
      RARCH_LOG("Start [%s].\n", libretro_path);
   }
   else
   {
      if(config_file_exists)
      {
         config_file_t * conf = config_file_new(default_paths.config_file);
         config_get_array(conf, "libretro_path", tmp_str, sizeof(tmp_str));
         snprintf(libretro_path, sizeof(libretro_path), tmp_str);
      }

      if(!config_file_exists || !strcmp(libretro_path, ""))
         find_and_set_first_file();
      else
      {
         RARCH_LOG("Start [%s] found in retroarch.cfg.\n", libretro_path);
      }
   }
}

static void get_environment_settings(void)
{
   getcwd(default_paths.port_dir, MAXPATHLEN);
   snprintf(default_paths.core_dir, sizeof(default_paths.core_dir), default_paths.port_dir);
   snprintf(default_paths.config_file, sizeof(default_paths.config_file), "%sretroarch.cfg", default_paths.port_dir);
   snprintf(default_paths.system_dir, sizeof(default_paths.system_dir), "%s/system", default_paths.core_dir);
   snprintf(default_paths.savestate_dir, sizeof(default_paths.savestate_dir), "%s/savestates", default_paths.core_dir);
   snprintf(default_paths.filesystem_root_dir, sizeof(default_paths.filesystem_root_dir), "/");
   snprintf(default_paths.filebrowser_startup_dir, sizeof(default_paths.filebrowser_startup_dir), default_paths.filesystem_root_dir);
   snprintf(default_paths.sram_dir, sizeof(default_paths.sram_dir), "%s/sram", default_paths.core_dir);
   snprintf(default_paths.input_presets_dir, sizeof(default_paths.input_presets_dir), "%s/presets/input", default_paths.core_dir);
   strlcpy(default_paths.executable_extension, ".dol", sizeof(default_paths.executable_extension));
   snprintf(default_paths.salamander_file, sizeof(default_paths.salamander_file), "boot.dol");
}

int main(int argc, char *argv[])
{
#ifdef HW_RVL
   L2Enhance();
#endif

#ifdef HAVE_LOGGER
   logger_init();
#endif

   fatInitDefault();

   get_environment_settings();

   //TODO: Add control options later - for 'set_first_file'
   {
      //normal executable loading path
      init_settings();
   }

#ifdef HAVE_LOGGER
   logger_shutdown();
#endif

   rarch_console_exec(libretro_path);

   return 1;
}