import time
import clickhouse_connect
import pandas as pd


client = clickhouse_connect.get_client(host='10.23.0.177', username='default', password='asdf')
client.command("DROP TABLE IF EXISTS sum_row")
client.command("CREATE TABLE sum_row (sum Float64, timestamp DateTime64) ENGINE = " \
      "MergeTree() PARTITION BY toYYYYMM(timestamp) ORDER BY (timestamp) PRIMARY KEY (timestamp)")
try:
    while True:
        last_row = client.query_df("SELECT * FROM slices_play WHERE timestamp > now() order by timestamp desc limit 1")
        #last_row = client.query_df("SELECT * FROM slices_play WHERE timestamp > subtractSeconds(now(),5) order by timestamp")#.tail(1)
        print(last_row)
        s = last_row.sum(axis=1, numeric_only=True).iloc[0]
        t = last_row["timestamp"].iloc[0]
        client.insert_df("sum_row", pd.DataFrame([{"sum": s, "timestamp": t}]))
        new_records = client.query_df("SELECT * FROM sum_row")
        print(new_records)
        time.sleep(5)
finally:
    print("disconnected")
