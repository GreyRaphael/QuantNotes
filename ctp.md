# CTP


preparation
- install gbk: `sudo dnf install glibc-langpack-zh`
- 交易所ctp: `pip install openctp-ctp`, visit [openctp](https://pypi.org/user/Jedore/) for details
- 模拟柜台ctp: `pip install openctp-tts`

> 如果是4097报错，一般是前置地址与柜台类型不匹配

```py
from openctp_tts import mdapi, tdapi
import threading
import time

# ----------------------------
# 0. 配置参数（替换为你的账户信息）
# ----------------------------
# get from http://www.openctp.cn/index.html
FRONT_TRADE = "tcp://121.37.80.177:20002"  # 交易前置地址
FRONT_MD = "tcp://121.37.80.177:20004"  # 行情前置地址
BROKER_ID = ""  # 经纪商代码
USER_ID = "your_name"
PASSWORD = "your_pwd"
INVESTOR_ID = USER_ID  # 投资者代码通常与用户ID相同


# ----------------------------
# 1. 行情订阅模块
# ----------------------------
class CMdImpl(mdapi.CThostFtdcMdSpi):
    def __init__(self, md_front):
        mdapi.CThostFtdcMdSpi.__init__(self)
        self.md_front = md_front
        self.api = None

    def Run(self):
        self.api = mdapi.CThostFtdcMdApi.CreateFtdcMdApi()
        self.api.RegisterFront(self.md_front)
        self.api.RegisterSpi(self)
        self.api.Init()

    def OnFrontConnected(self):
        print("OnFrontConnected")

        # Market channel doesn't check userid and password.
        req = mdapi.CThostFtdcReqUserLoginField()
        self.api.ReqUserLogin(req, 0)

    def OnFrontDisconnected(self, nReason: int):
        print(f"OnFrontDisconnected.[nReason={nReason}]")

    def OnRspUserLogin(self, pRspUserLogin: "CThostFtdcRspUserLoginField", pRspInfo: "CThostFtdcRspInfoField", nRequestID: "int", bIsLast: "bool"):
        if pRspInfo is not None and pRspInfo.ErrorID != 0:
            print(f"Login failed. {pRspInfo.ErrorMsg}")
            return
        print(f"Login succeed.{pRspUserLogin.TradingDay}")

        self.api.SubscribeMarketData(["rb2505".encode("utf-8")], 1)

    def OnRtnDepthMarketData(self, pDepthMarketData: "CThostFtdcDepthMarketDataField"):
        # print(pDepthMarketData)
        print(f"{pDepthMarketData.TradingDay}, {pDepthMarketData.OpenInterest} {pDepthMarketData.InstrumentID} - {pDepthMarketData.LastPrice} - {pDepthMarketData.Volume}")

    def OnRspSubMarketData(self, pSpecificInstrument: "CThostFtdcSpecificInstrumentField", pRspInfo: "CThostFtdcRspInfoField", nRequestID: "int", bIsLast: "bool"):
        if pRspInfo is not None and pRspInfo.ErrorID != 0:
            print(f"Subscribe failed. [{pSpecificInstrument.InstrumentID}] {pRspInfo.ErrorMsg}")
            return
        print(f"Subscribe succeed.{pSpecificInstrument.InstrumentID}")


# ----------------------------
# 3. 启动接口
# ----------------------------
if __name__ == "__main__":
    md = CMdImpl(FRONT_MD)
    md.Run()

    input("Press enter key to exit.")
```