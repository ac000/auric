/*
 * interface.h
 *
 * Copyright (C) 2012-3013	OpenTech Labs
 *				Andrew Clayton <andrew@digital-domain.net>
 *
 * Released under the GNU General Public License version 3.
 * See COPYING
 */

#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include <gtk/gtk.h>

#include "common.h"

struct vid {
	GtkWidget *window;
	GtkListStore *liststore;
	GtkWidget *treeview;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *col;
};

struct prefs {
	GtkWidget *prefs_window;
	GtkWidget *th0;
	GtkWidget *th_cb0;
	GtkWidget *th1;
	GtkWidget *th_cb1;
	GtkWidget *th2;
	GtkWidget *th_cb2;
	GtkWidget *th3;
	GtkWidget *th_cb3;
	GtkWidget *dp_spin;
};

struct notebook {
	GtkWidget *main_window;
	GtkWidget *text;
	GtkTextBuffer *buffer;
	GtkWidget *image;
	GtkWidget *graph;
	GtkListStore *liststore;
	GtkTreeModel *filter;
	GtkTreeModel *sort;
	GtkWidget *treeview;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *col;
};

struct widgets {
	GtkWidget *main_window;
	GtkWidget *tmpl_error;
	GtkWidget *about;
	GtkWidget *file_chooser;
	GtkListStore *entity_liststore;
	GtkWidget *entity_filter_list;
	GtkWidget *esi_filter_box;
	GtkWidget *esi_operator;
	GtkWidget *esi_entry;
	GtkWidget *esi_ftype;

	struct notebook notebook[NR_TABS];
	struct prefs prefs;
};

void format_cell_value(GtkTreeViewColumn *col,
		       GtkCellRenderer *cell, GtkTreeModel *model,
		       GtkTreeIter *iter, gpointer col_no);
void get_widgets(struct widgets *widgets, GtkBuilder *builder);

#endif /* _INTERFACE_H_ */
