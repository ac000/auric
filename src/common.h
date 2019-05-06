/*
 * common.h
 *
 * Copyright (C) 2012		OpenTech Labs
 * 				Andrew Clayton <andrew@digital-domain.net>
 *		 2017		Andrew Clayton <andrew@digital-domain.net>
 *
 * Released under the GNU General Public License version 3.
 * See COPYING
 */

#ifndef _COMMON_H
#define _COMMON_H

#define NR_TABS		8

#define CENT_TAB	3
#define HI_TAB		4
#define RV_TAB		5
#define RI_TAB		6
#define ESI_TAB		7

#define THRESH_0	(1 << 0)
#define THRESH_1	(1 << 1)
#define THRESH_2	(1 << 2)
#define THRESH_3	(1 << 3)

#define ENT_TCT_COL	"entity_name"
#define INV_TCT_COL	"inv_no"

#define __unused	__attribute__((unused))

extern char val_fmt[5];

#endif /* _COMMON_H_ */
