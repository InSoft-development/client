import signal
import time
from datetime import datetime
import pandas as pd
from clickhouse_driver import Client

terminate_flag = False


def handler_stop(signum, frame):
    global terminate_flag
    terminate_flag = True


# soft stop at next cycle iteration
signal.signal(signal.SIGTERM, handler_stop)
signal.signal(signal.SIGINT, handler_stop)

t = datetime.now()
print("data read")

client = Client(user="default", password="asdf", host="10.23.0.177")
slices = client.query_dataframe("SELECT * from slices where timestamp > \'2021-06-04 00:00:00\' order by timestamp")
# def dateparse(x): return pd.to_datetime(x)  # datetime.datetime.strptime(x, '%Y-%m-%d %H:%M:%S.%f')
# tp = pd.read_csv("slices.csv",
#                  # nrows = 100,
#                  delimiter=",",
#                  # dtype={'kks':'str','value':'float','timestamp':'str','status':'str'},
#                  parse_dates=['timestamp'],
#                  infer_datetime_format=True,
#                  date_parser=dateparse,
#                  engine='c',
#                  on_bad_lines='warn',
#                  verbose=True,
#                  low_memory=False,
#                  chunksize=100000
#                  )
# slices = pd.concat(tp, ignore_index=True)
# print(datetime.now() - t)
# print(slices)

client.execute("DROP TABLE IF EXISTS slices_play")
sql = "CREATE TABLE slices_play (\"" + '\" Float64, \"'.join(slices.keys()) + "\" DateTime64) ENGINE = " \
      "MergeTree() PARTITION BY toYYYYMM(timestamp) ORDER BY (timestamp) PRIMARY KEY (timestamp)"
# print(sql, "\n\n")
client.execute(sql)
while not terminate_flag:
    for index, row in slices.iterrows():
        t = datetime.now()
        print(row["timestamp"])
        row["timestamp"] = t
        d = pd.DataFrame([row])
        sql = 'INSERT INTO slices_play (*) VALUES'
        client.insert_dataframe(sql, d, settings=dict(use_numpy=True))
        #length = client.execute("SELECT count() from slices_play")[0][0]
        #print(length)
        if terminate_flag:
            break
        else:
            time.sleep(5)
    print("from begin")
length = client.execute("SELECT count() from slices_play")[0][0]
print("inserted", length, "rows")
client.disconnect()
print("disconnected")
