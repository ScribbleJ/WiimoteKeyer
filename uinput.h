/* 
 * UINPUT C interface
 * (c)2009, Christopher "ScribbleJ" Jansen
 *
 */
#ifndef __UINPUT_H__
#define __UINPUT_H__

int uin_init();
int uin_destroy();
int uin_finish_keys();
int uin_keydown(int keycode);
int uin_keyup(int keycode);
int uin_mousemove(int code, int value);


#endif 
