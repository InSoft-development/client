import time

import pandas as pd
from clickhouse_driver import Client

client = Client(user="default", password="asdf", host="10.23.0.177")
client.execute("DROP TABLE IF EXISTS sum_row")
client.execute("CREATE TABLE sum_row (sum Float64, timestamp DateTime64) ENGINE = " \
      "MergeTree() PARTITION BY toYYYYMM(timestamp) ORDER BY (timestamp) PRIMARY KEY (timestamp)")
try:
    while True:
        slices = client.query_dataframe("SELECT * FROM slices_play WHERE timestamp > subtractSeconds(now(),5)").tail(1)
        print(slices)
        s = slices.sum(axis=1, numeric_only=True)
        sql = 'INSERT INTO sum_row (*) VALUES'
        client.insert_dataframe(sql, pd.DataFrame({"sum": s, "timestamp": slices["timestamp"][0]}),
                                settings=dict(use_numpy=True))
        slices = client.query_dataframe("SELECT * FROM sum_row").tail(1)
        print(slices)
        time.sleep(5)
finally:
    client.disconnect()
    print("disconnected")
