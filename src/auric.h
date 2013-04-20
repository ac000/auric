/*
 * auric.h
 *
 * Copyright (C) 2012		OpenTech Labs
 * 				Andrew Clayton <andrew@digital-domain.net>
 *
 * Released under the GNU General Public License version 3.
 * See COPYING
 */

#ifndef _AURIC_H_
#define _AURIC_H_

#include <stdbool.h>

#include <gtk/gtk.h>

#include "interface.h"

bool mkdir_p(const char *path);
void load_prefs(struct widgets *widgets);
void set_def_prefs(struct widgets *widgets);
void set_prefs(struct widgets *widgets);
void view_invoice_details(const char *search, const char *what);
void runit(GtkWidget *widget, struct widgets *widgets);

#endif /* _AURIC_H_ */
