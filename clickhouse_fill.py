import clickhouse_connect
import pandas as pd
from datetime import datetime

t = datetime.now()
print("data read")
def dateparse(x): return pd.to_datetime(x) #datetime.datetime.strptime(x, '%Y-%m-%d %H:%M:%S.%f')
tp = pd.read_csv("slices.csv",
   #nrows = 100,
   delimiter=",",
   #dtype={'kks':'str','value':'float','timestamp':'str','status':'str'},
   parse_dates=['timestamp'],
   infer_datetime_format=True,
   date_parser=dateparse,
   engine='c',
   on_bad_lines = 'warn',
   verbose=True,
   low_memory=False,
   chunksize=100000
   )
slices = pd.concat(tp,ignore_index=True)
print(datetime.now() - t)
print(slices)
client = clickhouse_connect.get_client(host='localhost', username='default', password='')
client.command("DROP TABLE IF EXISTS slices")
sql = "CREATE TABLE slices (\"" + '\" Float64, \"'.join(slices.keys()) + "\" DateTime64) ENGINE = " \
    "MergeTree() PARTITION BY toYYYYMM(timestamp) ORDER BY (timestamp) PRIMARY KEY (timestamp)"
#print(sql, "\n\n")
client.command(sql)

t = datetime.now()
client.insert_df("slices", slices)
print(datetime.now() - t)

t = datetime.now()
slices_new = client.query_df("SELECT * from slices")
print(datetime.now() - t)

print(slices_new)
