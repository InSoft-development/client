#!/usr/bin/env python3
import sqlite3
import datetime
import time
import csv
import matplotlib.pyplot as plt
import argparse
import pandas as pd
import numpy as np
import traces as tr
import numba
import sqlite3
import dateutil.parser
import clickhouse_connect
import re
import pytz




def parse_args():
    parser = argparse.ArgumentParser(description="convert (id,timestamp,value) format to slices")
    parser.add_argument("--delta", "-d", type=int, help="delta between slices in miliseconds (default 60 sec)", default=60000)
    parser.add_argument("--interval", "--time", "-t", nargs=2, required=True, type=str,
                        help="two timestamps, period, format \"YYY-MM-DD HH:MM:SS.SSS\" \"YYY-MM-DD HH:MM:SS.SSS\"" )
    parser.add_argument("--output", "-o", default="slices.csv", type=str,
                        help="output:csv file, sqlite file or clickhouse ip. In sqlite - table synchro_data would"
                             " be dropped. In clickhouse - no drop, append with new data")
    parser.add_argument("--input", "-i", default="data.sqlite", type=str,
                        help="input: sqlite file or clickhouse ip. Tables dynamic_data and static_data would be used")

    return parser.parse_args()


# @numba.njit
# def mean5(data, nums):
#     # средние
#     means_for_5 = pd.DataFrame()
#     print("means forming")
#     start_time = time.time()
#     sampling_period = datetime.timedelta(seconds=15)
#     start = datetime.datetime(2021, 6, 1)
#     end = datetime.datetime(2022,10, 30)
#     t = start
#     value_kks = dict.fromkeys(nums + ["timestamp"])
#     for a in value_kks.keys():
#         value_kks[a] = []
#     mean = 0
#     while t < end:
#         t2 = t + sampling_period
#         data_t = data.query("timestamp >= @t and timestamp < @t2")
#         value_kks["timestamp"].append(t2)
#         for k in nums:
#             data_k = data_t[data_t.kks == k]
#             if len(data_k) > 0:
#                 mean = data_k["value"].mean()
#             elif len(value_kks[k]) > 0:
#                 mean = value_kks[k][-1]
#             else:
#                 mean = 0
#             value_kks[k].append(mean)
#         t = t2
#     return value_kks

if __name__ == '__main__':
    args = parse_args()
    # delta = datetime.timedelta(seconds=args.delta)
    pattern = re.compile("([0-9]{1,3}[\.]){3}[0-9]{1,3}")

    if args.input[-6:] == "sqlite":
        in_format = "sqlite"
        infile = args.input
        conn_raw_in = sqlite3.connect(infile)
    elif args.input == "localhost" or pattern.match(args.input):
        in_format = "clickhouse"
        client_in = clickhouse_connect.get_client(host=args.input, username='default', password='asdf')
    else:
        print("unknown in format: clickhouse or sqlite available")
        exit(1)

    if args.output[-3:] == "csv":
        outfile = args.output
        out_format = "csv"
    if args.output[-6:] == "sqlite":
        out_format = "sqlite"
        outfile = args.output
        conn_raw_out = sqlite3.connect(outfile)
    elif args.output == "localhost" or pattern.match(args.input):
        out_format = "clickhouse"
        client_out = clickhouse_connect.get_client(host=args.output, username='default', password='asdf')
    else:
        print("unknown out format: clickhouse, sqlite or csv available")
        exit(1)

    print("\nin: "+in_format)
    print("\nout: " + out_format)

    nums = pd.read_csv("kks.csv", header=None)[0].to_list()  # ["20MAD11CY004"] #
    start_date = dateutil.parser.parse(args.interval[0])  # datetime.datetime(2021, 6, 1)
    end_date = dateutil.parser.parse(args.interval[1])  # datetime.datetime(2022, 10, 30)

    delta = args.delta 
    # загрузить датафрейм из цсв
    # start_time = time.time()
    # print("data read")
    # def dateparse(x): return pd.to_datetime(x) #datetime.datetime.strptime(x, '%Y-%m-%d %H:%M:%S.%f')
    # tp = pd.read_csv(infile,
    #                    #nrows = 1000000,
    #                    delimiter=";",
    #                    #dtype={'kks':'str','value':'float','timestamp':'str','status':'str'},
    #                    parse_dates=['timestamp'],
    #                    infer_datetime_format=True,
    #                    date_parser=dateparse,
    #                    engine='c',
    #                    on_bad_lines = 'warn',
    #                    verbose=True,
    #                    low_memory=False,
    #                    chunksize=100000
    #
    #                    )
    # data = pd.concat(tp,ignore_index=True)

    # print(data)
    # data.to_dict()
    # data.drop([0], inplace=True)
    # data.set_index("timestamp", inplace=True)

    # print(time.time() - start_time)
    # найти первое вхождение каждого датчика
    # print("index first")
    # first_kks = {}
    # for kks in nums:
    #     first_kks[kks] = (data.kks == kks).idxmax()
    # start_index = max(first_kks.values())
    # print(start_index)

    # для каждого датчика - вычленить из датафрейма кусок и трэйссом превратить в колонку
    # slices = pd.DataFrame({"timestamp": []})
    # print("slices forming")
    # start_time = time.time()
    # for kks in nums:
    #     #print(kks)
    #     data_column = data[data.kks == kks][["timestamp", "value"]]
    #     #print(len(data_column))
    #     ts = tr.TimeSeries(data_column.values)
    #     column = ts.sample(sampling_period=datetime.timedelta(minutes=5),
    #                        start=datetime.datetime(2021, 6, 1),
    #                        end=datetime.datetime(2021, 6, 30),
    #                        interpolate='linear')
    #     slices = slices.merge(pd.DataFrame(column, columns=["timestamp", kks]),
    #                  how="outer")
    # print(time.time() - start_time)
    # print(slices)

    # средние за 5 после слайсов
    slices_1 = pd.DataFrame({"timestamp": []})

    print("slices forming")
    start_time = time.time()
    for kks in nums:
        print(kks)

        sql = ("SELECT t as timestamp,val as value FROM "
               "dynamic_data JOIN static_data ON static_data.id=dynamic_data.id WHERE name=\'" +
               kks + "\' and t > \'" + args.interval[0]+ "\' and t < \'" +
               args.interval[1] + "\'")
        print(sql)
        if in_format == "sqlite":
            data_column = pd.read_sql(sql, conn_raw_in,parse_dates=['timestamp'])
        elif in_format == "clickhouse":
            data_column = client_in.query_df(sql)
            start_date = start_date.replace(tzinfo=pytz.UTC)
            end_date = end_date.replace(tzinfo=pytz.UTC)
        if len(data_column) == 0:
            print("missing data!!")
            data_column = pd.DataFrame([{"timestamp": start_date, "value": 0},
                                        {"timestamp": end_date, "value": 0}])
        print(data_column)
        ts = tr.TimeSeries(data_column.values)
        column = ts.sample(sampling_period=datetime.timedelta(milliseconds=delta/5),
                           start=start_date,
                           end=end_date)  # ,
        # interpolate='previous')
        slices_1 = slices_1.merge(pd.DataFrame(column, columns=["timestamp", kks]),
                                  how="outer")

    agg_func = dict(zip(nums, ["mean" for a in nums]))
    agg_func["timestamp"] = "last"
    print(agg_func)
    slices_5mean = slices_1.groupby(np.arange(len(slices_1)) // 5).agg(agg_func)
    print(time.time() - start_time)
    print(slices_5mean)

    # start_time = time.time()
    # means = pd.DataFrame.from_dict(mean5(data,nums))
    # print(time.time() - start_time)
    # print(means)

    slices_5mean.fillna(method = 'bfill',inplace=True)
    # slices.to_csv("slices_vib.csv")
    # means.to_csv("mean_slices_vib.csv")
    # slices_1.to_csv("slices_GT1m.csv")
    if out_format == "csv":
        slices_5mean.to_csv(outfile,index=False)
    elif out_format == "clickhouse":
        client_out.insert_df('synchro_data', slices_5mean)
    elif out_format == "sqlite":
        conn_raw_out.execute("DROP TABLE IF EXISTS synchro_data")
        slices_5mean.to_sql('synchro_data', conn_raw_out)

    # slices_5mean.plot(x="timestamp")

    # data_k = data[data.kks == "20MAD11CY004"]
    # plt.plot(data_k["timestamp"],data_k["value"],"g",label = "real data")
    # plt.plot(slices["timestamp"], slices["20MAD11CY004"], "b", label = "interpolation5")
    # plt.plot(slices_5mean["timestamp"], slices_5mean["20MAD11CY004"], "m", label = "interpolation1 and mean5")
    # plt.plot(means["timestamp"], means["20MAD11CY004"], "r", label = "mean")
    # plt.legend(loc="upper left")
    # plt.show()
    # slices = slices[start_index:]
    # print("slices saving")

    # data.to_csv("true_data_vib.csv")
