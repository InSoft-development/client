# client library for work with OPC UA data

* client_lesson02 - a c++ client for online data receiving from OpcUA and history access
* slicer.py - transform raw historical data from timeseries VQT format
into slices
* clickhouse_fill.py - send data from slices.csv to clickhouse
* clickhouse_play.py - play data from table slices to table slices_play, as an emulation of working stand, accelerated 60 times (each 5 seconds instead 5 minutes)
* last_row.py - example of receiving data from clickhouse in online mode

## client - read data from OPC UA

options can be observed using option --help(-h). For working we need file server.conf wit ip to opc ua server and kks.csv with list of tags

examples:

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
