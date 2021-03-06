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

#include <android/keycodes.h>
#include <unistd.h>
#include "android_general.h"
#include "../../../performance.h"
#include "../../../general.h"
#include "../../../driver.h"

#define AKEY_EVENT_NO_ACTION 255
#define MAX_PADS 8

enum {
   AKEYCODE_ESCAPE          = 111,
   AKEYCODE_BREAK           = 121,
   AKEYCODE_F2              = 132,
   AKEYCODE_F3              = 133,
   AKEYCODE_F4              = 134,
   AKEYCODE_F5              = 135,
   AKEYCODE_F6              = 136,
   AKEYCODE_F7              = 137,
   AKEYCODE_F8              = 138,
   AKEYCODE_F9              = 139,
   AKEYCODE_BUTTON_1        = 188,
   AKEYCODE_BUTTON_2        = 189,
   AKEYCODE_BUTTON_3        = 190,
   AKEYCODE_BUTTON_4        = 191,
   AKEYCODE_BUTTON_5        = 192,
   AKEYCODE_BUTTON_6        = 193,
   AKEYCODE_BUTTON_7        = 194,
   AKEYCODE_BUTTON_8        = 195,
   AKEYCODE_BUTTON_9        = 196,
   AKEYCODE_BUTTON_10       = 197,
   AKEYCODE_BUTTON_11       = 198,
   AKEYCODE_BUTTON_12       = 199,
   AKEYCODE_BUTTON_13       = 200,
   AKEYCODE_BUTTON_14       = 201,
   AKEYCODE_BUTTON_15       = 202,
   AKEYCODE_BUTTON_16       = 203,
   AKEYCODE_ASSIST          = 219,
};

#define LAST_KEYCODE AKEYCODE_ASSIST

#define PRESSED_UP(x, y)   ((-0.80f > y) && (x >= -1.00f))
#define PRESSED_DOWN(x, y) ((0.80f  < y) && (y <= 1.00f))
#define PRESSED_LEFT(x, y) ((-0.80f > x) && (x >= -1.00f))
#define PRESSED_RIGHT(x, y) ((0.80f  < x) && (x <= 1.00f))

#define MAX_DEVICE_IDS 50

static unsigned pads_connected;
static uint64_t state[MAX_PADS];
static int8_t state_device_ids[MAX_DEVICE_IDS];
static uint64_t keycode_lut[LAST_KEYCODE];

static void setup_keycode_lut(void)
{
   /* eight 8-bit values are packed into one uint64_t
    * one for each of the 8 pads */
   uint8_t shift = 8;

   for(int j = 0; j < LAST_KEYCODE; j++)
      keycode_lut[j] = 0;

   for(int i = 0; i < MAX_PADS; i++)
   {
      /* Control scheme 1
       * fd=196
       * path='/dev/input/event4'
       * name='Logitech Logitech RumblePad 2 USB'
       * classes=0x80000141
       * configuration=''
       * keyLayout='/system/usr/keylayout/Generic.kl'
       * keyCharacterMap='/system/usr/keychars/Generic.kcm'
       * builtinKeyboard=false
       */

      /* Hack - we have to add '1' to the bit mask here because
       * RETRO_DEVICE_ID_JOYPAD_B is 0
       */

      keycode_lut[AKEYCODE_BUTTON_2] |=  ((RETRO_DEVICE_ID_JOYPAD_B+1)      << shift);
      keycode_lut[AKEYCODE_BUTTON_1] |=  ((RETRO_DEVICE_ID_JOYPAD_Y+1)      << shift);
      keycode_lut[AKEYCODE_BUTTON_9] |=  ((RETRO_DEVICE_ID_JOYPAD_SELECT+1) << shift);
      keycode_lut[AKEYCODE_BUTTON_10] |= ((RETRO_DEVICE_ID_JOYPAD_START+1)  << shift);
      keycode_lut[AKEYCODE_BUTTON_3] |=  ((RETRO_DEVICE_ID_JOYPAD_A+1)      << shift);
      keycode_lut[AKEYCODE_BUTTON_4] |=  ((RETRO_DEVICE_ID_JOYPAD_X+1)      << shift);
      keycode_lut[AKEYCODE_BUTTON_5] |=  ((RETRO_DEVICE_ID_JOYPAD_L+1)      << shift);
      keycode_lut[AKEYCODE_BUTTON_6] |=  ((RETRO_DEVICE_ID_JOYPAD_R+1)      << shift);
      keycode_lut[AKEYCODE_BUTTON_7] |=  ((RETRO_DEVICE_ID_JOYPAD_L2+1)     << shift);
      keycode_lut[AKEYCODE_BUTTON_8] |=  ((RETRO_DEVICE_ID_JOYPAD_R2+1)     << shift);
      keycode_lut[AKEYCODE_BUTTON_11] |= ((RETRO_DEVICE_ID_JOYPAD_L3+1)     << shift);
      keycode_lut[AKEYCODE_BUTTON_12] |= ((RETRO_DEVICE_ID_JOYPAD_R3+1)     << shift);

      /* Control scheme 2
       * Tested with: SNES Pad USB converter
       * fd=196
       * path='/dev/input/event4'
       * name='HuiJia  USB GamePad'
       * classes=0x80000141
       * configuration=''
       * keyLayout='/system/usr/keylayout/Generic.kl'
       * keyCharacterMap='/system/usr/keychars/Generic.kcm'
       * builtinKeyboard=false
       */

      keycode_lut[AKEYCODE_BUTTON_C] |= ((RETRO_DEVICE_ID_JOYPAD_B+1) << shift);
      keycode_lut[AKEYCODE_BUTTON_X] |= ((RETRO_DEVICE_ID_JOYPAD_Y+1) << shift);
      keycode_lut[AKEYCODE_BUTTON_L2] |= ((RETRO_DEVICE_ID_JOYPAD_SELECT+1) << shift);
      keycode_lut[AKEYCODE_BUTTON_R2] |= ((RETRO_DEVICE_ID_JOYPAD_START+1) << shift);
      keycode_lut[AKEYCODE_BUTTON_B] |= ((RETRO_DEVICE_ID_JOYPAD_A+1) << shift);
      keycode_lut[AKEYCODE_BUTTON_A] |= ((RETRO_DEVICE_ID_JOYPAD_X+1) << shift);
      keycode_lut[AKEYCODE_BUTTON_L1] |= ((RETRO_DEVICE_ID_JOYPAD_L+1) << shift);
      keycode_lut[AKEYCODE_BUTTON_R1] |= ((RETRO_DEVICE_ID_JOYPAD_R+1) << shift);

      /* Control scheme 3
       * fd=196
       * path='/dev/input/event4'
       * name='Microsoft® Microsoft® SideWinder® Game Pad USB'
       * classes=0x80000141
       * configuration=''
       * keyLayout='/system/usr/keylayout/Generic.kl'
       * keyCharacterMap='/system/usr/keychars/Generic.kcm'
       * builtinKeyboard=false
       */

      /*
         keycode_lut[AKEYCODE_BUTTON_A] = ANDROID_GAMEPAD_CROSS;
         keycode_lut[AKEYCODE_BUTTON_X] = ANDROID_GAMPAD_SQUARE:
         keycode_lut[AKEYCODE_BUTTON_R2] = ANDROID_GAMEPAD_SELECT;
         keycode_lut[AKEYCODE_BUTTON_L2] = ANDROID_GAMEPAD_START;
         keycode_lut[AKEYCODE_BUTTON_B] = ANDROID_GAMEPAD_CIRCLE;
         keycode_lut[AKEYCODE_BUTTON_Y] = ANDROID_GAMEPAD_TRIANGLE;
         keycode_lut[AKEYCODE_BUTTON_L1] = ANDROID_GAMEPAD_L1;
         keycode_lut[AKEYCODE_BUTTON_R1] = ANDROID_GAMEPAD_R1;
         keycode_lut[AKEYCODE_BUTTON_Z] = ANDROID_GAMEPAD_L2;
         keycode_lut[AKEYCODE_BUTTON_C] = ANDROID_GAMEPAD_R2;
         keycode_lut[AKEYCODE_BUTTON_11] = ANDROID_GAMEPAD_L3;
         keycode_lut[AKEYCODE_BUTTON_12] = ANDROID_GAMEPAD_R3;
         */

      /* Control scheme 4
       * Tested with: Sidewinder Dual Strike
       * fd=196
       * path='/dev/input/event4'
       * name='Microsoft SideWinder Dual Strike USB version 1.0'
       * classes=0x80000141
       * configuration=''
       * keyLayout='/system/usr/keylayout/Generic.kl'
       * keyCharacterMap='/system/usr/keychars/Generic.kcm'
       * builtinKeyboard=false
       */

      /*
         keycode_lut[AKEYCODE_BUTTON_4] = ANDROID_GAMEPAD_CROSS;
         keycode_lut[AKEYCODE_BUTTON_2] = ANDROID_GAMPAD_SQUARE:
         keycode_lut[AKEYCODE_BUTTON_6] = ANDROID_GAMEPAD_SELECT;
         keycode_lut[AKEYCODE_BUTTON_5] = ANDROID_GAMEPAD_START;
         keycode_lut[AKEYCODE_BUTTON_3] = ANDROID_GAMEPAD_CIRCLE;
         keycode_lut[AKEYCODE_BUTTON_1] = ANDROID_GAMEPAD_TRIANGLE;
         keycode_lut[AKEYCODE_BUTTON_7] = ANDROID_GAMEPAD_L1;
         keycode_lut[AKEYCODE_BUTTON_8] = ANDROID_GAMEPAD_R1;
         keycode_lut[AKEYCODE_BUTTON_9] = ANDROID_GAMEPAD_L2;
         */

      /* Control scheme 5
       * fd=196
       * path='/dev/input/event4'
       * name='WiseGroup.,Ltd MP-8866 Dual USB Joypad'
       * classes=0x80000141
       * configuration=''
       * keyLayout='/system/usr/keylayout/Generic.kl'
       * keyCharacterMap='/system/usr/keychars/Generic.kcm'
       * builtinKeyboard=false
       */

      /*
         keycode_lut[AKEYCODE_BUTTON_3] = ANDROID_GAMEPAD_CROSS;
         keycode_lut[AKEYCODE_BUTTON_4] = ANDROID_GAMPAD_SQUARE:
         keycode_lut[AKEYCODE_BUTTON_10] = ANDROID_GAMEPAD_SELECT;
         keycode_lut[AKEYCODE_BUTTON_9] = ANDROID_GAMEPAD_START;
         keycode_lut[AKEYCODE_BUTTON_2] = ANDROID_GAMEPAD_CIRCLE;
         keycode_lut[AKEYCODE_BUTTON_1] = ANDROID_GAMEPAD_TRIANGLE;
         keycode_lut[AKEYCODE_BUTTON_7] = ANDROID_GAMEPAD_L1;
         keycode_lut[AKEYCODE_BUTTON_8] = ANDROID_GAMEPAD_R1;
         keycode_lut[AKEYCODE_BUTTON_5] = ANDROID_GAMEPAD_L2;
         keycode_lut[AKEYCODE_BUTTON_6] = ANDROID_GAMEPAD_R2;
         keycode_lut[AKEYCODE_BUTTON_11] = ANDROID_GAMEPAD_L3;
         keycode_lut[AKEYCODE_BUTTON_12] = ANDROID_GAMEPAD_R3;
         */

      /* Control scheme 6
       * Keyboard
       * TODO: Map L2/R2/L3/R3
       * */

      keycode_lut[AKEYCODE_Z] |= ((RETRO_DEVICE_ID_JOYPAD_B+1) << shift);
      keycode_lut[AKEYCODE_A] |= ((RETRO_DEVICE_ID_JOYPAD_Y+1) << shift);
      keycode_lut[AKEYCODE_SHIFT_RIGHT] |= ((RETRO_DEVICE_ID_JOYPAD_SELECT+1) << shift);
      keycode_lut[AKEYCODE_ENTER] |= ((RETRO_DEVICE_ID_JOYPAD_START+1) << shift);
      keycode_lut[AKEYCODE_DPAD_UP] |= ((RETRO_DEVICE_ID_JOYPAD_UP+1) << shift);
      keycode_lut[AKEYCODE_DPAD_DOWN] |= ((RETRO_DEVICE_ID_JOYPAD_DOWN+1) << shift);
      keycode_lut[AKEYCODE_DPAD_LEFT] |= ((RETRO_DEVICE_ID_JOYPAD_LEFT+1) << shift);
      keycode_lut[AKEYCODE_DPAD_RIGHT] |= ((RETRO_DEVICE_ID_JOYPAD_RIGHT+1) << shift);
      keycode_lut[AKEYCODE_X] |= ((RETRO_DEVICE_ID_JOYPAD_A+1) << shift);
      keycode_lut[AKEYCODE_S] |= ((RETRO_DEVICE_ID_JOYPAD_X+1) << shift);
      keycode_lut[AKEYCODE_Q] |= ((RETRO_DEVICE_ID_JOYPAD_L+1) << shift);
      keycode_lut[AKEYCODE_W] |= ((RETRO_DEVICE_ID_JOYPAD_R+1) << shift);

      /* Misc control scheme */
      keycode_lut[AKEYCODE_BACK] |= ((RARCH_QUIT_KEY+1) << shift);
      keycode_lut[AKEYCODE_F2] |= ((RARCH_SAVE_STATE_KEY+1) << shift);
      keycode_lut[AKEYCODE_F4] |= ((RARCH_LOAD_STATE_KEY+1) << shift);
      keycode_lut[AKEYCODE_F7] |= ((RARCH_STATE_SLOT_PLUS+1) << shift);
      keycode_lut[AKEYCODE_F6] |= ((RARCH_STATE_SLOT_MINUS+1) << shift);
      keycode_lut[AKEYCODE_SPACE] |= ((RARCH_FAST_FORWARD_KEY+1) << shift);
      keycode_lut[AKEYCODE_L] |= ((RARCH_FAST_FORWARD_HOLD_KEY+1) << shift);
      keycode_lut[AKEYCODE_ESCAPE] |= ((RARCH_QUIT_KEY+1) << shift);
      keycode_lut[AKEYCODE_BREAK] |= ((RARCH_PAUSE_TOGGLE+1) << shift);
      keycode_lut[AKEYCODE_K] |= ((RARCH_FRAMEADVANCE+1) << shift);
      keycode_lut[AKEYCODE_H] |= ((RARCH_RESET+1) << shift);
      keycode_lut[AKEYCODE_R] |= ((RARCH_REWIND+1) << shift);
      keycode_lut[AKEYCODE_F9] |= ((RARCH_MUTE+1) << shift);
      shift += 8;
   }
}

static void *android_input_init(void)
{
   pads_connected = 0;

   for(unsigned player = 0; player < 4; player++)
      for(unsigned i = 0; i < RARCH_FIRST_META_KEY; i++)
      {
         g_settings.input.binds[player][i].id = i;
         g_settings.input.binds[player][i].joykey = 0;
      }

   for(int player = 0; player < 4; player++)
   {
      g_settings.input.binds[player][RETRO_DEVICE_ID_JOYPAD_B].joykey = (1ULL << RETRO_DEVICE_ID_JOYPAD_B);
      g_settings.input.binds[player][RETRO_DEVICE_ID_JOYPAD_Y].joykey = (1ULL << RETRO_DEVICE_ID_JOYPAD_Y);
      g_settings.input.binds[player][RETRO_DEVICE_ID_JOYPAD_SELECT].joykey = (1ULL << RETRO_DEVICE_ID_JOYPAD_SELECT);
      g_settings.input.binds[player][RETRO_DEVICE_ID_JOYPAD_START].joykey = (1ULL << RETRO_DEVICE_ID_JOYPAD_START);
      g_settings.input.binds[player][RETRO_DEVICE_ID_JOYPAD_UP].joykey = (1ULL << RETRO_DEVICE_ID_JOYPAD_UP);
      g_settings.input.binds[player][RETRO_DEVICE_ID_JOYPAD_DOWN].joykey = (1ULL << RETRO_DEVICE_ID_JOYPAD_DOWN);
      g_settings.input.binds[player][RETRO_DEVICE_ID_JOYPAD_LEFT].joykey = (1ULL << RETRO_DEVICE_ID_JOYPAD_LEFT);
      g_settings.input.binds[player][RETRO_DEVICE_ID_JOYPAD_RIGHT].joykey = (1ULL << RETRO_DEVICE_ID_JOYPAD_RIGHT);
      g_settings.input.binds[player][RETRO_DEVICE_ID_JOYPAD_A].joykey = (1ULL << RETRO_DEVICE_ID_JOYPAD_A);
      g_settings.input.binds[player][RETRO_DEVICE_ID_JOYPAD_X].joykey = (1ULL << RETRO_DEVICE_ID_JOYPAD_X);
      g_settings.input.binds[player][RETRO_DEVICE_ID_JOYPAD_L].joykey = (1ULL << RETRO_DEVICE_ID_JOYPAD_L);
      g_settings.input.binds[player][RETRO_DEVICE_ID_JOYPAD_R].joykey = (1ULL << RETRO_DEVICE_ID_JOYPAD_R);
      g_settings.input.binds[player][RETRO_DEVICE_ID_JOYPAD_L2].joykey = (1ULL << RETRO_DEVICE_ID_JOYPAD_L2);
      g_settings.input.binds[player][RETRO_DEVICE_ID_JOYPAD_R2].joykey = (1ULL << RETRO_DEVICE_ID_JOYPAD_R2);
      g_settings.input.binds[player][RETRO_DEVICE_ID_JOYPAD_L3].joykey = (1ULL << RETRO_DEVICE_ID_JOYPAD_L3);
      g_settings.input.binds[player][RETRO_DEVICE_ID_JOYPAD_R3].joykey = (1ULL << RETRO_DEVICE_ID_JOYPAD_R3);
   }

   for(int i = 0; i < MAX_DEVICE_IDS; i++)
      state_device_ids[i] = -1;

   setup_keycode_lut();

   return (void*)-1;
}

static void android_input_poll(void *data)
{
   (void)data;

   RARCH_PERFORMANCE_INIT(input_poll);
   RARCH_PERFORMANCE_START(input_poll);

   struct android_app* android_app = g_android.app;

   g_extern.lifecycle_state &= ~((1ULL << RARCH_RESET) | (1ULL << RARCH_REWIND) | (1ULL << RARCH_FAST_FORWARD_KEY) | (1ULL << RARCH_FAST_FORWARD_HOLD_KEY) | (1ULL << RARCH_MUTE) | (1ULL << RARCH_SAVE_STATE_KEY) | (1ULL << RARCH_LOAD_STATE_KEY) | (1ULL << RARCH_STATE_SLOT_PLUS) | (1ULL << RARCH_STATE_SLOT_MINUS));

   // Read all pending events.
   while(AInputQueue_hasEvents(android_app->inputQueue))
   {
      AInputEvent* event = NULL;
      AInputQueue_getEvent(android_app->inputQueue, &event);

      if (AInputQueue_preDispatchEvent(android_app->inputQueue, event))
         continue;

      int32_t handled = 1;

      int id = AInputEvent_getDeviceId(event);
      int type = AInputEvent_getType(event);
      int state_id = state_device_ids[id];

      if(state_id == -1)
         state_id = state_device_ids[id] = pads_connected++;

      int motion_action = AMotionEvent_getAction(event);
      bool motion_do = ((motion_action == AMOTION_EVENT_ACTION_DOWN) || (motion_action ==
               AMOTION_EVENT_ACTION_POINTER_DOWN) || (motion_action == AMOTION_EVENT_ACTION_MOVE));

      if(type == AINPUT_EVENT_TYPE_MOTION && motion_do)
      {
         float x = AMotionEvent_getX(event, 0);
         float y = AMotionEvent_getY(event, 0);
#ifdef RARCH_INPUT_DEBUG
         char msg[128];
         snprintf(msg, sizeof(msg), "RetroPad %d : x = %f, y = %f.\n", i, x, y);
         msg_queue_push(g_extern.msg_queue, msg, 0, 30);
#endif
         state[state_id] &= ~((1ULL << RETRO_DEVICE_ID_JOYPAD_LEFT) | (1ULL << RETRO_DEVICE_ID_JOYPAD_RIGHT) |
               (1ULL << RETRO_DEVICE_ID_JOYPAD_UP) | (1ULL << RETRO_DEVICE_ID_JOYPAD_DOWN));
         state[state_id] |= PRESSED_LEFT(x, y)  ? (1ULL << RETRO_DEVICE_ID_JOYPAD_LEFT)  : 0;
         state[state_id] |= PRESSED_RIGHT(x, y) ? (1ULL << RETRO_DEVICE_ID_JOYPAD_RIGHT) : 0;
         state[state_id] |= PRESSED_UP(x, y)    ? (1ULL << RETRO_DEVICE_ID_JOYPAD_UP)    : 0;
         state[state_id] |= PRESSED_DOWN(x, y)  ? (1ULL << RETRO_DEVICE_ID_JOYPAD_DOWN)  : 0;
      }
      else
      {
         int keycode = AKeyEvent_getKeyCode(event);

         /* Hack - we have to decrease the unpacked value by 1
          * because we 'added' 1 to each entry in the LUT -
          * RETRO_DEVICE_ID_JOYPAD_B is 0
          */
         uint8_t unpacked = (keycode_lut[keycode] >> ((state_id+1) << 3)) - 1;
         uint64_t input_state = (1ULL << unpacked);
#ifdef RARCH_INPUT_DEBUG
         char msg[128];
         snprintf(msg, sizeof(msg), "Keycode RetroPad %d : %d.\n", i, keycode);
         msg_queue_push(g_extern.msg_queue, msg, 0, 30);
#endif
         int action  = AKeyEvent_getAction(event);
         uint64_t *key = NULL;

         if(input_state < (1ULL << RARCH_FIRST_META_KEY))
            key = &state[state_id];
         else if(input_state)
            key = &g_extern.lifecycle_state;

         if(key != NULL)
         {
            if(action == AKEY_EVENT_ACTION_DOWN)
               *key |= input_state;
            else if(action == AKEY_EVENT_ACTION_UP)
               *key &= ~(input_state);
         }

         if(keycode == AKEYCODE_VOLUME_UP || keycode == AKEYCODE_VOLUME_DOWN)
            handled = 0;
      }
      AInputQueue_finishEvent(android_app->inputQueue, event, handled);
   }

   RARCH_PERFORMANCE_STOP(input_poll);
}

static int16_t android_input_state(void *data, const struct retro_keybind **binds, unsigned port, unsigned device, unsigned index, unsigned id)
{
   switch (device)
   {
      case RETRO_DEVICE_JOYPAD:
         return ((state[port] & binds[port][id].joykey) && (port < pads_connected));
      default:
         return 0;
   }
}

static bool android_input_key_pressed(void *data, int key)
{
   (void)data;

   if(g_extern.lifecycle_state & (1ULL << key))
      return true;

   return false;
}

static void android_input_free_input(void *data)
{
   (void)data;
}

const input_driver_t input_android = {
   android_input_init,
   android_input_poll,
   android_input_state,
   android_input_key_pressed,
   android_input_free_input,
   "android_input",
};

