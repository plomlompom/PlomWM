/* PlomWM 0.3 / written by Christian Heller <c.heller@plomlompom.de> / based on Nick Welch's TinyWM */
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#define MAX(a, b) ((a) > (b) ? (a) : (b))

int main(void) {
  /* Connect to X server and determine root window. */
  Display * dpy = XOpenDisplay(NULL);
  Window root   = DefaultRootWindow(dpy);

  /* Stuff needed for window properties manipulation. */
  Atom IsFullscreen = XInternAtom(dpy, "PLOMWM_ISFULLSCREEN", False);
  Atom Geometry     = XInternAtom(dpy, "PLOMWM_GEOMETRY",     False);
  Atom            dump_atom; 
  int             dump_int;
  unsigned long   dump_long;
  unsigned char * r_fullscreen;
  char            a_fullscreen[1];
  short *         r_geometry;
  short           a_geometry[4];

  /* Get fullscreen geometry. */
  XWindowAttributes x;
  XGetWindowAttributes(dpy, root, &x);
  int full_width  = x.width; 
  int full_height = x.height;

  /* Select / grab certain window and user action events. */
  XSelectInput(dpy, root, SubstructureNotifyMask);
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F11")), Mod4Mask,           root, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("Tab")), Mod4Mask,           root, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("Tab")), Mod4Mask|ShiftMask, root, True, GrabModeAsync, GrabModeAsync);
  XGrabButton(dpy, 1, Mod4Mask, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
  XGrabButton(dpy, 3, Mod4Mask, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);

  /* Window event loop. */
  XWindowAttributes attr;
  XButtonEvent start;
  XEvent ev;
  for (;;) {
    XNextEvent(dpy, &ev);

    if (ev.type == KeyPress && ev.xkey.subwindow != None) {
      /* Catch Mod4+F11 for toggling fullscreen view of window under cursor. */
      if (ev.xkey.keycode == XKeysymToKeycode(dpy, XStringToKeysym("F11")) ) {
          XRaiseWindow(dpy, ev.xkey.subwindow); 
          XGetWindowProperty(dpy, ev.xkey.subwindow, IsFullscreen, 0, 1, False, XA_INTEGER, 
                             &dump_atom, &dump_int, &dump_long, &dump_long, &r_fullscreen); 
          if ( r_fullscreen[0] == 0 ) {
            XMoveResizeWindow(dpy, ev.xkey.subwindow, 0, 0, full_width, full_height);
            a_fullscreen[0] = 1;
            XChangeProperty(dpy, ev.xkey.subwindow, IsFullscreen, XA_INTEGER, 8, 
                            PropModeReplace, (unsigned char *) &a_fullscreen, 1); }
          else {
            XGetWindowProperty(dpy, ev.xkey.subwindow, Geometry, 0, 2, False, XA_INTEGER,
                               &dump_atom, &dump_int, &dump_long, &dump_long, (unsigned char **) &r_geometry);
            XMoveResizeWindow(dpy, ev.xkey.subwindow, r_geometry[0], r_geometry[1], r_geometry[2], r_geometry[3]);
            a_fullscreen[0] = 0;
            XChangeProperty(dpy, ev.xkey.subwindow, IsFullscreen, XA_INTEGER, 8,
                            PropModeReplace, (unsigned char *) &a_fullscreen, 1); } }

      /* Mod4+Tab circulates window stacking order. Mod4+Tab+Shift does it backwards. */
      else if (ev.xkey.keycode == XKeysymToKeycode(dpy, XStringToKeysym("Tab")) ) {
        if (ev.xkey.state == Mod4Mask)
          XCirculateSubwindows(dpy, root, RaiseLowest);
        else if (ev.xkey.state == Mod4Mask|ShiftMask)
          XCirculateSubwindows(dpy, root, LowerHighest); } }

    /* At pointer button press + Mod4 key press, record current window attributes and start grabbing pointer's motion. */
    else if (ev.type == ButtonPress && ev.xbutton.subwindow != None) { 
      XRaiseWindow(dpy, ev.xbutton.subwindow); 
      XGrabPointer(dpy, ev.xbutton.subwindow, True, PointerMotionMask | ButtonReleaseMask, 
                   GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
      XGetWindowAttributes(dpy, ev.xbutton.subwindow, &attr);
      start = ev.xbutton; }

    /* As long as pointer's motion is grabbed, keep changing the window's geometry and storing non-fullscreen state. */
    else if (ev.type == MotionNotify) {
      while (XCheckTypedEvent(dpy, MotionNotify, &ev));
      int xdiff = ev.xbutton.x_root - start.x_root;
      int ydiff = ev.xbutton.y_root - start.y_root;
      XMoveResizeWindow(dpy, ev.xmotion.window, attr.x + (start.button == 1 ? xdiff : 0),
                                                attr.y + (start.button == 1 ? ydiff : 0),
                                                MAX(1, attr.width  + (start.button == 3 ? xdiff : 0)), 
                                                MAX(1, attr.height + (start.button == 3 ? ydiff : 0))); 
      a_fullscreen[0] = 0;
      XChangeProperty(dpy, ev.xmotion.window, IsFullscreen, XA_INTEGER, 8,
                      PropModeReplace, (unsigned char *) &a_fullscreen, 1); }

    /* As button is released, stop grabbing the pointer's motion and store non-fullscreen state. */
    else if (ev.type == ButtonRelease) {
      XUngrabPointer(dpy, CurrentTime); }  

    /* Each time a window is (re-)configured to a non-fullscreen-state, store its geometry properties. */
    else if (ev.type == ConfigureNotify) {
     XGetWindowProperty(dpy, ev.xconfigure.window, IsFullscreen, 0, 1, False, XA_INTEGER, 
                        &dump_atom, &dump_int, &dump_long, &dump_long, &r_fullscreen); 
     if ( r_fullscreen[0] == 0 ) {     
       a_geometry[0] = ev.xconfigure.x;
       a_geometry[1] = ev.xconfigure.y;
       a_geometry[2] = ev.xconfigure.width;
       a_geometry[3] = ev.xconfigure.height;
       XChangeProperty(dpy, ev.xconfigure.window, Geometry, XA_INTEGER, 16,
                       PropModeReplace, (unsigned char *) &a_geometry, 4); } }

    /* If a window is created, store its fullscreen property, assuming it to be negative. */
    else if (ev.type == CreateNotify) {
      a_fullscreen[0] = 0;
      XChangeProperty(dpy, ev.xcreatewindow.window, IsFullscreen, XA_INTEGER, 8, 
                      PropModeReplace, (unsigned char *) &a_fullscreen, 1); } } }
