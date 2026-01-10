// User_Setup_Select.h (project override)
// Respect USER_SETUP_LOADED: if defined via build_flags, do not include local User_Setup.h

#ifndef USER_SETUP_LOADED
#include <User_Setup.h>
#endif
