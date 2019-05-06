/*
 * interface.c - Gather up the widgets needed to interact with
 *
 * Copyright (C) 2012		OpenTech Labs
 *				Andrew Clayton <andrew@digital-domain.net>
 *
 * Released under the GNU General Public License version 3.
 * See COPYING
 */

#define _GNU_SOURCE

#include <glib.h>
#include <gtk/gtk.h>

#include "interface.h"
#include "common.h"

static void create_tags(struct widgets *widgets)
{
	gtk_text_buffer_create_tag(widgets->notebook[HI_TAB].buffer,
			"hi0", "background", "#0000ff", NULL);
	gtk_text_buffer_create_tag(widgets->notebook[HI_TAB].buffer,
			"hi1", "background", "#0077ff", NULL);
	gtk_text_buffer_create_tag(widgets->notebook[HI_TAB].buffer,
			"hi2", "background", "#00ffff", NULL);
	gtk_text_buffer_create_tag(widgets->notebook[HI_TAB].buffer,
			"hi3", "background", "#00ff00", NULL);
	gtk_text_buffer_create_tag(widgets->notebook[HI_TAB].buffer,
			"hi4", "background", "#e6ff00", NULL);
	gtk_text_buffer_create_tag(widgets->notebook[HI_TAB].buffer,
			"hi5", "background", "#ff7700", NULL);
	gtk_text_buffer_create_tag(widgets->notebook[HI_TAB].buffer,
			"hi6", "background", "#ff0000", NULL);
}

void format_cell_value(GtkTreeViewColumn *col __unused,
		       GtkCellRenderer *cell, GtkTreeModel *model,
		       GtkTreeIter *iter, gpointer col_no)
{
	gdouble value;
	char *buf;

	gtk_tree_model_get(model, iter, GPOINTER_TO_INT(col_no), &value, -1);
	buf = g_strdup_printf(val_fmt, value);
	g_object_set(cell, "text", buf, NULL);
	g_free(buf);
}

void  get_widgets(struct widgets *widgets, GtkBuilder *builder)
{
	widgets->main_window = GTK_WIDGET(gtk_builder_get_object(builder,
				"main_window"));
	widgets->about = GTK_WIDGET(gtk_builder_get_object(builder,
				"about_auric"));
	widgets->prefs.prefs_window = GTK_WIDGET(gtk_builder_get_object(
				builder, "prefs_window"));
	widgets->entity_liststore = GTK_LIST_STORE(gtk_builder_get_object(
				builder, "entity_liststore"));
	widgets->entity_filter_list = GTK_WIDGET(gtk_builder_get_object(
				builder, "entity_filter_list"));
	widgets->esi_filter_box = GTK_WIDGET(gtk_builder_get_object(builder,
				"esi_filter_box"));
	widgets->esi_operator = GTK_WIDGET(gtk_builder_get_object(builder,
				"esi_operator"));
	widgets->esi_entry = GTK_WIDGET(gtk_builder_get_object(builder,
				"esi_entry"));
	widgets->esi_ftype = GTK_WIDGET(gtk_builder_get_object(builder,
				"esi_ftype"));
	widgets->file_chooser = GTK_WIDGET(gtk_builder_get_object(builder,
				"file_chooser"));
	widgets->notebook[0].text = GTK_WIDGET(gtk_builder_get_object(builder,
				"summary_1"));
	widgets->notebook[0].buffer = GTK_TEXT_BUFFER(gtk_builder_get_object(
				builder, "s_buffer_1"));
	widgets->notebook[0].graph = GTK_WIDGET(gtk_builder_get_object(builder,
				"graph_1"));
	widgets->notebook[1].text = GTK_WIDGET(gtk_builder_get_object(builder,
				"summary_2"));
	widgets->notebook[1].buffer = GTK_TEXT_BUFFER(gtk_builder_get_object(
				builder, "s_buffer_2"));
	widgets->notebook[1].graph = GTK_WIDGET(gtk_builder_get_object(builder,
				"graph_2"));
	widgets->notebook[2].text = GTK_WIDGET(gtk_builder_get_object(builder,
				"summary_3"));
	widgets->notebook[2].buffer = GTK_TEXT_BUFFER(gtk_builder_get_object(
				builder, "s_buffer_3"));
	widgets->notebook[2].graph = GTK_WIDGET(gtk_builder_get_object(builder,
				"graph_3"));
	widgets->notebook[CENT_TAB].text = GTK_WIDGET(gtk_builder_get_object(
				builder, "cent_summary"));
	widgets->notebook[CENT_TAB].buffer = GTK_TEXT_BUFFER(
			gtk_builder_get_object(builder, "cent_buffer"));
	widgets->notebook[CENT_TAB].graph = GTK_WIDGET(gtk_builder_get_object(
				builder, "cent_graph"));
	widgets->notebook[RV_TAB].text = GTK_WIDGET(gtk_builder_get_object(
				builder, "rv_summary"));
	widgets->notebook[RV_TAB].buffer = GTK_TEXT_BUFFER(
			gtk_builder_get_object(builder, "rv_buffer"));
	widgets->notebook[RV_TAB].graph = GTK_WIDGET(gtk_builder_get_object(
				builder, "rv_graph"));
	widgets->notebook[HI_TAB].text = GTK_WIDGET(gtk_builder_get_object(
				builder, "histogram_legend"));
	widgets->notebook[HI_TAB].buffer = GTK_TEXT_BUFFER(
			gtk_builder_get_object(builder, "hi_buffer"));
	widgets->notebook[HI_TAB].graph = GTK_WIDGET(gtk_builder_get_object(
				builder, "histogram_graph"));
	widgets->notebook[RI_TAB].liststore = GTK_LIST_STORE(
			gtk_builder_get_object(builder, "ri_liststore"));
	widgets->notebook[RI_TAB].treeview = GTK_WIDGET(
			gtk_builder_get_object(builder, "ri_treeview"));
	gtk_widget_set_name(GTK_WIDGET(widgets->notebook[RI_TAB].treeview),
			"ri_treeview");
	widgets->notebook[RI_TAB].col = GTK_TREE_VIEW_COLUMN(
			gtk_builder_get_object(builder,
				"ri_value"));
	widgets->notebook[RI_TAB].cell = GTK_CELL_RENDERER(
			gtk_builder_get_object(builder,
				"ri_v_text"));
	gtk_tree_view_column_set_cell_data_func(widgets->notebook[RI_TAB].col,
			widgets->notebook[RI_TAB].cell, format_cell_value,
			GINT_TO_POINTER(3), NULL);
	widgets->notebook[ESI_TAB].liststore = GTK_LIST_STORE(
			gtk_builder_get_object(builder, "esi_liststore"));
	widgets->notebook[ESI_TAB].treeview = GTK_WIDGET(
			gtk_builder_get_object(builder, "esi_treeview"));
	gtk_widget_set_name(GTK_WIDGET(widgets->notebook[ESI_TAB].treeview),
			"esi_treeview");
	widgets->notebook[ESI_TAB].filter = GTK_TREE_MODEL(
			gtk_builder_get_object(builder, "inv_filter"));
	gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(
				widgets->notebook[ESI_TAB].filter),
			3);
	widgets->notebook[ESI_TAB].sort = GTK_TREE_MODEL(
			gtk_builder_get_object(builder, "inv_sort"));
	widgets->notebook[ESI_TAB].col = GTK_TREE_VIEW_COLUMN(
			gtk_builder_get_object(builder,
				"spend"));
	widgets->notebook[ESI_TAB].cell = GTK_CELL_RENDERER(
			gtk_builder_get_object(builder,
				"s_text"));
	gtk_tree_view_column_set_cell_data_func(widgets->notebook[ESI_TAB].col,
			widgets->notebook[ESI_TAB].cell, format_cell_value,
			GINT_TO_POINTER(1), NULL);
	widgets->prefs.th_cb0 = GTK_WIDGET(gtk_builder_get_object(
				builder, "th_cb0"));
	gtk_widget_set_name(widgets->prefs.th_cb0, "th_cb0");
	widgets->prefs.th0 = GTK_WIDGET(gtk_builder_get_object(
				builder, "th0"));
	widgets->prefs.th_cb1 = GTK_WIDGET(gtk_builder_get_object(
				builder, "th_cb1"));
	gtk_widget_set_name(widgets->prefs.th_cb1, "th_cb1");
	widgets->prefs.th1 = GTK_WIDGET(gtk_builder_get_object(
				builder, "th1"));
	widgets->prefs.th_cb2 = GTK_WIDGET(gtk_builder_get_object(
				builder, "th_cb2"));
	gtk_widget_set_name(widgets->prefs.th_cb2, "th_cb2");
	widgets->prefs.th2 = GTK_WIDGET(gtk_builder_get_object(
				builder, "th2"));
	widgets->prefs.th_cb3 = GTK_WIDGET(gtk_builder_get_object(
				builder, "th_cb3"));
	gtk_widget_set_name(widgets->prefs.th_cb3, "th_cb3");
	widgets->prefs.th3 = GTK_WIDGET(gtk_builder_get_object(
				builder, "th3"));
	widgets->prefs.dp_spin = GTK_WIDGET(gtk_builder_get_object(
				builder, "dp_spin"));
	widgets->tmpl_error = GTK_WIDGET(gtk_builder_get_object(builder,
				"tmpl_error_dialog"));

	create_tags(widgets);
}
