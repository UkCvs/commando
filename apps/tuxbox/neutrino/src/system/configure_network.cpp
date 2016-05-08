/*
 * $Header: /cvs/tuxbox/apps/tuxbox/neutrino/src/system/configure_network.cpp,v 1.8 2012/04/13 12:15:22 rhabarber1848 Exp $
 *
 * (C) 2003 by thegoodguy <thegoodguy@berlios.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "configure_network.h"
#include "libnet.h"             /* netGetNameserver, netSetNameserver   */
#include "network_interfaces.h" /* getInetAttributes, setInetAttributes */
#include <stdlib.h>             /* system                               */
#include <stdio.h>
#include <iomanip>
#include <sstream>

#include <system/helper.h>

CNetworkConfig::CNetworkConfig(void)
{
	netGetNameserver(nameserver);
	ifname = "eth0";
	inet_static = getInetAttributes(ifname, automatic_start, address, netmask, broadcast, gateway);

	init_vars();
	copy_to_orig();
}

CNetworkConfig* CNetworkConfig::getInstance()
{
	static CNetworkConfig* network_config = NULL;

	if(!network_config)
	{
		network_config = new CNetworkConfig();
		printf("[network config] Instance created\n");
	}
	return network_config;
}

CNetworkConfig::~CNetworkConfig()
{

}

void CNetworkConfig::init_vars(void)
{
	unsigned char addr[6];

	netGetMacAddr(ifname, addr);

	std::stringstream mac_tmp;
	for(int i=0;i<6;++i)
		mac_tmp<<std::hex<<std::setfill('0')<<std::setw(2)<<(int)addr[i]<<':';

	mac_addr = mac_tmp.str().substr(0,17);
}

void CNetworkConfig::copy_to_orig(void)
{
	orig_automatic_start = automatic_start;
	orig_address         = address;
	orig_netmask         = netmask;
	orig_broadcast       = broadcast;
	orig_gateway         = gateway;
	orig_inet_static     = inet_static;
}

bool CNetworkConfig::modified_from_orig(void)
{
	return (
		(orig_automatic_start != automatic_start) ||
		(orig_address         != address        ) ||
		(orig_netmask         != netmask        ) ||
		(orig_broadcast       != broadcast      ) ||
		(orig_gateway         != gateway        ) ||
		(orig_inet_static     != inet_static    )
		);
}

void CNetworkConfig::commitConfig(void)
{
	if (modified_from_orig())
	{
		copy_to_orig();

		if (inet_static)
		{
			addLoopbackDevice("lo", true);
			setStaticAttributes(ifname, automatic_start, address, netmask, broadcast, gateway);
		}
		else
		{
			addLoopbackDevice("lo", true);
			setDhcpAttributes(ifname, automatic_start);
		}
	}
	if (nameserver != orig_nameserver)
	{
		orig_nameserver = nameserver;
		netSetNameserver(nameserver);
	}
}

void CNetworkConfig::startNetwork(void)
{
	std::string cmd = "/sbin/ifup " + ifname;

	my_system(3, "/bin/sh", "-c", cmd.c_str());
}

void CNetworkConfig::stopNetwork(void)
{
	std::string cmd = "/sbin/ifdown " + ifname;

	my_system(3, "/bin/sh", "-c", cmd.c_str());
}

