/**
 * @file
 * Analogy for Linux, instruction related features
 *
 * @note Copyright (C) 1997-2000 David A. Schleef <ds@schleef.org>
 * @note Copyright (C) 2008 Alexis Berlemont <alexis.berlemont@free.fr>
 *
 * Xenomai is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Xenomai is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xenomai; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef _COBALT_RTDM_ANALOGY_INSTRUCTION_H
#define _COBALT_RTDM_ANALOGY_INSTRUCTION_H

struct a4l_kernel_instruction {
	unsigned int type;
	unsigned int idx_subd;
	unsigned int chan_desc;
	unsigned int data_size;
	void *data;
	void *__udata;
};
typedef struct a4l_kernel_instruction a4l_kinsn_t;

struct a4l_kernel_instruction_list {
	unsigned int count;
	a4l_kinsn_t *insns;
	a4l_insn_t *__uinsns;
};
typedef struct a4l_kernel_instruction_list a4l_kilst_t;

/* Instruction related functions */

/* Upper layer functions */
int a4l_ioctl_insnlist(a4l_cxt_t * cxt, void *arg);
int a4l_ioctl_insn(a4l_cxt_t * cxt, void *arg);

#endif /* !_COBALT_RTDM_ANALOGY_BUFFER_H */