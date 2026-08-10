/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init_coders.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/04 17:00:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/10 19:35:23 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	reset_coder(t_coder *c, int id, t_sim *sim)
{
	if (!c || !sim)
		return ;
	c->id = id;
	c->n_compiled = 0;
	c->last_compile = 0;
	c->left = NULL;
	c->right = NULL;
	c->sim = sim;
}

void	link_coder_dongles(t_sim *sim)
{
	int	i;
	int	n;

	if (!sim || !sim->coders || !sim->dongles)
		return ;
	n = sim->cfg.n_coders;
	if (n == 1)
	{
		sim->coders[0].left = &sim->dongles[0];
		sim->coders[0].right = &sim->dongles[0];
		return ;
	}
	i = 0;
	while (i < n)
	{
		sim->coders[i].left = &sim->dongles[i];
		sim->coders[i].right = &sim->dongles[(i + 1) % n];
		i++;
	}
}

int	init_coders(t_sim *sim)
{
	int	i;

	if (!sim || !sim->coders)
		return (1);
	i = 0;
	while (i < sim->cfg.n_coders)
	{
		reset_coder(&sim->coders[i], i + 1, sim);
		i++;
	}
	link_coder_dongles(sim);
	return (0);
}
