/*DS and Func common b/w client and odr or server and odr*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include "unp.h"
#include "hw_addrs.h"
#include "list.h"

#define CLI_UN_PATH "/tmp/kool_cli_path-XXXXXX"
#define SRV_UN_PATH "/tmp/kool_srv_path-XXXXXX"
#define SERVER_PORT 57575
