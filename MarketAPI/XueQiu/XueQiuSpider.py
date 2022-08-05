#!/usr/bin/python
# -*- coding: UTF-8 -*-
import re  # 正则表达式，进行文字匹配`
import urllib.request, urllib.error  # 制定URL，获取网页数据
import json
import time
import requests


class StockMarketData:
    def __init__(self):
        self.Ticker = ""
        self.OpenPrice = 0.0
        self.LastPrice = 0.0
        self.PreClosePrice = 0.0
        self.HighestPrice = 0.0
        self.LowestPrice = 0.0
        self.TotalAmount = 0.0
        self.UpdateTime = ""
        self.MillSec = 0
        

class XueQiuSpider(object):
    def __init__(self, StockMarketDataURL):
        self.StockMarketDataURL = StockMarketDataURL
    
    # 爬取数据
    def PullData(self, url):
        head = {  # 模拟浏览器头部信息，向服务器发送消息
            "User-Agent": "Mozilla / 5.0(Windows NT 10.0; Win64; x64) AppleWebKit / 537.36(KHTML, like Gecko) Chrome / 80.0.3987.122  Safari / 537.36"
        }
        request = requests.get(StockMarketDataURL, headers=head)
        html = request.content.decode("utf-8")
        return html

    # 解析数据
    def ParseStockMarketData(self, body):
        datalist = []
        json_object = json.loads(body)
        records = json_object["data"]
        for record in records:
            item = StockMarketData()
            item.Ticker = record["symbol"]
            item.OpenPrice = record["open"]
            item.LastPrice = record["current"]
            item.PreClosePrice = record["last_close"]
            item.HighestPrice = record["high"]
            item.LowestPrice = record["low"]
            item.TotalAmount = record["amount"]

            timestamp = int(record["timestamp"])
            timeArray = time.localtime(timestamp / 1000)
            strTime = time.strftime("%Y-%m-%d %H:%M:%S", timeArray)
            item.UpdateTime = strTime
            item.MillSec = timestamp % 1000
            datalist.append(item)
        return datalist
    
    def PrintStockMarketData(self, datalist):
        lines = ""
        for item in datalist:
                line = item.Ticker + "," + str(item.OpenPrice) + "," +  str(item.LastPrice) + "," +  str(item.PreClosePrice) + "," + str(item.HighestPrice) + "," + str(item.LowestPrice) + "," + item.UpdateTime + "\n"
                lines += line
        print(lines, end='') # 结束不换行

    # 执行爬虫一次，输出结果
    def Exec(self):
        # 爬取数据
        html = self.PullData(self.StockMarketDataURL)
        data_list = self.ParseStockMarketData(html)
        self.PrintStockMarketData(data_list)


if __name__ == "__main__":  # 当程序执行时
    # http://qt.gtimg.cn/q=sh601003,sh601001
    StockMarketDataURL = "https://stock.xueqiu.com/v5/stock/realtime/quotec.json?symbol=SH000300,SH000905,SH000016,SH000852"
    spider = XueQiuSpider(StockMarketDataURL)
    spider.Exec()

        


