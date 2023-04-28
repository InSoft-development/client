import signal
import time
from datetime import datetime
import pandas as pd
import clickhouse_connect

terminate_flag = False


def handler_stop(signum, frame):
    global terminate_flag
    terminate_flag = True


# soft stop at next cycle iteration
signal.signal(signal.SIGTERM, handler_stop)
signal.signal(signal.SIGINT, handler_stop)

t = datetime.now()
print("data read")

client = clickhouse_connect.get_client(host='10.23.0.177', username='default', password='asdf')

slices = client.query_df("SELECT * from slices where timestamp > \'2021-06-04 00:00:00\' order by timestamp")
slices.set_index('timestamp', inplace=True)
print(slices)
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

client.command("DROP TABLE IF EXISTS slices_play")
sql = "CREATE TABLE slices_play (\"" + '\" Float64, \"'.join(slices.keys()) + "\" Float64, \"timestamp\" DateTime64(3,'Europe/Moscow'), \"model_timestamp\" DateTime64(3,'Europe/Moscow')) ENGINE = " \
      "MergeTree() PARTITION BY toYYYYMM(timestamp) ORDER BY (timestamp) PRIMARY KEY (timestamp)"
print(sql, "\n\n")
client.command(sql)
while not terminate_flag:
    for index, row in slices.iterrows():
        t = datetime.utcnow()
        row["model_timestamp"]=index
        row["timestamp"] = t
        d = pd.DataFrame([row])
        #print(d)
        client.insert_df('slices_play', d)
        #length = client.execute("SELECT count() from slices_play")[0][0]
        #print(length)
        if terminate_flag:
            break
        else:
            time.sleep(5)
        slices_new = client.query_df("SELECT * from slices_play order by timestamp DESC LIMIT 1")
        print(slices_new)

    print("from begin")
length = client.query("SELECT count() from slices_play").first_item
print("inserted", length, "rows")
