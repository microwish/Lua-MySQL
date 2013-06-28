package.cpath = "/home/microwish/lua-mysql/lib/?.so;" .. package.cpath

local mysql = require("mysql")

--[[
--mysql.OPTION.INIT_COMMAND = "aaa"

for k, v in pairs(mysql.OPTION) do
	print(k, v)
end
--]]

local dbconfig = {
	host = "10.1.1.1",
	port = 3606,
	user = "user",
	passwd = "passwd",
	--db = "dbname"
	db = "dbname2"
}

local options = {}
options[mysql.OPTION.CONNECT_TIMEOUT] = 3
options[mysql.OPTION.CHARSET_NAME] = "utf8"

local dbh = assert(mysql.new(dbconfig.host, dbconfig.user, dbconfig.passwd, dbconfig.db, dbconfig.port, options))

assert(dbh:set_charset("utf8"))

assert(dbh:select_db("dbname"))

local l, sql = assert(dbh:real_escape_string("SELECT * FROM `tablename` LIMIT 4"))
print(l, sql)

local r1, r2 = assert(dbh:query(sql))
print(r1, #r2)
--[[
for i, t in ipairs(r2) do
	for k, v in pairs(t) do
		print(k, v)
	end
end
--]]

--dbh:close()
mysql.delete(dbh)

--assert(dbh:set_charset("utf8"))
