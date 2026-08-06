/**
 * Copyright (c) 2026 Andrés Camilo Román Vargas
 *
 * @file mdns_service.h
 * @author Andres C. Román V. (camilo.vargas@technaid.com gh: @andrescvargasr)
 * @brief mDNS Service Discover.
 *      Requirements:
 *      - CONFIG_DNS_SD
 *      - CONFIG_MDNS_RESPONDER_DNS_SD
 *      - CONFIG_NET_SOCKETS
 *      - CONFIG_POSIX_API
 * @version 0.1
 * @date 2026-08-06
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MDNS_SERVICE_H
#define MDNS_SERVICE_H

void mdns_service(void);

#endif // End MCP_SERVER_H