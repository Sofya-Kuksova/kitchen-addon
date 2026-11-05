#pragma once
#include "kitchen_settings.h"
/* Standalone owner, retained until reset. No app/UI pointers cross this boundary. */
kitchen_storage_backend_t standalone_storage_init(void);
