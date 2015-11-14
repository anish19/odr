/*DS and Func common b/w client and odr or server and odr*/

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "unp.h"

#define CLI_UN_PATH "/tmp/kool_cli_path-XXXXXX"
#define SRV_UN_PATH "/tmp/kool_srv_path-XXXXXX"
#define SERVER_PORT 57575
