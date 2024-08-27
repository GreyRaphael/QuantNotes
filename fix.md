# FIX protocol

- [FIX protocol](#fix-protocol)
  - [FIX by cpp](#fix-by-cpp)


## FIX by cpp

`vcpkg install quickfix`

```bash
├── CMakeLists.txt
├── client.cfg
├── server.cfg
├── client.cpp
└── server.cpp
```

```ini
# client.cfg
[DEFAULT]
# Basic connection settings
ConnectionType=initiator
UseDataDictionary=N
ResetOnLogon=Y
ReconnectInterval=60
FileStorePath=store
FileLogPath=log
HeartBtInt=30

# Session active time settings
StartDay=Sunday
EndDay=Saturday
StartTime=00:00:00
EndTime=23:59:59

# Logon and logout settings
LogonDay=Sunday
LogoutDay=Saturday
LogonTime=00:00:00
LogoutTime=23:59:59

[SESSION]
# Session identification
BeginString=FIX.4.2
SenderCompID=TEST3
TargetCompID=I2test

# Network settings
SocketConnectHost=127.0.0.1
SocketConnectPort=9008
```

```ini
# server.cfg
# client.cfg
[DEFAULT]
# Basic connection settings
ConnectionType=acceptor
UseDataDictionary=N
ResetOnLogon=Y
ReconnectInterval=60
FileStorePath=store
FileLogPath=log

# Session active time settings
StartDay=Sunday
EndDay=Saturday
StartTime=00:00:00
EndTime=23:59:59

# Logon and logout settings
LogonDay=Sunday
LogoutDay=Saturday
LogonTime=00:00:00
LogoutTime=23:59:59

[SESSION]
# Session identification
BeginString=FIX.4.2
SenderCompID=I2test
TargetCompID=TEST3

# Network settings
SocketAcceptPort=9008
```

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20.0)
project(proj1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
add_executable(client client.cpp)
add_executable(server server.cpp)

find_package(quickfix CONFIG REQUIRED)
target_link_libraries(client PRIVATE quickfix)
target_link_libraries(server PRIVATE quickfix)
```

```cpp
// client.cpp
#include <quickfix/FieldTypes.h>
#include <quickfix/FileLog.h>
#include <quickfix/FixFields.h>
#include <quickfix/FixValues.h>
#include <quickfix/SocketInitiator.h>

#include <chrono>
#include <cstdarg>
#include <string>

#include "quickfix/Application.h"
#include "quickfix/FileStore.h"
#include "quickfix/MessageCracker.h"
#include "quickfix/Session.h"
#include "quickfix/fix42/ExecutionReport.h"
#include "quickfix/fix42/NewOrderSingle.h"

class TradeClient : public FIX::Application, public FIX::MessageCracker {
   public:
    TradeClient() {}

    void onCreate(const FIX::SessionID&) override {}
    void onLogon(const FIX::SessionID& sessionID) override {
        std::cout << "Logon - " << sessionID << std::endl;
    }
    void onLogout(const FIX::SessionID& sessionID) override {
        std::cout << "Logout - " << sessionID << std::endl;
    }
    void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) override {}
    void fromAdmin(const FIX::Message&, const FIX::SessionID&)
        QUICKFIX_THROW(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override {}

    void toApp(FIX::Message& message, const FIX::SessionID& sessionID)
        QUICKFIX_THROW(FIX::DoNotSend) override {
        std::cout << "Sending message: " << message << std::endl;
    }
    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID)
        QUICKFIX_THROW(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override {
        std::cout << "Received message: " << message << std::endl;
        crack(message, sessionID);
    }

    void onMessage(const FIX42::ExecutionReport& message, const FIX::SessionID& sessionID) override {
        std::cout << "Received Execution Report: " << message << std::endl;
        FIX::OrderQty orderQty;
        message.get(orderQty);
        auto end = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        auto x = orderQty.getString();
        auto value = std::stod(x);
        std::cout << "===>diff = " << end - value << '\n';
    }

    void sendOrder() {
        FIX::SessionID sessionID("FIX.4.2", "TEST3", "I2test");
        FIX42::NewOrderSingle newOrderSingle(
            FIX::ClOrdID("111111111"),
            FIX::HandlInst(FIX::HandlInst_AUTOMATED_EXECUTION_ORDER_PRIVATE_NO_BROKER_INTERVENTION),
            FIX::Symbol("AAPL"),
            FIX::Side(FIX::Side_BUY),
            FIX::TransactTime(),
            FIX::OrdType(FIX::OrdType_LIMIT));

        newOrderSingle.set(FIX::Price(100.2));
        newOrderSingle.set(FIX::Currency("CNY"));
        newOrderSingle.set(FIX::SecurityType(FIX::SecurityType_COMMON_STOCK));
        newOrderSingle.set(FIX::Text("gewei order"));
        auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        newOrderSingle.set(FIX::OrderQty(start));
        std::cout << "start=" << start << '\n';
        FIX::Session::sendToTarget(newOrderSingle, sessionID);
    }
};

int main(int argc, char** argv) {
    FIX::SessionSettings settings("client.cfg");
    TradeClient application;
    FIX::FileStoreFactory storeFactory(settings);
    FIX::FileLogFactory logFactory(settings);
    FIX::SocketInitiator initiator(application, storeFactory, settings, logFactory);

    initiator.start();

    std::cout << "Press <ENTER> to quit" << std::endl;
    std::cin.get();
    for (size_t i = 0; i < 10; ++i) {
        application.sendOrder();
    }

    std::cin.get();

    initiator.stop();
}
```

```cpp
// server.cpp
#include <quickfix/Acceptor.h>
#include <quickfix/FileStore.h>
#include <quickfix/FixValues.h>

#include "quickfix/Application.h"
#include "quickfix/MessageCracker.h"
#include "quickfix/Session.h"
#include "quickfix/SocketAcceptor.h"
#include "quickfix/Utility.h"
#include "quickfix/fix42/ExecutionReport.h"
#include "quickfix/fix42/NewOrderSingle.h"

class Application : public FIX::Application, public FIX::MessageCracker {
   public:
    Application() {}

    void onCreate(const FIX::SessionID&) {}
    void onLogon(const FIX::SessionID& sessionID) {}
    void onLogout(const FIX::SessionID& sessionID) {}
    void toAdmin(FIX::Message&, const FIX::SessionID&) {}
    void toApp(FIX::Message&, const FIX::SessionID&) QUICKFIX_THROW(FIX::DoNotSend) {}
    void fromAdmin(const FIX::Message&, const FIX::SessionID&)
        QUICKFIX_THROW(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) {}
    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID)
        QUICKFIX_THROW(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) {
        crack(message, sessionID);
    }

    // MessageCracker overloads
    void onMessage(const FIX42::NewOrderSingle&, const FIX::SessionID&);
};

void Application::onMessage(const FIX42::NewOrderSingle& message, const FIX::SessionID& sessionID) {
    FIX::Symbol symbol;
    FIX::Side side;
    FIX::OrdType ordType;
    FIX::OrderQty orderQty;
    FIX::Price price;
    FIX::ClOrdID clOrdID;
    FIX::Account account;

    message.get(ordType);

    if (ordType != FIX::OrdType_LIMIT)
        throw FIX::IncorrectTagValue(ordType.getTag());

    message.get(symbol);
    message.get(side);
    message.get(orderQty);
    message.get(price);
    message.get(clOrdID);

    FIX42::ExecutionReport executionReport = FIX42::ExecutionReport(FIX::OrderID("55555"),
                                                                    FIX::ExecID("66666"),
                                                                    FIX::ExecTransType(FIX::ExecTransType_NEW),
                                                                    FIX::ExecType(FIX::ExecType_FILL),
                                                                    FIX::OrdStatus(FIX::OrdStatus_PARTIALLY_FILLED),
                                                                    symbol,
                                                                    side,
                                                                    FIX::LeavesQty(0),
                                                                    FIX::CumQty(100),
                                                                    FIX::AvgPx(price));

    executionReport.set(clOrdID);
    executionReport.set(orderQty);
    executionReport.set(FIX::LastPx(price));

    if (message.isSet(account))
        executionReport.setField(message.get(account));

    try {
        FIX::Session::sendToTarget(executionReport, sessionID);
    } catch (FIX::SessionNotFound&) {
    }
}

int main(int argc, char** argv) {
    FIX::SessionSettings settings("server.cfg");
    Application application;
    FIX::FileStoreFactory storeFactory(settings);
    FIX::ScreenLogFactory logFactory(settings);
    FIX::SocketAcceptor acceptor{application, storeFactory, settings, logFactory};

    acceptor.start();
    std::cin.get();
    acceptor.stop();
}
```