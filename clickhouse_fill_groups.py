from clickhouse_driver import Client
import pandas as pd
from datetime import datetime

t = datetime.now()
print("data read")
kks = pd.read_csv("kks_with_groups3.csv",
   delimiter=";",
   )
print(kks)
client = Client(user="default", password="asdf", host="10.23.0.177")
client.execute("DROP TABLE IF EXISTS kks")
sql = "CREATE TABLE kks (kks String, name String, group String) ENGINE = " \
    "MergeTree() PRIMARY KEY (kks)"
client.execute(sql)
sql = 'INSERT INTO kks (kks,name,group) values'
client.insert_dataframe(sql, kks, settings=dict(use_numpy=True))

groups = pd.read_csv("groups.csv",
   delimiter=";",
   )
print(groups)
client.execute("DROP TABLE IF EXISTS groups")
sql = "CREATE TABLE groups (id Int, name String) ENGINE = " \
    "MergeTree() PRIMARY KEY (id)"
client.execute(sql)
sql = 'INSERT INTO groups (id,name) values'
client.insert_dataframe(sql, groups, settings=dict(use_numpy=True))


