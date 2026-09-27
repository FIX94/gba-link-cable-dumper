/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include "app/app.h"
#include "ui/ui.h"
#include "ui/input.h"

int main(int argc, char *argv[]) 
{
	ui_init();
	input_init();
	app_run(argc, argv);
	return 0;
}
