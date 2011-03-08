/*
 * Linux uinput hooks - allows you to emulate an input device.
 * (c)2009, Christopher "ScribbleJ" Jansen
 *
 */
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include <linux/input.h>
#include <linux/uinput.h>

#include "uinput.h"

char *ufn = "/dev/input/uinput";
int ufd;
struct uinput_user_dev uin_dev;

int uin_init()
{
  if((ufd = open(ufn, O_WRONLY)) < 0)
  {
    fprintf(stderr, "Error opening device %s: %s\n", ufn, strerror(errno));
    return 0;
  }

  if(ioctl(ufd, UI_SET_EVBIT, EV_KEY) < 0)
  {
    fprintf(stderr, "Error on UI_SET_EVBIT: %s\n", strerror(errno));
    close(ufd);
    return 0;
  }

  if(ioctl(ufd, UI_SET_EVBIT, EV_REP) < 0)
  {
    fprintf(stderr, "Error on UI_SET_EVBIT: %s\n", strerror(errno));
    close(ufd);
    return 0;
  }

  if(ioctl(ufd, UI_SET_EVBIT, EV_SYN) < 0)
  {
    fprintf(stderr, "Error on UI_SET_EVBIT: %s\n", strerror(errno));
    close(ufd);
    return 0;
  }



  // Tell uinput system our device can present any of the first 255 keycodes.
  int i;
  for(i=1;i<256;i++)
  {
    if(ioctl(ufd, UI_SET_KEYBIT, i) < 0)
    {
      fprintf(stderr, "Error on UI_SET_KEYBIT ioctl: %s\n", strerror(errno));
      close(ufd);
      return 0;
    }
  }

  if(ioctl(ufd, UI_SET_EVBIT, EV_REL) < 0)
  {
    fprintf(stderr, "Error on UI_SET_EVBIT, EV_REL ioctl: %s\n", strerror(errno));
    close(ufd);
    return 0;
  }

  if(ioctl(ufd, UI_SET_RELBIT, REL_X) < 0)
  {
    fprintf(stderr, "Error on UI_SET_RELBIT, REL_X ioctl: %s\n", strerror(errno));
    close(ufd);
    return 0;
  }

  if(ioctl(ufd, UI_SET_RELBIT, REL_Y) < 0)
  {
    fprintf(stderr, "Error on UI_SET_RELBIT, REL_Y ioctl: %s\n", strerror(errno));
    close(ufd);
    return 0;
  }

  if(ioctl(ufd, UI_SET_RELBIT, REL_WHEEL) < 0)
  {
    fprintf(stderr, "Error on UI_SET_RELBIT, REL_WHEEL ioctl: %s\n", strerror(errno));
    close(ufd);
    return 0;
  }


  ioctl(ufd, UI_SET_KEYBIT, BTN_MOUSE);
  ioctl(ufd, UI_SET_KEYBIT, BTN_LEFT);
  ioctl(ufd, UI_SET_KEYBIT, BTN_MIDDLE);
  ioctl(ufd, UI_SET_KEYBIT, BTN_RIGHT);
  ioctl(ufd, UI_SET_KEYBIT, BTN_4);
  ioctl(ufd, UI_SET_KEYBIT, BTN_5);

  memset(&uin_dev, 0, sizeof(uin_dev));
  strncpy(uin_dev.name, "Wiimote Keyer", UINPUT_MAX_NAME_SIZE);
  uin_dev.id.version = 1;
  uin_dev.id.bustype = BUS_BLUETOOTH;

  if(write(ufd, &uin_dev, sizeof(uin_dev)) != sizeof(uin_dev))
  {
    fprintf(stderr, "Error on device init: %s", strerror(errno));
    close(ufd);
    return 0;
  }


  if(ioctl(ufd, UI_DEV_CREATE) < 0)
  {
    fprintf(stderr, "Error on UI_DEV_CREATE: %s\n", strerror(errno));
    close(ufd);
    return 0;
  }

  return 1;
}

int uin_destroy()
{
  if(ioctl(ufd, UI_DEV_DESTROY) < 0)
  {
    fprintf(stderr, "Error on UI_DEV_DESTROY: %s\n", strerror(errno));
    close(ufd);
    return 0;
  }

 
  if(close(ufd))
  {
    fprintf(stderr, "Error on uinput device close: %s\n", strerror(errno));
    return 0;
  }
  return 1;
}

int uin_finish_keys()
{
  struct input_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.type = EV_SYN;
  ev.code = SYN_REPORT;
  ev.value = 0;

  if(write(ufd, &ev, sizeof(ev)) != sizeof(ev))
  {
    fprintf(stderr, "Error finishing keys to uinput: %s\n", strerror(errno));
    return 0;
  }
  return 1;
}

int uin_keydown(int keycode)
{
  struct input_event ev;
  memset(&ev, 0, sizeof(ev));
  gettimeofday(&ev.time, NULL);
  ev.type = EV_KEY;
  ev.code = keycode;
  ev.value = 1;

  if(write(ufd, &ev, sizeof(ev)) != sizeof(ev))
  {
    fprintf(stderr, "Error writing keydown to uinput: %s\n", strerror(errno));
    return 0;
  }
  return uin_finish_keys();
}

int uin_keyup(int keycode)
{
  struct input_event ev;
  memset(&ev, 0, sizeof(ev));
  gettimeofday(&ev.time, NULL);
  ev.type = EV_KEY;
  ev.code = keycode;
  ev.value = 0;

  if(write(ufd, &ev, sizeof(ev)) != sizeof(ev))
  {
    fprintf(stderr, "Error writing keyup to uinput: %s\n", strerror(errno));
    return 0;
  }
  return uin_finish_keys();
}

int uin_mousemove(int code, int value)
{
  struct input_event ev;
  memset(&ev, 0, sizeof(ev));
  gettimeofday(&ev.time, NULL);
  ev.type = EV_REL;
  ev.code = code;
  ev.value = value;

  if(write(ufd, &ev, sizeof(ev)) != sizeof(ev))
  {
    fprintf(stderr, "Error writing mousemove to uinput: %s\n", strerror(errno));
    return 0;
  }
  return uin_finish_keys();
}

/*
int main(int argc, char *argv[])
{
  if(uin_init())
  {
    sleep(1);
    uin_keydown(KEY_LEFTCTRL);
    uin_keydown(KEY_C);
    uin_keyup(KEY_C);
    uin_keyup(KEY_LEFTCTRL);
    sleep(10);
    return uin_destroy();
  }
  return -1;
}
*/
