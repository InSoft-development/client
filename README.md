# client

c++ client for online data receiving from OpcUA and history access

slicer for transform raw historical data from timeseries format into slices

for now - using local data.sqlite

clickhouse_fill.py - send data from slices.csv to clickhouse

clickhouse_play.py - play data from table slices to table slices_play, as an emulation of working stand, accelerated 60 times (each 5 seconds instead 5 minutes)

last_row.py - example of receiving data from clickhouse in online mode

client - read data from OPC UA

options:

--help(-h) this info

--ns(-s) number of space (1 by default)

--kks(-k) <id> kks browse mode 
                 list subobjects from <id> object, strings, values, variables etc.
                 all - from root folder,
                 begin - from begin of object folder

--recursive (-c) read tags recursively from all objects subobjects

--online(-o) online mode 

--history(-i) history mode (default)

ONLINE:

--delta(-d) miliseconds between reading from OPC UA, default 1000

--mean(-m) count of averaging: 1 means we don't calculate average and send each slice to DB, 5 - we calculate 5 slices to one mean and send it to DB. default 5

HISTORY MODE:

--begin(-b) <timestamp> in YYYY-MM-DDTHH:MM:SS.MMMZ format (e.g. 2021-06-01T00:00:00.000Z

--end(-e) <timestramp>

--pause(-p) <miliseconds> pause between requests

--timeout(-t) <ms> maximum timeout, that we are waiting for response from server

--read-bounds(-r) if we need to read bounds

--no-bounds(-n) if we don't want read bounds (default)

--rewrite(-w) rewrite db

--read-bad(-x) read also bad values (default false)


examples:

historical access:

./client -b 2021-06-01T00:00:00Z -e 2022-10-30T00:00:10Z -p 100 -t 10000

./slicer.py -t "2021-06-01" "2022-10-31"

online opc ua data access:

./client -o

kks browsing:

./client -k 00_Блок_2.01_Сочинская_ТЭС_блок_2.20BAC10GS001-MR -c
