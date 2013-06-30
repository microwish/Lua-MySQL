/**
 * MySQL extension for Lua 5.1
 *
 * this verion is short of cache/persistence mechanism as the complexity
 * see lmysqllib_withvmlevelcache_semifinished.c if you're interested, which is under a semi-finished status
 * TODO:
 * put cached/persistent connections in registry or luaopen_mysql's environment
 *
 * @author microwish@gmail.com
 */
#include <mysql/mysql.h>

#include <lua.h>
#include <lauxlib.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LUA_MYSQLHANDLE "MYSQL *"

static MYSQL **newmysql(lua_State *L)
{
	MYSQL **mpp = (MYSQL **)lua_newuserdata(L, sizeof(MYSQL *));
	*mpp = NULL;//set uninitialized
	luaL_getmetatable(L, LUA_MYSQLHANDLE);
	lua_setmetatable(L, -2);//set mt for ud to implement oo
	return mpp;//return leaving ud on the stack
}

static int i_new(lua_State *L)
{
	const char *host = NULL, *user = NULL, *passwd = NULL, *db = NULL, *unix_socket = NULL;
	unsigned int port = 0;
	int nargs = lua_gettop(L);

	switch (nargs) {
		case 7:
			if (lua_type(L, 7) != LUA_TSTRING) {
				lua_pushboolean(L, 0);
				lua_pushliteral(L, "Bad arg#7 type");
				return 2;
			}
			unix_socket = lua_tostring(L, 6);
			lua_pop(L, 1);
		case 6:
			if (lua_type(L, 6) != LUA_TTABLE) {
				lua_pushboolean(L, 0);
				lua_pushliteral(L, "Bad arg#6 type");
				return 2;
			}
		case 5:
			if (lua_type(L, 5) != LUA_TNUMBER) {
				lua_pushboolean(L, 0);
				lua_pushliteral(L, "Bad arg#5 type");
				return 2;
			}
			port = (unsigned int)lua_tonumber(L, 5);
		case 4:
		{
			switch (lua_type(L, 4)) {
				case LUA_TSTRING:
					db = lua_tostring(L, 4);
				case LUA_TNIL:
					break;
				default:
					lua_pushboolean(L, 0);
					lua_pushliteral(L, "Bad arg#4 type");
					return 2;
			}
		}
		case 3:
			if (lua_type(L, 1) != LUA_TSTRING
					|| lua_type(L, 2) != LUA_TSTRING
					|| lua_type(L, 3) != LUA_TSTRING) {
				lua_pushboolean(L, 0);
				lua_pushliteral(L, "Bad arg#4 type");
				return 2;
			}
			host = lua_tostring(L, 1);
			user = lua_tostring(L, 2);
			passwd = lua_tostring(L, 3);
			break;
		default:
			lua_pushboolean(L, 0);
			lua_pushliteral(L, "Number of arguments [3,7]");
			return 2;
	}

	if (nargs > 5) {
		lua_replace(L, 1);
		lua_pop(L, 5);
	} else {
		lua_pop(L, nargs);
	}

	unsigned long client_flag = 0;

	//create a new mysql handle
	MYSQL **mpp = newmysql(L);
	if (!(*mpp = mysql_init(NULL))) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "Cannot init MySQL");
		return 2;
	}

	if (lua_istable(L, 1)) {

		lua_pushnil(L);
		while (lua_next(L, 1)) {
			switch (lua_tointeger(L, -2)) {
				case MYSQL_OPT_CONNECT_TIMEOUT:
				{
					if (!lua_isnumber(L, -1)) {
						mysql_close(*mpp);
						lua_pushboolean(L, 0);
						lua_pushliteral(L, "Invalid option value for CONNECT_TIMEOUT");
						return 2;
					}
					unsigned int ui = (unsigned int)lua_tointeger(L, -1);
					mysql_options(*mpp, MYSQL_OPT_CONNECT_TIMEOUT, &ui);
					break;
				}
				case MYSQL_SET_CHARSET_NAME:
					if (!lua_isstring(L, -1)) {
						mysql_close(*mpp);
						lua_pushboolean(L, 0);
						lua_pushliteral(L, "Invalid option value for CHARSET_NAME");
						return 2;
					}
					mysql_options(*mpp, MYSQL_SET_CHARSET_NAME, lua_tostring(L, -1));
					break;
				case MYSQL_INIT_COMMAND:
					if (!lua_isstring(L, -1)) {
						mysql_close(*mpp);
						lua_pushboolean(L, 0);
						lua_pushliteral(L, "Invalid option value for INIT_COMMAND");
						return 2;
					}
					mysql_options(*mpp, MYSQL_INIT_COMMAND, lua_tostring(L, -1));
					break;
				default:
					mysql_close(*mpp);
					lua_pushboolean(L, 0);
					lua_pushfstring(L, "Invalid option");
					return 2;
			}
			lua_pop(L, 1);
		}
	}

	client_flag |= CLIENT_MULTI_RESULTS;
	client_flag &= ~CLIENT_MULTI_STATEMENTS;
	client_flag &= ~CLIENT_LOCAL_FILES;

	if (!mysql_real_connect(*mpp, host, user, passwd, db, port, unix_socket, client_flag)) {
		mysql_close(*mpp);
		lua_pushboolean(L, 0);
		lua_pushfstring(L, "errno [%f]: %s", (lua_Number)mysql_errno(*mpp), mysql_error(*mpp));
		return 2;
	}

	return 1;
}

static int i_close(lua_State *L)
{
	if (lua_gettop(L) != 1) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "argument error");
		return 2;
	}

	MYSQL **mpp = (MYSQL **)luaL_checkudata(L, 1, LUA_MYSQLHANDLE);
	if (!*mpp) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "Bad MySQL handle");
		return 2;
	}

	lua_pushboolean(L, 1);

	mysql_close(*mpp);

	*mpp = NULL;

	return 1;
}

static int gctm(lua_State *L)
{
	MYSQL **mpp = (MYSQL **)luaL_checkudata(L, 1, LUA_MYSQLHANDLE);
	if (*mpp) mysql_close(*mpp);
	*mpp = NULL;
	return 0;
}

static int i_set_charset(lua_State *L)
{
	if (lua_gettop(L) != 2) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "argument error");
		return 2;
	}

	MYSQL **mpp = (MYSQL **)luaL_checkudata(L, 1, LUA_MYSQLHANDLE);
	if (!*mpp) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "Bad MySQL handle");
		return 2;
	}

	if (lua_type(L, 2) != LUA_TSTRING) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "Charset name must be a string");
		return 2;
	}

	size_t u;
	const char *csname = lua_tolstring(L, 2, &u);

	if (!u) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "Charset name must not be empty string");
		return 2;
	}

	lua_pop(L, 1);

	if (mysql_set_character_set(*mpp, csname)) {
		lua_pushboolean(L, 0);
		lua_pushfstring(L, "%s failed, errno [%f]: %s", csname, (lua_Number)mysql_errno(*mpp), mysql_error(*mpp));
		return 2;
	}

	lua_pushboolean(L, 1);

	return 1;
}

static int i_select_db(lua_State *L)
{
	if (lua_gettop(L) != 2) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "argument error");
		return 2;
	}

	MYSQL **mpp = (MYSQL **)luaL_checkudata(L, 1, LUA_MYSQLHANDLE);
	if (!*mpp) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "Bad MySQL handle");
		return 2;
	}

	if (lua_type(L, 2) != LUA_TSTRING) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "DB name must be a string");
		return 2;
	}

	size_t u;
	const char *db = lua_tolstring(L, 2, &u);

	if (!u) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "DB name must not be empty string");
		return 2;
	}

	lua_pop(L, 1);

	if (mysql_select_db(*mpp, db)) {
		lua_pushboolean(L, 0);
		lua_pushfstring(L, "%s failed, errno [%f]: %s", db, (lua_Number)mysql_errno(*mpp), mysql_error(*mpp));
		return 2;
	}

	lua_pushboolean(L, 1);

	return 1;
}

static int i_real_escape_string(lua_State *L)
{
	if (lua_gettop(L) != 2) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "argument error");
		return 2;
	}

	MYSQL **mpp = (MYSQL **)luaL_checkudata(L, 1, LUA_MYSQLHANDLE);
	if (!*mpp) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "Bad MySQL handle");
		return 2;
	}

	if (lua_type(L, 2) != LUA_TSTRING) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "Must be a string");
		return 2;
	}

	size_t u;
	const char *orig = lua_tolstring(L, 2, &u);

	if (!u) {
		lua_pushinteger(L, 0);
		lua_pushfstring(L, "");
		return 2;
	}

	lua_pop(L, 1);

	char escaped[1204];

	u = mysql_real_escape_string(*mpp, escaped, orig, u);

	lua_pushinteger(L, u);
	lua_pushlstring(L, escaped, u);

	return 2;
}

static int i_autocommit(lua_State *L)
{
	if (lua_gettop(L) != 2) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "argument error");
		return 2;
	}

	MYSQL **mpp = (MYSQL **)luaL_checkudata(L, 1, LUA_MYSQLHANDLE);
	if (!*mpp) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "Bad MySQL handle");
		return 2;
	}

	if (lua_type(L, 2) != LUA_TBOOLEAN) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "Must be a boolean value");
		return 2;
	}

	int mode = lua_toboolean(L, 2);

	lua_pop(L, 1);

	if (mysql_autocommit(*mpp, mode)) {
		lua_pushboolean(L, 0);
		lua_pushfstring(L, "Switch autocommit failed");
		return 2;
	}

	lua_pushboolean(L, 1);

	return 1;
}

static int i_query(lua_State *L)
{
	if (lua_gettop(L) != 2) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "argument error");
		return 2;
	}

	MYSQL **mpp = (MYSQL **)luaL_checkudata(L, 1, LUA_MYSQLHANDLE);
	if (!*mpp) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "Bad MySQL handle");
		return 2;
	}

	if (lua_type(L, 2) != LUA_TSTRING) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "SQL statement must be a string");
		return 2;
	}

	size_t u;
	const char *stmt_str = lua_tolstring(L, 2, &u);
	if (!u) {
		lua_pushboolean(L, 0);
		lua_pushliteral(L, "SQL statement must not be empty string");
		return 2;
	}

	lua_pop(L, 1);

	if (mysql_real_query(*mpp, stmt_str, u)) {
		lua_pushboolean(L, 0);
		lua_pushfstring(L, "SQL [%s] errno [%f]: %s", stmt_str, (lua_Number)mysql_errno(*mpp), mysql_error(*mpp));
		return 2;
	}

	MYSQL_RES *mrp;
	unsigned long ul;

	//for SELECT or SELECT-like statement
	if ((u = mysql_field_count(*mpp))) {
		if (!(mrp = mysql_store_result(*mpp))) {
			lua_pushboolean(L, 0);
			lua_pushfstring(L, "SQL [%s] errno [%f]: %s", stmt_str, (lua_Number)mysql_errno(*mpp), mysql_error(*mpp));
			return 2;
		}
		ul = mysql_num_rows(mrp);
		if (ul == 0) {
			mysql_free_result(mrp);
			lua_pushnumber(L, 0);
			lua_newtable(L);
			return 2;
		} else {
			unsigned long i;
			size_t j;
			MYSQL_ROW row;
			MYSQL_FIELD *fields;
			unsigned long *lengths;

			lua_pushnumber(L, (lua_Number)ul);
			//XXX: overflow? ltable.c:luaH_new
			lua_createtable(L, ul, 0);//table at dimension 1

			fields = mysql_fetch_fields(mrp);
			i = 0;
			while ((row = mysql_fetch_row(mrp))) {
				if (!(lengths = mysql_fetch_lengths(mrp))) {
					mysql_free_result(mrp);
					lua_pushboolean(L, 0);
					lua_pushfstring(L, "SQL [%s] errno [%f]: %s", stmt_str, (lua_Number)mysql_errno(*mpp), mysql_error(*mpp));
					return 2;
				}
				lua_createtable(L, 0, u);//current table at dimension 2
				for (j = 0; j < u; j++) {
					lua_pushlstring(L, fields[j].name, fields[j].name_length);
					switch (fields[j].type) {
						case MYSQL_TYPE_LONG:
						case MYSQL_TYPE_LONGLONG:
						case MYSQL_TYPE_TINY:
						case MYSQL_TYPE_INT24:
						case MYSQL_TYPE_SHORT:
						case MYSQL_TYPE_DECIMAL:
						case MYSQL_TYPE_NEWDECIMAL:
						case MYSQL_TYPE_FLOAT:
						case MYSQL_TYPE_DOUBLE:
							lua_pushnumber(L, atof(row[j]));
							break;
						case MYSQL_TYPE_STRING:
						case MYSQL_TYPE_VAR_STRING:
						case MYSQL_TYPE_BLOB:
						case MYSQL_TYPE_DATETIME:
						case MYSQL_TYPE_DATE:
						case MYSQL_TYPE_TIMESTAMP:
						case MYSQL_TYPE_TIME:
							lua_pushlstring(L, row[j], lengths[j]);
							break;
						case MYSQL_TYPE_NULL:
							lua_pushnil(L);
							break;
						default:
							mysql_free_result(mrp);
							lua_pushboolean(L, 0);
							lua_pushfstring(L, "SQL [%s]. Unsupported type", stmt_str);
							return 2;
					}
					lua_rawset(L, -3);
				}
				lua_rawseti(L, -2, ++i);
			}

			mysql_free_result(mrp);

			if (i < ul) {
				lua_pushboolean(L, 0);
				lua_pushfstring(L, "SQL [%s] errno [%f]: %s", stmt_str, (lua_Number)mysql_errno(*mpp), mysql_error(*mpp));
			}

			return 2;
		}
	//for non-SELECT-like statement
	} else {
		lua_pushboolean(L, 1);
		return 1;
	}
}

static const luaL_Reg class_lib[] = {
	{ "new", i_new },
	{ "delete", i_close },
	{ NULL, NULL }
};

static const luaL_Reg obj_lib[] = {
	{ "select_db", i_select_db },
	{ "query", i_query },
	{ "close", i_close },
	{ "real_escape_string", i_real_escape_string },
	{ "set_charset", i_set_charset },
	{ "autocommit", i_autocommit },
	{ "__gc", gctm },
	{ NULL, NULL }
};

static void createmeta(lua_State *L)
{
	luaL_newmetatable(L, LUA_MYSQLHANDLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");//set mt.__index=mt, and push onto stack bottom (pos 1)
	luaL_register(L, NULL, obj_lib);//register obj methods into mt for oo
}

static int settablereadonly(lua_State *L)
{
	return luaL_error(L, "Must not update a read-only table");
}

#define LUA_MYSQLLIBNAME "mysql"

LUALIB_API int luaopen_mysql(lua_State *L)
{
	createmeta(L);

	luaL_register(L, LUA_MYSQLLIBNAME, class_lib);

	lua_newtable(L);

	lua_createtable(L, 0, 2);

	//accordant with enum mysql_option in mysql.h
	lua_createtable(L, 6, 0);
	lua_pushliteral(L, "CONNECT_TIMEOUT");
	lua_pushinteger(L, MYSQL_OPT_CONNECT_TIMEOUT);
	lua_rawset(L, -3);
	lua_pushliteral(L, "CHARSET_NAME");
	lua_pushinteger(L, MYSQL_SET_CHARSET_NAME);
	lua_rawset(L, -3);
	lua_pushliteral(L, "LOCAL_INFILE");
	lua_pushinteger(L, MYSQL_OPT_LOCAL_INFILE);
	lua_rawset(L, -3);
	lua_pushliteral(L, "INIT_COMMAND");
	lua_pushinteger(L, MYSQL_INIT_COMMAND);
	lua_rawset(L, -3);
	lua_pushliteral(L, "READ_DEFAULT_FILE");
	lua_pushinteger(L, MYSQL_READ_DEFAULT_FILE);
	lua_rawset(L, -3);
	lua_pushliteral(L, "READ_DEFAULT_GROUP");
	lua_pushinteger(L, MYSQL_READ_DEFAULT_GROUP);
	lua_rawset(L, -3);

	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, settablereadonly);
	lua_setfield(L, -2, "__newindex");

	lua_setmetatable(L, -2);

	lua_setfield(L, -2, "OPTION");

	return 1;
}
