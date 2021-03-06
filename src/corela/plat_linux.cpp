#ifdef __linux__

#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>
#include <GL/glx.h>
#include "corela.h"
#include "glutils.h"

#ifndef GLX_MESA_swap_control
#define GLX_MESA_swap_control 1
typedef int (*PFNGLXGETSWAPINTERVALMESAPROC)(void);
typedef void (*PFNGLXSWAPINTERVALEXTPROC)(Display *dpy, GLXDrawable drawable, int interval);
#endif

FRAME* frame;
EVENT_CALLBACK g_callback;
bool g_keys[256];
Display* display;
int screen;
Window win;
GLXContext context;
XF86VidModeModeInfo desktopMode;

// scancode to virtual keycode conversion
int ScancodeToVirtual(int k) {
    if(k==9) return 27;
    if(k==10) return 49;
    if(k==11) return 50;
    if(k==12) return 51;
    if(k==13) return 52;
    if(k==14) return 53;
    if(k==15) return 54;
    if(k==16) return 55;
    if(k==17) return 56;
    if(k==18) return 57;
    if(k==19) return 48;
    if(k==20) return 219;
    if(k==21) return 221;
    if(k==22) return 8;
    if(k==23) return 9;
    if(k==24) return 81;
    if(k==25) return 87;
    if(k==26) return 69;
    if(k==27) return 82;
    if(k==28) return 84;
    if(k==29) return 90;
    if(k==30) return 85;
    if(k==31) return 73;
    if(k==32) return 79;
    if(k==33) return 80;
    if(k==34) return 186;
    if(k==35) return 187;
    if(k==36) return 13;
    if(k==37) return 162;
    if(k==38) return 65;
    if(k==39) return 83;
    if(k==40) return 68;
    if(k==41) return 70;
    if(k==42) return 71;
    if(k==43) return 72;
    if(k==44) return 74;
    if(k==45) return 75;
    if(k==46) return 76;
    if(k==47) return 192;
    if(k==48) return 222;
    if(k==49) return 220;
    if(k==50) return 160;
    if(k==51) return 191;
    if(k==52) return 89;
    if(k==53) return 88;
    if(k==54) return 67;
    if(k==55) return 86;
    if(k==56) return 66;
    if(k==57) return 78;
    if(k==58) return 77;
    if(k==59) return 188;
    if(k==60) return 190;
    if(k==61) return 189;
    if(k==62) return 161;
    if(k==63) return 106;
    if(k==64) return 164;
    if(k==65) return 32;
    if(k==66) return 20;
    if(k==67) return 112;
    if(k==68) return 113;
    if(k==69) return 114;
    if(k==70) return 115;
    if(k==71) return 116;
    if(k==72) return 117;
    if(k==73) return 118;
    if(k==74) return 119;
    if(k==75) return 120;
    if(k==76) return 121;
    if(k==77) return 144;
    if(k==78) return 145;
    if(k==79) return 36;
    if(k==80) return 38;
    if(k==81) return 33;
    if(k==82) return 109;
    if(k==83) return 37;
    if(k==84) return 12;
    if(k==85) return 39;
    if(k==86) return 107;
    if(k==87) return 35;
    if(k==88) return 40;
    if(k==89) return 34;
    if(k==90) return 45;
    if(k==91) return 46;
    if(k==92) return 44;
    if(k==94) return 226;
    if(k==95) return 122;
    if(k==96) return 123;
    if(k==97) return 12;
    if(k==98) return 238;
    if(k==99) return 241;
    if(k==100) return 234;
    if(k==101) return 249;
    if(k==102) return 245;
    if(k==103) return 243;
    if(k==106) return 251;
    if(k==107) return 47;
    if(k==108) return 124;
    if(k==109) return 125;
    if(k==110) return 126;
    if(k==111) return 127;
    if(k==112) return 128;
    if(k==113) return 129;
    if(k==114) return 130;
    if(k==115) return 131;
    if(k==116) return 132;
    if(k==117) return 133;
    if(k==118) return 134;
    if(k==119) return 237;
    if(k==121) return 233;
    if(k==123) return 193;
    if(k==126) return 135;
    if(k==131) return 235;
    if(k==132) return 9;
    if(k==134) return 194;
}

/* X11 forces you to create a blank cursor if you want
   to disable the cursor. */
static Cursor MakeBlankCursor(Display *display, Window window) {
  static char data[1] = {0};
  Cursor cursor;
  Pixmap blank;
  XColor dummy;

  blank = XCreateBitmapFromData(display, window, data, 1, 1);
  cursor = XCreatePixmapCursor(display, blank, blank, &dummy, &dummy, 0, 0);
  XFreePixmap(display, blank);

  return cursor;
}
/**
 * Determine whether or not a GLX extension is supported.
 */
static int is_glx_extension_supported(Display *dpy, const char *query) {
	const int scrnum = DefaultScreen(dpy);
	const char *glx_extensions = NULL;
	const size_t len = strlen(query);
	const char *ptr;

	if (glx_extensions == NULL) {
		glx_extensions = glXQueryExtensionsString(dpy, scrnum);
	}

	ptr = strstr(glx_extensions, query);
	return ((ptr != NULL) && ((ptr[len] == ' ') || (ptr[len] == '\0')));
}

static void SetSwapInterval(Display *display, GLXDrawable drawable, int interval) {
	PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddressARB((const GLubyte *) "glXSwapIntervalEXT");
	if(!glXSwapIntervalEXT) { Log(LOG_WARNING, "OpenGL function not found: glXSwapIntervalEXT"); return; }
	glXSwapIntervalEXT(display, drawable, interval);
}

static void _FrameNoBorder(Display *dpy, Window w) {
	static const unsigned MWM_HINTS_DECORATIONS = (1 << 1);
	static const int PROP_MOTIF_WM_HINTS_ELEMENTS = 5;

	typedef struct {
		unsigned long flags;
		unsigned long functions;
		unsigned long decorations;
		long inputMode;
		unsigned long status;
	} PropMotifWmHints;

	PropMotifWmHints motif_hints;
	Atom prop, proptype;
	unsigned long flags = 0;

	/* setup the property */
	motif_hints.flags = MWM_HINTS_DECORATIONS;
	motif_hints.decorations = flags;

	/* get the atom for the property */
	prop = XInternAtom( dpy, "_MOTIF_WM_HINTS", True );
	if (!prop) { return; }

	/* not sure this is correct, seems to work, XA_WM_HINTS didn't work */
	proptype = prop;

	XChangeProperty( dpy, w, prop, proptype, 32, PropModeReplace, (unsigned char *) &motif_hints, PROP_MOTIF_WM_HINTS_ELEMENTS);
}

FRAME* FrameCreate(const char* name, EVENT_CALLBACK callback, void* user, long x, long y, long width, long height) {
	int winWidth = width;
	int winHeight = height;

	// remember callback globally
	g_callback = callback;

	// connect to x server
	display = XOpenDisplay(NULL);
	//display = XOpenDisplay(":0");
	if(!display) Log(LOG_FATAL, "Unable to connect to X server display (XOpenDisplay failed)!");

	// get default screen
	DefaultScreen(display);
	//if(!screen) Log(LOG_FATAL, "No default screen found (DefaultScreen failed)!");

	/*	if(true) { // fullscreen
			x = 0; y = 0;
			winWidth = DisplayWidth(display, screen);
			winHeight = DisplayHeight(display, screen);
		}*/

	// get desktop window
	Window root = RootWindow(display, screen);
	if(!root) Log(LOG_FATAL, "No default root window found (DefaultRootWindow failed)!");

#if 0
	// get all video modes
    int modeNum;
	XF86VidModeModeInfo **modes;
	XF86VidModeGetAllModeLines(display, screen, &modeNum, &modes);
	desktopMode = modes[0];

	// find best matching fullscreen mode
	int bestMode = -1;
	int match = 100000;
	Log(LOG_INFO, "Enumerating video modes:");
	for(int i=0; i<modeNum; i++) {
		Log(LOG_INFO, " - %dx%d", modes[i]->hdisplay, modes[i]->vdisplay);
	    int matchDif = abs(modes[i]->hdisplay - width) + abs(modes[i]->vdisplay - height);
		if(matchDif <= match  ||  modes[i]->hdisplay == width && modes[i]->vdisplay == height) {
			bestMode = i;
			match = matchDif;
		}
	}
	if(bestMode == -1) { Log(LOG_FATAL, "No fitting video mode found."); }
	winWidth = modes[bestMode]->hdisplay;
	winHeight = modes[bestMode]->vdisplay;

	// change screen mode
	Log(LOG_INFO, "Switching to mode %dx%d.", width, height);
	XF86VidModeSwitchToMode(display, screen, modes[bestMode]);
	XF86VidModeSetViewPort(display, screen, 0, 0);
	XFree(modes);
#endif

	// visual settings
	GLint gl_att[] = {
		GLX_RGBA,
		GLX_DOUBLEBUFFER,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_DEPTH_SIZE, 1,
		None
	};

	// find mode for the visual settings
	XVisualInfo* vi = glXChooseVisual(display, 0, gl_att);
	if(vi == NULL) Log(LOG_FATAL, "OpenGL visual settings not supported (glXChooseVisual failed)!");

	// create color map
	Colormap cmap = XCreateColormap(display, root, vi->visual, AllocNone);

	// set window attributes
	XSetWindowAttributes swa;
	swa.colormap = cmap;
	swa.border_pixel = 0;
	swa.background_pixel = 0;

	// set event mask
	swa.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;

	// create window
	win = XCreateWindow(display, root, x, y, winWidth, winHeight, 0, vi->depth, InputOutput, vi->visual, CWBackPixel | CWBorderPixel | CWColormap | CWEventMask, &swa);

#if 0
	// receive WM_DELETE_WINDOW client event
	Atom wmDelete = XInternAtom(display, "WM_DELETE_WINDOW", 0);
	XSetWMProtocols(display, win, &wmDelete, 1);

	// fullscreen
	Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
	Atom fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

	XEvent xev;
	memset(&xev, 0, sizeof(XEvent));
	xev.type = ClientMessage;
	xev.xclient.window = win;
	xev.xclient.message_type = wm_state;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 1;
	xev.xclient.data.l[1] = fullscreen;
	xev.xclient.data.l[2] = 0;

	XSendEvent(display, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
#endif

	// fullscreen borderless window
//	_FrameNoBorder(display, win);

	// set hints and properties
#if 1
	XSizeHints sizehints;
	sizehints.x = x;
	sizehints.y = y;
	sizehints.width  = winWidth;
	sizehints.height = winHeight;
	sizehints.flags = USSize | USPosition;
	XSetNormalHints(display, win, &sizehints);
	XSetStandardProperties(display, win, name, name, None, (char **)NULL, 0, &sizehints);
#endif

	// create gl context and make current
	context = glXCreateContext(display, vi, NULL, GL_TRUE);
	if(!context) Log(LOG_FATAL, "OpenGL context creation failed! (glXCreateContext failed)!");
	XFree(vi);

   // map window to display
	XMapWindow(display, win);
	XStoreName(display, win, name);
	glXMakeCurrent(display, win, context);
//	SetSwapInterval(display, win, 1);

	// create frame object
	frame = (FRAME*)malloc(sizeof(FRAME));
//	frame->hwnd = hwnd;
//	frame->hdc = hdc;
//	frame->rc = rc;
	frame->user = user;
	frame->width = winWidth;
	frame->height = winHeight;
	frame->vwidth = width;
	frame->vheight = height;
	frame->cb = callback;
	frame->showCursor = false;
	frame->hwnd = (void*)win;
//	SetWindowLongPtrA(hwnd, GWL_USERDATA, (LONG)frame);

	Log(LOG_INFO, "Frame created. Width: %.0f, Height: %.0f, VWidth: %.0f, VHeight: %.0f.", frame->width, frame->height, frame->vwidth, frame->vheight);

	// setup for 2D drawing
	glViewport(0, 0, frame->width, frame->height);
	glUtilsTo2D(0.0, 0.0, frame->vwidth, frame->vheight, 0.0, 1.0);

	// set some defaults
	glClearColor(0.0,0.0,0.0,1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	// clear buffer and swap
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//	glXSwapBuffers(display, win);

	// initialize opengl
	Log(LOG_INFO, "Initializing GL support ...");
	glUtilsInit();
	GLSLInit();
	Log(LOG_INFO, "OpenGL system created successfully.");

	// return
	return frame;
}

//#define EVENT_RAISE(e,_x,_y,_a,_b) {if(frame){fevent.id=e;fevent.sender=frame;fevent.user=frame->user;fevent.x=_x;fevent.y=_y;fevent.button=_a;fevent.key=_b;frame->cb(&fevent);}}
#define EVENT_RAISE(e,_x,_y,_px,_py,_a,_b,_t) {if(frame){fevent.id=e;fevent.sender=frame;fevent.user=frame->user;fevent.x=_x;fevent.y=_y;fevent.prevX=_px;fevent.prevY=_py;fevent.button=_a;fevent.key=_b;fevent.text=_t;frame->cb(&fevent);}}
bool FrameUpdate() {
    if(!frame) return false;

	static int px, py;
	static int mx, my;
	FRAME_EVENT fevent;

	XEvent event;
	XWindowAttributes gwa;

	while(XPending(display)) {
		XNextEvent(display, &event);

		if(event.type == ConfigureNotify) {
			//_FrameReshape(event.xconfigure.width, event.xconfigure.height);
		}

		else if(event.type == ClientMessage) {
			Log(LOG_DEBUG, "Window received: ClientMessage");
			EVENT_RAISE(EVENT_CLOSE, mx, my, px, py, 0, 0, "");
			return false;
		}

		else  if(event.type == Expose) {
/*			if(event.xexpose.count != 0)
				break;
			XGetWindowAttributes(display, win, &gwa);
			glViewport(0, 0, gwa.width, gwa.height);
			FrameFlip(frame);*/
		}

		else if(event.type == KeyPress) {
			if(event.xkey.keycode > 0 && event.xkey.keycode < 256) g_keys[ScancodeToVirtual(event.xkey.keycode)] = true;
			
			// TODO: Translate key to virtual keycode
			//int vkey = XLookupKeysym(&event.xkey, 0); or XLookupString?
			
			char keyuc[8];
			XLookupString(&event.xkey, keyuc, sizeof(keyuc), NULL, NULL);
			
			EVENT_RAISE(EVENT_KEY_DOWN, mx, my, px, py, 0, ScancodeToVirtual(event.xkey.keycode), keyuc);
			//if (buffer[0] == 27) { EVENT_RAISE(EVENT_CLOSE, mx, my, 0, 0); }
		}

		else if(event.type == KeyRelease) {
			if(event.xkey.keycode > 0 && event.xkey.keycode < 256) g_keys[ScancodeToVirtual(event.xkey.keycode)] = false;

			char keyuc[8];
			XLookupString(&event.xkey, keyuc, sizeof(keyuc), NULL, NULL);

			EVENT_RAISE(EVENT_KEY_UP, mx, my, px, py, 0, ScancodeToVirtual(event.xkey.keycode), keyuc);
		}

		else if(event.type == ButtonPress) {
			EVENT_RAISE(EVENT_MOUSE_BUTTON_DOWN, mx, my, px, py, event.xbutton.button, 0, "");
		}

		else if(event.type == ButtonRelease) {
			EVENT_RAISE(EVENT_MOUSE_BUTTON_UP, mx, my, px, py, event.xbutton.button, 0, "");
		}

		else if(event.type == MotionNotify) {
			mx = (double)event.xmotion.x / frame->width  * frame->vwidth;
			my = (double)event.xmotion.y / frame->height * frame->vheight;
			EVENT_RAISE(EVENT_MOUSE_MOVE, mx, my, px, py, 0, 0, "");
		}
	}

	return true;
}

void FrameClose(FRAME* frame) {
	glXMakeCurrent(display, None, NULL);
	glXDestroyContext(display, context);
	XDestroyWindow(display, win);
	XCloseDisplay(display);
}

void FrameFlip(FRAME* frame) {
	glXSwapBuffers(display, win);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void FrameShow(FRAME* frame, int show) { }
void FrameMaximize(FRAME* frame) { }
void FrameMinimize(FRAME* frame) { }
void FrameToggleFull(FRAME* frame) { }
void FrameSelect(FRAME* frame) { }

void FrameShowCursor(FRAME* frame, coBool show) {
	// hide cursor
	if(show) XUndefineCursor(display, win);
	else XDefineCursor(display, win, MakeBlankCursor(display, win));
}

void FrameSetScissor(FRAME* frame, int left, int top, int right, int bottom) {
	if(left >= 0) {
		glEnable(GL_SCISSOR_TEST);

		left = (int)( (float)left / frame->vwidth * frame->width );
		top = (int)( (float)top / frame->vheight * frame->height );
		right = (int)( (float)right / frame->vwidth * frame->width);
		bottom = (int)( (float)bottom / frame->vheight * frame->height );

		//glScissor(left, frame->height - bottom, right - left, bottom - top);
		glScissor(left, top, right - left, bottom - top);
	} else {
		glDisable(GL_SCISSOR_TEST);
	}
}

coBool KeyGet(int code) {
	if(code >= 0 && code < 256) return g_keys[code];
	return coFalse;
}

/*void MouseGet(int* x, int*y) {
	if(x) *x = mouseX;
	if(y) *y = mouseY;
}*/

std::string RequestFileName() {
    // TODO: zenity --file-selection --title="Select a File"
	return "";
}

#endif
