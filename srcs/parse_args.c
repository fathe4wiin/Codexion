/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_args.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: student <student@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/06 18:42:00 by student           #+#    #+#             */
/*   Updated: 2026/08/04 18:00:00 by student          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	parse_scheduler(const char *s, t_sched *out)
{
	if (!s || !out)
		return (1);
	if (strcmp(s, "fifo") == 0)
	{
		*out = CX_FIFO;
		return (0);
	}
	if (strcmp(s, "edf") == 0)
	{
		*out = CX_EDF;
		return (0);
	}
	return (1);
}

static int	fill_config(char **av, t_config *cfg)
{
	if (!parse_pos_int(av[1], &cfg->n_coders))
		return (1);
	if (!parse_nn_ll(av[2], &cfg->t_burnout))
		return (1);
	if (!parse_nn_int(av[3], &cfg->t_compile))
		return (1);
	if (!parse_nn_int(av[4], &cfg->t_debug))
		return (1);
	if (!parse_nn_int(av[5], &cfg->t_refactor))
		return (1);
	if (!parse_pos_int(av[6], &cfg->n_compiles))
		return (1);
	if (!parse_nn_ll(av[7], &cfg->dongle_cd))
		return (1);
	if (parse_scheduler(av[8], &cfg->sched))
		return (1);
	return (0);
}

int	parse_args(int ac, char **av, t_config *cfg)
{
	if (ac != 9 || !av || !cfg)
		return (1);
	return (fill_config(av, cfg));
}
