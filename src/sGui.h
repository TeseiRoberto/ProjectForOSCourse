
/*
Author:	Tesei Roberto
Creation Date: 	05/07/2022
Last Update:	20/08/2022

How to use sGui:

	1] First of all create a GraphicContext_t using sgui_OpenGraphicContext() (this function returns NULL on failure)
	NOTE: The GraphicContext_t struct encapsulate the connection with xServer and is in no way correlated with the Xlib's
	concept of a graphic context "GC".

	2] When you got a GraphicContext_t you can create a Window_t using sgui_CreateWindow(), note that the Window_t created by this 
	function will be bounded to the GraphicContext given as parameter.

	3] When you got a Window_t you can create various widgets (GUI elements) such as: buttons, text fields, simple text and... 
	nothing more for now :D,
	you can add widgets to a window using sgui_CreateWidget() and specifying the type of widget that you want to create and
	its properties.

	4] To respond to user's input you must call sgui_UpdateWindow(), this function will wait for some input event to occur
	(IT WILL BLOCK UNTIL ONE IS RECEIVED) and then it will dispatch the event, in particular if it is a "fundamental" event (like 
	a press on window's cross to close window or an Expose event) then the window itself will handle it, for all other types
	of event it will be called sgui_DispatchEvent(). The sgui_DispatchEvent() function will traverse the window's widget list
	until it finds the widget that generated the input event, it will then call the callback associate to the event type setted
	for that widget; if no such widget is found then the event is discarded.
	Note that sgui_UpdateWindow() will process only one event and then return control to caller, if you want to handle more than
	one event then you must call it in a while loop.

	5] Before quitting the program you must destroy all the Window_t created ,you can do this by calling the sgui_DestroyWindow(), 
	then you must close the GraphicContext_t using sgui_CloseGraphicContext(), note that destroying a window implies destroying
	all widgets inside that window.


NOTE on sGui usage:
	- You should not instatiate GraphicContext_t, Window_t or Widget_t explicitly, in your program you should only have pointers
	to this types and manage those using the functions provided by the API.

	- sGui allocates dynamically all his structures, this may not be what you want. The idea behind this approach is that you
	create the gui interface of the program before doing everything else so all the allocation will be made upfront when the program
	starts and will not affect the program during runtime.

	- sGui keeps track of widgets that are in a Window_t using a linked list.

	- If you create a Textfield widget and set the "OnKeypress" callback to NULL this will be actually setted to a default callback,
	in particular "sgui_UpdateTextfield"; this function acts as you would expect from a textfield so it will add or remove
	characters to the widget's text buffer when input is received from user.

	- There are some functions that are declared in this header that you should never call, those function are only for sgui's 
	internal use and they act on raw XLib's object's. All those function can be recognized by the fact that they contains an X
	in their name, they can be distinguished by actual XLib functions by the fact that the X does not appear as first character.
*/

#ifndef SGUI_H
#define SGUI_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define MAX_TEXT_SIZE 	64			// Size of the text array in the widget struct
#define WIDGET_ID_SIZE 	16			// Size of the string that represent widget's id

// Definitions of standar color values
#define SGUI_BLACK 	0x000000
#define SGUI_WHITE 	0xffffff
#define SGUI_RED 	0xff0000
#define SGUI_GREEN 	0x00ff00
#define SGUI_BLUE 	0x0000ff


typedef enum { SGUI_CUSTOM, SGUI_BUTTON, SGUI_TEXTFIELD, SGUI_TEXT } WidgetType_t;
typedef unsigned Color_t;

typedef struct sgui_GraphicContext {
	Display* XDisplay;			// XLib's native struct that represent the connection with the XServer
	int screenNum;
	uint32_t width;
	uint32_t height;
} GraphicContext_t;


typedef struct sgui_Window {
	Window XWindow;
	GraphicContext_t* boundedGC;		// Pointer to graphic context to which the window is bounded
	XWindowChanges properties;		// XLib's native struct that contains x pos, y pos, width and height of the XWindow
	XTextProperty name;			// XLib's native type taht contain Window's name
	int isActive;				// Indicates if the window is active (1) or has been closed (0)
	Color_t bgColor;			// Color used to draw window's background
	Color_t fgColor;			// Color used to draw window's foreground
	Atom windowCloseAtom;			// XLib's native type used to intercept the WM_DELETE_WINDOW event (caused whn user clicks on the "x" button of the window)

	struct sgui_Widget* firstWidget;	// Pointer to the first widget added to the window
	struct sgui_Widget* lastWidget;		// Pointer to the last widget added to the window
} Window_t;


typedef struct sgui_Widget {
	WidgetType_t type;			// Indicates the widget's type
	Window_t* parent;			// Pointer to the window that contains the widget
	Window XWindow;				// XLib's native window struct
	XWindowChanges properties;		// XLib's native struct that contains x pos, y pos, width and height of the XWindow
	GC style;				// Xlib's native struct that defines style for drawing stuff inside the widget
	XFontStruct* font;			// Pointer to XLib's native struct that defines font properties
	char text[MAX_TEXT_SIZE];
	int displayText;			// Indicates if the widget contains text to be drawn
	int cursorPos;				// Variable used by textfield, keeps track of index in which we are inserting chars in the text buffer
	Color_t bgColor;			// Color used to draw the widget background
	Color_t fgColor;			// Color used to draw the widget foreground

	void (*OnClick)(struct sgui_Widget*, void*);		// Callback function called when the user clicks on the widget
	void (*OnHover)(struct sgui_Widget*, void*);		// Callback function called when the user enters the widget with cursor
	void (*OnOutHover)(struct sgui_Widget*, void*);		// Callback function called when the user leaves the widget with cursor
	void (*OnKeyPress)(struct sgui_Widget*, void*);		// Callback function called when the user press keyboard's buttons
	
	struct sgui_Widget* next;		// Pointer to the next widget in the list
	struct sgui_Widget* prev;		// Pointer to the prev widget in the list
	char id[WIDGET_ID_SIZE];		// A string that identifies the widget
} Widget_t;


GraphicContext_t* sgui_OpenGraphicContext();
void sgui_CloseGraphicContext();

Window_t* sgui_CreateWindow(GraphicContext_t* context, char* name, int x, int y, uint32_t width, uint32_t height, Color_t fgColor, Color_t bgColor);
void sgui_DestroyWindow(Window_t* window);
void sgui_RedrawWindow(Window_t* window);
void sgui_UpdateWindow(Window_t* window);
void sgui_DispatchEvent(Window_t* window, XEvent event);
int sgui_IsWindowActive(Window_t* window);

void sgui_SetWindowName(Window_t* window, char* name);
void sgui_SetWindowColor(Window_t* window, Color_t newFgColor, Color_t newBgColor);

void sgui_GetWindowColor(Window_t* window, Color_t* fgColor, Color_t* bgColor);


// Functions for handling widgets
Widget_t* sgui_CreateWidget(Window_t* window, WidgetType_t type, char* id, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
				char* text, Color_t fgColor, Color_t bgColor, Color_t drawFgColor, Color_t drawBgColor, 
				void (*onClick)(Widget_t*, void*), void (*onHover)(Widget_t*, void*), 
				void (*onOutHover)(Widget_t*, void*), void (*onKeyPress)(Widget_t*, void*));

void sgui_DestroyWidget(Window_t* window, Widget_t* widget);
void sgui_DestroyAllWidgets(Window_t* window);
Widget_t* sgui_GetWidget(Window_t* window, const char* id);
void sgui_RedrawWidget(Widget_t* widget);

void sgui_SetWidgetPosition(Widget_t* widget, uint32_t x, uint32_t y);
void sgui_SetWidgetDimension(Widget_t* widget, uint32_t width, uint32_t height);
void sgui_SetWidgetColor(Widget_t* widget, Color_t fgColor, Color_t bgColor, Color_t drawFgColor, Color_t drawBgColor);
void sgui_SetWidgetText(Widget_t* widget, const char* newText, uint8_t isVisible);
void sgui_SetWidgetFont(Widget_t* widget, const char* fontName);
void sgui_SetWidgetCallback(Widget_t* widget, void (*onClick)(Widget_t*, void*), void (*onHover)(Widget_t*, void*),
				void (*onOutHover)(Widget_t*, void*), void (*onKeypress)(Widget_t*, void*));

void sgui_GetWidgetPosition(Widget_t* widget, uint32_t* x, uint32_t* y);
void sgui_GetWidgetDimension(Widget_t* widget, uint32_t* width, uint32_t* height);
void sgui_GetWidgetColor(Widget_t* widget, Color_t* fgColor, Color_t* bgColor, Color_t* drawFgColor, Color_t* drawBgColor);
char* sgui_GetWidgetText(Widget_t* widget);
void sgui_GetWidgetCallback(Widget_t* widget, void (**onClick)(Widget_t*, void*), void (**onHover)(Widget_t*, void*),
				void (**onOutHover)(Widget_t*, void*), void (**onKeyPress)(Widget_t*, void*));

// Function for handling XLib's object
Window sgui_CreateXWindow(Display* display, int screenNum, Window* parent, int x, int y, uint32_t width, uint32_t height, unsigned catchEventsMask, Color_t fgColor, Color_t bgColor);
void sgui_DestroyXWindow(Display* display, Window XWindow);
GC sgui_CreateXGC(Display* display, Window window, Color_t fgColor, Color_t bgColor, int lineWidth, int lineStyle, int capStyle, int joinStyle, int fillStyle, Font font);
void sgui_DestroyXGC(Display* display, GC gc);
XFontStruct* sgui_LoadXFont(Display* display, const char* fontName);
void sgui_UnloadXFont(Display* display, Font font);
void sgui_MapXWindow(Display* display, Window window);
void sgui_UnmapXWindow(Display* display, Window window);


// Utility functions
Color_t sgui_Rgb(uint8_t red, uint8_t green, uint8_t blue);
void sgui_UpdateTextfield(Widget_t* widget, void* arg);

#endif // SGUI_H

