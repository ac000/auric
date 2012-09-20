/*
 * auric.c - Financial Data Analyser
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
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <monetary.h>

#include <tcutil.h>
#include <tctdb.h>
#include <libgen.h>

#include <glib.h>

#include <gtk/gtk.h>

#include <cairo.h>

#include "common.h"
#include "interface.h"
#include "auric.h"

static const int x = 450;
static const int y = 450;
static const double width = 40.0;

static cairo_surface_t *surface;
static cairo_t *cr;

static double b[10][NR_POS];
static double p[10][NR_POS];
static int d[11][NR_POS];

static double thr_limits[NR_THRESHOLDS];
static int thr_amnts[NR_THRESHOLDS + 1];

static double total;
static double ll;
static double sl;
static double s;

static GArray *values;

struct repeat_values {
	int nr;
	double value;
} repeat_values;

char tct_db[PATH_MAX];

bool mkdir_p(const char *path)
{
	char *dir;
	char *token;
	char mdir[PATH_MAX] = "/";

	if (strlen(path) >= PATH_MAX)
		return false;

	dir = strdup(path);
	token = strtok(dir, "/");
	while (token != NULL) {
		strcat(mdir, token);
		mkdir(mdir, 0777);
		strcat(mdir, "/");
		token = strtok(NULL, "/");
	}
	free(dir);

	return true;
}

/*
 * See: http://en.wikipedia.org/wiki/Benford's_law
 *	http://www.journalofaccountancy.com/Issues/1999/May/nigrini
 *
 * Returns a value between 0..1
 */
static double benford(int digit, int pos)
{
	double e;
	double i;
	double prop = 0.0;

	if (digit == 0 && pos == 1)
		return -1.0;
	if (pos == 1)
		return log10(1 + (1.0 / digit));

	e = pow(10.0, (double)pos - 1);
	for (i = e; i < e * 10.0; i += 10.0)
		prop += log10(1 + 1/(digit + i));

	return prop;
}

static void populate_b(void)
{
	int i;
	int j;

	for (i = 0; i < 10; i++) {
		for (j = 0; j < NR_POS; j++)
			b[i][j] = benford(i, j + 1) * 100;
	}
}

static int cmp_values(const double *a, const double *b)
{
	if (*a < *b)
		return -1;
	else if (*a > *b)
		return 1;
	else
		return 0;
}

static int cmp_repeat_values(const struct repeat_values *a,
			     const struct repeat_values *b)
{
	if (a->nr < b->nr)
		return -1;
	else if (a->nr > b->nr)
		return 1;
	else
		return 0;
}

static void blank_image(void)
{
	/* Set background of image to white */
	cairo_rectangle(cr, 0.0, 0.0, x, y);
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_fill(cr);
}

static void init_image(struct widgets *widgets, int tab)
{
	GdkPixbuf *pixbuf;

	/* Some tab's doesn't have an image part */
	if (tab == RI_TAB || tab == ESI_TAB)
		return;

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, x, y);
	cr = cairo_create(surface);

	blank_image();

	pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, x, y);
	widgets->notebook[tab].image = gtk_image_new_from_pixbuf(pixbuf);
	gtk_container_add(GTK_CONTAINER(widgets->notebook[tab].graph),
			widgets->notebook[tab].image);
	gtk_widget_show(widgets->notebook[tab].image);

	g_object_unref(pixbuf);
}

static void init_graph(int tab)
{
	int i;
	int adj = width;
	char x_a[2];
	char y_a[10];

	blank_image();

	/* Draw axis */
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_set_line_width(cr, 1.5);

	cairo_move_to(cr, width * 0.5 + 10, y * 0.1);
	cairo_line_to(cr, width * 0.5 + 10, y * 0.9 + 25);
	cairo_stroke(cr);

	cairo_move_to(cr, width * 0.5 - 10, y * 0.9 + 10);
	cairo_line_to(cr, width * 10 + 20, y * 0.9 + 10);
	cairo_stroke(cr);

	/* Draw X-axis scale */
	if (tab == 0) {
		i = 1;
		adj = 0;
	} else {
		i = 0;
	}
	for ( ; i < 10; i++) {
		cairo_move_to(cr, width * i + adj, y * 0.96);
		snprintf(x_a, sizeof(x_a), "%d", i);
		cairo_show_text(cr, x_a);
	}

	/* Draw Y-axis scale */
	if (tab == 0)
		i = 1;
	else
		i = 0;

	if (tab == 0) {
		for ( ; i < 10; i++) {
			cairo_move_to(cr, 3, y - b[i][tab] * 10);
			snprintf(y_a, sizeof(y_a), "%4.1f", b[i][tab]);
			cairo_show_text(cr, y_a);
		}
	}

	/* Draw benford legend */
	cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
	cairo_move_to(cr, width * 6, y * 0.1);
	cairo_line_to(cr, width * 6.7, y * 0.1);
	cairo_stroke(cr);
	cairo_move_to(cr, width * 7, y * 0.1);
	cairo_show_text(cr, "Benford");

	/* Draw data legend */
	cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
	cairo_move_to(cr, width * 6, y * 0.14);
	cairo_line_to(cr, width * 6.7, y * 0.14);
	cairo_stroke(cr);
	cairo_move_to(cr, width * 7, y * 0.14);
	cairo_show_text(cr, "Datum");

	cairo_stroke(cr);
}

static void de_init_image(void)
{
	cairo_destroy(cr);
	cairo_surface_destroy(surface);
}

static void draw_curve(const char *who, double data[][NR_POS], int tab)
{
	int i = 0;
	int adj = 0;

	if (strcmp(who, "benford") == 0)
		cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
	else
		cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);

	cairo_set_line_width(cr, 1.0);

	if (tab == 0)
		i = 1;
	else
		adj = width;

	cairo_move_to(cr, width, y - data[i][tab] * 10);
	i++;
	for ( ; i < 10; i++)
		cairo_line_to(cr, width * i + adj, y - data[i][tab] * 10);

	cairo_stroke(cr);
}

static void draw_pie_chart(void)
{
	double deg;
	double circ = 0.0;
	char legend[24];

	blank_image();

	cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
        cairo_move_to(cr, 5.0, 20.0);
	snprintf(legend, sizeof(legend), "> %10.2f",
			thr_limits[NR_THRESHOLDS - 4]);
        cairo_show_text(cr, legend);

	if (!(thresholds & THRESH_0))
		goto draw_chart;
	cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
	cairo_move_to(cr, 5.0, 35.0);
	snprintf(legend, sizeof(legend), "<= %10.2f",
			thr_limits[NR_THRESHOLDS - 4]);
	cairo_show_text(cr, legend);

	if (!(thresholds & THRESH_1))
		goto draw_chart;
	cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
	cairo_move_to(cr, 5.0, 50.0);
	snprintf(legend, sizeof(legend), "<= %10.2f",
			thr_limits[NR_THRESHOLDS - 3]);
	cairo_show_text(cr, legend);

	if (!(thresholds & THRESH_2))
		goto draw_chart;
	cairo_set_source_rgb(cr, 0.0, 1.0, 1.0);
	cairo_move_to(cr, 5.0, 65.0);
	snprintf(legend, sizeof(legend), "<= %10.2f",
			thr_limits[NR_THRESHOLDS - 2]);
	cairo_show_text(cr, "<=   100.00");

	if (!(thresholds & THRESH_3))
		goto draw_chart;
	cairo_set_source_rgb(cr, 1.0, 0.0, 1.0);
	cairo_move_to(cr, 5.0, 80.0);
	snprintf(legend, sizeof(legend), "<= %10.2f",
			thr_limits[NR_THRESHOLDS - 1]);
	cairo_show_text(cr, legend);

draw_chart:
	cairo_set_line_width(cr, 2.0);

	cairo_new_sub_path(cr);
	cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
	deg = ((double)thr_amnts[NR_THRESHOLDS - 4]/d[10][0]) * 360.0;
	cairo_arc(cr, x >> 1, y >> 1, x * 0.40, circ * (M_PI/180.0),
			(circ+deg) * (M_PI/180.0));
	cairo_line_to(cr, x >> 1, y >> 1);
	cairo_close_path(cr);
	cairo_fill(cr);
	cairo_stroke(cr);
	circ += deg;

	cairo_new_sub_path(cr);
	cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
	deg = ((double)thr_amnts[NR_THRESHOLDS - 3]/d[10][0]) * 360.0;
	cairo_arc(cr, x >> 1, y >> 1, x * 0.40, circ * (M_PI/180.0),
			(circ+deg) * (M_PI/180.0));
	cairo_line_to(cr, x >> 1, y >> 1);
	cairo_close_path(cr);
	cairo_fill(cr);
	cairo_stroke(cr);
	circ += deg;

	cairo_new_sub_path(cr);
	cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
	deg = ((double)thr_amnts[NR_THRESHOLDS - 2]/d[10][0]) * 360.0;
	cairo_arc(cr, x >> 1, y >> 1, x * 0.40, circ * (M_PI/180.0),
			(circ+deg) * (M_PI/180.0));
	cairo_line_to(cr, x >> 1, y >> 1);
	cairo_close_path(cr);
	cairo_fill(cr);
	cairo_stroke(cr);
	circ += deg;

	cairo_new_sub_path(cr);
	cairo_set_source_rgb(cr, 0.0, 1.0, 1.0);
	deg = ((double)thr_amnts[NR_THRESHOLDS - 1]/d[10][0]) * 360.0;
	cairo_arc(cr, x >> 1, y >> 1, x * 0.40, circ * (M_PI/180.0),
			(circ+deg) * (M_PI/180.0));
	cairo_line_to(cr, x >> 1, y >> 1);
	cairo_close_path(cr);
	cairo_fill(cr);
	cairo_stroke(cr);
	circ += deg;

	cairo_new_sub_path(cr);
	cairo_set_source_rgb(cr, 1.0, 0.0, 1.0);
	deg = ((double)thr_amnts[NR_THRESHOLDS]/d[10][0]) * 360.0;
	cairo_arc(cr, x >> 1, y >> 1, x * 0.40, circ * (M_PI/180.0),
			(circ+deg) * (M_PI/180.0));
	cairo_line_to(cr, x >> 1, y >> 1);
	cairo_close_path(cr);
	cairo_fill(cr);
	cairo_stroke(cr);
}

static void display_summary(struct widgets *widgets, int tab)
{
	int i = 0;
	char line[LINE_MAX];
	char monetary[32];
	PangoFontDescription *font_desc;

	font_desc = pango_font_description_from_string("Monospace");
	gtk_widget_override_font(widgets->notebook[tab].text, font_desc);
	pango_font_description_free(font_desc);

	strfmon(monetary, sizeof(monetary), "%!n", total);
	snprintf(line, sizeof(line), "Total of %s over %d items\n\n",
			monetary, d[10][0]);
	gtk_text_buffer_set_text(widgets->notebook[tab].buffer, line, -1);

	/* When viewing the 1st digit proportions, we skip 0 */
	if (tab == 0)
		i = 1;
	for ( ; i < 10; i++) {
		p[i][tab] = (double)d[i][tab] / d[10][tab] * 100;
		snprintf(line, sizeof(line), "\t%d : %10d\t%5.1f%%\t%5.1f\n",
				i, d[i][tab], p[i][tab],
				p[i][tab] - b[i][tab]);
		gtk_text_buffer_insert_at_cursor(
				widgets->notebook[tab].buffer, line, -1);
	}
	gtk_text_buffer_insert_at_cursor(widgets->notebook[tab].buffer, "\n",
			-1);
	snprintf(line, sizeof(line), "\tMaximum value     : %12.2f\n", ll);
	gtk_text_buffer_insert_at_cursor(widgets->notebook[tab].buffer,
			line, -1);
	snprintf(line, sizeof(line), "\t2nd maximum value : %12.2f\n", sl);
	gtk_text_buffer_insert_at_cursor(widgets->notebook[tab].buffer,
			line, -1);
	snprintf(line, sizeof(line), "\tMinimum value     : %12.2f\n", s);
        gtk_text_buffer_insert_at_cursor(widgets->notebook[tab].buffer,
			line, -1);
	snprintf(line, sizeof(line), "\tAverage value     : %12.2f\n",
			total / d[10][0]);
	gtk_text_buffer_insert_at_cursor(widgets->notebook[tab].buffer,
			line, -1);
	snprintf(line, sizeof(line), "\tRSF               : %12.2f\n",
			ll / sl);
	gtk_text_buffer_insert_at_cursor(widgets->notebook[tab].buffer,
			line, -1);
}

static void display_cent_summary(struct widgets *widgets)
{
	char line[LINE_MAX];
	PangoFontDescription *font_desc;

	font_desc = pango_font_description_from_string("Monospace");
	gtk_widget_override_font(widgets->notebook[CENT_TAB].text, font_desc);
	pango_font_description_free(font_desc);

	snprintf(line, sizeof(line), "%d values inspected\n\n", d[10][0]);
	gtk_text_buffer_set_text(widgets->notebook[CENT_TAB].buffer, line, -1);

	snprintf(line, sizeof(line), "\t>  %10.2f : %8d%10.2f%%\n",
			thr_limits[NR_THRESHOLDS - 4],
			thr_amnts[NR_THRESHOLDS - 4],
			(float)thr_amnts[0]/d[10][0] * 100);
	gtk_text_buffer_insert_at_cursor(widgets->notebook[CENT_TAB].buffer,
			line, -1);
	if (!(thresholds & THRESH_0))
		return;
	snprintf(line, sizeof(line), "\t<= %10.2f : %8d%10.2f%%\n",
			thr_limits[NR_THRESHOLDS - 4],
			thr_amnts[NR_THRESHOLDS - 3],
			(float)thr_amnts[NR_THRESHOLDS - 3]/d[10][0] * 100);
	gtk_text_buffer_insert_at_cursor(widgets->notebook[CENT_TAB].buffer,
			line, -1);
	if (!(thresholds & THRESH_1))
		return;
	snprintf(line, sizeof(line), "\t<= %10.2f : %8d%10.2f%%\n",
			thr_limits[NR_THRESHOLDS - 3],
			thr_amnts[NR_THRESHOLDS - 2],
			(float)thr_amnts[NR_THRESHOLDS - 2]/d[10][0] * 100);
	gtk_text_buffer_insert_at_cursor(widgets->notebook[CENT_TAB].buffer,
			line, -1);
	if (!(thresholds & THRESH_2))
		return;
	snprintf(line, sizeof(line), "\t<= %10.2f : %8d%10.2f%%\n",
			thr_limits[NR_THRESHOLDS - 2],
			thr_amnts[NR_THRESHOLDS - 1],
			(float)thr_amnts[NR_THRESHOLDS - 1]/d[10][0] * 100);
	gtk_text_buffer_insert_at_cursor(widgets->notebook[CENT_TAB].buffer,
			line, -1);
	if (!(thresholds & THRESH_3))
		return;
	snprintf(line, sizeof(line), "\t<= %10.2f : %8d%10.2f%%\n",
			thr_limits[NR_THRESHOLDS - 1],
			thr_amnts[NR_THRESHOLDS],
			(float)thr_amnts[NR_THRESHOLDS]/d[10][0] * 100);
	gtk_text_buffer_insert_at_cursor(widgets->notebook[CENT_TAB].buffer,
			line, -1);
}

static void display_repeat_values(struct widgets *widgets)
{
	GList *l = NULL;
	double prev_value;
	double *value;
	int nr = 0;
	int nr_values = d[10][0];
	int i;
	int asize = 0;
	struct repeat_values *rv;
	char line[LINE_MAX];
	PangoFontDescription *font_desc;

	g_array_sort(values, (GCompareFunc)cmp_values);
	value = &g_array_index(values, double, nr_values - 1);
	prev_value = *value;

	for (i = nr_values - 1; i >= 0; i--) {
		value = &g_array_index(values, double, i);
		if (*value == prev_value) {
			nr++;
		} else {
			rv = malloc(sizeof(struct repeat_values));
			rv->nr = nr;
			rv->value = prev_value;
			l = g_list_prepend(l, rv);
			nr = 1;
			asize++;
		}
		prev_value = *value;
	}
	l = g_list_sort(l, (GCompareFunc)cmp_repeat_values);
	l = g_list_reverse(l);

	font_desc = pango_font_description_from_string("Monospace");
	gtk_widget_override_font(widgets->notebook[RV_TAB].text, font_desc);
	pango_font_description_free(font_desc);
	snprintf(line, sizeof(line),
			"The top %d repeated values from %d inspected\n\n",
			NR_RV, nr_values);
	gtk_text_buffer_set_text(widgets->notebook[RV_TAB].buffer, line, -1);

	/* For small data sets we may have less than NR_RV in the array */
	for (i = 0; i < (NR_RV > asize ? asize : NR_RV); i++) {
		rv = g_list_nth_data(l, i);
		snprintf(line, sizeof(line), "\t%10d : %12.2f\n",
				rv->nr, rv->value);
		gtk_text_buffer_insert_at_cursor(
				widgets->notebook[RV_TAB].buffer, line, -1);
	}

	rv = g_list_nth_data(l, i + 1);
	if (rv) {
		nr = rv->nr;
		if (rv->nr == nr) {
			snprintf(line, sizeof(line),
				"\n\n* There was at least one more value that "
				"had been seen %d time(s).\n", nr);
			gtk_text_buffer_insert_at_cursor(
				widgets->notebook[RV_TAB].buffer, line, -1);
		}
	}
	g_list_free_full(l, free);
}

static void display_repeat_invoices(struct widgets *widgets,
				    const char *entity)
{
	TCTDB *tdb;
	TDBQRY *qry;
	TCLIST *res;
	TCMAP *cols;
	int nres;
	int i;
	int rsize;
	int dups = 0;
	char *l_invoice;
	char *l_entity;
	char *l_des;
	const char *rbuf;
	double l_value;
	bool prev = false;
	GtkTreeModel *model;

	tdb = tctdbnew();
	tctdbopen(tdb, tct_db, TDBOREADER);

	qry = tctdbqrynew(tdb);
	if (entity)
		tctdbqryaddcond(qry, "entity_name", TDBQCSTREQ, entity);
	tctdbqrysetorder(qry, INV_TCT_COL, TDBQOSTRASC);
	res = tctdbqrysearch(qry);
	nres = tclistnum(res);
	if (nres == 0)
		goto out;

	rbuf = tclistval(res, 0, &rsize);
	cols = tctdbget(tdb, rbuf, rsize);
	l_invoice = strdup(tcmapget2(cols, INV_TCT_COL));
	l_entity = strdup(tcmapget2(cols, "entity_name"));
	l_des = strdup(tcmapget2(cols, "acc_des"));
	l_value = strtod(tcmapget2(cols, "gross"), NULL);
	tcmapdel(cols);

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(
				widgets->notebook[RI_TAB].treeview));
	g_object_ref(model);
	gtk_tree_view_set_model(GTK_TREE_VIEW(
				widgets->notebook[RI_TAB].treeview), NULL);
	for (i = 1; i < nres; i++) {
		const char *invoice;

		rbuf = tclistval(res, i, &rsize);
		cols = tctdbget(tdb, rbuf, rsize);
		invoice = tcmapget2(cols, INV_TCT_COL);
		if (strcmp(invoice, l_invoice) == 0) {
			GtkTreeIter iter;
			const char *bg;

			if (dups % 2 == 0)
				bg = "white";
			else
				bg = "skyblue";

			/*
			 * We only want to do this once for a set of
			 * duplicates or we'll _get_ duplicates.
			 */
			if (!prev) {
				gtk_list_store_append(
					widgets->notebook[RI_TAB].liststore,
					&iter);
				gtk_list_store_set(
					widgets->notebook[RI_TAB].liststore,
					&iter,
					0, l_invoice,
					1, l_entity,
					2, l_des,
					3, l_value,
					4, bg,
					-1);
				prev = true;
			}
			gtk_list_store_append(
					widgets->notebook[RI_TAB].liststore,
					&iter);
			gtk_list_store_set(
					widgets->notebook[RI_TAB].liststore,
					&iter,
					0, tcmapget2(cols, INV_TCT_COL),
					1, tcmapget2(cols, "entity_name"),
					2, tcmapget2(cols, "acc_des"),
					3, strtod(tcmapget2(cols, "gross")
						,NULL),
					4, bg,
					-1);
		} else {
			if (prev)
				dups++;
			prev = false;
		}
		free(l_invoice);
		free(l_entity);
		free(l_des);
		if (i == nres - 1) {
			tcmapdel(cols);
			break;
		}
		l_invoice = strdup(tcmapget2(cols, INV_TCT_COL));
		l_entity = strdup(tcmapget2(cols, "entity_name"));
		l_des = strdup(tcmapget2(cols, "acc_des"));
		l_value = strtod(tcmapget2(cols, "gross"), NULL);
		tcmapdel(cols);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(
				widgets->notebook[RI_TAB].treeview), model);
	g_object_unref(model);
out:
	tclistdel(res);
	tctdbqrydel(qry);
	tctdbclose(tdb);
}

static void display_entities(struct widgets *widgets, const char *entity,
			     int sort_column)
{
	TCTDB *tdb;
	TDBQRY *qry;
	TCLIST *res;
	int nres;
	int i;
	unsigned int invoices = 0;
	double total = 0.0;
	char *last_entity = strdup("");

	tdb = tctdbnew();
	tctdbopen(tdb, tct_db, TDBOREADER);

	qry = tctdbqrynew(tdb);
	if (entity)
		tctdbqryaddcond(qry, "entity_name", TDBQCSTREQ, entity);
	tctdbqrysetorder(qry, "entity_name", TDBQOSTRASC);
	res = tctdbqrysearch(qry);
	nres = tclistnum(res);
	if (nres == 0)
		goto out;

	for (i = 0; i < nres; i++) {
		TCMAP *cols;
		const char *rbuf;
		const char *entity;
		int rsize;

		rbuf = tclistval(res, i, &rsize);
		cols = tctdbget(tdb, rbuf, rsize);
		entity = tcmapget2(cols, "entity_name");
		if (strcmp(entity, last_entity) == 0 || i == 0) {
			total += strtod(tcmapget2(cols, "gross"), NULL);
			invoices++;
		}
		if ((strcmp(entity, last_entity) != 0 && i > 0) ||
		    i == nres - 1) {
			GtkTreeIter iter;

			gtk_list_store_append(
					widgets->notebook[ESI_TAB].liststore,
					&iter);
			gtk_list_store_set(
					widgets->notebook[ESI_TAB].liststore,
					&iter,
					0, (strlen(last_entity) == 0) ?
						entity : last_entity,
					1, total,
					2, invoices,
					3, TRUE,
					-1);

			/* Grab the first total of the _next_ entity. */
			total = strtod(tcmapget2(cols, "gross"), NULL);
			invoices = 1;
		}
		free(last_entity);
		last_entity = strdup(entity);
		tcmapdel(cols);
	}
	/* Order the list either by spend or by number of invoices */
	gtk_tree_sortable_set_sort_column_id(
			GTK_TREE_SORTABLE(widgets->notebook[ESI_TAB].sort),
			sort_column, GTK_SORT_DESCENDING);
out:
	free(last_entity);
	tclistdel(res);
	tctdbqrydel(qry);
	tctdbclose(tdb);
}

static void display_graph(struct widgets *widgets, int tab)
{
	GdkPixbuf *pixbuf;

	pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, x, y);
	gtk_image_set_from_pixbuf(GTK_IMAGE(widgets->notebook[tab].image),
			pixbuf);

	g_object_unref(pixbuf);
}

static void process_value(const char *value)
{
	double val;

	switch (value[0]) {
	case '1':
		d[1][0]++;
		break;
	case '2':
		d[2][0]++;
		break;
	case '3':
		d[3][0]++;
		break;
	case '4':
		d[4][0]++;
		break;
	case '5':
		d[5][0]++;
		break;
	case '6':
		d[6][0]++;
		break;
	case '7':
		d[7][0]++;
		break;
	case '8':
		d[8][0]++;
		break;
	case '9':
		d[9][0]++;
		break;
	default:
		/*
		 * We don't examine values that begin with a 0.
		 * Basically we ignore entries woth values < 1.0
		 */
		return;
	}
	d[10][0]++;
	val = strtod(value, NULL);
	total += val;

	if (val > ll) {
		sl = ll;
		ll = val;
	} else if (val > sl && val != ll) {
		sl = val;
	} else if (val < s) {
		s = val;
	} else if (s == 0.0) {
		s = val;
	}

	/* Gather number of invoices, e.g; > 10000 > 1000 > 100 > 10  > 0 */
	if (val > thr_limits[NR_THRESHOLDS - 4])
		thr_amnts[0]++;
	else if (val <= thr_limits[NR_THRESHOLDS - 4] &&
		 val >= thr_limits[NR_THRESHOLDS - 3] + 0.01)
		thr_amnts[1]++;
	else if (val <= thr_limits[NR_THRESHOLDS - 3] &&
		 val >= thr_limits[NR_THRESHOLDS - 2] + 0.01)
		thr_amnts[2]++;
	else if (val <= thr_limits[NR_THRESHOLDS - 2] &&
		 val >= thr_limits[NR_THRESHOLDS - 1] + 0.01)
		thr_amnts[3]++;
	else if (val <= thr_limits[NR_THRESHOLDS - 1])
		thr_amnts[4]++;

	g_array_append_val(values, val);

	if (value[1] < '0' || value[1] > '9')
		return;

	switch (value[1]) {
	case '0':
		d[0][1]++;
		break;
	case '1':
		d[1][1]++;
		break;
	case '2':
		d[2][1]++;
		break;
	case '3':
		d[3][1]++;
		break;
	case '4':
		d[4][1]++;
		break;
	case '5':
		d[5][1]++;
		break;
	case '6':
		d[6][1]++;
		break;
	case '7':
		d[7][1]++;
		break;
	case '8':
		d[8][1]++;
		break;
	case '9':
		d[9][1]++;
		break;
	default:
		return;
	}
	d[10][1]++;

	if (value[2] < '0' || value[2] > '9')
		return;

	switch (value[2]) {
	case '0':
		d[0][2]++;
		break;
	case '1':
		d[1][2]++;
		break;
	case '2':
		d[2][2]++;
		break;
	case '3':
		d[3][2]++;
		break;
	case '4':
		d[4][2]++;
		break;
	case '5':
		d[5][2]++;
		break;
	case '6':
		d[6][2]++;
		break;
	case '7':
		d[7][2]++;
		break;
	case '8':
		d[8][2]++;
		break;
	case '9':
		d[9][2]++;
		break;
	default:
		return;
	}
	d[10][2]++;
}

static void process_tct_values(const char *entity)
{
	TCTDB *tdb;
	TDBQRY *qry;
	TCLIST *res;
	int nres;
	int i;

	tdb = tctdbnew();
	tctdbopen(tdb, tct_db, TDBOREADER);

	qry = tctdbqrynew(tdb);
	if (entity)
		tctdbqryaddcond(qry, "entity_name", TDBQCSTREQ, entity);
	res = tctdbqrysearch(qry);
	nres = tclistnum(res);
	for (i = 0; i < nres; i++) {
		TCMAP *cols;
		const char *rbuf;
		const char *value;
		int rsize;

		rbuf = tclistval(res, i, &rsize);
		cols = tctdbget(tdb, rbuf, rsize);
		value = tcmapget2(cols, "gross");
		process_value(value);
		tcmapdel(cols);
        }
	tclistdel(res);
	tctdbqrydel(qry);
	tctdbclose(tdb);
}

static void gen_entity_list(struct widgets *widgets)
{
	TCTDB *tdb;
	TDBQRY *qry;
	TCLIST *res;
	int nres;
	int i;
	char *last_entity = NULL;
	GtkTreeIter iter;

	tdb = tctdbnew();
	tctdbopen(tdb, tct_db, TDBOREADER);

	qry = tctdbqrynew(tdb);
	tctdbqrysetorder(qry, "entity_name", TDBQOSTRASC);
	res = tctdbqrysearch(qry);
	nres = tclistnum(res);
	if (nres == 0)
		goto out;

	/* Add an empty entry to the entity filter list, for all entities */
	gtk_list_store_append(widgets->entity_liststore, &iter);
	gtk_list_store_set(widgets->entity_liststore, &iter, 0, NULL, -1);
	for (i = 0; i < nres; i++) {
		TCMAP *cols;
		const char *rbuf;
		const char *entity;
		int rsize;

		rbuf = tclistval(res, i, &rsize);
		cols = tctdbget(tdb, rbuf, rsize);
		entity = tcmapget2(cols, "entity_name");
		if (!last_entity || strcmp(entity, last_entity) != 0) {
			gtk_list_store_append(widgets->entity_liststore,
					&iter);
			gtk_list_store_set(widgets->entity_liststore, &iter,
					0, entity,
					-1);
		}
		free(last_entity);
		last_entity = strdup(entity);
		tcmapdel(cols);
	}
out:
	free(last_entity);
	tclistdel(res);
	tctdbqrydel(qry);
	tctdbclose(tdb);
}

static void disp_vars(gpointer value, gpointer user_data)
{
	fprintf(stderr, "%s\t", (char *)value);
}

static void dump_tct(void)
{
	TCTDB *tdb;
	TDBQRY *qry;
	TCLIST *res;
	int i;
	int nres;

	tdb = tctdbnew();
	tctdbopen(tdb, tct_db, TDBOREADER);

	qry = tctdbqrynew(tdb);
	tctdbqrysetorder(qry, INV_TCT_COL, TDBQOSTRASC);
	tctdbqrysetorder(qry, "gross", TDBQOSTRASC);
	res = tctdbqrysearch(qry);
	nres = tclistnum(res);
	fprintf(stderr, "%d item(s)\n", nres);
	for (i = 0; i < nres; i++) {
		TCMAP *cols;
		const char *rbuf;
		const char *cname;
		int rsize;

		rbuf = tclistval(res, i, &rsize);
		cols = tctdbget(tdb, rbuf, rsize);
		tcmapiterinit(cols);
		while ((cname = tcmapiternext2(cols)) != NULL)
			fprintf(stderr, "%s\t", tcmapget2(cols, cname));
		fprintf(stderr, "\n");
		tcmapdel(cols);
	}
	tclistdel(res);
	tctdbqrydel(qry);
	tctdbclose(tdb);
}

static void read_tmpl(const char *file)
{
	FILE *fp;
	char line[LINE_MAX];
	TCTDB *tdb;

        tdb = tctdbnew();
        tctdbopen(tdb, tct_db, TDBOWRITER | TDBOTRUNC | TDBOCREAT);
//	tctdbsetindex(tdb, "entity_name", TDBITLEXICAL);
//	tctdbsetindex(tdb, "po_no", TDBITLEXICAL);

	fp = fopen(file, "r");
	while (fgets(line, sizeof(line), fp) != NULL) {
		char *token;
		char *running;
		char *sptr;
		GPtrArray *vars = NULL;

		vars = g_ptr_array_new_with_free_func(free);
		running = strdup(line);
		/*
		 * We can't free running after its been through strsep(),
		 * so save a pointer to it that we can free.
		 */
		sptr = running;
		token = strsep(&running, "\t");
		while (token != NULL) {
			if (strlen(token) == 0)
				g_ptr_array_add(vars, strdup(""));
			else
				g_ptr_array_add(vars,
						strdup(g_strstrip(token)));
			token = strsep(&running, "\t");
		}
		free(sptr);
		int primary_key_size;
		char pkbuf[256];
		TCMAP *cols;

		process_value(g_ptr_array_index(vars, 5));
//		g_ptr_array_foreach(vars, disp_vars, NULL);
//		fprintf(stderr, "\n");
		primary_key_size = snprintf(pkbuf, sizeof(pkbuf), "%ld",
				(int64_t)tctdbgenuid(tdb));
		cols = tcmapnew3("entity_code", g_ptr_array_index(vars, 0),
				"entity_name", g_ptr_array_index(vars, 1),
				"post_code", g_ptr_array_index(vars, 2),
				"vat_no", g_ptr_array_index(vars, 3),
				"inv_no", g_ptr_array_index(vars, 4),
				"gross", g_ptr_array_index(vars, 5),
				"vat", g_ptr_array_index(vars, 6),
				"net", g_ptr_array_index(vars, 7),
				"po_no", g_ptr_array_index(vars, 8),
				"buyer", g_ptr_array_index(vars, 9),
				"cost_centre", g_ptr_array_index(vars, 10),
				"acc_code", g_ptr_array_index(vars, 11),
				"acc_des", g_ptr_array_index(vars, 12),
				(char *)NULL);
		tctdbputkeep(tdb, pkbuf, primary_key_size, cols);
		tcmapdel(cols);
		g_ptr_array_free(vars, TRUE);
		vars = NULL;
	}
	fclose(fp);

	tctdbclose(tdb);
	tctdbdel(tdb);

//	fprintf(stderr, "\n\n");
//	dump_tct();
}

void load_prefs(struct widgets *widgets)
{
	FILE *fp;
	char line[LINE_MAX];
	char *option;
	char *value;
	char *token;
	char config[PATH_MAX];

        snprintf(config, sizeof(config), "%s/.config/auric/prefs",
			getenv("HOME"));

	fp = fopen(config, "r");
	if (!fp)
		return;

	while (fgets(line, LINE_MAX, fp)) {
		token = strtok(line, "=");
		option = token;
		token = strtok(NULL, "=");
		value = token;

		/* Loose the trailing \n */
		value[strlen(value) - 1] = '\0';
		if (strcmp(option, "th_cb0") == 0) {
			if (atoi(value) == 1) {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
						widgets->prefs.th_cb0), TRUE);
				thresholds |= THRESH_0;
			} else {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
						widgets->prefs.th_cb0), FALSE);
				thresholds &= ~THRESH_0;
			}
		} else if (strcmp(option, "th_cb1") == 0) {
			if (atoi(value) == 1) {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
						widgets->prefs.th_cb1), TRUE);
				thresholds |= THRESH_1;
			} else {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
						widgets->prefs.th_cb1), FALSE);
				thresholds &= ~THRESH_1;
			}
		} else if (strcmp(option, "th_cb2") == 0) {
			if (atoi(value) == 1) {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
						widgets->prefs.th_cb2), TRUE);
				thresholds |= THRESH_2;
			} else {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
						widgets->prefs.th_cb2), FALSE);
				thresholds &= ~THRESH_2;
			}
		} else if (strcmp(option, "th_cb3") == 0) {
			if (atoi(value) == 1) {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
						widgets->prefs.th_cb3), TRUE);
				thresholds |= THRESH_3;
			} else {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
						widgets->prefs.th_cb3), FALSE);
				thresholds &= ~THRESH_3;
			}
		} else if (strcmp(option, "th0") == 0) {
			gtk_entry_set_text(GTK_ENTRY(widgets->prefs.th0),
					value);
		} else if (strcmp(option, "th1") == 0) {
			gtk_entry_set_text(GTK_ENTRY(widgets->prefs.th1),
					value);
		} else if (strcmp(option, "th2") == 0) {
			gtk_entry_set_text(GTK_ENTRY(widgets->prefs.th2),
					value);
		} else if (strcmp(option, "th3") == 0) {
			gtk_entry_set_text(GTK_ENTRY(widgets->prefs.th3),
					value);
		} else if (strcmp(option, "dp") == 0) {
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(
						widgets->prefs.dp_spin),
					strtod(value, NULL));
		}
	}
	fclose(fp);
}

void set_def_prefs(struct widgets *widgets)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets->prefs.th_cb0),
			TRUE);
	thresholds |= THRESH_0;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets->prefs.th_cb1),
			TRUE);
	thresholds |= THRESH_1;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets->prefs.th_cb2),
			TRUE);
	thresholds |= THRESH_2;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgets->prefs.th_cb3),
			TRUE);
	thresholds |= THRESH_3;

	gtk_entry_set_text(GTK_ENTRY(widgets->prefs.th0), "10000.00");
	gtk_entry_set_text(GTK_ENTRY(widgets->prefs.th1), "1000.00");
	gtk_entry_set_text(GTK_ENTRY(widgets->prefs.th2), "100.00");
	gtk_entry_set_text(GTK_ENTRY(widgets->prefs.th3), "10.00");

	snprintf(val_fmt, sizeof(val_fmt), "%%.2f");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widgets->prefs.dp_spin),
			2.0);
}

void set_prefs(struct widgets *widgets)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
					widgets->prefs.th_cb0))) {
		thr_limits[NR_THRESHOLDS - 4] =
			strtod(gtk_entry_get_text(GTK_ENTRY(
					widgets->prefs.th0)), NULL);
		thresholds |= THRESH_0;
	} else {
		thr_limits[NR_THRESHOLDS - 4] = 0.0;
		thresholds &= ~THRESH_0;
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
					widgets->prefs.th_cb1))) {
		thr_limits[NR_THRESHOLDS - 3] =
			strtod(gtk_entry_get_text(GTK_ENTRY(
					widgets->prefs.th1)), NULL);
		thresholds |= THRESH_1;
	} else {
		thr_limits[NR_THRESHOLDS - 3] = 0.0;
		thresholds &= ~THRESH_1;
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
					widgets->prefs.th_cb2))) {
		thr_limits[NR_THRESHOLDS - 2] =
			strtod(gtk_entry_get_text(GTK_ENTRY(
					widgets->prefs.th2)), NULL);
		thresholds |= THRESH_2;
	} else {
		thr_limits[NR_THRESHOLDS - 2] = 0.0;
		thresholds &= ~THRESH_2;
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
					widgets->prefs.th_cb3))) {
		thr_limits[NR_THRESHOLDS - 1] =
			strtod(gtk_entry_get_text(GTK_ENTRY(
					widgets->prefs.th3)), NULL);
		thresholds |= THRESH_3;
	} else {
		thr_limits[NR_THRESHOLDS - 1] = 0.0;
		thresholds &= ~THRESH_3;
	}
}

void view_invoice_details(const char *invoice)
{
	GtkBuilder *builder;
	GError *error = NULL;
	struct ri_vid *ri_vid;
	TCTDB *tdb;
	TDBQRY *qry;
	TCLIST *res;
	int nres;
	int i;
	int rsize;
	char *window_title;
	GtkTreeModel *model;

	builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(builder, "auric_ri_vid.glade", &error))
		g_warning("%s", error->message);

	ri_vid = g_slice_new(struct ri_vid);
	ri_vid->window = GTK_WIDGET(gtk_builder_get_object(builder,
				"window1"));
	ri_vid->liststore = GTK_LIST_STORE(gtk_builder_get_object(builder,
				"liststore1"));
	ri_vid->treeview = GTK_WIDGET(gtk_builder_get_object(builder,
				"treeview1"));
	ri_vid->col = GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder,
				"treeviewcolumn6"));
	ri_vid->cell = GTK_CELL_RENDERER(gtk_builder_get_object(builder,
				"gross"));
	gtk_tree_view_column_set_cell_data_func(ri_vid->col, ri_vid->cell,
			format_cell_value, GINT_TO_POINTER(5), NULL);
	ri_vid->col = GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder,
				"treeviewcolumn7"));
	ri_vid->cell = GTK_CELL_RENDERER(gtk_builder_get_object(builder,
				"vat"));
	gtk_tree_view_column_set_cell_data_func(ri_vid->col, ri_vid->cell,
			format_cell_value, GINT_TO_POINTER(6), NULL);
	ri_vid->col = GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder,
			"treeviewcolumn8"));
	ri_vid->cell = GTK_CELL_RENDERER(gtk_builder_get_object(builder,
				"net"));
	gtk_tree_view_column_set_cell_data_func(ri_vid->col, ri_vid->cell,
			format_cell_value, GINT_TO_POINTER(7), NULL);

	gtk_builder_connect_signals(builder, ri_vid);
	g_object_unref(G_OBJECT(builder));

	window_title = g_strdup_printf("auric / invoice / %s", invoice);
	gtk_window_set_title(GTK_WINDOW(ri_vid->window), window_title);
	g_free(window_title);

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(ri_vid->treeview));
	g_object_ref(model);
	gtk_tree_view_set_model(GTK_TREE_VIEW(ri_vid->treeview), NULL);

	tdb = tctdbnew();
	tctdbopen(tdb, tct_db, TDBOREADER);

	qry = tctdbqrynew(tdb);
	tctdbqryaddcond(qry, INV_TCT_COL, TDBQCSTREQ, invoice);
	res = tctdbqrysearch(qry);
	nres = tclistnum(res);

	/*
	 * Be a bit smart in the window size, try to size it correctly for
	 * the number of rows, upto some limit.
	 */
	gtk_window_resize(GTK_WINDOW(ri_vid->window), 1400,
			(nres > 20) ? 640 : (nres - 2) * 24 + 160);
	gtk_widget_show(ri_vid->window);

	for (i = 0; i < nres; i++) {
		GtkTreeIter iter;
		TCMAP *cols;
		const char *rbuf;

		rbuf = tclistval(res, i, &rsize);
		cols = tctdbget(tdb, rbuf, rsize);

		gtk_list_store_append(ri_vid->liststore, &iter);
		gtk_list_store_set(ri_vid->liststore, &iter,
				0, tcmapget2(cols, "entity_code"),
				1, tcmapget2(cols, "entity_name"),
				2, tcmapget2(cols, "post_code"),
				3, tcmapget2(cols, "vat_no"),
				4, tcmapget2(cols, "inv_no"),
				5, strtod(tcmapget2(cols, "gross"), NULL),
				6, strtod(tcmapget2(cols, "vat"), NULL),
				7, strtod(tcmapget2(cols, "net"), NULL),
				8, tcmapget2(cols, "po_no"),
				9, tcmapget2(cols, "buyer"),
				10, tcmapget2(cols, "cost_centre"),
				11, tcmapget2(cols, "acc_code"),
				12, tcmapget2(cols, "acc_des"),
				-1);
		tcmapdel(cols);
	}
	tclistdel(res);
	tctdbqrydel(qry);
	tctdbclose(tdb);

	gtk_tree_view_set_model(GTK_TREE_VIEW(ri_vid->treeview), model);
	g_object_unref(model);
}

void runit(GtkWidget *widget, struct widgets *widgets)
{
	const char *entity;
	int i;

	entity = gtk_combo_box_get_active_id(GTK_COMBO_BOX(
				widgets->entity_filter_list));

	gtk_list_store_clear(widgets->notebook[RI_TAB].liststore);
	gtk_list_store_clear(widgets->notebook[ESI_TAB].liststore);
	values = g_array_new(FALSE, FALSE, sizeof(double));

	set_prefs(widgets);

	if (entity ||
	    strcmp(gtk_widget_get_name(widget), "GtkComboBox") == 0) {
		/*
		 * Either we are running a filtered entity or we are
		 * wanting all the entities from the _current_ data
		 * and there is no point re-loading it all again.
		 */
		process_tct_values(entity);
	} else {
		unsigned long sigid;
		const char *file;

		file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
					widgets->file_chooser));
		if (!file)
			return;

		/* Loading data from a new file. */
		read_tmpl(file);

		/*
		 * We need to temporarily block the "changed" signal from
		 * the entity combo list while we clear and repopulate it,
		 * otherwise the cb_run() function gets called for every entry
		 * that gets added in gen_entity_list() generally ending up
		 * in a segfault.
		 *
		 * TODO: While we only have one signal "changed" attached to
		 * the combo box, the below probably isn't technically
		 * correct as it would match whatever is the first signal.
		 */
		sigid = g_signal_handler_find(widgets->entity_filter_list,
				G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL,
				widgets);
		g_signal_handler_block(widgets->entity_filter_list, sigid);
		gtk_list_store_clear(widgets->entity_liststore);
		gen_entity_list(widgets);
		g_signal_handler_unblock(widgets->entity_filter_list, sigid);
	}

	for (i = 0; i < NR_POS; i++) {
		display_summary(widgets, i);

		init_graph(i);
		draw_curve("benford", b, i);
		draw_curve("data", p, i);
		display_graph(widgets, i);
	}
	display_cent_summary(widgets);
	draw_pie_chart();
	display_graph(widgets, CENT_TAB);
	display_repeat_values(widgets);
	display_repeat_invoices(widgets, entity);
	display_entities(widgets, entity, SORT_COL_SPD);

	memset(p, 0, sizeof(p));
	memset(d, 0, sizeof(d));
	memset(thr_amnts, 0, sizeof(thr_amnts));
	total = 0.0;
	ll = 0.0;
	sl = 0.0;
	s = 0.0;
	g_array_free(values, TRUE);
}

int main(int argc, char **argv)
{
	GtkBuilder *builder;
	GError *error = NULL;
	struct widgets *widgets;
	int i;

	gtk_init(&argc, &argv);

	builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(builder, "auric.glade", &error)) {
		g_warning("%s", error->message);
		exit(EXIT_FAILURE);
	}

	widgets = g_slice_new(struct widgets);
	get_widgets(widgets, builder);
	gtk_builder_connect_signals(builder, widgets);
	g_object_unref(G_OBJECT(builder));

	for (i = 0; i < NR_TABS; i++)
		init_image(widgets, i);
	gtk_widget_show(widgets->main_window);
	populate_b();

	set_def_prefs(widgets);
	load_prefs(widgets);
	snprintf(tct_db, sizeof(tct_db), "/tmp/auric.tct-%d", getpid());

	gtk_main();

	g_slice_free(struct widgets, widgets);
	de_init_image();
	unlink(tct_db);

	exit(EXIT_SUCCESS);
}
