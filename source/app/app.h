/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef __APP_H__
#define __APP_H__

// runs the dumper, only returns on allocation failure
// argv[0] is used to find settings next to the dol when the loader provides it
void app_run(int argc, char *argv[]);

#endif
