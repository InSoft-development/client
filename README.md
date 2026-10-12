# client library for work with OPC UA data

* client_lesson02 - a c++ client for online data receiving from OpcUA and history access
* slicer.py - transform raw historical data from timeseries VQT format
into slices
* clickhouse_fill.py - send data from slices.csv to clickhouse
* clickhouse_play.py - play data from table slices to table slices_play, as an emulation of working stand, accelerated 60 times (each 5 seconds instead 5 minutes)
* last_row.py - example of receiving data from clickhouse in online mode

## client - read data from OPC UA

### options (as in -h option)

read data from OPC UA
options:
--help(-h) this info
--opc-server (-a) opc server address 
--clickhouse-server (-u) clickhouse server ip (table dynamic_data and static_data would be used)
--file (-f) store result in local csv file
--ns(-s) number of space (1 by default)
--kks-file (-K) specify kks file (defult kks.csv)
KKS  MODE:
--kks(-k) <id> kks browse mode. List subobjects from <id> object. "all" - from root folder, "begin" - from begin of object folder. Results would be printed to out - or file, if  (-f) used 
--recursive (-c) <type> read tags recursively from all objects subobjects, filtered by type, type "all" or "false" means no filtration. Storing to file, if (-f) 
ONLINE:
--online(-o) online mode 
--delta(-d) miliseconds between reading from OPC UA, default 1000
--mean(-m) count of averaging: 1 means we don't calculate average and send each slice to DB, 5 - we calculate 5 slices to one mean and send it to DB. default 5
SUBSCRIPTION:
--subscription(-S) subscription mode
HISTORY MODE:
--history(-i) history mode (default)
--begin(-b) <timestamp> in YYYY-MM-DDTHH:MM:SS.MMMZ format (e.g. 2021-06-01T00:00:00.000Z
--end(-e) <timestramp>
--pause(-p) <miliseconds> pause between requests. default 5000. After successfull reading of each tag - connection would be disconnected, wait 3*pause and reconnecetd. It's not recommended to set pause less, then 10. It's not recommended to read more than 1 million records from one tag at one time - use more short intervals (one month for example) foe big tags
--timeout(-t) <ms> maximum timeout, that we are waiting for response from server
--read-bounds(-r) if we need to read bounds
--no-bounds(-n) if we don't want read bounds (default)
--rewrite(-w) rewrite db:e xisted tables dynamic_data and static_data would be dropped
--read-bad(-x) read also bad values (default false)

### examples:

historical access:

this example would read all data between timestamps and send it to 
clickhouse database in "VQT" format into table dynamic_data. 
Also table "static_data" would be formed. Then, using slicer, 
we would transform data in clickhouse database: from table 
"dynamic_data" in VQT format - to slices into local csv file or table "synchro_data".
We can use multiple options: from clickhouse or sqlite -
to another clickhouse or sqlite, or csv.

./client_lesson02 -b 2021-06-01T00:00:00Z -e 2021-07-01T00:00:00Z -p 100 -t 10000 -u "10.23.23.32" -w

./slicer.py -t "2021-06-01" "2021-07-01 00:00:00" -i "10.23.23.32" -o slices.csv

online opc ua data access:

would retrieve actual data for tags, listed in kks.csv. With delta 100 miliseconds, and averaging for ten times (actually we would read every 10 milliseconds - and then calculate mean for ten values)

./client -o -d 100 -m 10

kks browsing:

Would recursively give all tags with there subtags for 1_Сочинская_ТЭС_блок_2 

./client -k 00_Блок_2.01_Сочинская_ТЭС_блок_2 -c
