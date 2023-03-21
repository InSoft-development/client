# client
c++ client for online data receiving from OpcUA and history access
slicer for transform raw historical data from timeseries format into slices
for now - using local data.sqlite

examples:
historical access:
./client_lesson02 -b 2021-06-01T00:00:00Z -e 2022-10-30T00:00:10Z -p 100 -t 10000
./slicer.py -t "2021-06-01" "2022-10-31"
online opc ua data access:
./client_lesson02 -o
