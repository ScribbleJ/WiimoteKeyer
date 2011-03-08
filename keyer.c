/*
 * Wiimote - Chording Keyboard Software
 * (c)2009, Christopher "ScribbleJ" Jansen
 *
 * Takes input from a wiimote and converts it to system input in a chording scheme.
 *
 */

#include "uinput.h"
#include <cwiid.h>
#include <bluetooth/bluetooth.h>
#include <linux/input.h>
#include <math.h>
#define PI  3.14159265358979323

// Mapping of keys-to-bits, LSB/key first, MSB/key last
int keyer_to_cwiid[8] = {
  CWIID_BTN_B,
  CWIID_BTN_DOWN,
  CWIID_BTN_UP,
  CWIID_BTN_RIGHT,
  CWIID_BTN_2,
  CWIID_BTN_LEFT,
  CWIID_BTN_1,
  CWIID_BTN_A};

struct key_info {
  int chord;
  int autoshift;
  int autohold;
  int keysym;};

static struct key_info keyer_mouse_lut_values[] = {
  {0x08, 0, 0, BTN_LEFT},
  {0x04, 0, 0, BTN_MIDDLE},
  {0x02, 0, 0, BTN_RIGHT},
  {0x20, 1, 1, REL_WHEEL},
  {0x80, 1, 0, REL_WHEEL},
  {0x00, 0, 0, 0},
};

static struct key_info keyer_lut_values[] = {
  //THUMB 0
  {0x08, 0, 0, KEY_E},
  {0x04, 0, 0, KEY_T},
  {0x02, 0, 0, KEY_A},
  {0x01, 0, 0, KEY_O},
  {0x0C, 0, 0, KEY_I},
  {0x06, 0, 0, KEY_S},
  {0x03, 0, 0, KEY_N},
  {0x0F, 0, 0, KEY_R},
  {0x0E, 0, 0, KEY_H},
  {0x07, 0, 0, KEY_L},
  {0x0D, 0, 0, KEY_D},
  {0x0B, 0, 0, KEY_U},
  {0x0A, 0, 0, KEY_C},
  {0x09, 0, 0, KEY_M},
  {0x05, 0, 0, KEY_Y},
  //THUMB 1
  {0x10, 0, 0, KEY_SPACE},
  {0x18, 0, 0, KEY_P},
  {0x14, 0, 0, KEY_G},
  {0x12, 0, 0, KEY_F},
  {0x11, 0, 0, KEY_W},
  {0x1C, 0, 0, KEY_B},
  {0x16, 0, 0, KEY_K},
  {0x13, 0, 0, KEY_V},
  {0x1F, 0, 0, KEY_X},
  {0x1E, 0, 0, KEY_Q},
  {0x17, 0, 0, KEY_J},
  {0x1D, 0, 0, KEY_Z},
  //0x1B, KEY_},
  //0x1A, KEY_},
  //0x19, KEY_},
  //0x15, KEY_},
  //THUMB 2
  {0x20, 0, 0, KEY_ENTER},
  {0x28, 0, 0, KEY_DOT},
  {0x24, 0, 0, KEY_APOSTROPHE},
  {0x22, 0, 0, KEY_COMMA},
  {0x21, 0, 0, KEY_MINUS},
  {0x2C, 1, 0, KEY_DOT},
  {0x26, 0, 0, KEY_LEFTBRACE},
  {0x23, 0, 0, KEY_RIGHTBRACE},
  {0x2F, 1, 0, KEY_MINUS},
  {0x2E, 0, 0, KEY_SEMICOLON},
  {0x27, 1, 0, KEY_SLASH},
  {0x2D, 0, 0, KEY_BACKSLASH},
  {0x2B, 1, 0, KEY_APOSTROPHE},
  {0x2A, 0, 0, KEY_EQUAL},
  {0x29, 1, 0, KEY_SEMICOLON},
  {0x25, 0, 0, KEY_SLASH},
  //THUMB 3
  {0x40, 0, 0, KEY_0},
  {0x48, 0, 0, KEY_8},
  {0x44, 0, 0, KEY_4},
  {0x42, 0, 0, KEY_2},
  {0x41, 0, 0, KEY_1},
  {0x4C, 1, 0, KEY_RIGHTBRACE},
  {0x46, 0, 0, KEY_6},
  {0x43, 0, 0, KEY_3},
  {0x4F, 1, 0, KEY_LEFTBRACE},
  {0x4E, 1, 0, KEY_EQUAL},
  {0x47, 0, 0, KEY_7},
  {0x4D, 1, 0, KEY_GRAVE},
  {0x4B, 1, 0, KEY_COMMA},
  {0x4A, 1, 0, KEY_BACKSLASH},
  {0x49, 0, 0, KEY_9},
  {0x45, 0, 0, KEY_5},
  //THUMB 4
  {0x80, 0, 1, KEY_LEFTSHIFT},
  {0x88, 0, 0, KEY_BACKSPACE},
  {0x84, 0, 0, KEY_DELETE},
  {0x82, 0, 0, KEY_TAB},
  {0x81, 0, 1, KEY_LEFTCTRL},
  {0x8C, 0, 1, KEY_LEFTALT},
  {0x86, 0, 0, KEY_HOME},
  {0x83, 0, 0, KEY_END},
  {0x8F, 0, 0, KEY_ESC},
  {0x8E, 0, 0, KEY_PAGEUP},
  {0x87, 0, 0, KEY_PAGEDOWN},
  {0x8D, 0, 0, KEY_INSERT},
  //0x8B, KEY_},
  //0x8A, KEY_},
  //0x89, KEY_},
  //0x85, KEY_},
  {0x00, 0, 0, 0x00}
};

#define MODE_DEFAULT 0x01
#define MODE_MOUSE   0x02

static struct key_info *keyer_lut[256];
static struct key_info *heldkeys[10];
static cwiid_wiimote_t *wiimote;
#define NEW_AMOUNT 0.1
#define OLD_AMOUNT (1.0-NEW_AMOUNT)
static double a_x = 0, a_y = 0, a_z = 0;
static float X_Scale = 1.0;
static float Y_Scale = 1.0;
static struct acc_cal acc_cal;
static KEYER_MODE = MODE_DEFAULT;

static void init_lut()
{
  struct key_info *r = keyer_lut_values;
  while(r->chord != 0)
  {
    keyer_lut[r->chord] = r;
    r++;
  }
}

static int start_accel()
{
  if (cwiid_get_acc_cal(wiimote, CWIID_EXT_NONE, &acc_cal)) {
      fprintf(stderr, "Error calibrating accelerometer.\n");
      return 0;
  }
  if(cwiid_set_rpt_mode(wiimote, CWIID_RPT_BTN | CWIID_RPT_STATUS | CWIID_RPT_ACC))
  {
    fprintf(stderr, "Error requesting command mode.\n");
    return 0;
  }
  if(cwiid_command(wiimote, CWIID_CMD_LED, CWIID_LED2_ON))
  {
    fprintf(stderr, "Error changing LED.\n");
    return 0;
  }
  return 1;
}

static int stop_accel()
{
  if(cwiid_set_rpt_mode(wiimote, CWIID_RPT_BTN | CWIID_RPT_STATUS))
  {
    fprintf(stderr, "Error requesting command mode.\n");
    return 0;
  }
  if(cwiid_command(wiimote, CWIID_CMD_LED, CWIID_LED1_ON))
  {
    fprintf(stderr, "Error changing LED.\n");
    return 0;
  }
  return 1;
}



// Copped from wminput acc.c plugin
static void process_acc(struct cwiid_acc_mesg *mesg)
{
  double a;
  double roll, pitch;

  a_x = (((double)mesg->acc[CWIID_X] - acc_cal.zero[CWIID_X]) /
        (acc_cal.one[CWIID_X] - acc_cal.zero[CWIID_X]))*NEW_AMOUNT +
        a_x*OLD_AMOUNT;
  a_y = (((double)mesg->acc[CWIID_Y] - acc_cal.zero[CWIID_Y]) /
        (acc_cal.one[CWIID_Y] - acc_cal.zero[CWIID_Y]))*NEW_AMOUNT +
        a_y*OLD_AMOUNT;
  a_z = (((double)mesg->acc[CWIID_Z] - acc_cal.zero[CWIID_Z]) /
        (acc_cal.one[CWIID_Z] - acc_cal.zero[CWIID_Z]))*NEW_AMOUNT +
        a_z*OLD_AMOUNT;

  a = sqrt(pow(a_x,2)+pow(a_y,2)+pow(a_z,2));
  roll = atan(a_x/a_z);
  if (a_z <= 0.0) {
    roll += PI * ((a_x > 0.0) ? 1 : -1);
  }

  pitch = atan(a_y/a_z*cos(roll));

  //data.axes[0].value = roll  * 1000 * Roll_Scale;
  //data.axes[1].value = pitch * 1000 * Pitch_Scale;

  if ((a > 0.85) && (a < 1.15)) {
    if ((fabs(roll)*(180/PI) > 10) && (fabs(pitch)*(180/PI) < 80)) {
      uin_mousemove(REL_X, roll * 5 * X_Scale);
    }
    if (fabs(pitch)*(180/PI) > 10) {
      uin_mousemove(REL_Y, pitch * 10 * Y_Scale);
    }
  }
}


static int disconnect_wiimote()
{
  if(cwiid_disconnect(wiimote))
  {
    fprintf(stderr, "Error on wiimote disconnect.\n");
    return 0;
  }
  return 1;
}

static int connect_to_wiimote(char *wiimoteaddr)
{
  bdaddr_t addr;

  if(wiimote != NULL)
  {
    fprintf(stderr, "Error: already connected.\n");
    return 0;
  }

  str2ba(wiimoteaddr, &addr);
  wiimote = cwiid_connect(&addr, CWIID_FLAG_MESG_IFC);
  if(wiimote == NULL)
  {
    fprintf(stderr, "Error connecting to wiimote.\n");
    return 0;
  }

  cwiid_set_mesg_callback(wiimote, NULL);

  if(cwiid_enable(wiimote, CWIID_FLAG_MESG_IFC))
  {
    fprintf(stderr, "Error enable mesg.\n");
    disconnect_wiimote();
    return 0;
  }

  // Also tell it to report button pushes.
  if(cwiid_command(wiimote, CWIID_CMD_RPT_MODE, CWIID_RPT_BTN | CWIID_RPT_STATUS))
  {
    fprintf(stderr, "Error requesting command mode.\n");
    disconnect_wiimote();
    return 0;
  }

  return 1;
}

 
int main(int argc, char *argv[])
{
  if(argc < 2) 
  {
    fprintf(stderr, "Error, address required on commandline.\n");
    return -1;
  }

  if(!connect_to_wiimote(argv[1]))
  {
    return -1;
  }

  if(cwiid_command(wiimote, CWIID_CMD_LED, CWIID_LED1_ON))
  {
    fprintf(stderr, "Error changing LED.\n");
    disconnect_wiimote();
    return 0;
  }

  init_lut();
  if(!uin_init())
  {
    fprintf(stderr, "Cannot init uinput.\n");
    disconnect_wiimote();
    return 0;
  }


  int mcount=0;
  union cwiid_mesg *mesg;
  struct timespec t;
  int c = 0;
  int stop = 0;
  uint16_t prev = 0;
  uint16_t cur  = 0;
  int inchord = 0;
  int got_button = 0;
  while(!cwiid_get_mesg(wiimote, &mcount, &mesg, &t) && !stop)
  {
    got_button = 0;
    for(c=0;c<mcount;c++)
    {
      //printf("Got message %d of %d.\n",c+1,mcount);
      if(mesg[c].type == CWIID_MESG_BTN)
      {
        if(mesg[c].btn_mesg.buttons & CWIID_BTN_HOME)
        {
          printf("Destroying keyer device...\n");
          stop = 1;
        }
        if(mesg[c].btn_mesg.buttons & CWIID_BTN_MINUS)
        {
          if(KEYER_MODE != MODE_MOUSE)
          {
            start_accel();
            KEYER_MODE = MODE_MOUSE;
          }
          else
          {
            stop_accel();
            KEYER_MODE = MODE_DEFAULT;
          }
        }

        prev = cur;
        cur = 0;
 
        int k=0;
        for(k = 0; k < 8; k++)
        {
          int finger = 1 << k;
          if(mesg[c].btn_mesg.buttons & keyer_to_cwiid[k])
          {
            got_button = 1;
            // printf("Finger applied: %d\n", k);
            cur |= finger;
          }
          else if(prev & finger)
          {
            got_button = 1;
          }
        }

      }
      else if(mesg[c].type == CWIID_MESG_ACC)
      {
        process_acc(&mesg[c].acc_mesg);
      }

    }

    if(!got_button)
    {
      continue;
    }

    //printf("Prev: %d, Cur: %d, Inchord: %d\n", prev, cur, inchord);
    if(KEYER_MODE == MODE_MOUSE)
    {
      struct key_info *t = &keyer_mouse_lut_values[0];
      int foo = 1;
      while(t->chord != 0)
      {
        int pressednow = cur & t->chord;
        int pressedthen = prev & t->chord;
        if(t->autoshift)
        {
          //printf("Wheel on %d.\n", foo);
          if(pressednow)
          {
            //printf("Wheel move\n");
            if(t->autohold)
            {
              uin_mousemove(REL_WHEEL, 5);
            }
            else
              uin_mousemove(REL_WHEEL, -5);
          }
        }
        else if(pressednow && !pressedthen)
        {
          //printf("Pressing mouse button %d\n", foo);
          uin_keydown(t->keysym);
        }
        else if(pressedthen && !pressednow)
        {
          //printf("Releasing mouse button %d\n", foo);
          uin_keyup(t->keysym);
        }
        t++;
        foo++;
      }
      // printf("Keyer mouse mode.\n");
    }
    else 
    {
      if(prev > cur && inchord && keyer_lut[prev] != NULL) // Button released
      {
        //printf("Chord: %d \n", prev);
        if(keyer_lut[prev]->autoshift)
        {
          uin_keydown(KEY_LEFTSHIFT);
        }

        uin_keydown(keyer_lut[prev]->keysym);

        if(keyer_lut[prev]->autohold)
        {
          int i=0;
          for(i=0; i<10; i++)
          {
            if(heldkeys[i] == NULL)
            {
              heldkeys[i] = keyer_lut[prev];
            }
          }
        }
        else
        {
          uin_keyup(keyer_lut[prev]->keysym);
          int i=0;
          for(i=0; i<10; i++)
          {
            if(heldkeys[i] != NULL)
            {
              uin_keyup(heldkeys[i]->keysym);
              heldkeys[i] = NULL;
            }
          }
        }

        if(keyer_lut[prev]->autoshift)
        {
          uin_keyup(KEY_LEFTSHIFT);
        }

        inchord = 0;
      }
      else if(prev < cur) // Button pressed
      {
        //printf("Button push.\n");
        inchord = 1;
      }
      else if(prev == cur)
      {
        //printf("Message we don't care about.\n");
      }
      else
      {
        //printf("Message, button release.\n");
      }
      
    }

   
  }

  disconnect_wiimote();

  return -1;

}
