/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2012 - Hans-Kristian Arntzen
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gfx_context.h"

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

static const gfx_ctx_driver_t *gfx_ctx_drivers[] = {
#if defined(__CELLOS_LV2__)
   &gfx_ctx_ps3,
#endif
#if defined(HAVE_VIDEOCORE)
   &gfx_ctx_videocore,
#endif
#if defined(HAVE_SDL) && defined(HAVE_OPENGL)
   &gfx_ctx_sdl_gl,
#endif
#if defined(HAVE_XVIDEO) && defined(HAVE_EGL)
   &gfx_ctx_x_egl,
#endif
#if defined(HAVE_KMS)
   &gfx_ctx_drm_egl,
#endif
};

const gfx_ctx_driver_t *gfx_ctx_find_driver(const char *ident)
{
   for (unsigned i = 0; i < sizeof(gfx_ctx_drivers) / sizeof(gfx_ctx_drivers[0]); i++)
   {
      if (strcmp(gfx_ctx_drivers[i]->ident, ident) == 0)
         return gfx_ctx_drivers[i];
   }

   return NULL;
}

const gfx_ctx_driver_t *gfx_ctx_init_first(enum gfx_ctx_api api)
{
   for (unsigned i = 0; i < sizeof(gfx_ctx_drivers) / sizeof(gfx_ctx_drivers[0]); i++)
   {
      if (gfx_ctx_drivers[i]->bind_api(api))
      {
         if (gfx_ctx_drivers[i]->init())
            return gfx_ctx_drivers[i];
      }
   }

   return NULL;
}