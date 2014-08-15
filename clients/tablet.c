/*
 * Copyright © 2014 Lyude
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE
 */
#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <math.h>
#include <assert.h>

#include <linux/input.h>
#include <wayland-client.h>

#include "window.h"

#define AXIS2DOUBLE(a) (wl_fixed_to_double(a)/65535.0)

struct tablet_view {
	struct display *display;
	struct window *window;
	struct widget *widget;

	int cursor;

	cairo_surface_t *buffer;

	enum {
		TABLET_TOOL_UP = 0,
		TABLET_TOOL_DOWN = 1,
	} tablet_contact_status;

	struct {
		int32_t x, y;
	} dot;

	struct {
		int32_t x, y;
		int32_t old_x, old_y;
		float w;
	} line;

	int reset;
};

static void
draw_line(struct tablet_view *tablet_view, cairo_t *cr,
	  struct rectangle *allocation)
{
	cairo_t *bcr;
	cairo_surface_t *tmp_buffer = NULL;

	if (tablet_view->reset) {
		tmp_buffer = tablet_view->buffer;
		tablet_view->buffer = NULL;
		tablet_view->line.x = -1;
		tablet_view->line.y = -1;
		tablet_view->line.old_x = -1;
		tablet_view->line.old_y = -1;
		tablet_view->reset = 0;
	}

	if (tablet_view->buffer == NULL) {
		tablet_view->buffer =
			cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
						   allocation->width,
						   allocation->height);
		bcr = cairo_create(tablet_view->buffer);
		cairo_set_source_rgba(bcr, 0, 0, 0, 0);
		cairo_rectangle(bcr,
				0, 0,
				allocation->width, allocation->height);
		cairo_fill(bcr);
	}
	else
		bcr = cairo_create(tablet_view->buffer);

	if (tmp_buffer) {
		cairo_set_source_surface(bcr, tmp_buffer, 0, 0);
		cairo_rectangle(bcr, 0, 0,
				allocation->width, allocation->height);
		cairo_clip(bcr);
		cairo_paint(bcr);

		cairo_surface_destroy(tmp_buffer);
	}

	if (tablet_view->line.x != -1 && tablet_view->line.y != -1) {
		if (tablet_view->line.old_x != -1 &&
		    tablet_view->line.old_y != -1) {
			cairo_set_line_width(bcr, 4.0*tablet_view->line.w);
			cairo_set_source_rgb(bcr, 1, 1, 1);
			cairo_translate(bcr,
					-allocation->x, -allocation->y);

			cairo_move_to(bcr,
				      tablet_view->line.old_x,
				      tablet_view->line.old_y);
			cairo_line_to(bcr,
				      tablet_view->line.x,
				      tablet_view->line.y);

			cairo_stroke(bcr);
		}

		tablet_view->line.old_x = tablet_view->line.x;
		tablet_view->line.old_y = tablet_view->line.y;
	}
	cairo_destroy(bcr);

	cairo_set_source_surface(cr, tablet_view->buffer,
				 allocation->x, allocation->y);
	cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
	cairo_rectangle(cr,
			allocation->x, allocation->y,
			allocation->width, allocation->height);
	cairo_clip(cr);
	cairo_paint(cr);
}

static void
redraw_handler(struct widget *widget, void *data)
{
	static const double r = 10.0;
	struct tablet_view *tablet_view = data;
	cairo_surface_t *surface;
	cairo_t *cr;
	struct rectangle allocation;

	widget_get_allocation(tablet_view->widget, &allocation);

	surface = window_get_surface(tablet_view->window);

	cr = cairo_create(surface);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr,
			allocation.x,
			allocation.y,
			allocation.width,
			allocation.height);
	cairo_set_source_rgba(cr, 0, 0, 0, 0.8);
	cairo_fill(cr);

	draw_line(tablet_view, cr, &allocation);

	cairo_translate(cr, tablet_view->dot.x + 0.5, tablet_view->dot.y + 0.5);
	cairo_set_line_width(cr, 1.0);
	cairo_set_source_rgb(cr, 0.1, 0.9, 0.9);

	cairo_move_to(cr, 0.0, -r);
	cairo_line_to(cr, 0.0, r);
	cairo_move_to(cr, -r, 0.0);
	cairo_line_to(cr, r, 0.0);
	cairo_stroke(cr);

	cairo_set_source_rgba(cr, 0.9, 0.1, 0.1, tablet_view->line.w);
	cairo_arc(cr, 0.0, 0.0, r, 0.0, 2.0 * M_PI);
	cairo_fill(cr);
	cairo_set_source_rgb(cr, 0.9, 0.1, 0.1);
	cairo_arc(cr, 0.0, 0.0, r, 0.0, 2.0 * M_PI);
	cairo_stroke(cr);

	cairo_destroy(cr);

	cairo_surface_destroy(surface);
}

static void
keyboard_focus_handler(struct window *window,
		       struct input *device, void *data)
{
	struct tablet_view *tablet_view = data;

	window_schedule_redraw(tablet_view->window);
}

static void
key_handler(struct window *window, struct input *input, uint32_t time,
	    uint32_t key, uint32_t sym,
	    enum wl_keyboard_key_state state, void *data)
{
	struct tablet_view *tablet_view = data;

	if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
		return;

	switch (sym) {
	case XKB_KEY_Escape:
		display_exit(tablet_view->display);
		break;
	}
}

static void
button_handler(struct widget *widget, struct tablet *tablet, uint32_t button,
	       enum wl_tablet_button_state state, uint32_t time, void *data)
{
	struct tablet_view *tablet_view = data;

	if (state == WL_TABLET_BUTTON_STATE_PRESSED && button == BTN_STYLUS)
		tablet_get_position(tablet,
				    &tablet_view->dot.x, &tablet_view->dot.y);

	widget_schedule_redraw(widget);
}

static int
motion_handler(struct widget *widget, struct tablet *tablet, float x, float y,
	       uint32_t time, void *data)
{
	struct tablet_view *tablet_view = data;

	if (tablet_view->tablet_contact_status == TABLET_TOOL_DOWN) {
		tablet_view->line.x = x;
		tablet_view->line.y = y;

		window_schedule_redraw(tablet_view->window);
	}

	return tablet_view->cursor;
}

static void
pressure_handler(struct widget *widget, struct tablet *tablet, uint32_t time,
		 wl_fixed_t pressure, void *data)
{
	struct tablet_view *tablet_view = data;

	if (tablet_view->tablet_contact_status == TABLET_TOOL_DOWN) {
		tablet_view->line.w = AXIS2DOUBLE(pressure);

		window_schedule_redraw(tablet_view->window);
	}
}

static void
resize_handler(struct widget *widget,
	       int32_t width, int32_t height,
	       void *data)
{
	struct tablet_view *tablet_view = data;

	tablet_view->reset = 1;
}

static void
down_handler(struct widget *widget, struct tablet *tablet, uint32_t time,
	     void *data)
{
	struct tablet_view *tablet_view = data;

	tablet_view->tablet_contact_status = TABLET_TOOL_DOWN;
}

static void
up_handler(struct widget *widget, struct tablet *tablet, uint32_t time,
	   void *data)
{
	struct tablet_view *tablet_view = data;

	tablet_view->tablet_contact_status = TABLET_TOOL_UP;
}

static void
proximity_in_handler(struct widget *widget, struct tablet *tablet,
		     struct tablet_tool *tool, void *data)
{
	struct tablet_view *tablet_view = data;

	if (tablet_tool_get_type(tool) == WL_TABLET_TOOL_TYPE_PEN)
		tablet_view->cursor = CURSOR_BLANK;
	else
		tablet_view->cursor = CURSOR_HAND1;
}

static struct tablet_view *
tablet_view_create(struct display *display)
{
	struct tablet_view *tablet_view;

	tablet_view = xzalloc(sizeof *tablet_view);
	tablet_view->window = window_create(display);
	tablet_view->widget = window_frame_create(tablet_view->window, tablet_view);
	window_set_title(tablet_view->window, "Wayland Tablet");
	tablet_view->display = display;
	tablet_view->buffer = NULL;

	window_set_key_handler(tablet_view->window, key_handler);
	window_set_user_data(tablet_view->window, tablet_view);
	window_set_keyboard_focus_handler(tablet_view->window,
					  keyboard_focus_handler);

	widget_set_redraw_handler(tablet_view->widget, redraw_handler);
	widget_set_tablet_button_handler(tablet_view->widget, button_handler);
	widget_set_tablet_motion_handler(tablet_view->widget, motion_handler);
	widget_set_resize_handler(tablet_view->widget, resize_handler);
	widget_set_tablet_down_handler(tablet_view->widget, down_handler);
	widget_set_tablet_up_handler(tablet_view->widget, up_handler);
	widget_set_tablet_proximity_in_handler(tablet_view->widget,
					       proximity_in_handler);
	widget_set_tablet_pressure_handler(tablet_view->widget, pressure_handler);


	widget_schedule_resize(tablet_view->widget, 1000, 800);
	tablet_view->dot.x = 250;
	tablet_view->dot.y = 200;
	tablet_view->line.x = -1;
	tablet_view->line.y = -1;
	tablet_view->line.old_x = -1;
	tablet_view->line.old_y = -1;
	tablet_view->reset = 0;

	return tablet_view;
}

static void
tablet_view_destroy(struct tablet_view *tablet_view)
{
	if (tablet_view->buffer)
		cairo_surface_destroy(tablet_view->buffer);
	widget_destroy(tablet_view->widget);
	window_destroy(tablet_view->window);
	free(tablet_view);
}

int
main(int argc, char *argv[])
{
	struct display *display;
	struct tablet_view *tablet_view;

	display = display_create(&argc, argv);
	if (display == NULL) {
		fprintf(stderr, "failed to create display: %m\n");
		return -1;
	}

	tablet_view = tablet_view_create(display);

	display_run(display);

	tablet_view_destroy(tablet_view);
	display_destroy(display);

	return 0;
}
