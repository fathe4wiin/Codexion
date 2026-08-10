/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_number.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/06 18:42:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/10 19:35:23 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	parse_pos_int(const char *s, int *out)
{
	long long	value;
	int			digit;

	if (!s || !*s || !out)
		return (0);
	value = 0;
	while (*s)
	{
		if (*s < '0' || *s > '9')
			return (0);
		digit = *s - '0';
		if (value > (INT_MAX - digit) / 10)
			return (0);
		value = value * 10 + digit;
		s++;
	}
	if (value <= 0)
		return (0);
	*out = (int)value;
	return (1);
}

int	parse_nn_int(const char *s, int *out)
{
	long long	value;
	int			digit;

	if (!s || !*s || !out)
		return (0);
	value = 0;
	while (*s)
	{
		if (*s < '0' || *s > '9')
			return (0);
		digit = *s - '0';
		if (value > (INT_MAX - digit) / 10)
			return (0);
		value = value * 10 + digit;
		s++;
	}
	*out = (int)value;
	return (1);
}

int	parse_nn_ll(const char *s, long long *out)
{
	unsigned long long	value;
	int					digit;

	if (!s || !*s || !out)
		return (0);
	value = 0;
	while (*s)
	{
		if (*s < '0' || *s > '9')
			return (0);
		digit = *s - '0';
		if (value > (unsigned long long)(LLONG_MAX - digit) / 10ULL)
			return (0);
		value = value * 10ULL + (unsigned long long)digit;
		s++;
	}
	*out = (long long)value;
	return (1);
}
