# client
c++ client for online data receiving from OpcUA and history access

slicer for transform raw historical data from timeseries format into slices
for now - using local data.sqlite

clickhouse_fill.py - send data from slices.csv to clickhouse

clickhouse_play.py - play data from table slices to table slices_play, as an emulation of working stand, accelerated 60 times (each 5 seconds instead 5 minutes)
last_row.py - example of receiving data from clickhouse in online mode

examples:

historical access:

./client_lesson02 -b 2021-06-01T00:00:00Z -e 2022-10-30T00:00:10Z -p 100 -t 10000

./slicer.py -t "2021-06-01" "2022-10-31"

online opc ua data access:

./client_lesson02 -o
