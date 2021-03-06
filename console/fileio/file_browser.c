/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2012 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2012 - Daniel De Matteis
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

#include "file_browser.h"

static bool filebrowser_parse_directory(filebrowser_t *filebrowser, unsigned stack_size, 
const char *path, const char * extensions)
{
   struct string_list *list = dir_list_new(path, extensions, true);

   if(list != NULL)
   {
      strlcpy(filebrowser->directory_path, path, sizeof(filebrowser->directory_path));

      if(filebrowser->current_dir.list != NULL)
         dir_list_free(filebrowser->current_dir.list);

      filebrowser->current_dir.list = list;
      filebrowser->current_dir.ptr   = 0;
      filebrowser->directory_stack_size = stack_size;
      strlcpy(filebrowser->extensions, extensions, sizeof(filebrowser->extensions));

      dir_list_sort(filebrowser->current_dir.list, true);
      
      return true;
   }
   else
      return false;
}

static bool filebrowser_new(filebrowser_t *filebrowser, const char *start_dir, 
const char *extensions)
{
   bool ret = filebrowser_parse_directory(filebrowser, 0, start_dir, extensions);

   return ret;
}

void filebrowser_set_root(filebrowser_t *filebrowser, const char *root_dir)
{
   strlcpy(filebrowser->root_dir, root_dir, sizeof(filebrowser->root_dir));
}

void filebrowser_set_root_and_ext(filebrowser_t *browser, const char *ext, const char *root_dir)
{
   strlcpy(browser->extensions, ext, sizeof(browser->extensions));
   filebrowser_set_root(browser, root_dir);
   filebrowser_iterate(browser, FILEBROWSER_ACTION_RESET);
}

void filebrowser_free(filebrowser_t * filebrowser)
{
   dir_list_free(filebrowser->current_dir.list);

   filebrowser->current_dir.list = NULL;
   filebrowser->current_dir.ptr   = 0;
}

static bool filebrowser_push_directory(filebrowser_t * filebrowser, const char * path,
bool with_extension)
{
   bool ret = true;
   char extensions[256];
   unsigned push_dir = filebrowser->directory_stack_size + 1;

   if(with_extension)
      snprintf(extensions, sizeof(extensions), filebrowser->extensions);
   else
      snprintf(extensions, sizeof(extensions), "empty");

   ret = filebrowser_parse_directory(filebrowser, push_dir, path, extensions);

   return ret;
}

static bool filebrowser_pop_directory (filebrowser_t * filebrowser)
{
   bool ret = true;
   char previous_dir[PATH_MAX], directory_path_tmp[PATH_MAX];
   unsigned pop_dir = filebrowser->directory_stack_size;

   if (filebrowser->directory_stack_size > 0)
      pop_dir -= 1;

   fill_pathname_parent_dir(previous_dir, filebrowser->directory_path, sizeof(previous_dir));
   strlcpy(directory_path_tmp, filebrowser->directory_path, sizeof(directory_path_tmp));

   //test first if previous directory can be accessed
   ret = filebrowser_parse_directory(filebrowser, pop_dir, previous_dir,
   filebrowser->extensions);

   if(!ret)
   {
      //revert to previous directory
      strlcpy(filebrowser->directory_path, directory_path_tmp, sizeof(filebrowser->directory_path));
      ret = filebrowser_parse_directory(filebrowser, pop_dir, filebrowser->directory_path,
      filebrowser->extensions);
   }

   return ret;
}

const char *filebrowser_get_current_dir (filebrowser_t *filebrowser)
{
   return filebrowser->directory_path;
}

const char *filebrowser_get_current_path (filebrowser_t *filebrowser)
{
   return filebrowser->current_dir.list->elems[filebrowser->current_dir.ptr].data;
}

bool filebrowser_get_current_path_isdir (filebrowser_t *filebrowser)
{
   return filebrowser->current_dir.list->elems[filebrowser->current_dir.ptr].attr.b;
}

size_t filebrowser_get_current_index (filebrowser_t *filebrowser)
{
   return filebrowser->current_dir.ptr;
}

void filebrowser_set_current_at (filebrowser_t *filebrowser, size_t pos)
{
   filebrowser->current_dir.ptr = pos;
}

static void filebrowser_set_current_increment (filebrowser_t *filebrowser)
{
   filebrowser->current_dir.ptr++;
   if (filebrowser->current_dir.ptr >= filebrowser->current_dir.list->size)
      filebrowser->current_dir.ptr = 0;
}

static void filebrowser_set_current_decrement (filebrowser_t *filebrowser)
{
   filebrowser->current_dir.ptr--;
   if (filebrowser->current_dir.ptr >= filebrowser->current_dir.list->size)
      filebrowser->current_dir.ptr = filebrowser->current_dir.list->size - 1;
}

bool filebrowser_iterate(filebrowser_t *filebrowser, filebrowser_action_t action)
{
   bool ret = true;
   unsigned entries_to_scroll = 19;

   switch(action)
   {
      case FILEBROWSER_ACTION_UP:
         filebrowser_set_current_decrement(filebrowser);
         break;
      case FILEBROWSER_ACTION_DOWN:
         filebrowser_set_current_increment(filebrowser);
         break;
      case FILEBROWSER_ACTION_LEFT:
         if (filebrowser->current_dir.ptr <= 5)
            filebrowser->current_dir.ptr = 0;
         else
            filebrowser->current_dir.ptr -= 5;
         break;
      case FILEBROWSER_ACTION_RIGHT:
         filebrowser->current_dir.ptr = (min(filebrowser->current_dir.ptr + 5, 
         filebrowser->current_dir.list->size-1));
         break;
      case FILEBROWSER_ACTION_SCROLL_UP:
         if (filebrowser->current_dir.ptr <= entries_to_scroll)
            filebrowser->current_dir.ptr= 0;
         else
            filebrowser->current_dir.ptr -= entries_to_scroll;
         break;
      case FILEBROWSER_ACTION_SCROLL_DOWN:
         filebrowser->current_dir.ptr = (min(filebrowser->current_dir.ptr + 
         entries_to_scroll, filebrowser->current_dir.list->size-1));
         break;
      case FILEBROWSER_ACTION_OK:
         ret = filebrowser_push_directory(filebrowser, filebrowser_get_current_path(filebrowser), true);
         break;
      case FILEBROWSER_ACTION_CANCEL:
         ret = filebrowser_pop_directory(filebrowser);
         break;
      case FILEBROWSER_ACTION_RESET:
         ret = filebrowser_new(filebrowser, filebrowser->root_dir, filebrowser->extensions);
         break;
      case FILEBROWSER_ACTION_NOOP:
      default:
         break;
   }

   return ret;
}
