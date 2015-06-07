/*
Part of the Enhanced Native Trainer project.
https://github.com/gtav-ent/GTAV-EnhancedNativeTrainer
(C) Rob Pridham and fellow contributors 2015
*/

#include "database.h"
#include "debuglog.h"
#include "script.h"

#include <string>
#include <sstream>

/**This value should be increased whenever you change the schema and a release is made.
However you must also put in code to upgrade from older versions, in ENTDatabase::handle_version,
as they will be deployed in the wild already.*/
const int DATABASE_VERSION = 3;

static int singleIntResultCallback(void *data, int count, char **rows, char **azColName)
{
	if (count == 1 && rows)
	{
		*static_cast<int*>(data) = atoi(rows[0]);
		return 0;
	}
	return 1;
}

static int emptyCallback(void *data, int count, char **rows, char **azColName)
{
	return 0;
}

static int featureEnablementFetchCallback(void *data, int count, char **rows, char **azColName)
{
	std::vector<FeatureEnabledLocalDefinition> defs = *static_cast<std::vector<FeatureEnabledLocalDefinition>*>(data);
	int enabledInt = atoi(rows[1]);
	char *name = rows[0];

	for (int i = 0; i < defs.size(); i++)
	{
		FeatureEnabledLocalDefinition def = defs.at(i);

		if (strcmp(def.name, name) == 0)
		{
			*def.enabled = enabledInt ? 1 : 0;
			if (def.updateFlag != NULL)
			{
				*def.updateFlag = true;
			}
			break;
		}
	}

	return 0;
}

static int genericSettingPairsFetchCallback(void *data, int count, char **rows, char **azColName)
{
	std::vector<StringPairSettingDBRow> *results = static_cast<std::vector<StringPairSettingDBRow>*>(data);
	std::string k = std::string(rows[0]);
	std::string v = std::string(rows[1]);
	StringPairSettingDBRow setting{k,v};
	results->push_back(setting);
	return 0;
}

void ENTDatabase::handle_version(int oldVersion)
{
	if (oldVersion == -1)
	{
		write_text_to_log_file("Feature enablement table not found, so creating it");
		char* CREATE_VERSION_TABLE_QUERY = "create table ENT_FEATURE_ENABLEMENT (FEATURE_NAME TEXT PRIMARY KEY NOT NULL, FEATURE_ENABLED INT NOT NULL)";
		int rcFeat = sqlite3_exec(db, CREATE_VERSION_TABLE_QUERY, NULL, 0, &zErrMsg);
		if (rcFeat != SQLITE_OK)
		{
			write_text_to_log_file("Enablement table creation problem");
			sqlite3_free(zErrMsg);
		}
		else
		{
			write_text_to_log_file("Enablement table created");
		}
	}

	if (oldVersion < 2)
	{
		write_text_to_log_file("Generic setting pairs table not found, so creating it");
		char* CREATE_VERSION_TABLE_QUERY = "create table ENT_SETTING_PAIRS (SETTING_NAME TEXT PRIMARY KEY NOT NULL, SETTING_VALUE TEXT NOT NULL)";
		int rcGen = sqlite3_exec(db, CREATE_VERSION_TABLE_QUERY, NULL, 0, &zErrMsg);
		if (rcGen != SQLITE_OK)
		{
			write_text_to_log_file("Generic setting pairs table creation problem");
			sqlite3_free(zErrMsg);
		}
		else
		{
			write_text_to_log_file("Generic setting pairs table created");
		}
	}

	if (oldVersion < 3)
	{
		write_text_to_log_file("Main vehicle table not found, so creating it");
		char* CREATE_VEHICLE_TABLE_QUERY = "CREATE TABLE ENT_SAVED_VEHICLES ( \
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, \
			saveName TEXT NOT NULL, \
			model INTEGER NOT NULL, \
			colourPrimary INTEGER, \
			colourSecondary INTEGER, \
			colourExtraPearl INTEGER, \
			colourExtraWheel INTEGER, \
			colourMod1Type INTEGER, \
			colourMod1Colour INTEGER, \
			colourMod1P3 INTEGER, \
			colourMod2Type INTEGER, \
			colourMod2Colour INTEGER, \
			colourCustom1R INTEGER, \
			colourCustom1G INTEGER, \
			colourCustom1B INTEGER, \
			colourCustom2R INTEGER, \
			colourCustom2G INTEGER, \
			colourCustom2B INTEGER, \
			livery INTEGER, \
			plateText TEXT, \
			plateType INTEGER, \
			wheelType INTEGER, \
			windowTint INTEGER, \
			burstableTyres INTEGER \
			)";
		int rcVeh1 = sqlite3_exec(db, CREATE_VEHICLE_TABLE_QUERY, NULL, 0, &zErrMsg);
		if (rcVeh1 != SQLITE_OK)
		{
			write_text_to_log_file("Main vehicle table creation problem");
			sqlite3_free(zErrMsg);
		}
		else
		{
			write_text_to_log_file("Main vehicle table created");
		}

		write_text_to_log_file("Vehicle extras table not found, so creating it");
		char* CREATE_VEHEXTRAS_TABLE_QUERY = "CREATE TABLE ENT_VEHICLE_EXTRAS ( \
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, \
			parentId INTEGER NOT NULL, \
			extraId INTEGER NOT NULL, \
			extraState INTEGER NOT NULL, \
			UNIQUE(parentId, extraId), \
			FOREIGN KEY (parentId) REFERENCES ENT_SAVED_VEHICLES(id) ON DELETE CASCADE \
			)";
		int rcVeh2 = sqlite3_exec(db, CREATE_VEHEXTRAS_TABLE_QUERY, NULL, 0, &zErrMsg);																																																																													int rc = sqlite3_exec(db, CREATE_VEHEXTRAS_TABLE_QUERY, NULL, 0, &zErrMsg);
		if (rcVeh2 != SQLITE_OK)
		{
			write_text_to_log_file("Vehicle extras table creation problem");
			sqlite3_free(zErrMsg);
		}
		else
		{
			write_text_to_log_file("Vehicle extras table created");
		}

		write_text_to_log_file("Vehicle mods table not found, so creating it");
		char* CREATE_VEHMODS_TABLE_QUERY = "CREATE TABLE ENT_VEHICLE_MODS ( \
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, \
			parentId INTEGER NOT NULL, \
			modId INTEGER NOT NULL, \
			modState INTEGER NOT NULL, \
			isToggle INTEGER NOT NULL, \
			UNIQUE(parentId, modId), \
			FOREIGN KEY (parentId) REFERENCES ENT_SAVED_VEHICLES(id) ON DELETE CASCADE \
			)";

		int rcVeh3 = sqlite3_exec(db, CREATE_VEHMODS_TABLE_QUERY, NULL, 0, &zErrMsg);
		if (rcVeh3 != SQLITE_OK)
		{
			write_text_to_log_file("Vehicle mods table creation problem");
			sqlite3_free(zErrMsg);
		}
		else
		{
			write_text_to_log_file("Vehicle mods table created");
		}
	}
}

bool ENTDatabase::open()
{
	std::stringstream ss;

	write_text_to_log_file("Initialising DB engine");

	sqlite3_initialize();

	write_text_to_log_file("Opening DB file");

	char* db_path = get_storage_dir_path("ent.db");

	write_text_to_log_file(std::string(db_path));

	int rc = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	delete db_path;
	if (rc == SQLITE_OK)
	{
		write_text_to_log_file("DB opened");
	}
	else
	{
		write_text_to_log_file("DB couldn't be opened or created");
		ss.str(""); ss.clear();
		ss << "DB error code was: " << rc;
		write_text_to_log_file(ss.str());
		close();
		return false;
	}

	int count = 0;
	char* VERSION_TABLE_EXISTS_QUERY = "select count(type) from sqlite_master where type = 'table' and name = 'ENT_DB_MANIFEST' ";
	rc = sqlite3_exec(db, VERSION_TABLE_EXISTS_QUERY, singleIntResultCallback, &count, &zErrMsg);
	if (rc != SQLITE_OK)
	{
		write_text_to_log_file("Couldn't query sqlite_master");
		sqlite3_free(zErrMsg);
	}

	if (count == 0 )
	{
		write_text_to_log_file("Manifest table not found, so creating it");
		char* CREATE_VERSION_TABLE_QUERY = "create table ENT_DB_MANIFEST (MANIFEST_KEY TEXT PRIMARY KEY NOT NULL, MANIFEST_VALUE TEXT NOT NULL)";
		rc = sqlite3_exec(db, CREATE_VERSION_TABLE_QUERY, NULL, 0, &zErrMsg);
		if (rc != SQLITE_OK)
		{
			write_text_to_log_file("Manifest table creation problem");
			sqlite3_free(zErrMsg);
			return false;
		}
		else
		{
			write_text_to_log_file("Manifest table created");
		}
	}

	int dbversion = -1;
	char* FIND_VERSION_TABLE_QUERY = "select CAST(MANIFEST_VALUE as integer) from ENT_DB_MANIFEST where MANIFEST_KEY='VERSION'";
	rc = sqlite3_exec(db, FIND_VERSION_TABLE_QUERY, singleIntResultCallback, &dbversion, &zErrMsg);
	if (rc != SQLITE_OK)
	{
		write_text_to_log_file("Version not found");
		sqlite3_free(zErrMsg);
	}
	else
	{
		ss.str(""); ss.clear();
		ss << "Version found: " << dbversion;
		write_text_to_log_file(ss.str());
		handle_version(dbversion);
	}

	if (dbversion != DATABASE_VERSION)
	{
		ss.str(""); ss.clear();
		ss << "INSERT OR REPLACE INTO ENT_DB_MANIFEST (MANIFEST_KEY, MANIFEST_VALUE) VALUES('VERSION', '" << DATABASE_VERSION << "')";
		auto ssStr = ss.str();
		rc = sqlite3_exec(db, ssStr.c_str(), emptyCallback, NULL, &zErrMsg);
		if (rc != SQLITE_OK)
		{
			write_text_to_log_file("Couldn't update version");
			write_text_to_log_file(ss.str());
			write_text_to_log_file(zErrMsg);
			sqlite3_free(zErrMsg);
		}
		else
		{
			write_text_to_log_file("Updated version");
		}
	}

	return true;
}

void ENTDatabase::close()
{
	write_text_to_log_file("DB closing");
	if (db != NULL)
	{
		sqlite3_close_v2(db);
		db = NULL;
	}
	write_text_to_log_file("DB closed");
	sqlite3_shutdown();
	write_text_to_log_file("DB shutdown");
}

void ENTDatabase::store_feature_enabled_pairs(std::vector<FeatureEnabledLocalDefinition> values)
{
	write_text_to_log_file("Asked to store feature pairs");
	for (int i = 0; i < values.size(); i++)
	{
		FeatureEnabledLocalDefinition def = values.at(i);
		std::stringstream ss;
		ss << "REPLACE INTO ENT_FEATURE_ENABLEMENT VALUES ('" << def.name << "', " << (*def.enabled ? 1 : 0) << ")";
		auto ssStr = ss.str();
		int rc = sqlite3_exec(db, ssStr.c_str(), emptyCallback, NULL, &zErrMsg);
		if (rc != SQLITE_OK)
		{
			write_text_to_log_file("Failed to insert feature enablement row");
			write_text_to_log_file(ss.str());
			write_text_to_log_file(zErrMsg);
			sqlite3_free(zErrMsg);
			return;
		}
	}
	write_text_to_log_file("Done storing feature pairs");
}

void ENTDatabase::load_feature_enabled_pairs(std::vector<FeatureEnabledLocalDefinition> values)
{
	{
		std::stringstream ss;
		ss << "Asked to load " << values.size() << " feature pairs";
		write_text_to_log_file(ss.str());
	}
	std::vector<FeatureEnabledDBRow> dbPairs;
	char* QUERY = "select FEATURE_NAME, FEATURE_ENABLED from ENT_FEATURE_ENABLEMENT";
	int rc = sqlite3_exec(db, QUERY, featureEnablementFetchCallback, &values, &zErrMsg);
	if (rc != SQLITE_OK)
	{
		write_text_to_log_file("Pairs not loaded");
		write_text_to_log_file(zErrMsg);
		sqlite3_free(zErrMsg);
	}
	else
	{
		write_text_to_log_file("Done loading feature pairs");
		return;
	}
}

void ENTDatabase::store_setting_pairs(std::vector<StringPairSettingDBRow> values)
{
	write_text_to_log_file("Asked to store generic pairs");
	for (int i = 0; i < values.size(); i++)
	{
		StringPairSettingDBRow setting = values.at(i);
		std::stringstream ss;
		ss << "REPLACE INTO ENT_SETTING_PAIRS VALUES (?, ?);";

		sqlite3_stmt *stmt;
		const char *pzTest;
		auto ssStr = ss.str();
		int rc = sqlite3_prepare_v2(db, ssStr.c_str(), ssStr.length(), &stmt, &pzTest);

		if (rc == SQLITE_OK)
		{
			// bind the value
			sqlite3_bind_text(stmt, 1, setting.name.c_str(), setting.name.length(), 0);
			sqlite3_bind_text(stmt, 2, setting.value.c_str(), setting.value.length(), 0);

			// commit
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
		else
		{
			write_text_to_log_file("Failed to prepare setting row");
			write_text_to_log_file(ss.str());
			write_text_to_log_file(sqlite3_errmsg(db));
			break;
		}
	}
	write_text_to_log_file("Done storing generic pairs");
}

std::vector<StringPairSettingDBRow> ENTDatabase::load_setting_pairs()
{
	write_text_to_log_file("Asked to load generic pairs");
	std::vector<StringPairSettingDBRow> dbPairs;
	char* QUERY = "select SETTING_NAME, SETTING_VALUE from ENT_SETTING_PAIRS";
	int rc = sqlite3_exec(db, QUERY, genericSettingPairsFetchCallback, &dbPairs, &zErrMsg);
	if (rc != SQLITE_OK)
	{
		write_text_to_log_file("Pairs not loaded");
		write_text_to_log_file(zErrMsg);
		sqlite3_free(zErrMsg);
	}
	write_text_to_log_file("Done loading generic pairs");
	return dbPairs;
}

void ENTDatabase::save_vehicle_extras(Vehicle veh, int rowID)
{
	for (int i = 1; i < 10; i++)
	{
		if (!VEHICLE::DOES_EXTRA_EXIST(veh, i))
		{
			continue;
		}

		std::stringstream ss;
		ss << "INSERT INTO ENT_VEHICLE_EXTRAS VALUES (?, ?, ?, ?)";

		sqlite3_stmt *stmt;
		const char *pzTest;
		auto ssStr = ss.str();
		int rc = sqlite3_prepare_v2(db, ssStr.c_str(), ssStr.length(), &stmt, &pzTest);

		if (rc != SQLITE_OK)
		{
			write_text_to_log_file("Vehicle extras save failed");
			write_text_to_log_file(sqlite3_errmsg(db));
			return;
		}

		int index = 1;
		sqlite3_bind_null(stmt, index++);
		sqlite3_bind_int(stmt, index++, rowID);
		sqlite3_bind_int(stmt, index++, i);
		sqlite3_bind_int(stmt, index++, VEHICLE::IS_VEHICLE_EXTRA_TURNED_ON(veh, i) ? 1 : 0 );

		// commit
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
	
}

void ENTDatabase::save_vehicle_mods(Vehicle veh, int rowID)
{
	for (int i = 0; i < 25; i++)
	{
		std::stringstream ss;
		ss << "INSERT INTO ENT_VEHICLE_MODS VALUES (?, ?, ?, ?, ?)";

		sqlite3_stmt *stmt;
		const char *pzTest;
		auto ssStr = ss.str();
		int rc = sqlite3_prepare_v2(db, ssStr.c_str(), ssStr.length(), &stmt, &pzTest);

		if (rc != SQLITE_OK)
		{
			write_text_to_log_file("Vehicle mods save failed");
			write_text_to_log_file(sqlite3_errmsg(db));
			return;
		}

		bool isToggleable = (i >= 17 && i <= 22);

		int index = 1;
		sqlite3_bind_null(stmt, index++);
		sqlite3_bind_int(stmt, index++, rowID);
		sqlite3_bind_int(stmt, index++, i);

		if (isToggleable)
		{
			sqlite3_bind_int(stmt, index++, VEHICLE::IS_TOGGLE_MOD_ON(veh, i) ? 1 : 0);
		}
		else
		{
			sqlite3_bind_int(stmt, index++, VEHICLE::GET_VEHICLE_MOD(veh, i));
		}
		sqlite3_bind_int(stmt, index++, isToggleable ? 1 : 0);
		// commit
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
}

bool ENTDatabase::save_vehicle(Vehicle veh, std::string saveName, int slot)
{
	std::stringstream ss;
	ss << "INSERT INTO ENT_SAVED_VEHICLES VALUES (?, ?, ?, ?, ?, \
			?, ?, ?, ?, ?, \
		  	?, ?, ?, ?, ?, \
			?, ?, ?, ?, ?, \
			?, ?, ?, ? );";

	/*
	id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, \ 1
			saveName TEXT NOT NULL, \ 2
			model INTEGER NOT NULL, \ 3
			colourPrimary INTEGER, \ 4
			colourSecondary INTEGER, \ 5
			colourExtraPearl INTEGER, \ 6
			colourExtraWheel INTEGER, \ 7
			colourMod1Type INTEGER, \ 8
			colourMod1Colour INTEGER, \ 9
			colourMod1P3 INTEGER, \ 10
			colourMod2Type INTEGER, \ 11
			colourMod2Colour INTEGER, \ 12
			colourCustom1R INTEGER, \ 14
			colourCustom1G INTEGER, \ 15
			colourCustom1B INTEGER, \ 16
			colourCustom2R INTEGER, \ 17
			colourCustom2G INTEGER, \ 18
			colourCustom2B INTEGER, \ 19
	*/

	sqlite3_stmt *stmt;
	const char *pzTest;
	auto ssStr = ss.str();
	int rc = sqlite3_prepare_v2(db, ssStr.c_str(), ssStr.length(), &stmt, &pzTest);

	if (rc != SQLITE_OK)
	{
		write_text_to_log_file("Vehicle save failed");
		write_text_to_log_file(sqlite3_errmsg(db));
		return false;
	}

	int index = 1;
	sqlite3_bind_null(stmt, index++);
	sqlite3_bind_text(stmt, index++, saveName.c_str(), saveName.length(), 0); //save name
	sqlite3_bind_int(stmt, index++, ENTITY::GET_ENTITY_MODEL(veh)); //model

	int primaryCol, secondaryCol;
	VEHICLE::GET_VEHICLE_COLOURS(veh, &primaryCol, &secondaryCol);
	sqlite3_bind_int(stmt, index++, primaryCol);
	sqlite3_bind_int(stmt, index++, secondaryCol);

	int pearlCol, wheelCol;
	VEHICLE::GET_VEHICLE_EXTRA_COLOURS(veh, &pearlCol, &wheelCol);
	sqlite3_bind_int(stmt, index++, pearlCol);
	sqlite3_bind_int(stmt, index++, wheelCol);

	int mod1a, mod1b, mod1c;
	VEHICLE::GET_VEHICLE_MOD_COLOR_1(veh, &mod1a, &mod1b, &mod1c);
	sqlite3_bind_int(stmt, index++, mod1a);
	sqlite3_bind_int(stmt, index++, mod1b);
	sqlite3_bind_int(stmt, index++, mod1c);

	int mod2a, mod2b;
	VEHICLE::GET_VEHICLE_MOD_COLOR_2(veh, &mod2a, &mod2b);
	sqlite3_bind_int(stmt, index++, mod2a);
	sqlite3_bind_int(stmt, index++, mod2b);

	int custR1, custG1, custB1;
	VEHICLE::GET_VEHICLE_CUSTOM_PRIMARY_COLOUR(veh, &custR1, &custG1, &custB1 );
	sqlite3_bind_int(stmt, index++, custR1);
	sqlite3_bind_int(stmt, index++, custG1);
	sqlite3_bind_int(stmt, index++, custB1);

	int custR2, custG2, custB2;
	VEHICLE::GET_VEHICLE_CUSTOM_SECONDARY_COLOUR(veh, &custR2, &custG2, &custB2);
	sqlite3_bind_int(stmt, index++, custR2);
	sqlite3_bind_int(stmt, index++, custG2);
	sqlite3_bind_int(stmt, index++, custB2);

	/*
	livery INTEGER, \ 20
		plateText TEXT, \ 21
		plateType INTEGER, \ 22
		wheelType INTEGER, \ 23
		windowTint INTEGER, \ 24
		burstableTyres INTEGER \ 25
		*/

	sqlite3_bind_int(stmt, index++, VEHICLE::GET_VEHICLE_LIVERY(veh));
	
	char* plateText = VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(veh);
	sqlite3_bind_text(stmt, index++, plateText, strlen(plateText), 0);

	sqlite3_bind_int(stmt, index++, VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT_INDEX(veh));
	sqlite3_bind_int(stmt, index++, VEHICLE::GET_VEHICLE_WHEEL_TYPE(veh));
	sqlite3_bind_int(stmt, index++, VEHICLE::GET_VEHICLE_WINDOW_TINT(veh));
	sqlite3_bind_int(stmt, index++, VEHICLE::GET_VEHICLE_TYRES_CAN_BURST(veh) ? 1 : 0);

	// commit
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	int newRowID = sqlite3_last_insert_rowid(db);
	save_vehicle_extras(veh, newRowID);
	save_vehicle_mods(veh, newRowID);

	return true;
}

std::vector<SavedVehicleDBRow*> ENTDatabase::get_saved_vehicles(int index)
{
	write_text_to_log_file("Asked to load saved vehicles");

	sqlite3_stmt *stmt;
	const char *pzTest;

	std::stringstream ss;
	ss << "select * from ENT_SAVED_VEHICLES";
	if (index != -1)
	{
		ss << " WHERE id = ?";
	}
	auto qStr = ss.str();
	int rc = sqlite3_prepare_v2(db, qStr.c_str(), qStr.length(), &stmt, &pzTest);

	std::vector<SavedVehicleDBRow*> results;

	if (rc == SQLITE_OK)
	{
		// bind the value
		if (index != -1)
		{
			sqlite3_bind_int(stmt, 1, index);
		}

		int r = sqlite3_step(stmt);
		while (r == SQLITE_ROW)
		{
			write_text_to_log_file("Vehicle row found");

			SavedVehicleDBRow *veh = new SavedVehicleDBRow();

			int index = 0;
			veh->rowID = sqlite3_column_int(stmt, index++);
			veh->saveName = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, index++)));
			veh->model = sqlite3_column_int(stmt, index++);
			veh->colourPrimary = sqlite3_column_int(stmt, index++);
			veh->colourSecondary = sqlite3_column_int(stmt, index++);
			veh->colourExtraPearl = sqlite3_column_int(stmt, index++);
			veh->colourExtraWheel = sqlite3_column_int(stmt, index++);
			veh->colourMod1Type = sqlite3_column_int(stmt, index++);
			veh->colourMod1Colour = sqlite3_column_int(stmt, index++);
			veh->colourMod1P3 = sqlite3_column_int(stmt, index++);
			veh->colourMod2Type = sqlite3_column_int(stmt, index++);
			veh->colourMod2Colour = sqlite3_column_int(stmt, index++);
			veh->colourCustom1RGB[0] = sqlite3_column_int(stmt, index++);
			veh->colourCustom1RGB[1] = sqlite3_column_int(stmt, index++);
			veh->colourCustom1RGB[2] = sqlite3_column_int(stmt, index++);
			veh->colourCustom2RGB[0] = sqlite3_column_int(stmt, index++);
			veh->colourCustom2RGB[1] = sqlite3_column_int(stmt, index++);
			veh->colourCustom2RGB[2] = sqlite3_column_int(stmt, index++);

			veh->livery = sqlite3_column_int(stmt, index++);
			veh->plateText = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, index++)));
			veh->plateType = sqlite3_column_int(stmt, index++);
			veh->wheelType = sqlite3_column_int(stmt, index++);
			veh->windowTint = sqlite3_column_int(stmt, index++);
			veh->burstableTyres = sqlite3_column_int(stmt, index++) == 1 ? true : false;

			results.push_back(veh);

			r = sqlite3_step(stmt);
		}
	}
	else
	{

	}

	return results;
}

void ENTDatabase::populate_saved_vehicle(SavedVehicleDBRow *entry)
{
	sqlite3_stmt *stmt;
	const char *pzTest;
	auto qStr = "select * from ENT_VEHICLE_EXTRAS WHERE parentId=?";
	int rc = sqlite3_prepare_v2(db, qStr, strlen(qStr), &stmt, &pzTest);

	if (rc == SQLITE_OK)
	{
		// bind the value
		sqlite3_bind_int(stmt, 1, entry->rowID);

		// commit
		int r = sqlite3_step(stmt);
		while (r == SQLITE_ROW)
		{
			SavedVehicleExtraDBRow *extra = new SavedVehicleExtraDBRow();
			//0 and 1 are IDs
			extra->extraID = sqlite3_column_int(stmt, 2);
			extra->extraState = sqlite3_column_int(stmt, 3);
			entry->extras.push_back(extra);
			r = sqlite3_step(stmt);
		}

		sqlite3_finalize(stmt);
	}
	else
	{
		write_text_to_log_file("Failed to fetch saved vehicle extras");
		write_text_to_log_file(sqlite3_errmsg(db));
	}

	sqlite3_stmt *stmt2;
	const char *pzTest2;
	auto qStr2 = "select * from ENT_VEHICLE_MODS WHERE parentId=?";
	int rc2 = sqlite3_prepare_v2(db, qStr2, strlen(qStr2), &stmt2, &pzTest2);

	if (rc2 == SQLITE_OK)
	{
		// bind the value
		sqlite3_bind_int(stmt2, 1, entry->rowID);

		// commit
		int r = sqlite3_step(stmt2);
		while (r == SQLITE_ROW)
		{
			SavedVehicleModDBRow *mod = new SavedVehicleModDBRow();
			//0 and 1 are IDs
			mod->modID = sqlite3_column_int(stmt, 2);
			mod->modState = sqlite3_column_int(stmt, 3);
			mod->isToggle = (sqlite3_column_int(stmt, 4) == 1) ? true : false;
			entry->mods.push_back(mod);
			r = sqlite3_step(stmt2);
		}

		sqlite3_finalize(stmt2);
	}
	else
	{
		write_text_to_log_file("Failed to fetch saved vehicle mods");
		write_text_to_log_file(sqlite3_errmsg(db));
	}

	write_text_to_log_file("Done loading saved vehicles");
	return;
}

void ENTDatabase::delete_saved_vehicle(int slot)
{
	sqlite3_stmt *stmt;
	const char *pzTest;
	auto qStr = "DELETE FROM ENT_SAVED_VEHICLES WHERE id=?";
	int rc = sqlite3_prepare_v2(db, qStr, strlen(qStr), &stmt, &pzTest);

	if (rc == SQLITE_OK)
	{
		// bind the value
		sqlite3_bind_int(stmt, 1, slot);

		// commit
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
	else
	{
		write_text_to_log_file("Failed to delete saved vehicle");
		write_text_to_log_file(sqlite3_errmsg(db));
	}
}