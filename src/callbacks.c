/*
 * callbacks.c - GTK Callback functions
 *
 * Copyright (C) 2012		OpenTech Labs
 * 				Andrew Clayton <andrew@opentechlabs.co.uk>
 *
 * Released under the GNU General Public License version 3.
 * See COPYING
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>

#include <glib.h>

#include <gtk/gtk.h>

#include "common.h"
#include "interface.h"
#include "auric.h"

unsigned char thresholds;
char val_fmt[5];

static gboolean show_entity_row(GtkTreeModel *model, GtkTreePath *path,
				GtkTreeIter *iter, struct widgets *widgets)
{
	double value;
	unsigned int invoices;
	double spend;
	const char *type;
	char *operator;

	value = strtod(gtk_entry_get_text(GTK_ENTRY(widgets->esi_entry)),
			NULL);
	type = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(
				widgets->esi_ftype));
	operator = gtk_combo_box_text_get_active_text(
			GTK_COMBO_BOX_TEXT(widgets->esi_operator));
	gtk_tree_model_get(model, iter, 1, &spend, -1);
	gtk_tree_model_get(model, iter, 2, &invoices, -1);

	if (value == 0 || !type || !operator)
		goto out_true;
	if (*operator == '>') {
		if (strcmp(type, "invoices") == 0) {
			if (invoices > value)
				goto out_true;
			else
				goto out_false;
		} else if (strcmp(type, "spend") == 0) {
			if (spend > value)
				goto out_true;
			else
				goto out_false;
		}
	} else if (*operator == '<') {
		if (strcmp(type, "invoices") == 0) {
			if (invoices < value)
				goto out_true;
			else
				goto out_false;
		} else if (strcmp(type, "spend") == 0) {
			if (spend < value)
				goto out_true;
			else
				goto out_false;
		}
	}

out_true:
	gtk_list_store_set(widgets->notebook[ESI_TAB].liststore, iter, 3, TRUE,
			-1);
	goto out;
out_false:
	gtk_list_store_set(widgets->notebook[ESI_TAB].liststore, iter, 3,
			FALSE, -1);
	goto out;
out:
	return FALSE;
}

void cb_quit(void)
{
	gtk_main_quit();
}

void cb_ri_vid_close(GtkWidget *button, struct ri_vid *ri_vid)
{
	gtk_widget_destroy(ri_vid->window);
	g_slice_free(struct ri_vid, ri_vid);
}

void cb_reset_prefs_config(GtkWidget *button, struct widgets *widgets)
{
	load_prefs(widgets);
}

void cb_reset_prefs_default(GtkWidget *button, struct widgets *widgets)
{
	set_def_prefs(widgets);
}

void cb_save_prefs(GtkWidget *button, struct widgets *widgets)
{
	char config_dir[PATH_MAX];
	const char prefs[] = "prefs";
	const char prefs_tmp[] = ".prefs.tmp";
	int dirfd;
	int fd;
	FILE *fp;

	snprintf(config_dir, sizeof(config_dir) - strlen(prefs),
			"%s/.config/auric/", getenv("HOME"));
	mkdir_p(config_dir);

	dirfd = open(config_dir, O_RDONLY);
	fd = openat(dirfd, prefs_tmp, O_CREAT | O_WRONLY | O_TRUNC, 0666);
	fp = fdopen(fd, "w");

	fprintf(fp, "th_cb0=%d\n", (thresholds & THRESH_0) ? 1 : 0);
	fprintf(fp, "th_cb1=%d\n", (thresholds & THRESH_1) ? 1 : 0);
	fprintf(fp, "th_cb2=%d\n", (thresholds & THRESH_2) ? 1 : 0);
	fprintf(fp, "th_cb3=%d\n", (thresholds & THRESH_3) ? 1 : 0);
	fprintf(fp, "th0=%.2f\n", strtod(gtk_entry_get_text(GTK_ENTRY(
					widgets->prefs.th0)), NULL));
	fprintf(fp, "th1=%.2f\n", strtod(gtk_entry_get_text(GTK_ENTRY(
					widgets->prefs.th1)), NULL));
	fprintf(fp, "th2=%.2f\n", strtod(gtk_entry_get_text(GTK_ENTRY(
					widgets->prefs.th2)), NULL));
	fprintf(fp, "th3=%.2f\n", strtod(gtk_entry_get_text(GTK_ENTRY(
					widgets->prefs.th3)), NULL));
	fprintf(fp, "dp=%g\n", gtk_spin_button_get_value(
				GTK_SPIN_BUTTON(widgets->prefs.dp_spin)));

	fclose(fp);
	renameat(dirfd, prefs_tmp, dirfd, prefs);
	close(dirfd);
}

void cb_hide_prefs_window(GtkWidget *close_button, struct widgets *widgets)
{
	gtk_widget_hide(widgets->prefs.prefs_window);
}

void cb_display_prefs_window(GtkWidget *prefs_button, struct widgets *widgets)
{
	gtk_widget_show(widgets->prefs.prefs_window);
}

void cb_select_file(GtkWidget *file_chooser, struct widgets *widgets)
{
	const char *file;
	char *window_title;

	file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
				widgets->file_chooser));
	if (!file)
		return;

	window_title = g_strdup_printf("auric (%s)", file);
	gtk_window_set_title(GTK_WINDOW(widgets->main_window), window_title);
	g_free(window_title);

	gtk_combo_box_set_active_id(GTK_COMBO_BOX(
				widgets->entity_filter_list), 0);
}

void cb_toggle_cb(GtkWidget *cb, struct widgets *widgets)
{
	set_prefs(widgets);
}

void cb_dp_spin(GtkWidget *spin)
{
	snprintf(val_fmt, sizeof(val_fmt), "%%.%df",
			gtk_spin_button_get_value_as_int(
				GTK_SPIN_BUTTON(spin)));
}

void cb_switch_tab(GtkNotebook *notebook, GtkWidget *page, guint page_num,
		   struct widgets *widgets)
{
	if (page_num == ESI_TAB)
		gtk_widget_show(widgets->esi_filter_box);
	else
		gtk_widget_hide(widgets->esi_filter_box);
}

void cb_display_entity(GtkEntry *entry, struct widgets *widgets)
{
	const char *type;

	gtk_tree_model_foreach(GTK_TREE_MODEL(
				widgets->notebook[ESI_TAB].liststore),
			(GtkTreeModelForeachFunc)show_entity_row,
			widgets);
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(
				widgets->notebook[ESI_TAB].filter));

	/*
	 * Order the list either by the number of invoices or the
	 * spend amount.
	 */
	type = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(
				widgets->esi_ftype));
	if (!type)
		return;
	gtk_tree_sortable_set_sort_column_id(
			GTK_TREE_SORTABLE(widgets->notebook[ESI_TAB].sort),
			(strcmp(type, "invoices") == 0) ? 2 : 1,
			GTK_SORT_DESCENDING);
}

void cb_display_entity_clear(GtkEntry *entry, GtkEntryIconPosition icon_pos,
			     GdkEvent *event,  struct widgets *widgets)
{
	gtk_entry_set_text(GTK_ENTRY(widgets->esi_entry), "");
	cb_display_entity(entry, widgets);
}

void cb_toggle_esi(GtkWidget *widget, struct widgets *widgets)
{
	const char *entity = gtk_combo_box_get_active_id(GTK_COMBO_BOX(
				widgets->entity_filter_list));

	if (!entity)
		gtk_widget_set_sensitive(widgets->esi_filter_box, TRUE);
	else
		gtk_widget_set_sensitive(widgets->esi_filter_box, FALSE);
}

void cb_ri_row(GtkTreeView *treeview, GtkTreePath *path,
	       GtkTreeViewColumn *col, struct widgets *widgets)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	model = gtk_tree_view_get_model(treeview);
	if (gtk_tree_model_get_iter(model, &iter, path)) {
		char *invoice;

		gtk_tree_model_get(model, &iter, 0, &invoice, -1);
		view_invoice_details(invoice);
		free(invoice);
	}
}

void cb_spawn(void)
{
	pid_t pid;
	struct sigaction sa;

	sa.sa_handler = SIG_DFL;
	sa.sa_flags = SA_NOCLDWAIT;
	sigaction(SIGCHLD, &sa, NULL);

	pid = fork();
	if (pid == 0) {
		if (access("./auric", X_OK) == -1)
			execlp("auric", "auric", (char *)NULL);
		else
			execlp("./auric", "auric", (char *)NULL);
	}
}

void cb_run(GtkWidget *button, struct widgets *widgets)
{
	runit(button, widgets);
}
