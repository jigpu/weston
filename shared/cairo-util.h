/*
 * Copyright © 2008 Kristian Høgsberg
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#ifndef _CAIRO_UTIL_H
#define _CAIRO_UTIL_H

#include <stdint.h>
#include <cairo.h>

void
surface_flush_device(cairo_surface_t *surface);

void
tile_mask(cairo_t *cr, cairo_surface_t *surface,
	  int x, int y, int width, int height, int margin, int top_margin);

void
tile_source(cairo_t *cr, cairo_surface_t *surface,
	    int x, int y, int width, int height, int margin, int top_margin);

void
rounded_rect(cairo_t *cr, int x0, int y0, int x1, int y1, int radius);

cairo_surface_t *
load_cairo_surface(const char *filename);

struct theme {
	cairo_surface_t *active_frame;
	cairo_surface_t *inactive_frame;
	cairo_surface_t *shadow;
	int frame_radius;
	int margin;
	int width;
	int titlebar_height;
};

struct theme *
theme_create(void);
void
theme_destroy(struct theme *t);

enum {
	THEME_FRAME_ACTIVE = 1,
	THEME_FRAME_MAXIMIZED = 2,
	THEME_FRAME_NO_TITLE = 4
};

void
theme_set_background_source(struct theme *t, cairo_t *cr, uint32_t flags);
void
theme_render_frame(struct theme *t, 
		   cairo_t *cr, int width, int height,
		   const char *title, uint32_t flags);

enum theme_location {
	THEME_LOCATION_INTERIOR = 0,
	THEME_LOCATION_RESIZING_TOP = 1,
	THEME_LOCATION_RESIZING_BOTTOM = 2,
	THEME_LOCATION_RESIZING_LEFT = 4,
	THEME_LOCATION_RESIZING_TOP_LEFT = 5,
	THEME_LOCATION_RESIZING_BOTTOM_LEFT = 6,
	THEME_LOCATION_RESIZING_RIGHT = 8,
	THEME_LOCATION_RESIZING_TOP_RIGHT = 9,
	THEME_LOCATION_RESIZING_BOTTOM_RIGHT = 10,
	THEME_LOCATION_RESIZING_MASK = 15,
	THEME_LOCATION_EXTERIOR = 16,
	THEME_LOCATION_TITLEBAR = 17,
	THEME_LOCATION_CLIENT_AREA = 18,
};

enum theme_location
theme_get_location(struct theme *t, int x, int y, int width, int height, int flags);

struct frame;

enum frame_status {
	FRAME_STATUS_NONE = 0,
	FRAME_STATUS_REPAINT = 0x1,
	FRAME_STATUS_MINIMIZE = 0x2,
	FRAME_STATUS_MAXIMIZE = 0x4,
	FRAME_STATUS_CLOSE = 0x8,
	FRAME_STATUS_MENU = 0x10,
	FRAME_STATUS_RESIZE = 0x20,
	FRAME_STATUS_MOVE = 0x40,
	FRAME_STATUS_ALL = 0x7f
};

enum frame_flag {
	FRAME_FLAG_ACTIVE = 0x1,
	FRAME_FLAG_MAXIMIZED = 0x2
};

enum {
	FRAME_BUTTON_NONE = 0,
	FRAME_BUTTON_CLOSE = 0x1,
	FRAME_BUTTON_MAXIMIZE = 0x2,
	FRAME_BUTTON_MINIMIZE = 0x4,
	FRAME_BUTTON_ALL = 0x7
};

enum frame_button_state {
	FRAME_BUTTON_RELEASED = 0,
	FRAME_BUTTON_PRESSED = 1
};

struct frame *
frame_create(struct theme *t, int32_t width, int32_t height, uint32_t buttons,
	     const char *title);

void
frame_destroy(struct frame *frame);

/* May set FRAME_STATUS_REPAINT */
int
frame_set_title(struct frame *frame, const char *title);

/* May set FRAME_STATUS_REPAINT */
void
frame_set_flag(struct frame *frame, enum frame_flag flag);

/* May set FRAME_STATUS_REPAINT */
void
frame_unset_flag(struct frame *frame, enum frame_flag flag);

/* May set FRAME_STATUS_REPAINT */
void
frame_resize(struct frame *frame, int32_t width, int32_t height);

/* May set FRAME_STATUS_REPAINT */
void
frame_resize_inside(struct frame *frame, int32_t width, int32_t height);

int32_t
frame_width(struct frame *frame);

int32_t
frame_height(struct frame *frame);

void
frame_interior(struct frame *frame, int32_t *x, int32_t *y,
	       int32_t *width, int32_t *height);
void
frame_input_rect(struct frame *frame, int32_t *x, int32_t *y,
		 int32_t *width, int32_t *height);
void
frame_opaque_rect(struct frame *frame, int32_t *x, int32_t *y,
		  int32_t *width, int32_t *height);

int
frame_get_shadow_margin(struct frame *frame);

uint32_t
frame_status(struct frame *frame);

void
frame_status_clear(struct frame *frame, enum frame_status status);

/* May set FRAME_STATUS_REPAINT */
enum theme_location
frame_pointer_enter(struct frame *frame, void *pointer, int x, int y);

/* May set FRAME_STATUS_REPAINT */
enum theme_location
frame_pointer_motion(struct frame *frame, void *pointer, int x, int y);

/* May set FRAME_STATUS_REPAINT */
void
frame_pointer_leave(struct frame *frame, void *pointer);

/* Call to indicate that a button has been pressed/released.  The return
 * value for a button release will be the same as for the corresponding
 * press.  This allows you to more easily track grabs.  If you want the
 * actual location, simply keep the location from the last
 * frame_pointer_motion call.
 *
 * May set:
 *	FRAME_STATUS_MINIMIZE
 *	FRAME_STATUS_MAXIMIZE
 *	FRAME_STATUS_CLOSE
 *	FRAME_STATUS_MENU
 *	FRAME_STATUS_RESIZE
 *	FRAME_STATUS_MOVE
 */
enum theme_location
frame_pointer_button(struct frame *frame, void *pointer,
		     uint32_t button, enum frame_button_state state);

void
frame_touch_down(struct frame *frame, void *data, int32_t id, int x, int y);

void
frame_touch_up(struct frame *frame, void *data, int32_t id);

/* May set FRAME_STATUS_REPAINT */
enum theme_location
frame_tablet_motion(struct frame *frame, void *pointer, int x, int y);

void
frame_repaint(struct frame *frame, cairo_t *cr);

#endif
