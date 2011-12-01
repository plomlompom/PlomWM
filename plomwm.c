/* PlomWM 0.2.2
 * Written by Christian Heller <c.heller@plomlompom.de>
 * Based on Nick Welch's TinyWM */

#include <X11/Xlib.h>
#include <stdlib.h>
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* wdata type ties together relevant data about a window: its id, its geometry */
typedef struct {
  Window id;
  int x, y, width, height, fullscreen; } wdata;

int window_i(wdata windows[], Window window) {
/* Return index of a given window in windows[]. */
  int i, j;
  for ( j = 0; 1 ; j++ ) {
    if ( windows[j].id == window ) {
      i = j;
      break; } }
  return i; }

int main(void) {
  /* Connect to X server and determine root window. */
  Display * dpy = XOpenDisplay(NULL);
  Window root = DefaultRootWindow(dpy);

  /* Determine fullscreen geometry from root window. */
  XWindowAttributes x; int full_width, full_height;
  XGetWindowAttributes(dpy, root, &x);
  full_width = x.width; full_height = x.height;

  /* Select and grab certain window and user action events / button and key presses from the X server. */
  XSelectInput(dpy, root, SubstructureNotifyMask);
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F11")), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("Tab")), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("Tab")), Mod1Mask | ShiftMask, root, True, GrabModeAsync, 
GrabModeAsync);
  XGrabButton(dpy, 1, AnyModifier, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
  XGrabButton(dpy, 3, Mod1Mask, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);

  /* Database for window ids and geometries. */
  wdata * windows = malloc(0);
  int w_i = 0;

  /* Window event loop. */
  XWindowAttributes attr; XButtonEvent start; XEvent ev;
  for (;;) {
    XNextEvent(dpy, &ev);

    if (ev.type == KeyPress) {

      /* F11+ALT key press switches to fullscreen or back again (to the last non-fullscreen geometry). */
      if (ev.xkey.keycode == XKeysymToKeycode(dpy, XStringToKeysym("F11")) ) {
        if (ev.type == KeyPress && ev.xkey.subwindow != None) { 
          int i = window_i(windows, ev.xkey.subwindow);
          if (windows[i].fullscreen == 0) {
            XMoveResizeWindow(dpy, ev.xkey.subwindow, 0, 0, full_width, full_height);
            windows[i].fullscreen = 1; }
          else {
            XMoveResizeWindow(dpy, ev.xkey.subwindow, windows[i].x, windows[i].y, windows[i].width, windows[i].height); 
            windows[i].fullscreen = 0; } } }
      
      /* TAB+ALT circulates the window stacking order. Add SHIFT to do it backwards. */
      else if (ev.xkey.keycode == XKeysymToKeycode(dpy, XStringToKeysym("Tab")) ) {
        if (ev.xkey.state == Mod1Mask) XCirculateSubwindows(dpy, root, RaiseLowest);
        else                           XCirculateSubwindows(dpy, root, LowerHighest); } }

    else if (ev.type == ButtonPress && ev.xbutton.subwindow != None) { 
      /* At button press + ALT, record current window attributes and start grabbing the pointer's motion. */
      if (ev.xbutton.state == Mod1Mask) {
        XGrabPointer(dpy, ev.xbutton.subwindow, True, PointerMotionMask | ButtonReleaseMask,
                     GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
        XGetWindowAttributes(dpy, ev.xbutton.subwindow, &attr);
        start = ev.xbutton; }

     /* If button is pressed over a window without ALT, raise that window. */
     else {
        XRaiseWindow(dpy, ev.xbutton.subwindow); } }

    /* As long as pointer's motion is grabbed, keep changing the window's geometry. */
    else if (ev.type == MotionNotify) {
      while (XCheckTypedEvent(dpy, MotionNotify, &ev));
      int xdiff = ev.xbutton.x_root - start.x_root; int ydiff = ev.xbutton.y_root - start.y_root;
      XMoveResizeWindow(dpy, ev.xmotion.window,
                        attr.x + (start.button == 1 ? xdiff : 0),
                        attr.y + (start.button == 1 ? ydiff : 0),
                        MAX(1, attr.width + (start.button == 3 ? xdiff : 0)),
                        MAX(1, attr.height + (start.button == 3 ? ydiff : 0)));
      int i = window_i(windows, ev.xmotion.window);
      windows[i].fullscreen = 0; }

    /* As button is released, stop grabbing the pointer's motion. */
    else if (ev.type == ButtonRelease)
    { XUngrabPointer(dpy, CurrentTime);
      int i = window_i(windows, ev.xbutton.window); 
      windows[i].fullscreen = 0; }

    /* As window geometry is configured, record it to windows[] as long as it is not fullscreened. */
    else if (ev.type == ConfigureNotify) {
      int i = window_i(windows, ev.xconfigure.window);
      if ( windows[i].fullscreen == 0 ) {
        int x = ev.xconfigure.x; int y = ev.xconfigure.y; 
        int width = ev.xconfigure.width; int height = ev.xconfigure.height;
        windows[i].x = x; windows[i].y = y; windows[i].width = width; windows[i].height = height; } } 

    /* If a new window is created, add its id to windows[] and augment the window counter. */
    else if (ev.type == CreateNotify) {
      windows = realloc(windows, (w_i + 1) * sizeof(wdata));
      windows[w_i].id = ev.xcreatewindow.window;
      windows[w_i].fullscreen = 0; 
      w_i++; } 

    /* If a window is destroyed, shrink windows[]. */
    else if (ev.type == DestroyNotify) {
      w_i--;
      int i = window_i(windows, ev.xdestroywindow.window);
      if ( i != w_i) {
        windows[i] = windows[w_i]; }
      windows = realloc(windows, (w_i + 1) * sizeof(wdata)); } } }
