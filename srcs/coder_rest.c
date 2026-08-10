/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_rest.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: student <student@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/08 16:00:00 by student           #+#    #+#             */
/*   Updated: 2026/08/08 16:00:00 by student          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	do_debug(t_coder *c)
{
	if (!c || !c->sim || is_stopped(c->sim))
		return (1);
	log_msg(c->sim, c->id, ST_DEBUG);
	return (act_sleep(c->sim, c->sim->cfg.t_debug));
}

int	do_refactor(t_coder *c)
{
	if (!c || !c->sim || is_stopped(c->sim))
		return (1);
	log_msg(c->sim, c->id, ST_REFACTOR);
	return (act_sleep(c->sim, c->sim->cfg.t_refactor));
}
