/*
Some of this code began its life as a part of GTA V SCRIPT HOOK SDK.
http://dev-c.com
(C) Alexander Blade 2015

It is now part of the Enhanced Native Trainer project.
https://github.com/gtav-ent/GTAV-EnhancedNativeTrainer
(C) Rob Pridham and fellow contributors 2015
*/

#pragma once

#include "inc\natives.h"
#include "inc\types.h"
#include "inc\enums.h"

#include "inc\main.h"

#include "menu_functions.h"

#include <string>

/***
* METHODS
*/

void process_veh_menu();

bool process_carspawn_menu();

void update_vehicle_features(BOOL playerExists, Ped playerPed);

void reset_vehicle_globals();

bool process_spawn_menu_cars();

bool process_spawn_menu_indus();

bool process_spawn_menu_generic(int topMenuSelection);

bool onconfirm_spawn_menu_vehicle_selection(MenuItem<std::string> choice);

bool do_spawn_vehicle(std::string modelName, std::string modelTitle);

bool process_carmod_menu();

bool onconfirm_carmod_menu(MenuItem<int> choice);

bool process_carmod_options(int category);

bool onconfirm_carmod_options(MenuItem<int> choice);

void give_veh_upgrade(int upgradeType, int upgradeLevel);

bool process_carmod_respray_menu(int section);

bool onconfirm_carmod_respray_menu(MenuItem<int> choice);

void give_veh_body_color(int section, int paintType, int color);
