#include <string.h>
#include "session.h"

static char s_login[21] = "";
static char s_role[12] = "";

void session_ouvrir(const char *login, const char *role)
{
    strncpy(s_login, login, sizeof s_login - 1);
    s_login[sizeof s_login - 1] = '\0';
    strncpy(s_role, role, sizeof s_role - 1);
    s_role[sizeof s_role - 1] = '\0';
}

void session_fermer(void)
{
    s_login[0] = '\0';
    s_role[0] = '\0';
}

const char *session_login(void) { return s_login; }
const char *session_role(void)  { return s_role; }
