package.cpath = "/home/work/caoguoliang/lua-mysql/lib/?.so;" .. package.cpath

local mysql = require("mysql")

--[[
--mysql.OPTION.INIT_COMMAND = "aaa"

for k, v in pairs(mysql.OPTION) do
  print(k, v)
end
--]]

local dbconfig = {
	host = "10.10.10.10",
	port = 3606,
	user = "user",
	passwd = "passwd",
	db = "dbname"
}

local options = {}
options[mysql.OPTION.CONNECT_TIMEOUT] = 3
options[mysql.OPTION.CHARSET_NAME] = "utf8"

local dbh = mysql.new(dbconfig.host, dbconfig.user, dbconfig.passwd, dbconfig.db, dbconfig.port, options)

dbh:set_charset("utf8")

local l, sql = dbh:real_escape_string("SELECT * FROM `developer` LIMIT 4")
print(l, sql)

local r1, r2 = dbh:query(sql)
print(r1, #r2)
for i, t in ipairs(r2) do
	for k, v in pairs(t) do
		print(k, v)
	end
end

dbh:close()
